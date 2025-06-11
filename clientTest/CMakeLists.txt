# Minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Project name
project(ProxyClientTest)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# Specify the source file
set(SOURCE_FILE proxy_client_test.c)

# Add the executable target
add_executable(proxy_client_test ${SOURCE_FILE})

# Link libraries if needed (e.g., pthread, networking libraries)
target_link_libraries(proxy_client_test PRIVATE pthread)