cmake_minimum_required(VERSION 3.16)
project(nano_com_minimal C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Build outputs into a predictable folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory(src)


enable_testing()
add_subdirectory(tests)


option(NANO_GENERATE_FROM_IDL "Run nidlgen to generate headers from idl/*.idl" OFF)
add_subdirectory(tools/nidl)

if (NANO_GENERATE_FROM_IDL)
  set(NIDL_OUT_DIR ${CMAKE_BINARY_DIR}/generated)
  add_custom_command(
    OUTPUT ${NIDL_OUT_DIR}/include/nano_ids.h
    COMMAND nidlgen ${PROJECT_SOURCE_DIR}/idl/nano.idl ${NIDL_OUT_DIR}
    DEPENDS nidlgen ${PROJECT_SOURCE_DIR}/idl/nano.idl
    COMMENT "Generating Nano-COM headers from IDL"
    VERBATIM
  )
  add_custom_target(nidl_generated DEPENDS ${NIDL_OUT_DIR}/include/nano_ids.h)
endif()

add_subdirectory(tools/uuid_tool)


option(NANO_BUILD_DOCS "Build Doxygen documentation (requires doxygen)" OFF)
if (NANO_BUILD_DOCS)
  find_package(Doxygen REQUIRED)
  set(DOXYGEN_IN ${PROJECT_SOURCE_DIR}/docs/Doxyfile.in)
  set(DOXYGEN_OUT ${CMAKE_BINARY_DIR}/docs/Doxyfile)
  configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
  add_custom_target(docs
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen"
    VERBATIM
  )
endif()
