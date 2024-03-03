cmake_minimum_required(VERSION 3.13)

# Define the library
add_library(edf_scheduler STATIC src/edf_scheduler.c)

# Add FreeRTOS
add_subdirectory(../../freertos)

# Specify include directories
target_include_directories(edf_scheduler PUBLIC include)

# Link dependencies (if any, e.g., hardware_i2c for RP2040)
target_link_libraries(edf_scheduler freertos)