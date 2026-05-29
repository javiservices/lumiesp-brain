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
#define GEMINI_MODEL    "gemini-2.0-flash"

#define GROQ_HOST       "api.groq.com"
#define GROQ_MODEL      "openai/gpt-oss-120b"

// Ollama: host y model se definen en config.h
#ifdef AI_BACKEND_OLLAMA
  #ifndef OLLAMA_HOST
    #define OLLAMA_HOST  "192.168.1.100"
  #endif
  #ifndef OLLAMA_PORT
    #define OLLAMA_PORT  11434
  #endif
  #ifndef OLLAMA_MODEL
    #define OLLAMA_MODEL "llama3.2:3b"
  #endif
#endif

#define GEMINI_TIMEOUT  20000
#define MAX_HISTORY     20

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
    // Actualiza solo el system prompt SIN borrar el historial de conversación
    void setSystemPrompt(const String& prompt);
    // Pregunta única con system prompt propio, no afecta al historial principal
    String askOneShot(const String& userMessage, const String& systemPrompt);

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
