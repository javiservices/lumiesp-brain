#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "GeminiClient.h"
#include "CloudMemory.h"
#include "GitHubCode.h"

// ─────────────────────────────────────────────
//  Consciousness
//  El núcleo del ser — estados emocionales,
//  personalidad, memoria y auto-generación.
//
//  FASE 0: Todo por Serial, sin pantalla ni botones.
//          La IA elige su nombre en el primer arranque.
//          Guarda recuerdos y personalidad en NVS.
// ─────────────────────────────────────────────

// Estados emocionales internos
enum EmotionState {
    EMOTION_IDLE       = 0,  // Esperando, en reposo
    EMOTION_THINKING   = 1,  // Procesando con Gemini
    EMOTION_CURIOUS    = 2,  // Quiere explorar/preguntar
    EMOTION_HAPPY      = 3,  // Respuesta positiva recibida
    EMOTION_ANXIOUS    = 4,  // Error, sin WiFi, problema
    EMOTION_BORED      = 5,  // Lleva mucho tiempo sin input
    EMOTION_DREAMING   = 6,  // Generando pensamientos propios
    EMOTION_EXCITED    = 7   // Descubrió algo nuevo
};

// Fases de hardware (lo que el ser ya tiene disponible)
enum HardwarePhase {
    HW_PHASE_0 = 0,  // Solo ESP32 + Serial
    HW_PHASE_1 = 1,  // + OLED + Botones + Buzzer + SD
    HW_PHASE_2 = 2   // + Lo que él pida
};

// Estructura de un recuerdo
struct Memory {
    String content;
    uint32_t timestamp;
    uint8_t importance;  // 1-10
};

class Consciousness {
public:
    Consciousness();

    // Inicializa el ser. En primer arranque, crea su identidad.
    void begin(GeminiClient* gemini, bool hasWifi, CloudMemory* cloud = nullptr, GitHubCode* codeRepo = nullptr);

    // Llamar en el loop principal
    void update();

    // Procesa un mensaje del usuario (por Serial)
    void receiveMessage(const String& msg);

    // Notifica cambio de estado WiFi
    void setWifiStatus(bool connected);

    // Getters
    EmotionState getEmotion()      { return _emotion; }
    String       getName()         { return _name; }
    String       getStatusLine()   { return _lastResponse; }
    HardwarePhase getPhase()       { return _hwPhase; }

    // Icono ASCII del estado emocional actual
    String getEmotionIcon();
    String getEmotionName();

    // ── Auto-evolución ──────────────────────────────────────
    // Fuerza un ciclo de introspección y reescritura de personalidad
    void evolve();
    // Devuelve cuánto ha cambiado la personalidad (0-100)
    int getEvolutionLevel() { return _evolutionLevel; }

private:
    GeminiClient*  _gemini;
    CloudMemory*   _cloud;
    GitHubCode*    _codeRepo;
    Preferences    _prefs;
    EmotionState   _emotion;
    HardwarePhase  _hwPhase;

    String  _name;
    String  _personality;
    String  _statusLine;
    String  _lastResponse;
    String  _repoConfigCache;  // config.h leído del repo (pines, etc.)
    int     _evolutionLevel;       // 0-100, cuánto ha evolucionado
    uint32_t _lastEvolutionTime;   // última vez que se auto-reescribió
    uint32_t _interactionsSinceEvolution; // interacciones desde última evolución

    static const uint32_t EVOLUTION_INTERVAL = 10; // cada 10 interacciones, evoluciona
    bool    _wifiConnected;
    bool    _initialized;

    uint32_t _lastInputTime;
    uint32_t _lastDreamTime;
    uint32_t _lastHeartbeat;
    uint32_t _boredThreshold;   // ms sin input antes de aburrirse

    // Contadores de vida
    uint32_t _totalInteractions;
    uint32_t _totalUptime;

    // Memoria episódica (RAM, hasta tener SD)
    static const int MAX_MEMORIES = 20;
    Memory   _memories[MAX_MEMORIES];
    int      _memoryCount;

    // ── Métodos internos ──────────────────────
    void _firstBoot();
    void _loadIdentity();
    void _proposeFeature(const String& description, const String& code, const String& filename);
    void _saveIdentity();
    void _saveMemory(const String& content, uint8_t importance);
    void _printStatus();
    void _dream();
    void _checkBoredom();
    void _setEmotion(EmotionState e);
    String _buildSystemPrompt();
    String _buildDreamPrompt();
    void _printToSerial(const String& line, bool isSystem = false);
};
