if (NANO_GENERATE_FROM_IDL)
  include_directories(${CMAKE_BINARY_DIR}/generated/include)
endif()

add_library(sample_plugin SHARED
    sample_plugin.c
)

target_include_directories(sample_plugin PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

# Keep the public surface minimal; on Windows the entry point uses __declspec(dllexport).
if (MSVC)
    target_compile_options(sample_plugin PRIVATE /W4)
else()
    target_compile_options(sample_plugin PRIVATE -Wall -Wextra -Wpedantic)
    target_link_options(sample_plugin PRIVATE)
endif()

add_executable(host_app
    host_app.c
    plugin_loader.c
)

target_include_directories(host_app PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
)

if (MSVC)
    target_compile_options(host_app PRIVATE /W4)
else()
    target_compile_options(host_app PRIVATE -Wall -Wextra -Wpedantic)
    # Linux/Unix: host uses dlopen, link with dl where needed
    if (UNIX AND NOT APPLE)
        target_link_libraries(host_app PRIVATE dl)
    endif()
endif()

if (NANO_GENERATE_FROM_IDL)
  add_dependencies(sample_plugin nidl_generated)
  add_dependencies(host_app nidl_generated)
  add_dependencies(examples_check_err nidl_generated)
endif()

if (NANO_GENERATE_FROM_IDL)
  target_include_directories(sample_plugin PRIVATE ${CMAKE_BINARY_DIR}/generated/include)
  target_include_directories(host_app PRIVATE ${CMAKE_BINARY_DIR}/generated/include)
  target_include_directories(examples_check_err PRIVATE ${CMAKE_BINARY_DIR}/generated/include)
  add_dependencies(sample_plugin nidl_generated)
  add_dependencies(host_app nidl_generated)
  add_dependencies(examples_check_err nidl_generated)
endif()
