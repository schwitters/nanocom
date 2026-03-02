#pragma once

#include <ncom/errors.h>
#include <ncom/plugin.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque handle representing a loaded plugin library.
 * * Encapsulates the OS-specific dynamic library handle (HMODULE on Windows, 
 * void* on POSIX) and the resolved API function table.
 */
typedef struct ncom_plugin_handle_s ncom_plugin_handle_t;

/**
 * @brief Loads an ncom plugin library dynamically from the file system.
 * * Resolves the required entry point symbol (`ncom_plugin_get_api_v1`) and 
 * retrieves the API function table used to instantiate components. If the plugin
 * provides an initialization hook (`plugin_init`), it is executed automatically.
 * @param path       The file system path to the dynamic library (.dll, .so, .dylib).
 * @param out_handle Receives the opaque handle to the loaded plugin.
 * @param out_api    Receives the read-only API function table.
 * @return ncom_status_t NCOM_OK on success, or an error code (e.g., NCOM_E_NOT_FOUND 
 * if the file is missing, NCOM_E_NOT_IMPL if the entry symbol is missing).
 */
ncom_status_t ncom_plugin_load(
    const char *path, 
    ncom_plugin_handle_t **out_handle, 
    const ncom_plugin_api_v1_t **out_api
);

/**
 * @brief Unloads a previously loaded ncom plugin library.
 * * If the plugin provides a shutdown hook (`plugin_shutdown`), it is executed 
 * before the library is unloaded from the process memory.
 * * @param handle The plugin handle returned by ncom_plugin_load(). 
 * If NULL, the function does nothing.
 */
void ncom_plugin_unload(ncom_plugin_handle_t *handle);

#ifdef __cplusplus
}
#endif