# Matrix Screensaver

Este proyecto implementa un “screensaver” estilo Matrix en C++, usando SFML para renderizar cascadas de caracteres pseudoaleatorios en pantalla.

---

## 📁 Estructura del repositorio

```bash
Proyecto1/
├── CMakeLists.txt
├── README.md
├── .gitignore
├── include/
│   └── TextRender.h
├── src/
│   ├── main.cpp
│   └── TextRender.cpp
├── assets/
│   └── fonts/
│       └── Matrix-MZ4P.ttf
├── examples/
│   └── run_screensaver.sh
└── build/                # Artefactos de compilación (Crearla localmente)
```

- **include/**  
  Cabeceras públicas de tus clases (por ejemplo `TextRender.h`).  
- **src/**  
  Implementación en C++:  
  - `main.cpp` — inicializa SFML, carga la fuente y arranca el bucle de render.  
  - `TextRender.cpp` — gestiona la creación y dibujo de N caracteres.  
- **assets/fonts/**  
  Tipografía Matrix (`.ttf`) usada para renderizar caracteres.  
- **examples/**  
  Script de ejemplo para compilar y ejecutar: `run_screensaver.sh`.  
- **build/**  
  Carpeta donde CMake genera Makefiles y objetos.

---

## ⚙️ Dependencias

- **CMake** ≥ 3.10  
- **C++17**  
- **SFML** ≥ 2.5 (librerías `graphics`, `window`, `system`)  
  - En Ubuntu/WSL:  
    ```bash
    sudo apt update
    sudo apt install libsfml-dev
    ```
---

## 🛠️ Compilación

Creamos un .sh que compila y ejecuta automaticamente el archivo.

---

## ▶️ Ejecución

Usa el script de ejemplo (desde la raíz del proyecto):

```bash
chmod +x example/run_screensaver.sh
./examples/run_screensaver.sh [N] [ANCHOxALTO]
```

- **N**: número de caracteres a renderizar (por defecto: `200`).  
- **ANCHOxALTO**: resolución de la ventana (por defecto: `800x600`).  

Ejemplos:

```bash
# 150 caracteres en 1024×768
./example/run_screensaver.sh 150 1024x768

# Valores por defecto
./example/run_screensaver.sh
```

El script recompila automáticamente y luego lanza el programa **desde la raíz** para que encuentre `assets/fonts/Matrix-MZ4P.ttf`.

---

## 🐞 Troubleshooting

- **Error “Could not find SFMLConfig.cmake”**  
  Instala SFML o apunta `-DSFML_DIR=/ruta/a/SFML/lib/cmake/SFML` cuando invoques `cmake`.

- **Error al cargar la fuente**  
  Asegúrate de:  
  1. Ejecutar el binario desde la raíz del proyecto.  
  2. Que el archivo `assets/fonts/Matrix-MZ4P.ttf` existe y coincide el nombre en `main.cpp`.  

---