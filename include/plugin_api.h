#pragma once
#include <stdint.h>
#include "nano_status.h"
#include "nano_ids.h"

/* Plugin API v1:
   The host loads a shared library and looks up PLUGIN_GET_API_V1_SYMBOL.
   The returned table stays valid until the library is unloaded. */

typedef status_t (*create_instance_fn)(clsid_t clsid, iid_t iid, void **out);

typedef struct plugin_api_v1_s {
    uint32_t abi_version;         /* must be 1 */
    create_instance_fn create_instance;
    void (*plugin_init)(void);    /* optional, may be NULL */
    void (*plugin_shutdown)(void);/* optional, may be NULL */
} plugin_api_v1_t;

/* Export name for the single entry point. */
#define PLUGIN_GET_API_V1_SYMBOL "plugin_get_api_v1"

#ifdef _WIN32
  #define NANO_EXPORT __declspec(dllexport)
#else
  #define NANO_EXPORT __attribute__((visibility("default")))
#endif

typedef const plugin_api_v1_t *(*plugin_get_api_v1_fn)(void);
