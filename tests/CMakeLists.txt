add_executable(nano_tests
    nano_tests.c
    ../src/plugin_loader.c
)

target_include_directories(nano_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
)

if (MSVC)
    target_compile_options(nano_tests PRIVATE /W4)
else()
    target_compile_options(nano_tests PRIVATE -Wall -Wextra -Wpedantic)
    if (UNIX AND NOT APPLE)
        target_link_libraries(nano_tests PRIVATE dl)
    endif()
endif()

add_dependencies(nano_tests sample_plugin)

add_test(NAME nano_tests
    COMMAND nano_tests $<TARGET_FILE:sample_plugin>
)

if (NANO_GENERATE_FROM_IDL)
  target_include_directories(nano_tests PRIVATE ${CMAKE_BINARY_DIR}/generated/include)
  add_dependencies(nano_tests nidl_generated)
endif()
