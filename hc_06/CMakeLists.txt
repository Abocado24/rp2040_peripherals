# hc06/CMakeLists.txt

# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.13)

# Define the project name
project(hc_06)

# Define the library
add_library(hc_06 STATIC src/hc_06.c)

# Specify include directories
target_include_directories(hc_06 PUBLIC include)

# Link library with dependencies
target_link_libraries(hc_06 pico_stdlib circular_buffer)