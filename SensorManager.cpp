#include "SensorManager.h"

#define SENSOR_READ_INTERVAL 5000UL  // Leer cada 5 segundos

#if HW_DHT22
SensorManager::SensorManager() : _dht(HW_DHT22_PIN, DHT22) {}
#else
SensorManager::SensorManager() {}
#endif

void SensorManager::begin() {
#if HW_DHT22
    Serial.printf("[Sensors] Iniciando DHT22 en GPIO%d...\n", HW_DHT22_PIN);
    _dht.begin();
    delay(2000);  // El DHT22 necesita 2s tras el primer arranque

    float t = _dht.readTemperature();
    float h = _dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {
        _data.temperature = t;
        _data.humidity    = h;
        _data.dht22_ok    = true;
        _dhtOk            = true;
        _data.lastRead    = millis();
        Serial.printf("[Sensors] DHT22 OK — %.1f°C  %.0f%%HR\n", t, h);
    } else {
        Serial.println("[Sensors] DHT22 sin respuesta — verifica el cableado en GPIO" + String(HW_DHT22_PIN));
        _dhtOk = false;
    }
#endif
}

void SensorManager::update() {
    if (millis() - _lastUpdate < SENSOR_READ_INTERVAL) return;
    _lastUpdate = millis();

#if HW_DHT22
    float t = _dht.readTemperature();
    float h = _dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
        _data.temperature = t;
        _data.humidity    = h;
        _data.dht22_ok    = true;
        _dhtOk            = true;
        _data.lastRead    = millis();
    }
#endif
}

String SensorManager::summary() const {
    String s = "";
#if HW_DHT22
    if (_dhtOk && !isnan(_data.temperature)) {
        s += "DHT22 (GPIO" + String(HW_DHT22_PIN) + "): ";
        s += String(_data.temperature, 1) + "°C, ";
        s += String(_data.humidity, 0) + "% HR\n";
    } else {
        s += "DHT22 (GPIO" + String(HW_DHT22_PIN) + "): conectado pero sin lectura válida aún\n";
    }
#endif
    return s;
}
