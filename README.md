# Matrix Screensaver — C++/SFML + OpenMP (Nebula + OBJ)

Proyecto de **Computación Paralela y Distribuida**: efecto de “lluvia de caracteres” estilo *Matrix* con **SFML**. 
Incluye **versión secuencial** y **paralela** con **OpenMP** (paralelizando la fase de `update`), un modo extra **Nebula** que renderiza un **modelo OBJ** en *wireframe*, y **scripts de benchmarking** que registran métricas por cuadro a CSV.

> Probado en Linux/WSL con **SFML 2.6.x**, **g++ (C++17)**, **OpenMP** y **OpenGL** (para Nebula/OBJ).

---

## 🗂️ Estructura real del repo

```
Parallel_Screen_Saver/
  .gitignore
  CMakeLists.txt
  DOCUMENTATION.md
  README.md
  bench_par.csv
  bench_seq.csv
  assets/
    fonts/
      Matrix-MZ4P.ttf
    models/
      center.obj
  bench/
    bench_20250819_173500.csv
    bench_20250819_174146.csv
    bench_20250826_164337.csv
    test_20250819_174238.csv
  example/
    bench_matrix.sh
    run_screensaver.sh
  include/
    ObjModel.h
    TextRender.h
  scripts/
    analyze_bench.cpp
  src/
    ObjModel.cpp
    TextRender.cpp
    main.cpp
```

Carpetas clave:
- `src/` — `main.cpp`, `TextRender.cpp`, `ObjModel.cpp`
- `include/` — `TextRender.h`, `ObjModel.h`
- `assets/fonts/Matrix-MZ4P.ttf` — fuente usada por los glifos
- `assets/models/center.obj` — modelo usado por el modo **Nebula**
- `example/` — scripts `run_screensaver.sh` y `bench_matrix.sh`
- `scripts/analyze_bench.cpp` — analizador del CSV (C++)

---

## ✨ Características
- **Matrix Rain** (columna con cabeza brillante y cola verde, *wrap* infinito por columna).
- Modos extra: **Bounce**, **Spiral** (partículas), y **Nebula** (wireframe de un OBJ).
- **OpenMP** en bucles de `update` (lluvia por columnas, líneas punteadas, partículas).
- **Render** monohilo (SFML) — importante para interpretar el **speedup** total.
- **CLI** flexible: N, resolución, modo, paleta, velocidad, hilos, benchmark.
- **Benchmark** a CSV por cuadro: `update_ms`, `render_ms`, `total_ms`, `fps` (+ `exec`, `threads_eff`).

---

## 📦 Requisitos

Ubuntu/WSL (ejemplo):
```bash
sudo apt update
sudo apt install -y build-essential cmake libsfml-dev libgl1-mesa-dev
# Opcional para headless/CI:
sudo apt install -y xvfb
```

---

## 🔧 Compilación y ▶️ Ejecución rápida

Desde la **raíz** del repo (para que encuentre `assets/`):

### Con script
```bash
# Sintaxis:
./example/run_screensaver.sh [N] [ANCHOxALTO] [FRAMES] [CSV] [MODE]

# Ejemplos:
./example/run_screensaver.sh                  # 200 @ 800x600, rain
./example/run_screensaver.sh 300 1280x720     # N=300, rain
./example/run_screensaver.sh 1000 1920x1080 300 bench/run.csv rain
```

---

## 🧰 CLI (opciones del programa)

```
Uso: matrix_screensaver [opciones] [N] [ANCHOxALTO]

Posicionales:
  N                     Número de caracteres (defecto: 200)
  ANCHOxALTO            Resolución, p.ej. 1024x768 (defecto: 800x600)

Opciones:
  --seq                 Fuerza modo secuencial
  --threads K           Sugerir K hilos (OpenMP)
  --mode rain|bounce|spiral|nebula   (defecto: rain)
  --palette mono|neon|rainbow        (defecto: mono)
  --speed V             Velocidad base px/s (defecto: 160)
  --bench FILE          CSV de benchmark
  --bench-frames K      Detener tras K frames
  -h, --help            Ayuda
```

Notas:
- **Nebula** carga `assets/models/center.obj` automáticamente; ajusta rotación/desplazamiento internamente.
- El *render* es monohilo (limitación SFML). El paralelismo acelera **update**, por lo que el **speedup total** depende de cuánto pese render en tu configuración (Amdahl).

---

## 🧪 Benchmarks y Bitácora (≥10 mediciones)

### Batería automática (12 corridas)
```bash
chmod +x example/bench_matrix.sh
./example/bench_matrix.sh 2000 1024x768 300
# -> bench/bench_YYYYmmdd_HHMMSS.csv
```

### Columnas esperadas en el CSV
```
exec,mode,threads_req,threads_eff,width,height,N,speed,frame,dt_s,update_ms,render_ms,total_ms,fps
```

### Analizador (C++)
```bash
g++ -std=c++17 -O2 -o scripts/analyze_bench scripts/analyze_bench.cpp
./scripts/analyze_bench bench/bench_YYYYmmdd_HHMMSS.csv
# -> bench/bench_summary.csv y bench/Anexo3_Bitacora.md
```

> También hay CSVs de ejemplo en la raíz: `bench_seq.csv`, `bench_par.csv`.

---

## 🧵 Paralelización (OpenMP) — Resumen técnico
- **Rain**: actualización por columnas + glifos (`collapse(2)`), *wrap* vertical sin huecos, *flicker* determinista (evita RNG compartido).
- **Dash lines**: avance y reposicionamiento paralelos.
- **Bounce/Spiral**: partículas independientes (loops paralelos).
- **Nebula**: wireframe basado en `ObjModel` (carga *OBJ*, normaliza, rasteriza aristas con `sf::VertexArray`).
- *Render* SFML se mantiene en el hilo principal.

---

## 🧯 Troubleshooting
- **No encuentra fuente**: `Failed to load font "assets/fonts/Matrix-MZ4P.ttf"` → ejecutar desde la raíz o verificar la ruta.
- **Avisos MESA/GLX**: son *warnings* típicos en WSL; si molestan, usa `xvfb-run`.
- **FPS bajo**: reduce tamaño de fuente, N, o desactiva temporalmente elementos costosos (p.ej., las líneas punteadas) para aislar `update`.

---

## 👥 Créditos
- **SFML** (render 2D).
- **OpenMP** (paralelización en CPU).
- **OpenGL** (soporte requerido para el modo Nebula/OBJ vía toolchain).
- Inspiración visual: *The Matrix*.
