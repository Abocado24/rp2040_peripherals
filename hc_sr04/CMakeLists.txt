# hc_sr04/CMakeLists.txt

# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.13)

# Define the project name
project(hc_sr04)

# Define the library
add_library(hc_sr04 STATIC src/hc_sr04.c)

# Specify include directories
target_include_directories(hc_sr04 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link library with dependencies
target_link_libraries(hc_sr04 pico_stdlib)