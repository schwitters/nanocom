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

# 1. Das Plugin
add_library(sample_plugin SHARED sample_plugin.c)

# Binde das ncom Framework ein (vererbt automatisch include/ Pfade!)
target_link_libraries(sample_plugin PRIVATE ncom)

# Generiere den C-Header aus der IDL vollautomatisch und binde ihn an das Plugin
ncom_generate_headers(TARGET sample_plugin IDLS idl/demo.idl)

if (MSVC)
    target_compile_options(sample_plugin PRIVATE /W4)
else()
    target_compile_options(sample_plugin PRIVATE -Wall -Wextra -Wpedantic)
endif()

# 2. Die Host-Anwendung
add_executable(host_app host_app.c)

# Der Host linkt auch gegen ncom (für den Plugin Loader und die Base Interfaces)
target_link_libraries(host_app PRIVATE ncom)

# Der Host braucht ebenfalls den generierten Header der IDL
ncom_generate_headers(TARGET host_app IDLS idl/demo.idl)