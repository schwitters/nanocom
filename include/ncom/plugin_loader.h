/*
 * Copyright 2026 nano_com authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file plugin_loader.h
 * @brief Host-side dynamic loader for ncom plugins.
 */
#ifndef NCOM_PLUGIN_LOADER_H
#define NCOM_PLUGIN_LOADER_H

#include <ncom/errors.h>
#include <ncom/plugin.h>
#include <ncom/error_info.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ncom_plugin_loader Plugin loader
 * @brief Host API for loading/unloading ncom plugins.
 * @{
 */

/**
 * @brief Opaque handle representing a loaded plugin.
 *
 * The handle encapsulates the OS-specific dynamic library handle (e.g., HMODULE on Windows)
 * and a resolved @ref ncom_plugin_api_v1_t table.
 */
typedef struct ncom_plugin_handle_s ncom_plugin_handle_t;

/**
 * @brief Load an ncom plugin shared library and resolve its API table.
 *
 * The loader resolves @ref NCOM_PLUGIN_GET_API_V1_SYMBOL and validates `abi_version`.
 * If the plugin provides `plugin_init`, the loader calls it automatically.
 *
 * @param path       File system path to the dynamic library (.dll/.so/.dylib).
 * @param out_handle Receives the opaque plugin handle on success (owned by caller).
 * @param out_api    Receives a pointer to the plugin's API table (read-only).
 * @param out_err    Optional. Receives an AddRef'ed error info object on failure.
 * @return Status code.
 */
ncom_status_t ncom_plugin_load(
    const char                 *path,
    ncom_plugin_handle_t      **out_handle,
    const ncom_plugin_api_v1_t **out_api,
    ncom_ierror_info_t        **out_err
);

/**
 * @brief Unload a previously loaded plugin.
 *
 * If the plugin provides `plugin_shutdown`, the loader calls it before unloading.
 * Passing NULL is a no-op.
 */
void ncom_plugin_unload(ncom_plugin_handle_t *handle);

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NCOM_PLUGIN_LOADER_H */
