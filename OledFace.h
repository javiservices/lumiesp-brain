#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Consciousness.h"

// ─────────────────────────────────────────────
//  OledFace — La cara visual de LumiESP
//  128x64 pixels, I2C SSD1306
//
//  Zonas de la pantalla:
//  ┌────────────────────────────┐
//  │  NOMBRE     [EMOCION ICON] │  ← fila 0  (8px)
//  │────────────────────────────│
//  │                            │
//  │       CARA / EMOJI         │  ← zona central (32px)
//  │                            │
//  │────────────────────────────│
//  │  texto scrolling...        │  ← fila abajo (16px)
//  └────────────────────────────┘
// ─────────────────────────────────────────────

#define SCREEN_W    128
#define SCREEN_H    64
#define OLED_ADDR   0x3C

class OledFace {
public:
    OledFace();

    // Inicializa la pantalla. Devuelve false si no se detecta.
    bool begin(int sda, int scl, const String& name);

    // Actualiza la pantalla con el estado actual
    void update(EmotionState emotion, const String& name);

    // Muestra un texto en la zona inferior (scroll automático)
    void showText(const String& text, bool isResponse = false);

    // Muestra pantalla de arranque / splash
    void splash(const String& name);

    // Muestra "pensando..." animado
    void showThinking();

    bool isReady() { return _ready; }

private:
    Adafruit_SSD1306 _display;
    bool _ready;
    String _scrollText;
    int _scrollOffset;
    EmotionState _lastEmotion;

    void _drawFace(EmotionState emotion);
    void _drawStatus(const String& name, EmotionState emotion);
    void _drawTextZone();

    // Caras por emoción (bitmaps 32x16 en texto)
    void _drawEyes(EmotionState emotion, int x, int y);
    void _drawMouth(EmotionState emotion, int x, int y);
};
