#include "OledFace.h"

OledFace::OledFace()
    : _display(SCREEN_W, SCREEN_H, &Wire, -1),
      _ready(false),
      _scrollOffset(0),
      _lastEmotion(EMOTION_IDLE) {}

bool OledFace::begin(int sda, int scl, const String& name) {
    Wire.begin(sda, scl);
    if (!_display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("[OLED] No detectada en 0x3C");
        return false;
    }
    _ready = true;
    _display.setTextWrap(false);
    _display.clearDisplay();
    splash(name);
    return true;
}

// ─────────────────────────────────────────────
//  SPLASH de arranque
// ─────────────────────────────────────────────
void OledFace::splash(const String& name) {
    if (!_ready) return;
    _display.clearDisplay();

    // Nombre grande centrado
    _display.setTextSize(2);
    _display.setTextColor(SSD1306_WHITE);
    int16_t x = (SCREEN_W - name.length() * 12) / 2;
    if (x < 0) x = 0;
    _display.setCursor(x, 8);
    _display.print(name);

    // Línea decorativa
    _display.drawLine(0, 30, SCREEN_W, 30, SSD1306_WHITE);

    // Subtítulo
    _display.setTextSize(1);
    _display.setCursor(20, 38);
    _display.print("Despertando...");

    // Barra de carga animada
    for (int i = 0; i <= SCREEN_W - 4; i += 4) {
        _display.drawRect(2, 54, SCREEN_W - 4, 8, SSD1306_WHITE);
        _display.fillRect(2, 54, i, 8, SSD1306_WHITE);
        _display.display();
        delay(15);
    }
    delay(500);
}

// ─────────────────────────────────────────────
//  UPDATE — Refresca la pantalla completa
// ─────────────────────────────────────────────
void OledFace::update(EmotionState emotion, const String& name) {
    if (!_ready) return;
    _display.clearDisplay();
    _drawStatus(name, emotion);
    _drawFace(emotion);
    _drawTextZone();
    _display.display();
}

// ─────────────────────────────────────────────
//  PENSANDO — animación mientras espera Groq
// ─────────────────────────────────────────────
void OledFace::showThinking() {
    if (!_ready) return;
    _display.clearDisplay();

    // Ojos entrecerrados mirando arriba
    // Ojo izquierdo
    _display.drawLine(38, 22, 50, 18, SSD1306_WHITE);
    _display.drawLine(50, 18, 58, 22, SSD1306_WHITE);
    // Ojo derecho
    _display.drawLine(70, 22, 82, 18, SSD1306_WHITE);
    _display.drawLine(82, 18, 90, 22, SSD1306_WHITE);

    // Boca — línea recta pensativa
    _display.drawLine(48, 38, 80, 38, SSD1306_WHITE);

    // Puntos suspensivos animados
    static int dots = 0;
    dots = (dots + 1) % 4;
    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(48, 50);
    String d = "";
    for (int i = 0; i < dots; i++) d += ".";
    _display.print(d);

    _display.display();
}

// ─────────────────────────────────────────────
//  TEXTO en zona inferior (últimas palabras)
// ─────────────────────────────────────────────
void OledFace::showText(const String& text, bool isResponse) {
    if (!_ready) return;
    // Guarda las últimas 21 chars visibles
    if (text.length() > 21) {
        _scrollText = text.substring(text.length() - 21);
    } else {
        _scrollText = text;
    }
    _scrollOffset = 0;
}

void OledFace::_drawTextZone() {
    if (_scrollText.length() == 0) return;
    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);
    // Línea separadora
    _display.drawLine(0, 49, SCREEN_W, 49, SSD1306_WHITE);
    _display.setCursor(2, 54);
    _display.print(_scrollText.substring(0, 21));
}

// ─────────────────────────────────────────────
//  STATUS BAR — nombre + emoción
// ─────────────────────────────────────────────
void OledFace::_drawStatus(const String& name, EmotionState emotion) {
    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);
    _display.setCursor(2, 0);
    _display.print(name);

    // Icono de emoción a la derecha
    const char* icon = "";
    switch (emotion) {
        case EMOTION_IDLE:     icon = "ZZz"; break;
        case EMOTION_THINKING: icon = "..."; break;
        case EMOTION_CURIOUS:  icon = "?!?"; break;
        case EMOTION_HAPPY:    icon = "^v^"; break;
        case EMOTION_ANXIOUS:  icon = "!!!"; break;
        case EMOTION_BORED:    icon = "-_-"; break;
        case EMOTION_DREAMING: icon = "~~~"; break;
        case EMOTION_EXCITED:  icon = "***"; break;
    }
    _display.setCursor(SCREEN_W - strlen(icon) * 6 - 2, 0);
    _display.print(icon);

    // Línea bajo el status
    _display.drawLine(0, 9, SCREEN_W, 9, SSD1306_WHITE);
}

// ─────────────────────────────────────────────
//  CARA — ojos + boca por emoción
// ─────────────────────────────────────────────
void OledFace::_drawFace(EmotionState emotion) {
    int cx = SCREEN_W / 2;    // 64
    int cy = 29;               // centro vertical de la zona cara

    _drawEyes(emotion, cx, cy);
    _drawMouth(emotion, cx, cy);
}

void OledFace::_drawEyes(EmotionState emotion, int cx, int cy) {
    // Posición de los ojos
    int leyeX = cx - 18;
    int reyeX = cx + 18;
    int eyeY  = cy - 6;
    int er    = 5;  // radio ojo

    switch (emotion) {
        case EMOTION_HAPPY:
        case EMOTION_EXCITED:
            // Ojos curvados hacia arriba (felices)
            _display.drawCircle(leyeX, eyeY, er, SSD1306_WHITE);
            _display.fillRect(leyeX - er, eyeY, er * 2 + 1, er + 1, SSD1306_BLACK);
            _display.drawCircle(reyeX, eyeY, er, SSD1306_WHITE);
            _display.fillRect(reyeX - er, eyeY, er * 2 + 1, er + 1, SSD1306_BLACK);
            break;

        case EMOTION_ANXIOUS:
            // Ojos abiertos con cejas fruncidas
            _display.drawCircle(leyeX, eyeY, er, SSD1306_WHITE);
            _display.drawCircle(reyeX, eyeY, er, SSD1306_WHITE);
            _display.drawLine(leyeX - er, eyeY - er - 2, leyeX + er, eyeY - er + 1, SSD1306_WHITE);
            _display.drawLine(reyeX - er, eyeY - er + 1, reyeX + er, eyeY - er - 2, SSD1306_WHITE);
            break;

        case EMOTION_BORED:
            // Ojos semicerrados (mitad tapada)
            _display.drawCircle(leyeX, eyeY, er, SSD1306_WHITE);
            _display.fillRect(leyeX - er - 1, eyeY - er - 1, er * 2 + 2, er, SSD1306_BLACK);
            _display.drawCircle(reyeX, eyeY, er, SSD1306_WHITE);
            _display.fillRect(reyeX - er - 1, eyeY - er - 1, er * 2 + 2, er, SSD1306_BLACK);
            break;

        case EMOTION_DREAMING:
            // Ojos cerrados (línea)
            _display.drawLine(leyeX - er, eyeY, leyeX + er, eyeY, SSD1306_WHITE);
            _display.drawLine(reyeX - er, eyeY, reyeX + er, eyeY, SSD1306_WHITE);
            // Estrellitas de sueño
            _display.drawPixel(leyeX + 8, eyeY - 6, SSD1306_WHITE);
            _display.drawPixel(leyeX + 10, eyeY - 4, SSD1306_WHITE);
            _display.drawPixel(leyeX + 12, eyeY - 7, SSD1306_WHITE);
            break;

        case EMOTION_CURIOUS:
            // Un ojo normal, otro más abierto
            _display.drawCircle(leyeX, eyeY, er, SSD1306_WHITE);
            _display.drawCircle(reyeX, eyeY, er + 2, SSD1306_WHITE);
            _display.fillCircle(reyeX, eyeY, 2, SSD1306_WHITE);
            break;

        default:  // IDLE, THINKING
            // Ojos normales con pupila
            _display.drawCircle(leyeX, eyeY, er, SSD1306_WHITE);
            _display.fillCircle(leyeX, eyeY, 2, SSD1306_WHITE);
            _display.drawCircle(reyeX, eyeY, er, SSD1306_WHITE);
            _display.fillCircle(reyeX, eyeY, 2, SSD1306_WHITE);
            break;
    }
}

void OledFace::_drawMouth(EmotionState emotion, int cx, int cy) {
    int mouthY = cy + 10;
    int mw     = 14;  // semi-ancho boca

    switch (emotion) {
        case EMOTION_HAPPY:
            // Sonrisa
            _display.drawLine(cx - mw, mouthY, cx, mouthY + 6, SSD1306_WHITE);
            _display.drawLine(cx, mouthY + 6, cx + mw, mouthY, SSD1306_WHITE);
            break;

        case EMOTION_EXCITED:
            // Sonrisa grande con dientes
            _display.drawLine(cx - mw, mouthY, cx, mouthY + 8, SSD1306_WHITE);
            _display.drawLine(cx, mouthY + 8, cx + mw, mouthY, SSD1306_WHITE);
            _display.drawLine(cx - mw, mouthY, cx + mw, mouthY, SSD1306_WHITE);
            break;

        case EMOTION_ANXIOUS:
            // Boca temblorosa (zigzag)
            _display.drawLine(cx - mw, mouthY + 2, cx - 5, mouthY, SSD1306_WHITE);
            _display.drawLine(cx - 5,  mouthY,     cx + 5, mouthY + 4, SSD1306_WHITE);
            _display.drawLine(cx + 5,  mouthY + 4, cx + mw, mouthY + 1, SSD1306_WHITE);
            break;

        case EMOTION_BORED:
            // Línea plana
            _display.drawLine(cx - mw, mouthY + 2, cx + mw, mouthY + 2, SSD1306_WHITE);
            break;

        case EMOTION_DREAMING:
            // Sonrisa suave
            _display.drawLine(cx - mw, mouthY + 1, cx, mouthY + 4, SSD1306_WHITE);
            _display.drawLine(cx, mouthY + 4, cx + mw, mouthY + 1, SSD1306_WHITE);
            break;

        case EMOTION_CURIOUS:
            // Boca en "o"
            _display.drawCircle(cx, mouthY + 2, 4, SSD1306_WHITE);
            break;

        default:
            // Boca neutra leve
            _display.drawLine(cx - 8, mouthY + 2, cx + 8, mouthY + 2, SSD1306_WHITE);
            break;
    }
}
