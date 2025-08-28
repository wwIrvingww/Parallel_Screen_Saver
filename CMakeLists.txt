cmake_minimum_required(VERSION 3.10)
project(MatrixScreensaver)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)
find_package(OpenMP REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(matrix_screensaver
    src/main.cpp
    src/TextRender.cpp
    src/ObjModel.cpp           # <--- NUEVO
    include/TextRender.h
    include/ObjModel.h         # <--- NUEVO
)

target_include_directories(matrix_screensaver
    PRIVATE ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(matrix_screensaver
    PRIVATE sfml-graphics sfml-window sfml-system
            OpenMP::OpenMP_CXX
            OpenGL::GL
)
