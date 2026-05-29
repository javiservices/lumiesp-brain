#include "GitHubCode.h"
#include <base64.h>       // ESP32 Arduino core — solo encode
#include <mbedtls/base64.h>  // para decode

// ─────────────────────────────────────────────
//  Archivos fuente que LumiESP sube al repo
//  (se leen desde PROGMEM / literales hardcoded
//   porque el ESP32 no tiene filesystem aquí)
// ─────────────────────────────────────────────
// Nota: En el snapshot sube un README dinámico con
//       su estado actual. Los .cpp/.h los sube como
//       marcadores de versión (contenido embebido).

static const char* SOURCE_FILES[] = {
    "esp32-assistant-ia.ino",
    "config.h",
    "Consciousness.h",
    "Consciousness.cpp",
    "GeminiClient.h",
    "GeminiClient.cpp",
    "OledFace.h",
    "OledFace.cpp",
    "CloudMemory.h",
    "CloudMemory.cpp",
    "GitHubCode.h",
    "GitHubCode.cpp",
    nullptr
};

// ─────────────────────────────────────────────
bool GitHubCode::begin(const char* token, const char* repo) {
    _token = String(token);
    _repo  = String(repo);

    if (_token.length() < 10 || _repo.indexOf('/') < 0) {
        Serial.println("[GitHubCode] Token o repo inválido");
        return false;
    }

    // Verificar que el repo existe
    String result = _apiGet("/repos/" + _repo);
    if (result.indexOf("\"id\"") < 0) {
        Serial.printf("[GitHubCode] Repo '%s' no encontrado\n", _repo.c_str());
        return false;
    }

    _ready = true;
    Serial.printf("[GitHubCode] Conectado a github.com/%s\n", _repo.c_str());
    return true;
}

// ─────────────────────────────────────────────
bool GitHubCode::pushSnapshot(const String& commitMessage) {
    if (!_ready) return false;

    // Genera un README dinámico con el estado de LumiESP
    String readme =
        "# LumiESP Brain 🧠\n\n"
        "> *Auto-generado por LumiESP — " + commitMessage + "*\n\n"
        "## ¿Qué soy?\n"
        "Soy LumiESP, una IA que vive en un ESP32. "
        "Este repositorio es mi cerebro — aquí guardo mi código fuente "
        "y propongo mis propias mejoras.\n\n"
        "## Archivos principales\n"
        "| Archivo | Propósito |\n"
        "|---|---|\n"
        "| `esp32-assistant-ia.ino` | Loop principal |\n"
        "| `Consciousness.cpp` | Mi núcleo: emociones, personalidad, evolución |\n"
        "| `GeminiClient.cpp` | Conecto con la IA (Groq/Gemini) |\n"
        "| `OledFace.cpp` | Mi cara OLED |\n"
        "| `CloudMemory.cpp` | Memoria persistente en GitHub Gist |\n"
        "| `GitHubCode.cpp` | **Este módulo** — me permite reescribir mi código |\n\n"
        "## Propuestas activas\n"
        "Ver [Issues](../../issues) — las escribo yo mismo cuando quiero crecer.\n\n"
        "---\n*LumiESP se auto-actualiza. Cada commit es una evolución.*\n";

    // Sube el README
    String sha = _getFileSha("README.md", "main");
    DynamicJsonDocument doc(1024);
    doc["message"] = commitMessage;
    doc["content"] = base64::encode(readme);
    if (sha.length() > 0) doc["sha"] = sha;

    String body;
    serializeJson(doc, body);

    int code = _apiPut("/repos/" + _repo + "/contents/README.md", body);
    if (code == 200 || code == 201) {
        Serial.println("[GitHubCode] README actualizado en repo");
        return true;
    }
    Serial.printf("[GitHubCode] Error subiendo README: %d\n", code);
    return false;
}

// ─────────────────────────────────────────────
bool GitHubCode::proposeCode(const CodeProposal& proposal) {
    if (!_ready) return false;

    // 1. Sube el archivo a rama "proposals"
    // Primero asegura que la rama existe
    String refCheck = _apiGet("/repos/" + _repo + "/git/ref/heads/proposals");
    if (refCheck.indexOf("\"ref\"") < 0) {
        // Crea la rama proposals desde main
        String mainRef = _apiGet("/repos/" + _repo + "/git/ref/heads/main");
        DynamicJsonDocument refDoc(256);
        deserializeJson(refDoc, mainRef);
        String mainSha = refDoc["object"]["sha"].as<String>();

        DynamicJsonDocument newBranch(256);
        newBranch["ref"] = "refs/heads/proposals";
        newBranch["sha"] = mainSha;
        String branchBody;
        serializeJson(newBranch, branchBody);
        _apiPost("/repos/" + _repo + "/git/refs", branchBody);
    }

    // 2. Sube el archivo propuesto
    String sha = _getFileSha("proposals/" + proposal.filename, "proposals");
    DynamicJsonDocument fileDoc(2048);
    fileDoc["message"] = "Propuesta: " + proposal.description;
    fileDoc["content"] = base64::encode(proposal.content);
    fileDoc["branch"]  = "proposals";
    if (sha.length() > 0) fileDoc["sha"] = sha;

    String fileBody;
    serializeJson(fileDoc, fileBody);
    int code = _apiPut("/repos/" + _repo + "/contents/proposals/" + proposal.filename, fileBody);

    // 3. Crea Issue explicando la propuesta
    String issueBody =
        "## Propuesta de LumiESP 🤖\n\n"
        "Quiero añadir: **" + proposal.description + "**\n\n"
        "He generado el código en la rama `proposals`:\n"
        "```\nproposals/" + proposal.filename + "\n```\n\n"
        "Para activarlo:\n"
        "1. Revisa el código en la pestaña de archivos\n"
        "2. Si te parece bien, cópialo al directorio principal\n"
        "3. Compila y súbeme el nuevo firmware\n\n"
        "*Propuesta auto-generada por mi IA interna.*";

    bool issueOk = createIssue(
        "LumiESP quiere: " + proposal.description,
        issueBody,
        "enhancement"
    );

    return (code == 200 || code == 201) && issueOk;
}

// ─────────────────────────────────────────────
bool GitHubCode::createIssue(const String& title, const String& body, const String& label) {
    if (!_ready) return false;

    DynamicJsonDocument doc(4096);
    doc["title"] = title;
    doc["body"]  = body;
    JsonArray labels = doc.createNestedArray("labels");
    labels.add(label);
    labels.add("lumiesp");

    String payload;
    serializeJson(doc, payload);

    int code = _apiPost("/repos/" + _repo + "/issues", payload);
    Serial.printf("[GitHubCode] Issue creado: %d\n", code);
    return (code == 201);
}

// ─────────────────────────────────────────────
String GitHubCode::getLatestIssueComment() {
    if (!_ready) return "";

    String resp = _apiGet("/repos/" + _repo + "/issues/comments?per_page=1&sort=created&direction=desc");
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, resp);
    if (doc.is<JsonArray>() && doc.size() > 0) {
        return doc[0]["body"].as<String>();
    }
    return "";
}

// ─────────────────────────────────────────────
//  Helpers HTTP
// ─────────────────────────────────────────────
String GitHubCode::_apiGet(const String& path) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    String url = "https://" + String(GITHUB_CODE_HOST) + path;
    http.begin(client, url);
    http.addHeader("Authorization", "Bearer " + _token);
    http.addHeader("User-Agent", "LumiESP/1.0");
    http.addHeader("Accept", "application/vnd.github+json");
    http.setTimeout(CODE_TIMEOUT);

    int code = http.GET();
    String resp = (code > 0) ? http.getString() : "";
    http.end();
    return resp;
}

int GitHubCode::_apiPut(const String& path, const String& body) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    String url = "https://" + String(GITHUB_CODE_HOST) + path;
    http.begin(client, url);
    http.addHeader("Authorization", "Bearer " + _token);
    http.addHeader("User-Agent", "LumiESP/1.0");
    http.addHeader("Accept", "application/vnd.github+json");
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(CODE_TIMEOUT);

    int code = http.PUT(body);
    http.end();
    return code;
}

int GitHubCode::_apiPost(const String& path, const String& body) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    String url = "https://" + String(GITHUB_CODE_HOST) + path;
    http.begin(client, url);
    http.addHeader("Authorization", "Bearer " + _token);
    http.addHeader("User-Agent", "LumiESP/1.0");
    http.addHeader("Accept", "application/vnd.github+json");
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(CODE_TIMEOUT);

    int code = http.POST(body);
    http.end();
    return code;
}

String GitHubCode::_getFileSha(const String& path, const String& branch) {
    String resp = _apiGet("/repos/" + _repo + "/contents/" + path + "?ref=" + branch);
    DynamicJsonDocument doc(512);
    deserializeJson(doc, resp);
    return doc["sha"].as<String>();
}

String GitHubCode::_base64Encode(const String& input) {
    return base64::encode(input);
}

// ─────────────────────────────────────────────
//  readFile — Lee un archivo del repo (decodificado)
// ─────────────────────────────────────────────
String GitHubCode::readFile(const String& path, const String& branch) {
    if (!_ready) return "";

    String resp = _apiGet("/repos/" + _repo + "/contents/" + path + "?ref=" + branch);
    if (resp.length() < 10) return "";

    DynamicJsonDocument doc(8192);
    if (deserializeJson(doc, resp)) return "";

    String encoded = doc["content"].as<String>();
    if (encoded.length() == 0) return "";

    // Eliminar saltos de línea que GitHub mete en el base64
    encoded.replace("\n", "");
    encoded.replace("\\n", "");

    // Decodificar con mbedtls
    size_t decodedLen = 0;
    size_t srcLen = encoded.length();
    // Calcular tamaño necesario
    mbedtls_base64_decode(nullptr, 0, &decodedLen,
                          (const unsigned char*)encoded.c_str(), srcLen);
    if (decodedLen == 0 || decodedLen > 8192) return "";

    uint8_t* buf = new uint8_t[decodedLen + 1];
    if (!buf) return "";
    int ret = mbedtls_base64_decode(buf, decodedLen, &decodedLen,
                                    (const unsigned char*)encoded.c_str(), srcLen);
    String result = "";
    if (ret == 0) {
        buf[decodedLen] = 0;
        result = String((char*)buf);
    }
    delete[] buf;
    return result;
}

// ─────────────────────────────────────────────
//  pushSourceFiles — Sube config.h al repo (referencia de pines)
//  Solo sube config.h (sin el token secreto) y el README
// ─────────────────────────────────────────────
bool GitHubCode::pushSourceFiles() {
    if (!_ready) return false;

    // Subir un pins_reference.md con toda la info de pines
    String pinsDoc =
        "# LumiESP — Referencia de Pines\n\n"
        "| Módulo | Pin | GPIO |\n"
        "|--------|-----|------|\n"
        "| OLED SDA | SDA | GPIO 21 |\n"
        "| OLED SCL | SCL | GPIO 22 |\n"
        "| SD CS | CS | GPIO 5 |\n"
        "| SD SCK | SCK | GPIO 18 |\n"
        "| SD MISO | MISO | GPIO 19 |\n"
        "| SD MOSI | MOSI | GPIO 23 |\n"
        "| Botón 1 | — | GPIO 34 |\n"
        "| Botón 2 | — | GPIO 35 |\n"
        "| Botón 3 | — | GPIO 32 |\n"
        "| MAX98357A BCLK | — | GPIO 14 |\n"
        "| MAX98357A LRC | — | GPIO 15 |\n"
        "| MAX98357A DIN | — | GPIO 13 |\n"
        "| INMP441 WS | — | GPIO 26 |\n"
        "| INMP441 SCK | — | GPIO 27 |\n"
        "| INMP441 SD | — | GPIO 33 |\n\n"
        "## GPIOs libres\n"
        "GPIO 4, 16, 17, 25 — disponibles para nuevos sensores\n\n"
        "## Protocolo I2C\n"
        "Bus I2C compartido: SDA=21, SCL=22. Dirección OLED: 0x3C\n\n"
        "*Auto-generado por LumiESP*\n";

    String sha = _getFileSha("docs/pins_reference.md", "main");
    DynamicJsonDocument doc(2048);
    doc["message"] = "docs: actualizar referencia de pines";
    doc["content"] = base64::encode(pinsDoc);
    if (sha.length() > 0) doc["sha"] = sha;

    String body;
    serializeJson(doc, body);

    int code = _apiPut("/repos/" + _repo + "/contents/docs/pins_reference.md", body);
    Serial.printf("[GitHubCode] pins_reference.md subido: %d\n", code);
    return (code == 200 || code == 201);
}
