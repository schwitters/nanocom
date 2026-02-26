#pragma once
#include "nano_status.h"
#include "plugin_api.h"

typedef struct plugin_handle_s plugin_handle_t;

/* Load a plugin library from path and return its API v1. */
status_t plugin_load(const char *path, plugin_handle_t **out_handle, const plugin_api_v1_t **out_api);

/* Unload plugin library. Calls plugin_shutdown if present. */
void plugin_unload(plugin_handle_t *handle);
