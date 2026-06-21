# Copyright 2026 nano_com authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.16)
project(ncom VERSION 1.0.0 LANGUAGES C CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Project-wide C language settings.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# --- CMake options ---
option(NCOM_BUILD_TOOLS   "Build nidlgen and uuid_tool" ON)
option(NCOM_BUILD_SAMPLES "Build sample plugin and host app" OFF)
option(NCOM_BUILD_TESTS   "Build unit tests" OFF)
option(NCOM_ENABLE_ASAN   "Enable AddressSanitizer for all targets (GCC/Clang)" OFF)
option(NCOM_ENABLE_COVERAGE "Enable coverage instrumentation for all targets (GCC/Clang)" OFF)

if(MSVC)
    if(NCOM_ENABLE_ASAN)
        message(WARNING "NCOM_ENABLE_ASAN is currently intended for GCC/Clang toolchains.")
    endif()
    if(NCOM_ENABLE_COVERAGE)
        message(WARNING "NCOM_ENABLE_COVERAGE is currently intended for GCC/Clang toolchains.")
    endif()
else()
    if(NCOM_ENABLE_ASAN)
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
    if(NCOM_ENABLE_COVERAGE)
        if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
            add_compile_options(--coverage -O0 -g)
            add_link_options(--coverage)
        else()
            message(WARNING "Coverage flags requested, but compiler is neither GCC nor Clang.")
        endif()
    endif()
endif()

# --- ncom core library ---
add_library(ncom
    src/ncom/core_impl.c
    src/ncom/plugin_loader.c
)
add_library(ncom::ncom ALIAS ncom)
set_target_properties(ncom PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Export public include directories to all consumers via target_link_libraries().
target_include_directories(ncom PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(MSVC)
    target_compile_options(ncom PRIVATE /W4 /WX)
else()
    target_compile_options(ncom PRIVATE -Wall -Wextra -Werror -Wpedantic)
    # Linux requires libdl for the plugin loader (dlopen/dlsym).
    if(UNIX AND NOT APPLE)
        target_link_libraries(ncom PUBLIC dl)
    endif()
endif()

# Provide CMake helper macros (IDL header generation).
include(cmake/ncom_idl.cmake)

# --- Add subdirectories ---

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

# --- Install / Export ---
install(TARGETS ncom
    EXPORT ncomTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/ncom
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ncomConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ncomConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ncom
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/ncomConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/ncomConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/ncomConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ncom
)

install(EXPORT ncomTargets
    FILE ncomTargets.cmake
    NAMESPACE ncom::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ncom
)
