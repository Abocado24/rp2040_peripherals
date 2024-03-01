# mpu_6050/CMakeLists.txt

# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.13)

# Define the project name
project(mpu_6050)

# Add the common_lib/i2c_general directory to the build
add_subdirectory(../common_lib/i2c_general ${CMAKE_CURRENT_BINARY_DIR}/vector_lib_build)
# Add the common_lib/vector_lib directory to the build
add_subdirectory(../common_lib/vector_lib ${CMAKE_CURRENT_BINARY_DIR}/i2c_general_build)

# If mpu_6050 follows the include/src structure, adjust the source file path.
# For this example, I'm assuming mpu_6050.c is directly under mpu_6050 directory.
# Adjust this if your file is inside an src directory.
add_library(mpu_6050 STATIC src/mpu_6050.c) # Adjusted for src directory structure

# Specify where the compiler can find the headers for mpu_6050 library
# Adjust according to your project structure. If headers are in an include directory,
# you need to specify it.
target_include_directories(mpu_6050 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link mpu_6050 with pico_stdlib, hardware_i2c, and vector_lib
target_link_libraries(mpu_6050 pico_stdlib hardware_i2c i2c_general vector_lib)

# Since vector_lib is in another directory, specify where to find its headers.
# This is important if vector_lib's headers are not in a standard location.
target_include_directories(mpu_6050 PUBLIC ../common_lib/vector_lib/include)
