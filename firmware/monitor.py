#!/usr/bin/env python3
"""
monitor.py — Monitor serial inteligente para esp32-assistant-ia
- Por defecto conecta SIN resetear (LumiESP sigue donde estaba)
- Con --reset arranca desde el principio (útil tras flashear)
- Permite enviar mensajes escribiendo + Enter
- Muestra los datos limpios (filtra bytes basura del boot ROM)

Uso:
  python3 monitor.py           → conecta sin interrumpir a LumiESP
  python3 monitor.py --reset   → resetea y ve el arranque completo
"""
import serial
import threading
import sys
import time

PORT = "/dev/ttyUSB0"
BAUD = 115200

def reset_esp32(ser):
    """Reset via RTS/DTR"""
    ser.setDTR(False)
    ser.setRTS(True)
    time.sleep(0.2)
    ser.setRTS(False)
    time.sleep(0.1)

def read_serial(ser, skip_garbage=False):
    """Hilo de lectura continua"""
    buffer = b""
    boot_garbage = skip_garbage  # Solo filtrar basura si venimos de un reset
    boot_timeout = time.time() + 2

    while True:
        try:
            byte = ser.read(1)
            if not byte:
                continue

            # Ignorar bytes de basura del boot ROM (primeros 2 segundos)
            if boot_garbage:
                if time.time() > boot_timeout:
                    boot_garbage = False
                    buffer = b""
                continue

            buffer += byte

            if byte == b'\n':
                try:
                    line = buffer.decode('utf-8', errors='replace').rstrip()
                    if line:
                        print(line)
                except:
                    pass
                buffer = b""

        except serial.SerialException:
            print("\n[monitor.py] Puerto desconectado")
            break
        except KeyboardInterrupt:
            break

def main():
    do_reset = "--reset" in sys.argv

    print("=" * 50)
    print("  esp32-assistant-ia — Monitor interactivo")
    print("  Escribe tu mensaje y pulsa Enter")
    if do_reset:
        print("  Modo: RESET (LumiESP reiniciará)")
    else:
        print("  Modo: SILENCIOSO (LumiESP continúa donde estaba)")
    print("  Ctrl+C para salir")
    print("=" * 50)

    try:
        ser = serial.Serial(PORT, BAUD, timeout=0.1, dsrdtr=False, rtscts=False)
    except serial.SerialException as e:
        print(f"[Error] No se puede abrir {PORT}: {e}")
        sys.exit(1)

    # Sin reset: asegurar que RTS/DTR no toquen el chip
    if not do_reset:
        ser.setDTR(False)
        ser.setRTS(False)

    time.sleep(0.3)

    # Inicia hilo de lectura
    t = threading.Thread(target=read_serial, args=(ser, do_reset), daemon=True)
    t.start()

    if do_reset:
        print(f"[monitor.py] Reseteando ESP32 en {PORT}...")
        reset_esp32(ser)
        print("[monitor.py] ESP32 reiniciando...\n")
    else:
        print(f"[monitor.py] Conectado a {PORT} — LumiESP sigue corriendo\n")

    # Bucle de entrada del usuario
    try:
        while True:
            msg = input()
            if msg:
                ser.write((msg + "\n").encode('utf-8'))
    except KeyboardInterrupt:
        pass
    finally:
        ser.close()
        print("\n[monitor.py] Desconectado.")

if __name__ == "__main__":
    main()
