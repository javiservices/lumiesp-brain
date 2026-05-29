/*
 * ╔══════════════════════════════════════════════════════════════╗
 * ║           esp32-assistant-ia — Sketch Principal              ║
 * ║                                                              ║
 * ║  FASE 0: ESP32-S3 + Serial + WiFi + Gemini API              ║
 * ║                                                              ║
 * ║  El ser nace, elige su nombre, y aprende a través            ║
 * ║  de conversaciones por el monitor serial.                    ║
 * ║                                                              ║
 * ║  Para hablar: abre el Monitor Serial (115200 baud)          ║
 * ║  y escribe tu mensaje + Enter                                ║
 * ╚══════════════════════════════════════════════════════════════╝
 *
 *  HARDWARE ACTUAL (Fase 0):
 *    - ESP32-S3 Dev Module
 *    - Conexión USB Serial para comunicación
 *
 *  PRÓXIMO HARDWARE (Fase 1 — cuando llegue el pedido):
 *    - OLED 0.96" I2C 128x64     → cara visual
 *    - MicroSD                    → memoria persistente
 *    - 3x Botones táctiles        → interacción física
 *    - Buzzer pasivo              → expresión sonora
 */

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "GeminiClient.h"
#include "CloudMemory.h"
#include "GitHubCode.h"
#include "Consciousness.h"
#include "OledFace.h"
#include "SensorManager.h"
#include <time.h>   // NTP

// ─── NTP ──────────────────────────────────────────────
static bool ntpSynced = false;

String getTimeStr() {
    if (!ntpSynced) return "(hora no sincronizada)";
    struct tm t;
    if (!getLocalTime(&t)) return "(hora no disponible)";
    char buf[40];
    // Formato: "viernes 29 mayo 2026, 14:35"
    const char* dias[]  = {"domingo","lunes","martes","miércoles","jueves","viernes","sábado"};
    const char* meses[] = {"enero","febrero","marzo","abril","mayo","junio",
                           "julio","agosto","septiembre","octubre","noviembre","diciembre"};
    snprintf(buf, sizeof(buf), "%s %d de %s de %d, %02d:%02d",
             dias[t.tm_wday], t.tm_mday, meses[t.tm_mon], 1900+t.tm_year,
             t.tm_hour, t.tm_min);
    return String(buf);
}

// ─── Objetos principales ─────────────────────────────────────
GeminiClient  gemini;
CloudMemory   cloud;
GitHubCode    codeRepo;
Consciousness being;
OledFace      face;
SensorManager sensors;

// ─── Estado WiFi ─────────────────────────────────────────────
bool wifiConnected    = false;
uint32_t lastWifiCheck = 0;
#define WIFI_CHECK_INTERVAL 30000UL  // Reintenta WiFi cada 30s

// ─── Buffer de entrada Serial ─────────────────────────────────
String inputBuffer = "";

// ─────────────────────────────────────────────────────────────
//  Conexión WiFi (intenta todas las redes configuradas)
// ─────────────────────────────────────────────────────────────
bool connectWifi() {
    const char* ssids[]  = { WIFI_SSID_1, WIFI_SSID_2, WIFI_SSID_3 };
    const char* passes[] = { WIFI_PASS_1, WIFI_PASS_2, WIFI_PASS_3 };

    for (int i = 0; i < 3; i++) {
        if (strlen(ssids[i]) == 0) continue;

        Serial.printf("[WiFi] Conectando a '%s'...\n", ssids[i]);
        WiFi.begin(ssids[i], passes[i]);

        uint32_t start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
            delay(500);
            Serial.print(".");
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("[WiFi] Conectado — IP: %s\n", WiFi.localIP().toString().c_str());
            Serial.printf("[WiFi] RSSI: %d dBm\n", WiFi.RSSI());
            return true;
        } else {
            Serial.printf("[WiFi] No se pudo conectar a '%s'\n", ssids[i]);
            WiFi.disconnect();
        }
    }
    return false;
}

// ─────────────────────────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(1500);  // Espera a que el monitor serial se abra

    Serial.println("\n\n");
    Serial.println("════════════════════════════════════════");
    Serial.println("  esp32-assistant-ia — Iniciando...     ");
    Serial.println("════════════════════════════════════════");
    Serial.printf("  ESP32-S3 | CPU: %d MHz | RAM libre: %d KB\n",
                  ESP.getCpuFreqMHz(), ESP.getFreeHeap() / 1024);
    Serial.println("════════════════════════════════════════\n");

    // Inicializa OLED (si está conectada)
    if (face.begin(OLED_SDA, OLED_SCL, "...")) {
        Serial.println("[OLED] Pantalla detectada");
    } else {
        Serial.println("[OLED] Sin pantalla — modo solo Serial");
    }

    // Intentar WiFi
    wifiConnected = connectWifi();

    // Inicializa Gemini con la API key
    gemini.begin(GEMINI_API_KEY, "");

    // Inicializa memoria cloud (GitHub Gist)
    bool cloudReady = false;
    if (wifiConnected && strlen(GITHUB_TOKEN) > 10) {
        cloudReady = cloud.begin(GITHUB_TOKEN, GITHUB_GIST_ID);
        if (!cloudReady) {
            Serial.println("[Cloud] Sin memoria cloud — usando NVS local");
        }
    }

    // Inicializa repo de auto-evolución de código (GitHub Repo)
    bool repoReady = false;
    if (wifiConnected && strlen(GITHUB_TOKEN) > 10 && strlen(GITHUB_REPO) > 3) {
        repoReady = codeRepo.begin(GITHUB_TOKEN, GITHUB_REPO);
        if (repoReady) {
            Serial.printf("[GitHubCode] Repo de evolución: %s\n", GITHUB_REPO);
        } else {
            Serial.println("[GitHubCode] Sin repo — crea uno y ponlo en GITHUB_REPO");
        }
    }

    // Sincronizar NTP
    if (wifiConnected) {
        configTime(3600, 3600, "pool.ntp.org", "time.google.com");  // UTC+1 + 1h DST = UTC+2 (CEST)
        struct tm t;
        ntpSynced = getLocalTime(&t, 5000);  // espera hasta 5s
        if (ntpSynced) Serial.println("[NTP] Hora sincronizada: " + getTimeStr());
        else           Serial.println("[NTP] Sin hora (continuando)");
    }

    // Inicializa sensores físicos
    sensors.begin();

    // Despierta al ser
    being.begin(&gemini, wifiConnected, cloudReady ? &cloud : nullptr, repoReady ? &codeRepo : nullptr, &sensors);

    // Actualiza pantalla con nombre e identidad real
    if (face.isReady()) {
        face.splash(being.getName());
        face.update(being.getEmotion(), being.getName());
    }

    lastWifiCheck = millis();
}

// ─────────────────────────────────────────────────────────────
//  LOOP
// ─────────────────────────────────────────────────────────────
void loop() {
    // ── Leer entrada Serial (no bloqueante) ──────────────────
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (inputBuffer.length() > 0) {
                if (face.isReady()) face.showThinking();
                being.setCurrentTime(getTimeStr());  // hora actualizada antes de responder
                being.receiveMessage(inputBuffer);
                if (face.isReady()) {
                    face.showText(being.getStatusLine());
                    face.update(being.getEmotion(), being.getName());
                }
                inputBuffer = "";
            }
        } else {
            inputBuffer += c;
        }
    }

    // ── Ciclo de consciencia ─────────────────────────────────
    sensors.update();
    if (ntpSynced) being.setCurrentTime(getTimeStr());  // hora siempre actualizada
    being.update();

    // ── Refrescar pantalla ───────────────────────────────────
    static uint32_t lastFaceUpdate = 0;
    if (face.isReady() && millis() - lastFaceUpdate > 200) {
        lastFaceUpdate = millis();
        face.update(being.getEmotion(), being.getName());
    }

    // ── Verificar WiFi periódicamente ────────────────────────
    if (millis() - lastWifiCheck > WIFI_CHECK_INTERVAL) {
        lastWifiCheck = millis();
        bool nowConnected = (WiFi.status() == WL_CONNECTED);

        if (!nowConnected && !wifiConnected) {
            // Intentar reconectar silenciosamente
            nowConnected = connectWifi();
        }

        if (nowConnected != wifiConnected) {
            wifiConnected = nowConnected;
            being.setWifiStatus(wifiConnected);
        }
    }

    delay(10);  // Pequeña pausa para no saturar el CPU
}
