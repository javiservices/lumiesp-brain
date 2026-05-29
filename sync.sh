#!/bin/bash
# sync.sh — Sincroniza el firmware del ESP32 con el repo
# Uso: ./sync.sh [mensaje]
#
# Copia todos los archivos fuente desde ~/esp32-assistant-ia/
# al repo local y hace commit+push automáticamente.
# config.h está en .gitignore (tiene credenciales).

SRC="/home/jsaenz/esp32-assistant-ia"
DST="$(dirname "$0")/firmware"
MSG="${1:-sync: actualizar firmware}"

CYAN='\033[0;36m'; GREEN='\033[0;32m'; RED='\033[0;31m'; NC='\033[0m'

echo -e "${CYAN}═══ LumiESP Sync ═══${NC}"

# Copiar fuentes (sin config.h)
for f in *.ino *.cpp *.h *.py *.sh; do
    [ "$f" = "config.h" ] && continue
    [ -f "$SRC/$f" ] && cp "$SRC/$f" "$DST/$f"
done
echo -e "  Fuentes copiados → firmware/"

# Git
cd "$(dirname "$0")"
git add -A
if git diff --cached --quiet; then
    echo -e "  ${GREEN}Sin cambios que sincronizar${NC}"
    exit 0
fi

git commit -m "$MSG"
git push origin main 2>&1 | grep -E "main|error|rejected" | head -5

echo -e "  ${GREEN}✓ Repo actualizado${NC}"
