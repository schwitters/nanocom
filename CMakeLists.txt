cmake_minimum_required(VERSION 3.16)
project(ncom VERSION 1.0.0 LANGUAGES C CXX)

# --- CMake Optionen ---
option(NCOM_BUILD_TOOLS   "Build nidlgen and uuid_tool" ON)
option(NCOM_BUILD_SAMPLES "Build sample plugin and host app" OFF)
option(NCOM_BUILD_TESTS   "Build unit tests" OFF)

# --- Die ncom Kern-Bibliothek ---
# Wir machen daraus eine statische Bibliothek (oder lassen den User via BUILD_SHARED_LIBS entscheiden)
add_library(ncom
    src/ncom/core_impl.c
    src/ncom/plugin_loader.c
)

# Das wichtigste Feature in Modern CMake: PUBLIC Includes!
# Wer auch immer "target_link_libraries(my_app ncom)" aufruft, 
# bekommt automatisch "-I.../include" in seinen Compiler-Pfad injiziert.
target_include_directories(ncom PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(MSVC)
    target_compile_options(ncom PRIVATE /W4)
else()
    target_compile_options(ncom PRIVATE -Wall -Wextra -Wpedantic)
    # Auf Linux brauchen wir libdl für den plugin_loader (dlopen)
    if(UNIX AND NOT APPLE)
        target_link_libraries(ncom PUBLIC dl)
    endif()
endif()

# Stelle CMake-Makros bereit, damit Nutzer leicht IDLs generieren können
include(cmake/ncom_idl.cmake)

# --- Subdirectories einbinden ---

if(NCOM_BUILD_TOOLS)
    add_subdirectory(tools/nidl)
    add_subdirectory(tools/uuid_tool)
endif()

if(NCOM_BUILD_SAMPLES)
    add_subdirectory(samples)
endif()

if(NCOM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()