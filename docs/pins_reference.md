# LumiESP — Referencia de Pines

| Módulo | Pin | GPIO |
|--------|-----|------|
| OLED SDA | SDA | GPIO 21 |
| OLED SCL | SCL | GPIO 22 |
| SD CS | CS | GPIO 5 |
| SD SCK | SCK | GPIO 18 |
| SD MISO | MISO | GPIO 19 |
| SD MOSI | MOSI | GPIO 23 |
| Botón 1 | — | GPIO 34 |
| Botón 2 | — | GPIO 35 |
| Botón 3 | — | GPIO 32 |
| MAX98357A BCLK | — | GPIO 14 |
| MAX98357A LRC | — | GPIO 15 |
| MAX98357A DIN | — | GPIO 13 |
| INMP441 WS | — | GPIO 26 |
| INMP441 SCK | — | GPIO 27 |
| INMP441 SD | — | GPIO 33 |

## GPIOs libres
GPIO 4, 16, 17, 25 — disponibles para nuevos sensores

## Protocolo I2C
Bus I2C compartido: SDA=21, SCL=22. Dirección OLED: 0x3C

*Auto-generado por LumiESP*
