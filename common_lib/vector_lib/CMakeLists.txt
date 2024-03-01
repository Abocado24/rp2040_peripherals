# common_lib/vector_lib/CMakeLists.txt

# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.13)

# Define the project name and version
project(vector_lib VERSION 1.0 DESCRIPTION "Vector Library for RP2040")

# Specify the C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Include directories
include_directories(include)

# Add library source code
add_library(vector_lib STATIC src/vector_lib.c)

# Specify where the library's header file is located
target_include_directories(vector_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
