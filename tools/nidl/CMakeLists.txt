cmake_minimum_required(VERSION 3.16)

option(NANO_USE_RE2C_LEMON "Use re2c + lemon generated lexer/parser for nidlgen" OFF)
option(NANO_FETCH_DEPS "Fetch re2c + sqlite sources via git (recommended)" ON)

include(FetchContent)

add_executable(nidlgen
    main.c
    nidl_arena.c
    nidl_codegen_c.c
)
target_compile_definitions(nidlgen PRIVATE _CRT_SECURE_NO_WARNINGS)
target_include_directories(nidlgen PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/include
)

if (NANO_USE_RE2C_LEMON)

  if (NANO_FETCH_DEPS)
    # Configure re2c options BEFORE fetching.
    set(RE2C_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(RE2C_REBUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(RE2C_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(RE2C_BUILD_RE2GO OFF CACHE BOOL "" FORCE)
    set(RE2C_BUILD_RE2RUST OFF CACHE BOOL "" FORCE)
    set(RE2C_BUILD_RE2JAVA OFF CACHE BOOL "" FORCE)
    set(RE2C_BUILD_RE2JS OFF CACHE BOOL "" FORCE)
    set(RE2C_BUILD_RE2PY OFF CACHE BOOL "" FORCE)
    set(RE2C_BUILD_RE2OCAML OFF CACHE BOOL "" FORCE)
    set(RE2C_BUILD_RE2D OFF CACHE BOOL "" FORCE)
    set(RE2C_BUILD_RE2HS OFF CACHE BOOL "" FORCE)
    set(RE2C_BUILD_RE2V OFF CACHE BOOL "" FORCE)
    set(RE2C_BUILD_RE2ZIG OFF CACHE BOOL "" FORCE)
    set(RE2C_BUILD_RE2SWIFT OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(re2c_src
      GIT_REPOSITORY https://github.com/skvadrik/re2c.git
      GIT_TAG master
      GIT_SHALLOW TRUE
    )
    # MakeAvailable will add the re2c subdirectory exactly once.
    FetchContent_MakeAvailable(re2c_src)

    # sqlite: we only need tool/lemon.c and tool/lempar.c, do not add subdirectory.
    FetchContent_Declare(sqlite_src
      GIT_REPOSITORY https://github.com/sqlite/sqlite.git
      GIT_TAG master
      GIT_SHALLOW TRUE
    )
    FetchContent_GetProperties(sqlite_src)
    if (NOT sqlite_src_POPULATED)
      FetchContent_Populate(sqlite_src)
    endif()

    add_executable(lemon_tool ${sqlite_src_SOURCE_DIR}/tool/lemon.c)
    target_include_directories(lemon_tool PRIVATE ${sqlite_src_SOURCE_DIR}/tool)
    if (MSVC)
      target_compile_options(lemon_tool PRIVATE /W3)
    else()
      target_compile_options(lemon_tool PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    set(LEMPAR_TEMPLATE ${sqlite_src_SOURCE_DIR}/tool/lempar.c)
    # re2c target from upstream project is named "re2c"
    set(RE2C_EXECUTABLE $<TARGET_FILE:re2c>)

  else()
    find_program(RE2C_EXECUTABLE re2c)
    if (NOT RE2C_EXECUTABLE)
      message(FATAL_ERROR "NANO_USE_RE2C_LEMON=ON but re2c was not found in PATH (or enable NANO_FETCH_DEPS=ON).")
    endif()

    set(LEMON_EXECUTABLE "" CACHE FILEPATH "Path to lemon executable (SQLite)")
    if (NOT LEMON_EXECUTABLE)
      find_program(LEMON_EXECUTABLE lemon)
    endif()
    if (NOT LEMON_EXECUTABLE)
      message(FATAL_ERROR "NANO_USE_RE2C_LEMON=ON but lemon was not found. Provide -DLEMON_EXECUTABLE=/path/to/lemon or enable NANO_FETCH_DEPS=ON.")
    endif()

    set(LEMPAR_TEMPLATE "" CACHE FILEPATH "Path to SQLite tool/lempar.c template")
    if (NOT LEMPAR_TEMPLATE)
      message(FATAL_ERROR "Provide -DLEMPAR_TEMPLATE=/path/to/lempar.c or enable NANO_FETCH_DEPS=ON.")
    endif()
  endif()

  set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
  file(MAKE_DIRECTORY ${GEN_DIR})

  add_custom_command(
    OUTPUT ${GEN_DIR}/nidl_lex_gen.c
    COMMAND ${RE2C_EXECUTABLE} -o ${GEN_DIR}/nidl_lex_gen.c ${CMAKE_CURRENT_SOURCE_DIR}/nidl_lexer.re
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/nidl_lexer.re
    COMMENT "Generating lexer with re2c"
    VERBATIM
  )

  if (NANO_FETCH_DEPS)
    add_custom_command(
      OUTPUT ${GEN_DIR}/nidl_parser.c ${GEN_DIR}/nidl_parser.h
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/nidl_parser.y ${GEN_DIR}/nidl_parser.y
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LEMPAR_TEMPLATE} ${GEN_DIR}/lempar.c
      COMMAND $<TARGET_FILE:lemon_tool> -T${GEN_DIR}/lempar.c -q ${GEN_DIR}/nidl_parser.y
      WORKING_DIRECTORY ${GEN_DIR}
      DEPENDS lemon_tool ${CMAKE_CURRENT_SOURCE_DIR}/nidl_parser.y ${LEMPAR_TEMPLATE}
      COMMENT "Generating parser with lemon (sqlite/tool/lemon.c)"
      VERBATIM
    )
  else()
    add_custom_command(
      OUTPUT ${GEN_DIR}/nidl_parser.c
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/nidl_parser.y ${GEN_DIR}/nidl_parser.y
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${LEMPAR_TEMPLATE} ${GEN_DIR}/lempar.c
      COMMAND ${LEMON_EXECUTABLE} -T${GEN_DIR}/lempar.c -q ${GEN_DIR}/nidl_parser.y
      WORKING_DIRECTORY ${GEN_DIR}
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/nidl_parser.y ${LEMPAR_TEMPLATE}
      COMMENT "Generating parser with lemon"
      VERBATIM
    )
  endif()

  target_compile_definitions(nidlgen PRIVATE NANO_IDL_USE_RE2C_LEMON=1)

  target_sources(nidlgen PRIVATE
    ${GEN_DIR}/nidl_lex_gen.c
    ${GEN_DIR}/nidl_parser.c
    nidl_lex_re2c_wrap.c
    nidl_parse_lemon.c
  )
  target_include_directories(nidlgen PRIVATE ${GEN_DIR})

  if (NANO_FETCH_DEPS)
    add_dependencies(nidlgen re2c lemon_tool)
  endif()

else()
  target_sources(nidlgen PRIVATE
    nidl_lex.c
    nidl_parse.c
  )
endif()

if (MSVC)
  target_compile_options(nidlgen PRIVATE /W4)
else()
  target_compile_options(nidlgen PRIVATE -Wall -Wextra -Wpedantic)
endif()
