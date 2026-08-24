cmake_minimum_required(VERSION 3.20)
project(AirV1_Injector)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.h"
    "ext/ImGui/*.cpp"
    "ext/ImGui/*.h"
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/base
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/ImGui
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/nlohmann
)

# Create executable
add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME}
    d3d9
    d3dcompiler
    dxgi
)

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Preprocessor definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    _CRT_SECURE_NO_WARNINGS
    WIN32_LEAN_AND_MEAN
)
