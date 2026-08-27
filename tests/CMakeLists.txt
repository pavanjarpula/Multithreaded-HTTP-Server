set(TEST_SOURCES
    test_main.cpp
)

add_executable(server_tests ${TEST_SOURCES})
target_include_directories(server_tests PRIVATE ${CMAKE_SOURCE_DIR}/include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link the same library sources as the server
target_sources(server_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/http_parser.cpp
    ${CMAKE_SOURCE_DIR}/src/http_response.cpp
    ${CMAKE_SOURCE_DIR}/src/router.cpp
    ${CMAKE_SOURCE_DIR}/src/static_file_handler.cpp
    ${CMAKE_SOURCE_DIR}/src/logger.cpp
    ${CMAKE_SOURCE_DIR}/src/metrics.cpp
)

if(WIN32)
    target_link_libraries(server_tests PRIVATE ws2_32 mswsock shlwapi)
else()
    target_link_libraries(server_tests PRIVATE pthread)
endif()

add_test(NAME server_tests COMMAND server_tests)
