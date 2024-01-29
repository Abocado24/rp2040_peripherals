# hc_sr04/CMakeLists.txt

# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.13)

# Define the library
add_library(hc_sr04 STATIC hc_sr04.c)

# Specify that this library depends on the pico_stdlib
target_link_libraries(hc_sr04 pico_stdlib)