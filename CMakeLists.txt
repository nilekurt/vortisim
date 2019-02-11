cmake_minimum_required(VERSION 3.10)
project(vortisim VERSION 0.0.1 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(OpenGL 3 REQUIRED)
find_package(glm REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

file(GLOB INCLUDE "include/*.h")
file(GLOB SRC "src/*.cpp")
file(GLOB UI "src/*.ui")
file(GLOB RES "res/*.qrc")
file(GLOB SHAD "res/shaders/*.glsl")

add_executable(${PROJECT_NAME} ${UI} ${INCLUDE} ${SRC} ${RES} ${SHAD})
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/ui)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_link_libraries(${PROJECT_NAME} Qt5::Gui Qt5::Widgets OpenGL::GL glm)
