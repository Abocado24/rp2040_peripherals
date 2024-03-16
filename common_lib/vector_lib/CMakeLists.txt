# common_lib/vector_lib/CMakeLists.txt

# Set the minimum required version of CMake
cmake_minimum_required(VERSION 3.13)

# Define the library
add_library(vector_lib STATIC src/vector_lib.c)

# Specify where the library's header file is located
target_include_directories(vector_lib PUBLIC include)
