# common_lib/edf/CMakeLists.txt

# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.13)

# Define the library
add_library(edf STATIC src/edf.c)

# Specify include directories
target_include_directories(edf PUBLIC include)

# Link library with directories
target_link_libraries(edf pico_stdlib hardware_timer freertos)