# mpu_6050/CMakeLists.txt

# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.13)

# Define the project name
project(mpu_6050)

# Define the library
add_library(mpu_6050 STATIC src/mpu_6050.c)

# Specify include directories
target_include_directories(mpu_6050 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link library with dependencies
target_link_libraries(mpu_6050 pico_stdlib hardware_i2c i2c_general vector_lib)