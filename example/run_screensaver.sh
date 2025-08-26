#!/usr/bin/env bash
set -euo pipefail

# Uso: ./examples/run_screensaver.sh [N] [ANCHOxALTO] [FRAMES] [BENCH_CSV] [MODE]
# - N: numero de caracteres (def: 200)
# - ANCHOxALTO: resolucion, ej. 1024x768 (def: 800x600)
# - FRAMES: frames a registrar si bench (def: 0 = ilimitado)
# - BENCH_CSV: ruta CSV (def: vacio = no bench)
# - MODE: rain|bounce|spiral|nebula (def: rain)

N="${1:-200}"
RES="${2:-800x600}"
FRAMES="${3:-0}"
BENCH="${4:-}"
MODE="${5:-rain}"

WIDTH="${RES%x*}"
HEIGHT="${RES#*x}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"

mkdir -p "$BUILD_DIR"
pushd "$BUILD_DIR" >/dev/null
cmake ..
make -j
popd >/dev/null

mkdir -p "$PROJECT_ROOT/bench"

echo "Ejecutando matrix_screensaver N=$N RES=$RES FRAMES=$FRAMES MODE=$MODE BENCH='${BENCH:-<none>}''"
pushd "$PROJECT_ROOT" >/dev/null

ARGS=("$N" "$RES" "--mode" "$MODE")
if [[ -n "$BENCH" ]]; then
  ARGS+=("--bench" "$BENCH")
fi
if [[ "$FRAMES" != "0" ]]; then
  ARGS+=("--bench-frames" "$FRAMES")
fi

"$BUILD_DIR/matrix_screensaver" "${ARGS[@]}"
popd >/dev/null