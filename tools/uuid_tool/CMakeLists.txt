add_executable(uuid_tool uuid_tool.c)

if (MSVC)
  target_compile_options(uuid_tool PRIVATE /W4)
  target_link_libraries(uuid_tool PRIVATE bcrypt)
else()
  target_compile_options(uuid_tool PRIVATE -Wall -Wextra -Wpedantic)
endif()
