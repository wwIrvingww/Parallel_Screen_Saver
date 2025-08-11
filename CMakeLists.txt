cmake_minimum_required(VERSION 3.10)
project(MatrixScreensaver)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)   # para VSCode

find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

add_executable(matrix_screensaver
    src/main.cpp
    src/TextRender.cpp
    include/TextRender.h
)

target_include_directories(matrix_screensaver
    PRIVATE ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(matrix_screensaver
    PRIVATE sfml-graphics sfml-window sfml-system
)
