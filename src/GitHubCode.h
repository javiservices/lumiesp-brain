#pragma once
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ─────────────────────────────────────────────
//  GitHubCode — LumiESP escribe su propio código
//
//  Capacidades:
//   • Sube sus archivos fuente al repo en cada evolución
//   • Crea Issues con peticiones de hardware/features
//   • Puede proponer parches de código vía la IA
//   • El usuario hace git pull → flash → LumiESP mejoró
//
//  Flujo de auto-evolución:
//   1. LumiESP detecta que necesita algo (sensor, feature)
//   2. Pide a la IA que genere el código necesario
//   3. Sube el parche al repo como commit en rama "proposals"
//   4. Crea un Issue explicando qué y por qué
//   5. El usuario revisa, aprueba con un merge manual o auto
// ─────────────────────────────────────────────

#define GITHUB_CODE_HOST "api.github.com"
#define CODE_TIMEOUT     15000

struct CodeProposal {
    String filename;     // ej: "SoundModule.cpp"
    String content;      // código completo
    String description;  // por qué lo quiere LumiESP
};

class GitHubCode {
public:
    GitHubCode() : _ready(false) {}

    // token: ghp_...   repo: "usuario/nombre-repo"
    bool begin(const char* token, const char* repo);

    // Sube todos los archivos fuente actuales al repo (rama main)
    // Llámalo en cada evolución para tener historial
    bool pushSnapshot(const String& commitMessage);

    // LumiESP propone un nuevo archivo/módulo
    // Lo sube a rama "proposals" y crea un Issue
    bool proposeCode(const CodeProposal& proposal);

    // Crea un Issue en el repo (petición de hardware, idea, bug)
    bool createIssue(const String& title, const String& body, const String& label = "enhancement");

    // Lee el último Issue abierto (para saber qué le respondió el usuario)
    String getLatestIssueComment();

    // Lee el contenido de un archivo del repo (decodificado desde base64)
    String readFile(const String& path, const String& branch = "main");

    // Sube los archivos fuente al repo (llamar una vez al inicio)
    bool pushSourceFiles();

    bool isReady() { return _ready; }

private:
    String _token;
    String _repo;      // "usuario/nombre-repo"
    bool   _ready;

    // GET/PATCH/PUT helpers
    String _apiGet(const String& path);
    int    _apiPut(const String& path, const String& body);
    int    _apiPost(const String& path, const String& body);

    // Obtiene el SHA de un archivo (necesario para actualizarlo)
    String _getFileSha(const String& path, const String& branch = "main");

    // Codifica en base64 (GitHub API requiere contenido en b64)
    String _base64Encode(const String& input);
};
