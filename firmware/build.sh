#!/bin/bash
# ═══════════════════════════════════════════════════════════════
#  build.sh — Compilar y subir al ESP32-S3
#  Uso: ./build.sh [upload|compile|monitor|fresh]
#   - compile  → solo compila (por defecto)
#   - upload   → compila y sube al ESP32
#   - monitor  → abre el monitor serial
#   - fresh    → sube + abre monitor (el flujo completo)
# ═══════════════════════════════════════════════════════════════

SKETCH="esp32-assistant-ia.ino"
FQBN="esp32:esp32:esp32"
PORT="/dev/ttyUSB0"
BAUD=115200

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

ACTION=${1:-compile}

echo -e "${CYAN}═══════════════════════════════════════════${NC}"
echo -e "${CYAN}  esp32-assistant-ia — Build Script        ${NC}"
echo -e "${CYAN}═══════════════════════════════════════════${NC}"
echo -e "  Acción: ${YELLOW}$ACTION${NC}"
echo -e "  Placa:  $FQBN"
echo -e "  Puerto: $PORT"
echo ""

# Verificar que existe el sketch
if [ ! -f "$SKETCH" ]; then
    echo -e "${RED}Error: No se encuentra $SKETCH${NC}"
    exit 1
fi

# Verificar arduino-cli
if ! command -v arduino-cli &> /dev/null; then
    echo -e "${RED}Error: arduino-cli no encontrado${NC}"
    exit 1
fi

compile() {
    echo -e "${YELLOW}Compilando...${NC}"
    arduino-cli compile \
        --fqbn "$FQBN" \
        --libraries . \
        "$SKETCH"
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Compilación exitosa${NC}"
    else
        echo -e "${RED}✗ Error de compilación${NC}"
        exit 1
    fi
}

upload() {
    echo -e "${YELLOW}Subiendo al ESP32-S3 en $PORT...${NC}"
    arduino-cli upload \
        --fqbn "$FQBN" \
        --port "$PORT" \
        "$SKETCH"
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Upload exitoso${NC}"
    else
        echo -e "${RED}✗ Error en upload${NC}"
        echo -e "  Comprueba que el ESP32 está conectado en $PORT"
        echo -e "  Prueba con: ls /dev/ttyUSB*"
        exit 1
    fi
}

monitor() {
    echo -e "${YELLOW}Abriendo monitor serial ($BAUD baud)...${NC}"
    echo -e "${CYAN}[Ctrl+C para salir]${NC}"
    echo ""
    arduino-cli monitor --port "$PORT" --config "baudrate=$BAUD"
}

case "$ACTION" in
    compile)
        compile
        ;;
    upload)
        compile && upload
        ;;
    monitor)
        monitor
        ;;
    fresh)
        compile && upload
        sleep 3  # Espera al reinicio del ESP32
        monitor
        ;;
    *)
        echo -e "${RED}Acción no reconocida: $ACTION${NC}"
        echo "Uso: ./build.sh [compile|upload|monitor|fresh]"
        exit 1
        ;;
esac
