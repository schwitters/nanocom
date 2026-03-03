/*
 * Copyright 2026 nano_com authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file plugin.h
 * @brief Plugin ABI: exported entrypoint, API table, and symbol visibility helpers.
 *
 * A plugin is a shared library that exports exactly one well-known symbol:
 * @ref NCOM_PLUGIN_GET_API_V1_SYMBOL.
 *
 * The returned API table is treated as read-only and must have static storage duration.
 */
#ifndef NCOM_PLUGIN_H
#define NCOM_PLUGIN_H

#include <ncom/base.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ncom_plugin Plugin ABI
 * @brief Host-to-plugin boundary types and conventions.
 * @{
 */

/**
 * @def NCOM_PLUGIN_EXPORT
 * @brief Platform-independent macro to export plugin symbols.
 *
 * This macro ensures that the plugin's entry point is correctly exported
 * in the dynamic library's symbol table. 
 * - On Windows (MSVC/MinGW), it resolves to `__declspec(dllexport)` to make 
 * the symbol visible to dynamic loaders like `GetProcAddress()`.
 * - On POSIX systems (GCC/Clang), it uses `__attribute__((visibility("default")))`
 * to ensure the symbol is public. This is strictly required when compiling the 
 * plugin library with the `-fvisibility=hidden` flag.
 */
#if defined(_WIN32) || defined(__CYGWIN__)
#  define NCOM_PLUGIN_EXPORT __declspec(dllexport)
#elif defined(__GNUC__) && (__GNUC__ >= 4)
#  define NCOM_PLUGIN_EXPORT __attribute__((visibility("default")))
#else
#  define NCOM_PLUGIN_EXPORT
#endif

/**
 * @def NCOM_PLUGIN_GET_API_V1_SYMBOL
 * @brief Name of the mandatory plugin entrypoint symbol.
 */
#define NCOM_PLUGIN_GET_API_V1_SYMBOL "ncom_plugin_get_api_v1"

/** Optional initialization hook called immediately after the plugin is loaded. */
typedef void (*ncom_plugin_init_fn)(void);

/** Optional shutdown hook called right before the plugin is unloaded. */
typedef void (*ncom_plugin_shutdown_fn)(void);

/**
 * @brief Factory function signature for creating component instances.
 *
 * @param clsid Class identifier of the component to create.
 * @param iid   Interface identifier requested from the created instance.
 * @param out   Receives an AddRef'ed interface pointer on success.
 */
typedef ncom_status_t (*ncom_plugin_create_instance_fn)(
    const ncom_clsid_t *clsid,
    const ncom_iid_t   *iid,
    void              **out
);

/**
 * @brief Plugin API function table (version 1).
 *
 * The host validates `abi_version` before calling any other function pointer.
 */
typedef struct ncom_plugin_api_v1_s {
    uint32_t                       abi_version;     /**< Must be 1 for this layout. */
    ncom_plugin_create_instance_fn create_instance; /**< Must not be NULL. */
    ncom_plugin_init_fn            plugin_init;     /**< Optional; may be NULL. */
    ncom_plugin_shutdown_fn        plugin_shutdown; /**< Optional; may be NULL. */
} ncom_plugin_api_v1_t;

/**
 * @brief The mandatory entry point for every ncom-compatible plugin.
 *
 * The host application will search for this specific function by name
 * (defined by @ref NCOM_PLUGIN_GET_API_V1_SYMBOL) when loading the dynamic
 * library into memory. It must return a valid pointer to the plugin API v1
 * structure, which provides the factory function used to instantiate
 * the plugin's components.
 *
 * @return A constant pointer to the statically initialized ncom_plugin_api_v1_t structure.
 */
NCOM_PLUGIN_EXPORT const ncom_plugin_api_v1_t* ncom_plugin_get_api_v1(void);

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NCOM_PLUGIN_H */