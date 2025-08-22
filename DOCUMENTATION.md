## Catálogo de Funciones por Archivo

### 1. `src/main.cpp`

#### Funciones CLI y Parsing

**`print_usage(const char* prog)`**
- **Entrada**: Nombre del programa
- **Salida**: void
- **Descripción**: Imprime las instrucciones de uso del programa en la consola estándar

**`parse_resolution(const std::string& s, int& w, int& h)`**
- **Entrada**: String con formato "ANCHOxALTO", referencias a ancho y alto
- **Salida**: bool (true si el parsing fue exitoso)
- **Descripción**: Parsea una cadena de resolución y extrae ancho y alto usando expresiones regulares

**`file_exists(const std::string& path)`**
- **Entrada**: Ruta del archivo
- **Salida**: bool (true si el archivo existe)
- **Descripción**: Verifica si un archivo existe en el sistema de archivos

**`parse_cli(int argc, char** argv, CliOptions& opts)`**
- **Entrada**: Argumentos de línea de comandos, estructura de opciones por referencia
- **Salida**: bool (true si el parsing fue exitoso)
- **Descripción**: Procesa todos los argumentos de línea de comandos y llena la estructura CliOptions

#### Funciones de Ejecución

**`run_loop(const CliOptions& opts, bool vsync)`**
- **Entrada**: Opciones de configuración, flag de sincronización vertical
- **Salida**: int (código de salida)
- **Descripción**: Bucle principal de renderizado, maneja eventos SFML, actualiza renderer y genera métricas de benchmark

**`run_sequential(const CliOptions& opts)`**
- **Entrada**: Opciones de configuración
- **Salida**: int (código de salida)
- **Descripción**: Ejecuta el programa en modo secuencial

**`run_parallel(const CliOptions& opts)`**
- **Entrada**: Opciones de configuración
- **Salida**: int (código de salida)
- **Descripción**: Ejecuta el programa en modo paralelo con OpenMP, configura número de hilos y sincronización

#### Funciones Auxiliares

**`mode_to_cstr(MotionMode m)`**
- **Entrada**: Enum MotionMode
- **Salida**: const char* (string literal)
- **Descripción**: Convierte el enum MotionMode a string para salida CSV

**`main(int argc, char** argv)`**
- **Entrada**: Argumentos de línea de comandos
- **Salida**: int (código de salida del programa)
- **Descripción**: Función principal, parsea argumentos y delega ejecución a modo secuencial o paralelo

### 2. `src/TextRender.cpp`

#### Constructor y Destructor

**`TextRender::TextRender(int N, const sf::Font& font, unsigned int charSize, sf::Vector2u windowSize, MotionMode mode, float speed, Palette palette)`**
- **Entrada**: Número de caracteres, fuente SFML, tamaño de carácter, tamaño de ventana, modo de movimiento, velocidad, paleta
- **Salida**: void (constructor)
- **Descripción**: Inicializa el renderer con configuración específica, genera números aleatorios thread-safe e inicializa partículas según el modo

#### Funciones Principales de Actualización

**`TextRender::update(float dt)`**
- **Entrada**: Delta time en segundos
- **Salida**: void
- **Descripción**: Actualiza la simulación según el modo activo (Rain, Bounce, Spiral), usa paralelización OpenMP

**`TextRender::render(sf::RenderWindow& window)`**
- **Entrada**: Referencia a ventana SFML
- **Salida**: void
- **Descripción**: Renderiza todos los elementos gráficos en la ventana según el modo activo

**`TextRender::resize(sf::Vector2u newSize)`**
- **Entrada**: Nuevo tamaño de ventana
- **Salida**: void
- **Descripción**: Recalcula elementos gráficos cuando cambia el tamaño de ventana

#### Funciones de Inicialización

**`TextRender::initParticles(int N, const sf::Font& font)`**
- **Entrada**: Número de partículas, fuente SFML
- **Salida**: void
- **Descripción**: Inicializa partículas para modos Bounce y Spiral con propiedades aleatorias thread-safe

**`TextRender::initRain(int approxTotalGlyphs, const sf::Font& font)`**
- **Entrada**: Número aproximado de glifos, fuente SFML
- **Salida**: void
- **Descripción**: Inicializa columnas de lluvia Matrix con caracteres distribuidos verticalmente

**`TextRender::initDashes(const sf::Font& font, int count)`**
- **Entrada**: Fuente SFML, número de líneas punteadas
- **Salida**: void
- **Descripción**: Inicializa líneas punteadas horizontales que rebotan en los bordes

#### Funciones de Actualización Específicas

**`TextRender::updateRain(float dt)`**
- **Entrada**: Delta time
- **Salida**: void
- **Descripción**: Actualiza lluvia Matrix con paralelización collapse(2), maneja wrap vertical infinito y efectos de parpadeo

**`TextRender::updateBounce(Particle& p, float dt)`**
- **Entrada**: Referencia a partícula, delta time
- **Salida**: void
- **Descripción**: Actualiza una partícula en modo bounce con detección de colisiones en bordes

**`TextRender::updateSpiral(Particle& p, float dt)`**
- **Entrada**: Referencia a partícula, delta time
- **Salida**: void
- **Descripción**: Actualiza una partícula en modo spiral con movimiento circular 3D y proyección perspectiva

**`TextRender::updateDashes(float dt)`**
- **Entrada**: Delta time
- **Salida**: void
- **Descripción**: Actualiza líneas punteadas con movimiento horizontal y rebote en bordes, usa paralelización collapse(2)

#### Funciones de Generación Aleatoria Thread-Safe

**`TextRender::threadSafeRand(float min, float max) const`**
- **Entrada**: Valores mínimo y máximo
- **Salida**: float aleatorio en el rango
- **Descripción**: Genera número aleatorio flotante thread-safe usando mutex y std::uniform_real_distribution

**`TextRender::threadSafeRandInt(int min, int max) const`**
- **Entrada**: Valores mínimo y máximo enteros
- **Salida**: int aleatorio en el rango
- **Descripción**: Genera número aleatorio entero thread-safe usando mutex y std::uniform_int_distribution

#### Funciones de Color

**`TextRender::generatePseudoRandomColor(int seed)`**
- **Entrada**: Semilla para generación determinística
- **Salida**: sf::Color
- **Descripción**: Genera color pseudoaleatorio basado en semilla para consistencia visual

**`TextRender::neonGreen(unsigned char a) const`**
- **Entrada**: Valor alpha
- **Salida**: sf::Color verde neón
- **Descripción**: Retorna color verde neón Matrix con alpha especificado

**`TextRender::headColor() const`**
- **Entrada**: void
- **Salida**: sf::Color blanco/verde claro
- **Descripción**: Retorna color para la cabeza de las columnas Matrix

#### Funciones Auxiliares Estáticas

**`frand(float a, float b)`**
- **Entrada**: Rango mínimo y máximo
- **Salida**: float aleatorio
- **Descripción**: Función auxiliar para generar números aleatorios flotantes (no thread-safe, uso legacy)

### 3. `include/TextRender.h`

#### Enums

**`enum class MotionMode { Bounce, Spiral, Rain }`**
- **Descripción**: Define los modos de animación disponibles

**`enum class Palette { Mono, Neon, Rainbow }`**
- **Descripción**: Define las paletas de color (actualmente solo aplicable a modo Rain)

#### Estructuras Privadas

**`struct Particle`**
- **Descripción**: Estructura para partículas individuales en modos Bounce y Spiral
- **Campos**: text, pos, vel, baseSize, angle, angVel, baseRadius, radiusAmp, z, zVel, phase

**`struct Drop`**
- **Descripción**: Estructura para columnas de lluvia Matrix
- **Campos**: glyphs (vector de sf::Text), x, headY, spacing

**`struct DashLine`**
- **Descripción**: Estructura para líneas punteadas horizontales
- **Campos**: dots (vector de sf::Text), xLeft, y, vx, spacing, dotWidth

### 4. `scripts/analyze_bench.cpp`

#### Estructuras de Datos

**`struct Row`**
- **Descripción**: Representa una fila del CSV de benchmark
- **Campos**: exec, mode, threads_req, threads_eff, width, height, N, frame, speed, dt_s, update_ms, render_ms, total_ms, fps

**`struct Group`**
- **Descripción**: Agrupa filas por configuración para cálculo de promedios
- **Campos**: Configuración base + acumuladores estadísticos

**`struct GroupVec`**
- **Descripción**: Vector dinámico de grupos
- **Campos**: data, size, cap

#### Funciones de Parsing

**`trim(char* s)`**
- **Entrada**: String a limpiar
- **Salida**: char* (mismo string limpio)
- **Descripción**: Elimina espacios en blanco al inicio y final de string

**`split_csv(char* line, char* out[], int max_tokens)`**
- **Entrada**: Línea CSV, array de salida, máximo de tokens
- **Salida**: int (número de tokens encontrados)
- **Descripción**: Divide línea CSV por comas en tokens separados

**`parse_row_from_tokens(Row* r, char* toks[], int n)`**
- **Entrada**: Estructura Row, array de tokens, número de tokens
- **Salida**: int (1 si exitoso, 0 si falla)
- **Descripción**: Convierte tokens CSV a estructura Row tipada

#### Funciones de Agrupación

**`same_key(const Group* g, const Row* r)`**
- **Entrada**: Grupo existente, fila nueva
- **Salida**: int (1 si misma clave, 0 si no)
- **Descripción**: Determina si una fila pertenece al mismo grupo de configuración

**`add_row_to_group(Group* g, const Row* r)`**
- **Entrada**: Grupo destino, fila a agregar
- **Salida**: void
- **Descripción**: Acumula estadísticas de la fila en el grupo

**`init_group_from_row(Group* g, const Row* r)`**
- **Entrada**: Grupo a inicializar, fila base
- **Salida**: void
- **Descripción**: Inicializa un grupo nuevo con la primera fila

#### Funciones de Vector Dinámico

**`gv_init(GroupVec* v)`**
- **Entrada**: Vector a inicializar
- **Salida**: void
- **Descripción**: Inicializa vector dinámico vacío

**`gv_free(GroupVec* v)`**
- **Entrada**: Vector a liberar
- **Salida**: void
- **Descripción**: Libera memoria del vector dinámico

**`gv_find(GroupVec* v, const Row* r)`**
- **Entrada**: Vector, fila a buscar
- **Salida**: Group* (grupo encontrado o NULL)
- **Descripción**: Busca grupo existente que coincida con la configuración de la fila

**`gv_add(GroupVec* v, const Row* r)`**
- **Entrada**: Vector, fila base para nuevo grupo
- **Salida**: Group* (nuevo grupo o NULL si falla)
- **Descripción**: Añade nuevo grupo al vector, redimensionando si es necesario

#### Funciones de E/S

**`load_rows(const char* path, Row** out_rows, int* out_nrows)`**
- **Entrada**: Ruta del CSV, punteros de salida para filas y contador
- **Salida**: int (1 si exitoso, 0 si falla)
- **Descripción**: Carga y parsea archivo CSV completo en memoria

**`write_summary_csv(const char* dir, const GroupVec* v, const GroupVec* bases)`**
- **Entrada**: Directorio destino, vector de grupos, vector de baselines
- **Salida**: int (1 si exitoso, 0 si falla)
- **Descripción**: Escribe resumen CSV con promedios y speedups calculados

**`write_anexo_md(const char* dir, const GroupVec* v, const GroupVec* bases)`**
- **Entrada**: Directorio destino, vector de grupos, vector de baselines
- **Salida**: int (1 si exitoso, 0 si falla)
- **Descripción**: Escribe documentación Markdown con tabla de resultados

#### Funciones Auxiliares

**`dirname_from_path(const char* in, char* out, size_t outsz)`**
- **Entrada**: Ruta completa, buffer de salida, tamaño del buffer
- **Salida**: void
- **Descripción**: Extrae directorio padre de una ruta de archivo

**`main(int argc, char** argv)`**
- **Entrada**: Argumentos de línea de comandos
- **Salida**: int (código de salida)
- **Descripción**: Función principal que coordina carga, procesamiento y generación de reportes

## Características de Paralelización

### OpenMP en TextRender.cpp

- **`updateRain`**: Usa `#pragma omp parallel for` con `collapse(2)` para paralelizar bucles anidados de columnas y caracteres
- **`updateBounce`**: Paraleliza bucle de partículas con `schedule(static)`
- **`updateSpiral`**: Paraleliza bucle de partículas con `schedule(static)`
- **`updateDashes`**: Usa `collapse(2)` para paralelizar líneas y puntos

### Sincronización Thread-Safe

- **Mutex**: `rng_mutex_` protege generador de números aleatorios
- **Lock Guards**: `std::lock_guard` en `threadSafeRand` y `threadSafeRandInt`
- **Reduction**: `#pragma omp parallel for reduction(max:maxLen)` para encontrar máximos

### Prevención de Race Conditions

- Eliminación de `std::rand()` no thread-safe
- Uso de generadores determinísticos para efectos visuales
- Separación de datos por hilo (cada hilo trabaja con partículas independientes)
 