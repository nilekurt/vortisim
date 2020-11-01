cmake_minimum_required(VERSION 3.10)
project(vortisim VERSION 0.0.1 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(lto.helper)
include(tidy.helper)
include(with.helper)
include(GNUInstallDirs)

option(VORTISIM_CLANG_TIDY "Build with clang-tidy" ON)
option(VORTISIM_LTO "Build with Link-Time Optimization" OFF)

withmsg("clang-tidy" ${VORTISIM_CLANG_TIDY})
withmsg("Link-Time Optimization" ${VORTISIM_LTO})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

cmake_policy(SET CMP0072 NEW) # Default to GLVND
find_package(OpenGL 3 REQUIRED)
find_package(glm REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Eigen3 REQUIRED NO_MODULE)

add_library(vortisim_copts_common INTERFACE)
target_compile_options(vortisim_copts_common INTERFACE
    -pedantic
    -Wall
    -Wextra
    -Wcast-align
    -Wcast-qual
    -Wmissing-declarations
    -Wwrite-strings
    -Wundef
    -Wshadow
    -Wstrict-aliasing=2
    -Werror
    $<$<C_COMPILER_ID:Clang>:-Wshadow-all>
    $<$<CXX_COMPILER_ID:Clang>:-Wshadow-all>)

file(GLOB INCLUDE "include/*.h")
file(GLOB SRC "src/*.cpp")
file(GLOB UI "src/*.ui")
file(GLOB RES "res/*.qrc")
file(GLOB SHAD "res/shaders/*.glsl")

add_executable(vortisim ${UI} ${INCLUDE} ${SRC} ${RES} ${SHAD})
target_include_directories(vortisim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/ui)
target_link_libraries(vortisim PRIVATE
    vortisim_copts_common Qt5::Gui Qt5::Widgets OpenGL::GL Eigen3::Eigen)

configure_tidy(vortisim)
configure_lto(vortisim)

# Workaround for clang-tidy together with auto-generated files
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/dummy_tidy/.clang-tidy
    ${CMAKE_CURRENT_BINARY_DIR}/vortisim_autogen/.clang-tidy
    COPYONLY)