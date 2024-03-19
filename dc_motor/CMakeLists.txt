# dc_motor/CMakeLists.txt

# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.13)

# Define the project name
project(dc_motor)

# Define the library
add_library(dc_motor STATIC src/dc_motor.c)

# Specify include directories
target_include_directories(dc_motor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link library with directories
target_link_libraries(dc_motor pico_stdlib hardware_pwm)