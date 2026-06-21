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
 * @file sample_plugin.c
 * @brief Sample plugin.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Include the ncom framework and core implementations */
#include <ncom/core_impl.h>
#include <ncom/ncom.h>

/* Include the generated IDL header (assuming nidlgen produced demo.h) */
#include "demo.h"

/**
 * @brief The unified component implementing multiple interfaces.
 * * Memory layout: The interfaces are embedded directly.
 * NCOM_CONTAINER_OF is used to navigate from any interface pointer
 * back to this main structure.
 */
typedef struct {
  demo_ilogger_t logger_iface;
  demo_ilogger2_t logger2_iface;
  demo_iclock_t clock_iface;
  demo_iclock2_t clock2_iface;
  demo_icapabilities_t capabilities_iface;
  ncom_refcnt_t ref_cnt;
} sample_component_t;

typedef struct {
  demo_icomponent_factory_t iface;
  ncom_refcnt_t ref_cnt;
} sample_factory_t;

enum {
  SAMPLE_API_MAJOR = 1u,
  SAMPLE_API_MINOR = 0u
};

/* --- Common Lifetime Management --- */

static uint32_t sample_add_ref(sample_component_t* impl) {
  return ncom_refcnt_inc(&impl->ref_cnt);
}

static uint32_t sample_release(sample_component_t* impl) {
  uint32_t rc = ncom_refcnt_dec(&impl->ref_cnt);
  if (rc == 0) {
    free(impl);
  }
  return rc;
}

/* --- Common QueryInterface Logic --- */

/**
 * @brief Centralized QueryInterface logic to enforce the COM Identity Rule.
 * * No matter which interface the caller uses to call query_interface,
 * asking for NCOM_IID_IUNKNOWN MUST always return the exact same base pointer
 * (in this case, logger_iface).
 */
static ncom_status_t sample_common_qi(sample_component_t* impl,
                                      const ncom_iid_t* iid,
                                      void** out) {
  if (!iid || !out)
    return NCOM_E_INVALID_ARG;
  *out = NULL;

  /* COM Identity Rule: Always return the first interface for IUnknown */
  if (NCOM_IID_EQ(iid, &NCOM_IID_IUNKNOWN) ||
      NCOM_IID_EQ(iid, &DEMO_IID_ILOGGER)) {
    *out = &impl->logger_iface;
    sample_add_ref(impl);
    return NCOM_OK;
  }
  if (NCOM_IID_EQ(iid, &DEMO_IID_ILOGGER2)) {
    *out = &impl->logger2_iface;
    sample_add_ref(impl);
    return NCOM_OK;
  }
  if (NCOM_IID_EQ(iid, &DEMO_IID_ICLOCK)) {
    *out = &impl->clock_iface;
    sample_add_ref(impl);
    return NCOM_OK;
  }
  if (NCOM_IID_EQ(iid, &DEMO_IID_ICLOCK2)) {
    *out = &impl->clock2_iface;
    sample_add_ref(impl);
    return NCOM_OK;
  }
  if (NCOM_IID_EQ(iid, &DEMO_IID_ICAPABILITIES)) {
    *out = &impl->capabilities_iface;
    sample_add_ref(impl);
    return NCOM_OK;
  }

  return NCOM_E_NO_INTERFACE;
}

/* ============================================================================
 * Logger View Implementation
 * ============================================================================
 */

static ncom_status_t sample_logger_qi(ncom_iunknown_t* self_u,
                                      const ncom_iid_t* iid,
                                      void** out) {
  sample_component_t* impl =
      NCOM_CONTAINER_OF(self_u, sample_component_t, logger_iface);
  return sample_common_qi(impl, iid, out);
}

static uint32_t sample_logger_add_ref(ncom_iunknown_t* self_u) {
  return sample_add_ref(
      NCOM_CONTAINER_OF(self_u, sample_component_t, logger_iface));
}

static uint32_t sample_logger_release(ncom_iunknown_t* self_u) {
  return sample_release(
      NCOM_CONTAINER_OF(self_u, sample_component_t, logger_iface));
}

static void sample_log(demo_ilogger_t* self, int32_t level, const char* msg) {
  (void)self; /* Unused in this simple implementation */
  fprintf(stderr, "[sample_plugin][%d] %s\n", (int)level, msg ? msg : "(null)");
}

static const demo_ilogger_vtbl_t SAMPLE_LOGGER_VTBL = {
    .base = {.query_interface = sample_logger_qi,
             .add_ref = sample_logger_add_ref,
             .release = sample_logger_release},
    .log = sample_log};

/* ============================================================================
 * Logger2 View Implementation
 * ============================================================================
 */

static ncom_status_t sample_logger2_qi(ncom_iunknown_t* self_u,
                                       const ncom_iid_t* iid,
                                       void** out) {
  sample_component_t* impl =
      NCOM_CONTAINER_OF(self_u, sample_component_t, logger2_iface);
  return sample_common_qi(impl, iid, out);
}

static uint32_t sample_logger2_add_ref(ncom_iunknown_t* self_u) {
  return sample_add_ref(
      NCOM_CONTAINER_OF(self_u, sample_component_t, logger2_iface));
}

static uint32_t sample_logger2_release(ncom_iunknown_t* self_u) {
  return sample_release(
      NCOM_CONTAINER_OF(self_u, sample_component_t, logger2_iface));
}

static ncom_status_t sample_log_view(demo_ilogger2_t* self,
                                     log_level level,
                                     ncom_string_view_t msg) {
  char tmp[512];
  const uint8_t* ptr = msg.ptr;
  uint32_t n = msg.len;
  (void)self;

  if ((ptr == NULL && n != 0u) || n >= sizeof(tmp)) {
    return NCOM_E_INVALID_ARG;
  }

  if (!ptr || n == 0u) {
    sample_log((demo_ilogger_t*)self, level, "");
    return NCOM_OK;
  }

  memcpy(tmp, ptr, (size_t)n);
  tmp[n] = '\0';
  sample_log((demo_ilogger_t*)self, level, tmp);
  return NCOM_OK;
}

static const demo_ilogger2_vtbl_t SAMPLE_LOGGER2_VTBL = {
    .base = {
        .base = {
            .query_interface = sample_logger2_qi,
            .add_ref = sample_logger2_add_ref,
            .release = sample_logger2_release,
        },
        .log = sample_log,
    },
    .log_view = sample_log_view,
};

/* ============================================================================
 * Clock View Implementation
 * ============================================================================
 */

static ncom_status_t sample_clock_qi(ncom_iunknown_t* self_u,
                                     const ncom_iid_t* iid,
                                     void** out) {
  sample_component_t* impl =
      NCOM_CONTAINER_OF(self_u, sample_component_t, clock_iface);
  return sample_common_qi(impl, iid, out);
}

static uint32_t sample_clock_add_ref(ncom_iunknown_t* self_u) {
  return sample_add_ref(
      NCOM_CONTAINER_OF(self_u, sample_component_t, clock_iface));
}

static uint32_t sample_clock_release(ncom_iunknown_t* self_u) {
  return sample_release(
      NCOM_CONTAINER_OF(self_u, sample_component_t, clock_iface));
}

static ncom_status_t sample_now_unix_ms(demo_iclock_t* self, int64_t* out_ms) {
  (void)self;
  if (!out_ms)
    return NCOM_E_INVALID_ARG;
  *out_ms = 0;

  struct timespec ts;
#if defined(TIME_UTC)
  if (timespec_get(&ts, TIME_UTC) != TIME_UTC)
    return NCOM_E_FAIL;
#else
  if (clock_gettime(CLOCK_REALTIME, &ts) != 0)
    return NCOM_E_FAIL;
#endif

  *out_ms = (int64_t)ts.tv_sec * 1000 + (int64_t)ts.tv_nsec / 1000000;
  return NCOM_OK;
}

static const demo_iclock_vtbl_t SAMPLE_CLOCK_VTBL = {
    .base = {sample_clock_qi, sample_clock_add_ref, sample_clock_release},
    .now_unix_ms = sample_now_unix_ms};

/* ============================================================================
 * Clock2 View Implementation (Rich Errors)
 * ============================================================================
 */

static ncom_status_t sample_clock2_qi(ncom_iunknown_t* self_u,
                                      const ncom_iid_t* iid,
                                      void** out) {
  sample_component_t* impl =
      NCOM_CONTAINER_OF(self_u, sample_component_t, clock2_iface);
  return sample_common_qi(impl, iid, out);
}

static uint32_t sample_clock2_add_ref(ncom_iunknown_t* self_u) {
  return sample_add_ref(
      NCOM_CONTAINER_OF(self_u, sample_component_t, clock2_iface));
}

static uint32_t sample_clock2_release(ncom_iunknown_t* self_u) {
  return sample_release(
      NCOM_CONTAINER_OF(self_u, sample_component_t, clock2_iface));
}

static ncom_status_t sample_clock2_now_unix_ms(demo_iclock2_t* self,
                                               int64_t* out_ms,
                                               ncom_ierror_info_t** out_err) {
  ncom_status_t st = NCOM_OK;

  if (out_err)
    *out_err = NULL;

  if (!out_ms) {
    /* Use the new core implementation to create a rich error object
     * effortlessly! */
    if (out_err) {
      ncom_create_error_info(NCOM_E_INVALID_ARG,
                             "The out_ms pointer must not be NULL", out_err);
    }
    return NCOM_E_INVALID_ARG;
  }

  /* Reuse the v1 clock implementation. Cast is safe because base vtbl layout
   * matches. */
  st = sample_now_unix_ms((demo_iclock_t*)self, out_ms);
  if (NCOM_FAILED(st)) {
    if (out_err) {
      ncom_create_error_info(st, "Hardware clock acquisition failed internally",
                             out_err);
    }
  }
  return st;
}

static const demo_iclock2_vtbl_t SAMPLE_CLOCK2_VTBL = {
    .base =
        {
            .base =
                {
                    .query_interface = sample_clock2_qi,
                    .add_ref = sample_clock2_add_ref,
                    .release = sample_clock2_release,
                },
            .now_unix_ms = sample_now_unix_ms, /* v1 clock method */
        },
    .now_unix_ms = sample_clock2_now_unix_ms, /* v2 rich-error method */
};

/* ============================================================================
 * Capabilities View Implementation
 * ============================================================================
 */

static ncom_status_t sample_capabilities_qi(ncom_iunknown_t* self_u,
                                            const ncom_iid_t* iid,
                                            void** out) {
  sample_component_t* impl =
      NCOM_CONTAINER_OF(self_u, sample_component_t, capabilities_iface);
  return sample_common_qi(impl, iid, out);
}

static uint32_t sample_capabilities_add_ref(ncom_iunknown_t* self_u) {
  return sample_add_ref(
      NCOM_CONTAINER_OF(self_u, sample_component_t, capabilities_iface));
}

static uint32_t sample_capabilities_release(ncom_iunknown_t* self_u) {
  return sample_release(
      NCOM_CONTAINER_OF(self_u, sample_component_t, capabilities_iface));
}

static ncom_status_t sample_get_api_version(demo_icapabilities_t* self,
                                            uint32_t* out_major,
                                            uint32_t* out_minor) {
  (void)self;
  if (!out_major || !out_minor) {
    return NCOM_E_INVALID_ARG;
  }
  *out_major = SAMPLE_API_MAJOR;
  *out_minor = SAMPLE_API_MINOR;
  return NCOM_OK;
}

static ncom_status_t sample_get_component_capabilities(
    demo_icapabilities_t* self,
    uint32_t* out_supports_logger2,
    uint32_t* out_supports_clock2,
    uint32_t* out_supports_capabilities,
    uint32_t* out_supports_factory) {
  (void)self;
  if (!out_supports_logger2 || !out_supports_clock2 ||
      !out_supports_capabilities || !out_supports_factory) {
    return NCOM_E_INVALID_ARG;
  }

  *out_supports_logger2 = 1u;
  *out_supports_clock2 = 1u;
  *out_supports_capabilities = 1u;
  *out_supports_factory = 1u;
  return NCOM_OK;
}

static const demo_icapabilities_vtbl_t SAMPLE_CAPABILITIES_VTBL = {
    .base = {
        .query_interface = sample_capabilities_qi,
        .add_ref = sample_capabilities_add_ref,
        .release = sample_capabilities_release,
    },
    .get_api_version = sample_get_api_version,
    .get_component_capabilities = sample_get_component_capabilities,
};
/* ============================================================================
 * Factory & Plugin Export
 * ============================================================================
 */

static ncom_status_t sample_component_create(const ncom_iid_t* iid,
                                             void** out) {
  ncom_status_t st = NCOM_OK;
  sample_component_t* impl = NULL;

  if (!iid || !out)
    return NCOM_E_INVALID_ARG;
  *out = NULL;

  impl = (sample_component_t*)calloc(1, sizeof(*impl));
  if (!impl)
    return NCOM_E_NO_MEM;

  /* Initialize VTables */
  impl->logger_iface.vtbl = &SAMPLE_LOGGER_VTBL;
  impl->logger2_iface.vtbl = &SAMPLE_LOGGER2_VTBL;
  impl->clock_iface.vtbl = &SAMPLE_CLOCK_VTBL;
  impl->clock2_iface.vtbl = &SAMPLE_CLOCK2_VTBL;
  impl->capabilities_iface.vtbl = &SAMPLE_CAPABILITIES_VTBL;
  ncom_refcnt_init(&impl->ref_cnt, 1);

  /* Use the common QI to fetch the requested interface.
     If it fails, the object destroys itself cleanly. */
  st = sample_common_qi(impl, iid, out);

  /* Release the initial reference. If QI succeeded, ref_cnt is 1.
     If QI failed, ref_cnt drops to 0 and 'impl' is freed. */
  sample_release(impl);

  return st;
}

static ncom_status_t sample_factory_qi(ncom_iunknown_t* self_u,
                                       const ncom_iid_t* iid,
                                       void** out) {
  sample_factory_t* impl = NCOM_CONTAINER_OF(self_u, sample_factory_t, iface);
  if (!out || !iid) {
    return NCOM_E_INVALID_ARG;
  }
  *out = NULL;

  if (NCOM_IID_EQ(iid, &NCOM_IID_IUNKNOWN) ||
      NCOM_IID_EQ(iid, &DEMO_IID_ICOMPONENT_FACTORY)) {
    *out = &impl->iface;
    ncom_refcnt_inc(&impl->ref_cnt);
    return NCOM_OK;
  }
  return NCOM_E_NO_INTERFACE;
}

static uint32_t sample_factory_add_ref(ncom_iunknown_t* self_u) {
  sample_factory_t* impl = NCOM_CONTAINER_OF(self_u, sample_factory_t, iface);
  return ncom_refcnt_inc(&impl->ref_cnt);
}

static uint32_t sample_factory_release(ncom_iunknown_t* self_u) {
  sample_factory_t* impl = NCOM_CONTAINER_OF(self_u, sample_factory_t, iface);
  uint32_t rc = ncom_refcnt_dec(&impl->ref_cnt);
  if (rc == 0) {
    free(impl);
  }
  return rc;
}

static ncom_status_t sample_factory_create_component(
    demo_icomponent_factory_t* self,
    const ncom_iid_t* iid,
    ncom_iunknown_t** out_obj,
    ncom_ierror_info_t** out_err) {
  ncom_status_t st = NCOM_OK;
  const ncom_iid_t* req_iid = iid;
  (void)self;

  if (out_err) {
    *out_err = NULL;
  }
  if (!out_obj) {
    return NCOM_E_INVALID_ARG;
  }
  *out_obj = NULL;

  if (!req_iid) {
    req_iid = &NCOM_IID_IUNKNOWN;
  }

  st = sample_component_create(req_iid, (void**)out_obj);
  if (NCOM_FAILED(st) && out_err) {
    (void)ncom_create_error_info(st, "Factory failed to create sample component", out_err);
  }
  return st;
}

static const demo_icomponent_factory_vtbl_t SAMPLE_COMPONENT_FACTORY_VTBL = {
    .base = {
        .query_interface = sample_factory_qi,
        .add_ref = sample_factory_add_ref,
        .release = sample_factory_release,
    },
    .create_sample_component = sample_factory_create_component,
};

static ncom_status_t sample_component_factory_create(const ncom_iid_t* iid,
                                                     void** out) {
  ncom_status_t st = NCOM_OK;
  sample_factory_t* impl = NULL;
  ncom_iunknown_t* u = NULL;

  if (!iid || !out) {
    return NCOM_E_INVALID_ARG;
  }
  *out = NULL;

  impl = (sample_factory_t*)calloc(1, sizeof(*impl));
  if (!impl) {
    return NCOM_E_NO_MEM;
  }

  impl->iface.vtbl = &SAMPLE_COMPONENT_FACTORY_VTBL;
  ncom_refcnt_init(&impl->ref_cnt, 1);

  u = (ncom_iunknown_t*)&impl->iface;
  st = u->vtbl->query_interface(u, iid, out);
  u->vtbl->release(u);
  return st;
}

static ncom_status_t plugin_create_instance(const ncom_clsid_t* clsid,
                                            const ncom_iid_t* iid,
                                            void** out) {
  if (!clsid)
    return NCOM_E_INVALID_ARG;
  if (!iid)
    return NCOM_E_INVALID_ARG;
  if (!out)
    return NCOM_E_INVALID_ARG;
  *out = NULL;

  /* Compare against the generated CLSID using the macro */
  if (!NCOM_CLSID_EQ(clsid, &DEMO_CLSID_SAMPLE_COMPONENT)) {
    if (NCOM_CLSID_EQ(clsid, &DEMO_CLSID_SAMPLE_COMPONENT_FACTORY)) {
      return sample_component_factory_create(iid, out);
    }
    return NCOM_E_NOT_FOUND;
  }

  return sample_component_create(iid, out);
}

static void plugin_init_impl(void) { /* Optional setup logic */ }
static void plugin_shutdown_impl(void) { /* Optional cleanup logic */ }

/* Exported single entry point (uses NCOM_EXPORT if defined in headers, or
 * default visibility) */
NCOM_PLUGIN_EXPORT const ncom_plugin_api_v1_t* ncom_plugin_get_api_v1(void) {
  static const ncom_plugin_api_v1_t api = {
      .abi_version = 1,
      .create_instance = plugin_create_instance,
      .plugin_init = plugin_init_impl,
      .plugin_shutdown = plugin_shutdown_impl};
  return &api;
}
