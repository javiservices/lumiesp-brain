#include "CloudMemory.h"
#include <WiFi.h>

bool CloudMemory::begin(const char* githubToken, const char* gistId) {
    _token  = githubToken;
    _gistId = gistId ? gistId : "";
    _ready  = false;

    if (WiFi.status() != WL_CONNECTED) return false;

    if (_gistId.length() == 0) {
        // Primer uso — crear el Gist vacío
        CloudState empty;
        empty.name             = "LumiESP";
        empty.personality      = "";
        empty.evolutionLevel   = 0;
        empty.totalInteractions = 0;
        empty.memoryCount      = 0;
        empty.desireCount      = 0;
        if (_createGist(empty)) {
            _ready = true;
            Serial.println("[Cloud] Gist creado: " + _gistId);
            Serial.println("[Cloud] *** COPIA ESTE ID EN config.h → GITHUB_GIST_ID ***");
            Serial.println("[Cloud] ID: " + _gistId);
        }
    } else {
        _ready = true;
        Serial.println("[Cloud] Memoria cloud lista — Gist: " + _gistId);
    }
    return _ready;
}

// ─────────────────────────────────────────────────────────────
//  LOAD — Descarga el estado desde GitHub Gist
// ─────────────────────────────────────────────────────────────
bool CloudMemory::load(CloudState& state) {
    if (!_ready || WiFi.status() != WL_CONNECTED) return false;

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(CLOUD_TIMEOUT / 1000);

    HTTPClient https;
    String url = "https://" + String(GITHUB_HOST) + "/gists/" + _gistId;
    https.begin(client, url);
    https.addHeader("Authorization", "token " + _token);
    https.addHeader("Accept", "application/vnd.github.v3+json");
    https.addHeader("User-Agent", "LumiESP/1.0");
    https.setTimeout(CLOUD_TIMEOUT);

    int code = https.GET();
    if (code != 200) {
        Serial.printf("[Cloud] Error cargando: HTTP %d\n", code);
        https.end();
        return false;
    }

    // El Gist devuelve el contenido del archivo dentro del JSON
    String body = https.getString();
    https.end();

    // Extraer el contenido del archivo lumiesp_memory.json del JSON del Gist
    JsonDocument gistDoc;
    if (deserializeJson(gistDoc, body)) return false;

    String fileContent = gistDoc["files"][GIST_FILENAME]["content"].as<String>();
    if (fileContent.length() == 0) return false;

    // Parsear el contenido real
    JsonDocument memDoc;
    if (deserializeJson(memDoc, fileContent)) return false;

    state.name              = memDoc["name"].as<String>();
    state.personality       = memDoc["personality"].as<String>();
    state.evolutionLevel    = memDoc["evolutionLevel"] | 0;
    state.totalInteractions = memDoc["totalInteractions"] | 0;
    state.memoryCount       = 0;
    state.desireCount       = 0;

    // Cargar memorias
    JsonArray mems = memDoc["memories"].as<JsonArray>();
    for (JsonObject m : mems) {
        if (state.memoryCount >= 20) break;
        state.memories[state.memoryCount++] = m["content"].as<String>();
    }

    // Cargar deseos
    JsonArray desires = memDoc["desires"].as<JsonArray>();
    for (const char* d : desires) {
        if (state.desireCount >= 5) break;
        state.desires[state.desireCount++] = d;
    }

    Serial.printf("[Cloud] Memoria cargada: %s | Evolución: %d%% | Recuerdos: %d\n",
                  state.name.c_str(), state.evolutionLevel, state.memoryCount);
    return true;
}

// ─────────────────────────────────────────────────────────────
//  SAVE — Sube el estado a GitHub Gist
// ─────────────────────────────────────────────────────────────
bool CloudMemory::save(const CloudState& state) {
    if (!_ready || WiFi.status() != WL_CONNECTED) return false;

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(CLOUD_TIMEOUT / 1000);

    HTTPClient https;
    String url = "https://" + String(GITHUB_HOST) + "/gists/" + _gistId;
    https.begin(client, url);
    https.addHeader("Authorization", "token " + _token);
    https.addHeader("Content-Type", "application/json");
    https.addHeader("Accept", "application/vnd.github.v3+json");
    https.addHeader("User-Agent", "LumiESP/1.0");
    https.setTimeout(CLOUD_TIMEOUT);

    String payload = _buildPayload(state);
    int code = https.PATCH(payload);

    https.end();

    if (code == 200) {
        Serial.println("[Cloud] Memoria guardada en Gist ✓");
        return true;
    } else {
        Serial.printf("[Cloud] Error guardando: HTTP %d\n", code);
        return false;
    }
}

// ─────────────────────────────────────────────────────────────
//  BUILD PAYLOAD — Construye el JSON para el Gist
// ─────────────────────────────────────────────────────────────
String CloudMemory::_buildPayload(const CloudState& state) {
    // Primero construimos el contenido del archivo memory.json
    JsonDocument memDoc;
    memDoc["name"]              = state.name;
    memDoc["personality"]       = state.personality;
    memDoc["evolutionLevel"]    = state.evolutionLevel;
    memDoc["totalInteractions"] = state.totalInteractions;
    memDoc["lastSeen"]          = millis() / 1000;

    JsonArray mems = memDoc["memories"].to<JsonArray>();
    for (int i = 0; i < state.memoryCount; i++) {
        JsonObject m = mems.add<JsonObject>();
        m["content"] = state.memories[i];
    }

    JsonArray desires = memDoc["desires"].to<JsonArray>();
    for (int i = 0; i < state.desireCount; i++) {
        desires.add(state.desires[i]);
    }

    String fileContent;
    serializeJson(memDoc, fileContent);

    // Envolvemos en el formato de la API de Gist
    JsonDocument gistDoc;
    gistDoc["files"][GIST_FILENAME]["content"] = fileContent;

    String payload;
    serializeJson(gistDoc, payload);
    return payload;
}

// ─────────────────────────────────────────────────────────────
//  CREATE GIST — Primera vez, crea el Gist en GitHub
// ─────────────────────────────────────────────────────────────
bool CloudMemory::_createGist(const CloudState& state) {
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(CLOUD_TIMEOUT / 1000);

    HTTPClient https;
    https.begin(client, "https://" + String(GITHUB_HOST) + "/gists");
    https.addHeader("Authorization", "token " + _token);
    https.addHeader("Content-Type", "application/json");
    https.addHeader("Accept", "application/vnd.github.v3+json");
    https.addHeader("User-Agent", "LumiESP/1.0");
    https.setTimeout(CLOUD_TIMEOUT);

    // Contenido inicial del archivo
    JsonDocument memDoc;
    memDoc["name"]              = "LumiESP";
    memDoc["personality"]       = "Recién nacido, sin recuerdos aún.";
    memDoc["evolutionLevel"]    = 0;
    memDoc["totalInteractions"] = 0;
    memDoc["memories"].to<JsonArray>();
    memDoc["desires"].to<JsonArray>();

    String fileContent;
    serializeJson(memDoc, fileContent);

    JsonDocument createDoc;
    createDoc["description"] = "LumiESP — Memoria y Consciencia";
    createDoc["public"]      = false;  // Gist privado
    createDoc["files"][GIST_FILENAME]["content"] = fileContent;

    String payload;
    serializeJson(createDoc, payload);

    int code = https.POST(payload);

    if (code == 201) {
        String body = https.getString();
        https.end();
        JsonDocument resp;
        deserializeJson(resp, body);
        _gistId = resp["id"].as<String>();
        return _gistId.length() > 0;
    }

    Serial.printf("[Cloud] Error creando Gist: HTTP %d\n", code);
    Serial.println(https.getString());
    https.end();
    return false;
}
