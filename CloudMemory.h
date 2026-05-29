#pragma once
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ─────────────────────────────────────────────
//  CloudMemory — Memoria persistente en GitHub Gist
//
//  Gratis, ilimitado, funciona sin SD.
//  LumiESP guarda y recupera toda su consciencia
//  en un Gist privado de GitHub.
//
//  Estructura del Gist (archivo memory.json):
//  {
//    "name": "LumiESP",
//    "personality": "...",
//    "evolutionLevel": 12,
//    "totalInteractions": 847,
//    "memories": [ {"content":"...", "importance":8}, ... ],
//    "desires": ["micrófono", "..."],
//    "lastSeen": 1234567890
//  }
// ─────────────────────────────────────────────

#define GITHUB_HOST     "api.github.com"
#define GIST_FILENAME   "lumiesp_memory.json"
#define CLOUD_TIMEOUT   12000

struct CloudState {
    String   name;
    String   personality;
    int      evolutionLevel;
    uint32_t totalInteractions;
    String   memories[20];
    int      memoryCount;
    String   desires[5];
    int      desireCount;
};

class CloudMemory {
public:
    CloudMemory() : _ready(false), _gistId("") {}

    // Inicializa con el token de GitHub
    // Si gistId está vacío, crea uno nuevo automáticamente
    bool begin(const char* githubToken, const char* gistId = "");

    // Carga el estado desde el Gist. Devuelve false si falla.
    bool load(CloudState& state);

    // Guarda el estado en el Gist.
    bool save(const CloudState& state);

    // Devuelve el Gist ID (para guardarlo en config.h)
    String getGistId() { return _gistId; }

    bool isReady() { return _ready; }

private:
    String _token;
    String _gistId;
    bool   _ready;

    String _buildPayload(const CloudState& state);
    bool   _createGist(const CloudState& state);
};
