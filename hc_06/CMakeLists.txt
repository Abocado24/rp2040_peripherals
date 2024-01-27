# hc06/CMakeLists.txt

# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.13)

# Define the library
add_library(hc_06 STATIC hc_06.c)

# Specify that this library depends on the pico_stdlib
target_link_libraries(hc_06 pico_stdlib)