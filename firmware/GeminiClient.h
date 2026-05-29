#pragma once
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ─────────────────────────────────────────────
//  GeminiClient — Soporta Gemini (Google) y Groq (Meta Llama)
//  Selecciona el backend según config.h
// ─────────────────────────────────────────────

// Backend seleccionado en config.h:
//   AI_BACKEND_GEMINI  → Google Gemini 2.5 Flash (gratis con límites)
//   AI_BACKEND_GROQ    → Groq + Llama3 (gratis, sin restricciones IP)

#define GEMINI_HOST     "generativelanguage.googleapis.com"
#define GEMINI_MODEL    "gemini-2.5-flash"

#define GROQ_HOST       "api.groq.com"
#define GROQ_MODEL      "openai/gpt-oss-120b"

#define GEMINI_TIMEOUT  20000
#define MAX_HISTORY     10

struct Message {
    String role;
    String text;
};

class GeminiClient {
public:
    GeminiClient() : _historyCount(0), _requestCount(0) {}

    void begin(const char* apiKey, const String& systemPrompt);
    String ask(const String& userMessage);
    void clearHistory();

    int getRequestCount() { return _requestCount; }
    int getLastHttpCode() { return _lastHttpCode; }

private:
    String _apiKey;
    String _systemPrompt;
    Message _history[MAX_HISTORY];
    int _historyCount;
    int _requestCount;
    int _lastHttpCode;

    String _buildGeminiPayload(const String& userMessage);
    String _buildGroqPayload(const String& userMessage);
    void _addToHistory(const String& role, const String& text);
};
