cmake_minimum_required(VERSION 3.20)
project(AirV1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.h"
    "ext/minhook/*.c"
    "ext/minhook/*.h"
    "ext/imgui/*.cpp"
    "ext/imgui/*.h"
)

# Exclude AutoTool files
list(FILTER SOURCES EXCLUDE REGEX ".*autoTool.*")

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/base
    ${CMAKE_CURRENT_SOURCE_DIR}/ext
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/freetype
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/nlohmann
    ${CMAKE_CURRENT_SOURCE_DIR}/SDK/Include
)

# Link directories
link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/SDK/Lib/x64
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/jni
)

# Create DLL
add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    d3d11
    d3dcompiler
    dxgi
    opengl32
    gdiplus
    jvm
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Preprocessor definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    _CRT_SECURE_NO_WARNINGS
    WIN32_LEAN_AND_MEAN
)
