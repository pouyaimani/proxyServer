# Minimum required version of CMake
cmake_minimum_required(VERSION 3.10)

# Project name
project(ProxyServer)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# Specify the source file
set(SOURCE_FILE src/main.c src/logger.c src/signal_handler.c src/request_handler.c)

# Add the executable target
add_executable(proxy_server ${SOURCE_FILE})

target_include_directories(proxy_server PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Add CPU-specific GCC flags
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    # Example flags for CPU optimization
    target_compile_options(proxy_server PRIVATE -march=native -mtune=native)
endif()

# Link libraries if needed (e.g., pthread, networking libraries)
target_link_libraries(proxy_server PRIVATE pthread)