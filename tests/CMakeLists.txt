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
