#include <ncom/plugin.h>
#include <ncom/plugin_loader.h>
#include <ncom/style.h>

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #include <Windows.h>
  struct ncom_plugin_handle_s { HMODULE lib; const ncom_plugin_api_v1_t *api; };
  static void *sym(HMODULE lib, const char *name) { return (void *)GetProcAddress(lib, name); }
#else
  #include <dlfcn.h>
  struct ncom_plugin_handle_s { void *lib; const ncom_plugin_api_v1_t *api; };
  static void *sym(void *lib, const char *name) { return dlsym(lib, name); }
#endif

ncom_status_t ncom_plugin_load(const char *path, ncom_plugin_handle_t **out_handle, const ncom_plugin_api_v1_t **out_api)
{
    ncom_status_t st = NCOM_OK;
    ncom_plugin_handle_t *h = NULL;
    ncom_plugin_get_api_v1_fn get_api = NULL;
    const ncom_plugin_api_v1_t *api = NULL;

    if (out_handle) *out_handle = NULL;
    if (out_api) *out_api = NULL;

    NCOM_CHECK_NULL(path);
    NCOM_CHECK_NULL(out_handle);
    NCOM_CHECK_NULL(out_api);

    h = (ncom_plugin_handle_t *)calloc(1, sizeof(*h));
    if (!h) { st = NCOM_E_NO_MEM; goto cleanup; }

#ifdef _WIN32
    h->lib = LoadLibraryA(path);
    if (!h->lib) { st = NCOM_E_NOT_FOUND; goto cleanup; }
#else
    h->lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!h->lib) { st = NCOM_E_NOT_FOUND; goto cleanup; }
#endif

    // Retrieve the strictly defined C entry point symbol
    get_api = (ncom_plugin_get_api_v1_fn)sym(h->lib, NCOM_PLUGIN_GET_API_V1_SYMBOL);
    if (!get_api) { st = NCOM_E_NOT_IMPL; goto cleanup; }

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