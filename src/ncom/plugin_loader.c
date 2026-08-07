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
 * @file plugin_loader.c
 * @brief Plugin loader.
 */

#include <ncom/plugin.h>
#include <ncom/plugin_loader.h>
#include <ncom/style.h>
#include <ncom/core_impl.h>

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #include <Windows.h>
  struct ncom_plugin_handle_s { HMODULE lib; const ncom_plugin_api_v1_t *api; };
  static void *sym(HMODULE lib, const char *name) { return (void *)GetProcAddress(lib, name); }
  /* Map a LoadLibrary failure to NCOM_E_NOT_FOUND (file missing) or NCOM_E_FAIL (other). */
  static ncom_status_t win_load_error(ncom_ierror_info_t **out_err) {
      DWORD e = GetLastError();
      ncom_status_t st = (e == ERROR_FILE_NOT_FOUND || e == ERROR_PATH_NOT_FOUND || e == ERROR_MOD_NOT_FOUND)
             ? NCOM_E_NOT_FOUND : NCOM_E_FAIL;
      if (out_err) {
          char msg[512] = {0};
          FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                         NULL, e, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                         msg, sizeof(msg) - 1, NULL);
          size_t len = strlen(msg);
          while (len > 0 && (msg[len-1] == '\n' || msg[len-1] == '\r')) {
              msg[--len] = '\0';
          }
          ncom_create_error_info(st, msg, out_err);
      }
      return st;
  }
#else
  #include <dlfcn.h>
  #include <sys/stat.h>
  struct ncom_plugin_handle_s { void *lib; const ncom_plugin_api_v1_t *api; };
  /* Map a dlopen failure to NCOM_E_NOT_FOUND (file missing) or NCOM_E_FAIL (other, e.g. missing deps). */
  static ncom_status_t posix_load_error(const char *path, ncom_ierror_info_t **out_err) {
      struct stat sb;
      ncom_status_t st = (stat(path, &sb) != 0) ? NCOM_E_NOT_FOUND : NCOM_E_FAIL;
      if (out_err) {
          const char *err_msg = dlerror();
          ncom_create_error_info(st, err_msg ? err_msg : "Unknown dlopen error", out_err);
      }
      return st;
  }

  // POSIX dlsym() returns a data pointer; converting it to a function pointer is
  // technically not ISO C, but it is the standard POSIX pattern. We wrap it to
  // keep the cast in one place and silence -Wpedantic in most toolchains.
  static ncom_plugin_get_api_v1_fn ncom_sym_fnptr_get_api(void *lib, const char *name)
  {
      void *p = dlsym(lib, name);
      ncom_plugin_get_api_v1_fn fn = NULL;
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
      fn = (ncom_plugin_get_api_v1_fn)p;
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
      return fn;
  }
#endif

ncom_status_t ncom_plugin_load(const char *path, ncom_plugin_handle_t **out_handle, const ncom_plugin_api_v1_t **out_api, ncom_ierror_info_t **out_err)
{
    ncom_status_t st = NCOM_OK;
    ncom_plugin_handle_t *h = NULL;
    ncom_plugin_get_api_v1_fn get_api = NULL;
    const ncom_plugin_api_v1_t *api = NULL;

    if (out_handle) *out_handle = NULL;
    if (out_api) *out_api = NULL;
    if (out_err) *out_err = NULL;

    NCOM_CHECK_NULL(path);
    NCOM_CHECK_NULL(out_handle);
    NCOM_CHECK_NULL(out_api);

    h = (ncom_plugin_handle_t *)calloc(1, sizeof(*h));
    if (!h) { st = NCOM_E_NO_MEM; goto cleanup; }

#ifdef _WIN32
    h->lib = LoadLibraryA(path);
    if (!h->lib) { st = win_load_error(out_err); goto cleanup; }
#else
    h->lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!h->lib) { st = posix_load_error(path, out_err); goto cleanup; }
#endif

    // Retrieve the strictly defined C entry point symbol
#ifdef _WIN32
    get_api = (ncom_plugin_get_api_v1_fn)sym(h->lib, NCOM_PLUGIN_GET_API_V1_SYMBOL);
#else
    get_api = ncom_sym_fnptr_get_api(h->lib, NCOM_PLUGIN_GET_API_V1_SYMBOL);
#endif
    if (!get_api) { st = NCOM_E_FAIL; goto cleanup; } /* symbol not found: not a valid ncom plugin */

    api = get_api();
    if (!api || api->abi_version != 1 || !api->create_instance) {
        st = NCOM_E_FAIL;
        goto cleanup;
    }

    h->api = api;
    
    // Initialize the plugin if it provides an initialization hook
    if (api->plugin_init) {
        api->plugin_init();
    }

    *out_handle = h;
    *out_api = api;
    h = NULL;

cleanup:
    if (h) ncom_plugin_unload(h);
    return st;
}

void ncom_plugin_unload(ncom_plugin_handle_t *handle)
{
    if (!handle) return;

    if (handle->api && handle->api->plugin_shutdown) {
        handle->api->plugin_shutdown();
    }

#ifdef _WIN32
    if (handle->lib) FreeLibrary(handle->lib);
#else
    if (handle->lib) dlclose(handle->lib);
#endif
    
    free(handle);
}
