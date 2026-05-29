#pragma once
#include <Arduino.h>
#include "config.h"

#if HW_DHT22
#include <DHT.h>
#endif

// ─────────────────────────────────────────────────────────────
//  SensorManager — Lee todos los sensores físicos conectados
//  y expone sus valores para el system prompt y la consciencia.
// ─────────────────────────────────────────────────────────────

struct SensorData {
    bool    dht22_ok      = false;
    float   temperature   = NAN;   // °C
    float   humidity      = NAN;   // %
    uint32_t lastRead     = 0;     // millis() del último read
};

class SensorManager {
public:
    SensorManager();
    void    begin();
    void    update();          // llamar en loop(), lee cada 5s

    bool    hasDHT22()   const { return _dhtOk; }
    float   temperature() const { return _data.temperature; }
    float   humidity()    const { return _data.humidity; }

    // Descripción textual para el system prompt
    String  summary() const;

    const SensorData& data() const { return _data; }

private:
    SensorData _data;
    bool       _dhtOk  = false;
    uint32_t   _lastUpdate = 0;
#if HW_DHT22
    DHT        _dht;
#endif
};
