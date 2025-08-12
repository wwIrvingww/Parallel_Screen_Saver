#!/usr/bin/env bash
set -euo pipefail

# Parámetros
N="${1:-200}"
RES="${2:-800x600}"
WIDTH="${RES%x*}"
HEIGHT="${RES#*x}"

# Directorios
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

echo "🏗  Preparando build en $BUILD_DIR…"
mkdir -p "$BUILD_DIR"
pushd "$BUILD_DIR" > /dev/null

# configura y compila
cmake .. 
make -j

popd > /dev/null

echo "Ejecutando matrix_screensaver con N=$N en ${RES}…"

pushd "$PROJECT_ROOT" > /dev/null
"$BUILD_DIR/matrix_screensaver" "$N" "$RES"
popd > /dev/null
