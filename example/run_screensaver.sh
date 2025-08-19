#!/usr/bin/env bash
set -euo pipefail

# Uso: ./examples/run_screensaver.sh [N] [ANCHOxALTO] [FRAMES] [BENCH_CSV]
# - N: número de caracteres (def: 200)
# - ANCHOxALTO: resolución, ej. 1024x768 (def: 800x600)
# - FRAMES: nº de frames a registrar si se activa benchmark (def: 0 = ilimitado)
# - BENCH_CSV: ruta CSV para registrar métricas (def: vacío = no registrar)
#
# Ejemplos:
#   ./example/run_screensaver.sh 300 1280x720 300 bench/bench_$(date +%Y%m%d_%H%M%S).csv
#   OMP_NUM_THREADS=8 ./example/run_screensaver.sh 500 1024x768 600 bench/bench.csv
#   ./example/run_screensaver.sh 300 1280x720
#   ./example/run_screensaver.sh 

N="${1:-200}"
RES="${2:-800x600}"
FRAMES="${3:-0}"
BENCH="${4:-}"
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

echo "Ejecutando matrix_screensaver N=$N RES=$RES FRAMES=$FRAMES BENCH='${BENCH:-<none>}'"
pushd "$PROJECT_ROOT" >/dev/null

ARGS=("$N" "$RES")
if [[ -n "$BENCH" ]]; then
  ARGS+=("--bench" "$BENCH")
fi
if [[ "$FRAMES" != "0" ]]; then
  ARGS+=("--bench-frames" "$FRAMES")
fi

"$BUILD_DIR/matrix_screensaver" "${ARGS[@]}"
popd >/dev/null
