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

# 1) Plugin
add_library(sample_plugin SHARED sample_plugin.c)

# Link against ncom (public includes are propagated by the target).
target_link_libraries(sample_plugin PRIVATE ncom)

# Generate C headers from IDL and attach them to the target build graph.
ncom_generate_headers(TARGET sample_plugin IDLS idl/demo.idl)

if (MSVC)
    target_compile_options(sample_plugin PRIVATE /W4 /WX)
else()
    target_compile_options(sample_plugin PRIVATE -Wall -Wextra -Werror -Wpedantic)
endif()

# 2) Host application
add_executable(host_app host_app.c)

# Host links against ncom for plugin loader and base interfaces.
target_link_libraries(host_app PRIVATE ncom)

# Host also depends on the generated IDL header.
ncom_generate_headers(TARGET host_app IDLS idl/demo.idl)

if (MSVC)
    target_compile_options(host_app PRIVATE /W4 /WX)
else()
    target_compile_options(host_app PRIVATE -Wall -Wextra -Werror -Wpedantic)
endif()
