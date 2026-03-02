add_executable(nano_tests nano_tests.c)

target_link_libraries(nano_tests PRIVATE ncom)
ncom_generate_headers(TARGET nano_tests IDLS ../samples/idl/demo.idl)


if(MSVC)
    target_compile_options(nano_tests PRIVATE /W4)
    target_compile_definitions(nano_tests PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(nano_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

# (Optional) Den Test für 'ctest' registrieren
add_test(NAME NanoTests COMMAND nano_tests)
