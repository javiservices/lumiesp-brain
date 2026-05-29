#include "GeminiClient.h"
#include "config.h"
#include <WiFi.h>

void GeminiClient::begin(const char* apiKey, const String& systemPrompt) {
    if (apiKey == nullptr || strlen(apiKey) == 0) {
#ifdef AI_BACKEND_GROQ
        _apiKey = GROQ_API_KEY;
#else
        _apiKey = GEMINI_API_KEY;
#endif
    } else {
        _apiKey = apiKey;
    }
    _systemPrompt = systemPrompt;
    _historyCount = 0;
    _requestCount = 0;
    _lastHttpCode = 0;
}

void GeminiClient::clearHistory() {
    _historyCount = 0;
}

void GeminiClient::_addToHistory(const String& role, const String& text) {
    if (_historyCount < MAX_HISTORY) {
        _history[_historyCount++] = {role, text};
    } else {
        for (int i = 2; i < MAX_HISTORY; i++) {
            _history[i - 2] = _history[i];
        }
        _historyCount = MAX_HISTORY - 2;
        _history[_historyCount++] = {role, text};
    }
}

// ─── Payload para Google Gemini ──────────────────────────────
String GeminiClient::_buildGeminiPayload(const String& userMessage) {
    JsonDocument doc;
    JsonObject systemInstruction = doc["system_instruction"].to<JsonObject>();
    JsonArray siParts = systemInstruction["parts"].to<JsonArray>();
    siParts.add<JsonObject>()["text"] = _systemPrompt;

    JsonArray contents = doc["contents"].to<JsonArray>();
    for (int i = 0; i < _historyCount; i++) {
        JsonObject msg = contents.add<JsonObject>();
        msg["role"] = _history[i].role;
        msg["parts"].to<JsonArray>().add<JsonObject>()["text"] = _history[i].text;
    }
    JsonObject userMsg = contents.add<JsonObject>();
    userMsg["role"] = "user";
    userMsg["parts"].to<JsonArray>().add<JsonObject>()["text"] = userMessage;

    JsonObject genConfig = doc["generationConfig"].to<JsonObject>();
    genConfig["temperature"]     = 0.9;
    genConfig["maxOutputTokens"] = 512;
    genConfig["topP"]            = 0.95;

    String payload;
    serializeJson(doc, payload);
    return payload;
}

// ─── Payload para Groq (OpenAI-compatible) ───────────────────
String GeminiClient::_buildGroqPayload(const String& userMessage) {
    JsonDocument doc;
    doc["model"]       = GROQ_MODEL;
    doc["temperature"] = 0.9;
    doc["max_tokens"]  = 512;

    JsonArray messages = doc["messages"].to<JsonArray>();

    JsonObject sysMsg = messages.add<JsonObject>();
    sysMsg["role"]    = "system";
    sysMsg["content"] = _systemPrompt;

    for (int i = 0; i < _historyCount; i++) {
        JsonObject msg = messages.add<JsonObject>();
        msg["role"]    = (_history[i].role == "model") ? "assistant" : "user";
        msg["content"] = _history[i].text;
    }

    JsonObject userMsg = messages.add<JsonObject>();
    userMsg["role"]    = "user";
    userMsg["content"] = userMessage;

    String payload;
    serializeJson(doc, payload);
    return payload;
}

// ─── Llamada principal ────────────────────────────────────────
String GeminiClient::ask(const String& userMessage) {
    if (WiFi.status() != WL_CONNECTED) {
        return "";
    }

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(GEMINI_TIMEOUT / 1000);

    HTTPClient https;
    String response = "";

#ifdef AI_BACKEND_GROQ
    // ── GROQ ──────────────────────────────────────────────────
    https.begin(client, "https://" GROQ_HOST "/openai/v1/chat/completions");
    https.addHeader("Content-Type", "application/json");
    https.addHeader("Authorization", "Bearer " + _apiKey);
    https.setTimeout(GEMINI_TIMEOUT);

    String payload = _buildGroqPayload(userMessage);
    _lastHttpCode = https.POST(payload);

    if (_lastHttpCode == HTTP_CODE_OK) {
        String body = https.getString();
        https.end();
        JsonDocument respDoc;
        if (!deserializeJson(respDoc, body)) {
            String text = respDoc["choices"][0]["message"]["content"].as<String>();
            if (text.length() > 0) {
                _addToHistory("user", userMessage);
                _addToHistory("model", text);
                _requestCount++;
                response = text;
            }
        }
    } else {
        Serial.printf("[Groq] Error HTTP: %d\n", _lastHttpCode);
        if (_lastHttpCode > 0) Serial.println("[Groq] Body: " + https.getString());
        https.end();
    }

#else
    // ── GEMINI ────────────────────────────────────────────────
    String url = "https://";
    url += GEMINI_HOST;
    url += "/v1beta/models/";
    url += GEMINI_MODEL;
    url += ":generateContent?key=";
    url += _apiKey;

    https.begin(client, url);
    https.addHeader("Content-Type", "application/json");
    https.setTimeout(GEMINI_TIMEOUT);

    String payload = _buildGeminiPayload(userMessage);
    _lastHttpCode = https.POST(payload);

    if (_lastHttpCode == HTTP_CODE_OK) {
        String body = https.getString();
        https.end();
        JsonDocument respDoc;
        if (!deserializeJson(respDoc, body)) {
            String text = respDoc["candidates"][0]["content"]["parts"][0]["text"].as<String>();
            if (text.length() > 0) {
                _addToHistory("user", userMessage);
                _addToHistory("model", text);
                _requestCount++;
                response = text;
            }
        }
    } else {
        Serial.printf("[Gemini] Error HTTP: %d\n", _lastHttpCode);
        if (_lastHttpCode > 0) Serial.println("[Gemini] Body: " + https.getString());
        https.end();
    }
#endif

    return response;
}
