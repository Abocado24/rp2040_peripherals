cmake_minimum_required(VERSION 3.13)

# Define the library
add_library(i2c_general STATIC src/i2c_general.c)

# Specify include directories
target_include_directories(i2c_general PUBLIC include)

# Link dependencies (if any, e.g., hardware_i2c for RP2040)
target_link_libraries(i2c_general hardware_i2c)