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