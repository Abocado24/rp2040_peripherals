# common_lib/circular_buffer/CMakeLists.txt

# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.13)

# Define the library
add_library(circular_buffer STATIC src/circular_buffer.c)

# Specify include directories
target_include_directories(circular_buffer PUBLIC include)