cmake_minimum_required(VERSION 3.16)
project(http_server VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
add_compile_options(-Wall -Wextra -Wpedantic)

# Sources
set(SERVER_SOURCES
    src/main.cpp
    src/logger.cpp
    src/tcp_server.cpp
    src/http_parser.cpp
    src/http_response.cpp
    src/router.cpp
    src/static_file_handler.cpp
    src/metrics.cpp
)

# Main executable
add_executable(http_server ${SERVER_SOURCES})
target_include_directories(http_server PRIVATE include)

# Platform-specific linking
if(WIN32)
    target_link_libraries(http_server PRIVATE ws2_32 mswsock shlwapi)
else()
    target_link_libraries(http_server PRIVATE pthread)
endif()

# Tests
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
