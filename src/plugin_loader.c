#include "plugin_loader.h"
#include "nano_style.h"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #include <Windows.h>
  struct plugin_handle_s { HMODULE lib; const plugin_api_v1_t *api; };
  static void *sym(HMODULE lib, const char *name) { return (void *)GetProcAddress(lib, name); }
#else
  #include <dlfcn.h>
  struct plugin_handle_s { void *lib; const plugin_api_v1_t *api; };
  static void *sym(void *lib, const char *name) { return dlsym(lib, name); }
#endif

status_t plugin_load(const char *path, plugin_handle_t **out_handle, const plugin_api_v1_t **out_api)
{
    status_t st = STATUS_OK;
    plugin_handle_t *h = NULL;
    plugin_get_api_v1_fn get_api = NULL;
    const plugin_api_v1_t *api = NULL;

    if (out_handle) *out_handle = NULL;
    if (out_api) *out_api = NULL;

    CHECK_NULL(path);
    CHECK_NULL(out_handle);
    CHECK_NULL(out_api);

    h = (plugin_handle_t *)calloc(1, sizeof(*h));
    if (!h) { st = STATUS_E_NO_MEM; goto cleanup; }

#ifdef _WIN32
    h->lib = LoadLibraryA(path);
    if (!h->lib) { st = STATUS_E_NOT_FOUND; goto cleanup; }
#else
    h->lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!h->lib) { st = STATUS_E_NOT_FOUND; goto cleanup; }
#endif

    get_api = (plugin_get_api_v1_fn)sym(h->lib, PLUGIN_GET_API_V1_SYMBOL);
    if (!get_api) { st = STATUS_E_NOT_IMPL; goto cleanup; }

    api = get_api();
    if (!api || api->abi_version != 1 || !api->create_instance) {
        st = STATUS_E_FAIL;
        goto cleanup;
    }

    h->api = api;
    if (api->plugin_init) api->plugin_init();

    *out_handle = h;
    *out_api = api;
    h = NULL;

cleanup:
    if (h) plugin_unload(h);
    return st;
}

void plugin_unload(plugin_handle_t *handle)
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
