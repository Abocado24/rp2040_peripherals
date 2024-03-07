# common_lib/CMakeLists.txt

cmake_minimum_required(VERSION 3.12)
project(common_lib)

add_subdirectory(i2c_general)
add_subdirectory(vector_lib)
add_subdirectory(edf)

# Add the common_lib/i2c_general directory to the build
#add_subdirectory(i2c_general ${CMAKE_CURRENT_BINARY_DIR}/vector_lib_build)
# Add the common_lib/vector_lib directory to the build
#add_subdirectory(vector_lib ${CMAKE_CURRENT_BINARY_DIR}/i2c_general_build)
# Add the common_lib/edf directory to the build
#add_subdirectory(edf ${CMAKE_CURRENT_BINARY_DIR}/edf_build)