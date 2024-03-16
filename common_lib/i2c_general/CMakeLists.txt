# common_lib/i2c_general/CMakeLists.txt

# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.13)

# Define the library
add_library(i2c_general STATIC src/i2c_general.c)

# Specify include directories
target_include_directories(i2c_general PUBLIC include)

# Link library with directories
target_link_libraries(i2c_general hardware_i2c hardware_dma)