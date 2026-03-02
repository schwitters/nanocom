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

# Funktion: ncom_generate_headers
# Usage: ncom_generate_headers(TARGET my_plugin IDLS my_interface.idl another.idl)
function(ncom_generate_headers)
    cmake_parse_arguments(ARG "" "TARGET" "IDLS" ${ARGN})

    if(NOT ARG_TARGET)
        message(FATAL_ERROR "ncom_generate_headers requires a TARGET parameter.")
    endif()

    if(NOT ARG_IDLS)
        message(FATAL_ERROR "ncom_generate_headers requires at least one IDL file.")
    endif()

    set(OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated_${ARG_TARGET}")
    set(INC_DIR "${OUT_DIR}/include")
    
    file(MAKE_DIRECTORY ${INC_DIR})

    # Das Ziel-Verzeichnis automatisch als Include-Pfad für das Target setzen
    target_include_directories(${ARG_TARGET} PRIVATE ${INC_DIR})

    foreach(IDL_FILE ${ARG_IDLS})
        get_filename_component(IDL_NAME ${IDL_FILE} NAME_WE)
        set(OUT_HEADER "${INC_DIR}/${IDL_NAME}.h")

        # Wenn der Dateipfad nicht absolut ist, mache ihn absolut
        if(NOT IS_ABSOLUTE ${IDL_FILE})
            set(IDL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${IDL_FILE}")
        endif()

        # Custom Command, der nidlgen aufruft
        add_custom_command(
            OUTPUT ${OUT_HEADER}
            COMMAND nidlgen ${IDL_FILE} ${OUT_DIR}
            DEPENDS nidlgen ${IDL_FILE}
            COMMENT "Generating ncom header for ${IDL_NAME}.idl"
            VERBATIM
        )

        # Hänge den generierten Header an das Target, damit CMake weiß, 
        # dass er vor dem Kompilieren des Targets erzeugt werden muss.
        target_sources(${ARG_TARGET} PRIVATE ${OUT_HEADER})
    endforeach()
endfunction()