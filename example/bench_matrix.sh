#!/usr/bin/env bash
set -euo pipefail

# Ejecuta una batería de pruebas y registra métricas en un CSV con timestamp.
# Uso: ./examples/bench_matrix.sh [N] [ANCHOxALTO] [FRAMES]
# Ejemplo:
#   ./example/bench_matrix.sh 2000 1024x768 300


N="${1:-2000}"
RES="${2:-1024x768}"
FRAMES="${3:-300}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
BENCH_DIR="$PROJECT_ROOT/bench"

mkdir -p "$BUILD_DIR" "$BENCH_DIR"

TS="$(date +%Y%m%d_%H%M%S)"
CSV="$BENCH_DIR/bench_${TS}.csv"

echo "Compilando…"
pushd "$BUILD_DIR" >/dev/null
cmake ..
make -j
popd >/dev/null

run() {
  local mode="$1"
  shift
  echo ">> ${mode} :: $*"
  "$BUILD_DIR/matrix_screensaver" "$N" "$RES" --mode "$mode" --bench "$CSV" --bench-frames "$FRAMES" "$@"
}

# --- Pruebas: 16 ejecuciones (>=10) ---
# Secuencial (base)
run rain   --seq
run bounce --seq
run spiral --seq
run nebula --seq

# Paralelo con distintos hilos
for T in 2 4 8; do
  run rain   --threads "$T"
  run bounce --threads "$T"
  run spiral --threads "$T"
  run nebula --threads "$T"
done

echo "Archivo generado: $CSV"