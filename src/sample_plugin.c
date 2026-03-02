#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <time.h>

#include "plugin_api.h"
#include "nano_style.h"
#include "nano_atomic.h"
#include "nano_base.h"

#include "i_clock.h"
#include "i_clock2.h"

typedef struct sample_string_s {
    i_string_t iface;
    nano_refcnt_t ref_cnt;
    uint64_t len_bytes;
    char *data; /* NUL-terminated UTF-8 */
} sample_string_t;

static status_t sample_string_qi(i_unknown_t *self_u, nanoc_iid_t iid, void **out)
{
    sample_string_t *s = (sample_string_t *)self_u;
    if (!out) return STATUS_E_INVALID_ARG;
    *out = NULL;

    if (NANOC_IID_EQ(iid, IID_I_UNKNOWN) || NANOC_IID_EQ(iid, IID_I_STRING)) {
        *out = &s->iface;
        (void)nano_refcnt_inc(&s->ref_cnt);
        return STATUS_OK;
    }
    return STATUS_E_NOT_FOUND;
}

static uint32_t sample_string_add_ref(i_unknown_t *self_u)
{
    sample_string_t *s = (sample_string_t *)self_u;
    return nano_refcnt_inc(&s->ref_cnt);
}

static uint32_t sample_string_release(i_unknown_t *self_u)
{
    sample_string_t *s = (sample_string_t *)self_u;
    uint32_t rc = nano_refcnt_dec(&s->ref_cnt);
    if (rc == 0) {
        free(s->data);
        free(s);
    }
    return rc;
}

static status_t sample_string_c_str(i_string_t *self, const char **out_ptr)
{
    const sample_string_t *s = (sample_string_t *)self;
    if (!out_ptr) return STATUS_E_INVALID_ARG;
    *out_ptr = s->data ? s->data : "";
    return STATUS_OK;
}

static status_t sample_string_length(i_string_t *self, uint64_t *out_len_bytes)
{
    const sample_string_t *s = (sample_string_t *)self;
    if (!out_len_bytes) return STATUS_E_INVALID_ARG;
    *out_len_bytes = s->len_bytes;
    return STATUS_OK;
}

static const i_string_vtbl_t SAMPLE_STRING_VTBL = {
    .base = {
        .query_interface = sample_string_qi,
        .add_ref = sample_string_add_ref,
        .release = sample_string_release
    },
    .c_str = sample_string_c_str,
    .length = sample_string_length
};

static status_t sample_string_create(const char *utf8, i_string_t **out)
{
    status_t st = STATUS_OK;
    sample_string_t *s = NULL;
    size_t n = 0;

    if (out) *out = NULL;
    if (!out) return STATUS_E_INVALID_ARG;

    if (!utf8) utf8 = "";
    n = strlen(utf8);

    s = (sample_string_t *)calloc(1, sizeof(*s));
    if (!s) { st = STATUS_E_NO_MEM; goto cleanup; }

    s->data = (char *)calloc(n + 1, 1);
    if (!s->data) { st = STATUS_E_NO_MEM; goto cleanup; }

    memcpy(s->data, utf8, n);
    s->len_bytes = n;
    s->iface.vtbl = &SAMPLE_STRING_VTBL;
    nano_refcnt_init(&s->ref_cnt, 1);

    *out = &s->iface;
    s = NULL;

cleanup:
    if (s) { free(s->data); free(s); }
    return st;
}


typedef struct sample_error_info_s {
    i_error_info_t iface;
    nano_refcnt_t ref_cnt;
    status_t code;
    char *msg; /* NUL-terminated UTF-8 */
} sample_error_info_t;

static status_t sample_error_qi(i_unknown_t *self_u, nanoc_iid_t iid, void **out)
{
    sample_error_info_t *e = (sample_error_info_t *)self_u;
    if (!out) return STATUS_E_INVALID_ARG;
    *out = NULL;

    if (NANOC_IID_EQ(iid, IID_I_UNKNOWN) || NANOC_IID_EQ(iid, IID_I_ERROR_INFO)) {
        *out = &e->iface;
        (void)nano_refcnt_inc(&e->ref_cnt);
        return STATUS_OK;
    }
    return STATUS_E_NOT_FOUND;
}

static uint32_t sample_error_add_ref(i_unknown_t *self_u)
{
    sample_error_info_t *e = (sample_error_info_t *)self_u;
    return nano_refcnt_inc(&e->ref_cnt);
}

static uint32_t sample_error_release(i_unknown_t *self_u)
{
    sample_error_info_t *e = (sample_error_info_t *)self_u;
    uint32_t rc = nano_refcnt_dec(&e->ref_cnt);
    if (rc == 0) {
        free(e->msg);
        free(e);
    }
    return rc;
}

static status_t sample_error_get_code(i_error_info_t *self, status_t *out_code)
{
    const sample_error_info_t *e = (sample_error_info_t *)self;
    if (!out_code) return STATUS_E_INVALID_ARG;
    *out_code = e->code;
    return STATUS_OK;
}

static status_t sample_error_get_message_cstr(i_error_info_t *self, const char **out_msg)
{
    const sample_error_info_t *e = (sample_error_info_t *)self;
    if (!out_msg) return STATUS_E_INVALID_ARG;
    *out_msg = e->msg ? e->msg : "";
    return STATUS_OK;
}

static status_t sample_error_get_message_string(i_error_info_t *self, i_string_t **out_str)
{
    const char *m = NULL;
    if (out_str) *out_str = NULL;
    if (!self || !out_str) return STATUS_E_INVALID_ARG;
    (void)sample_error_get_message_cstr(self, &m);
    return sample_string_create(m, out_str);
}

/* Caller-buffer variant. out_len_incl_nul includes the NUL terminator. */
static status_t sample_error_get_message_buf(i_error_info_t *self, char *buf, uint64_t cap, uint64_t *out_len_incl_nul)
{
    const sample_error_info_t *e = (sample_error_info_t *)self;
    uint64_t need = 1;
    if (!out_len_incl_nul) return STATUS_E_INVALID_ARG;

    if (e->msg) need = (uint64_t)strlen(e->msg) + 1;
    *out_len_incl_nul = need;

    if (!buf || cap == 0) return STATUS_OK; /* sizing call */
    if (cap < need) return STATUS_E_MORE_DATA;

    if (need == 1) { buf[0] = '\0'; return STATUS_OK; }
    memcpy(buf, e->msg, (size_t)need);
    return STATUS_OK;
}

static const i_error_info_vtbl_t SAMPLE_ERROR_VTBL = {
    .base = {
        .query_interface = sample_error_qi,
        .add_ref = sample_error_add_ref,
        .release = sample_error_release
    },
    .get_code = sample_error_get_code,
    .get_message_cstr = sample_error_get_message_cstr,
    .get_message_string = sample_error_get_message_string,
    .get_message_buf = sample_error_get_message_buf
};

static status_t sample_error_create(status_t code, const char *msg, i_error_info_t **out_err)
{
    status_t st = STATUS_OK;
    sample_error_info_t *e = NULL;
    size_t n = 0;

    if (out_err) *out_err = NULL;
    if (!out_err) return STATUS_E_INVALID_ARG;

    if (!msg) msg = "";
    n = strlen(msg);

    e = (sample_error_info_t *)calloc(1, sizeof(*e));
    if (!e) { st = STATUS_E_NO_MEM; goto cleanup; }

    e->msg = (char *)calloc(n + 1, 1);
    if (!e->msg) { st = STATUS_E_NO_MEM; goto cleanup; }
    memcpy(e->msg, msg, n);

    e->code = code;
    e->iface.vtbl = &SAMPLE_ERROR_VTBL;
    nano_refcnt_init(&e->ref_cnt, 1);

    *out_err = &e->iface;
    e = NULL;

cleanup:
    if (e) { free(e->msg); free(e); }
    return st;
}

/* A single object implementing multiple interfaces (i_logger + i_clock).
   Each interface is a "view" pointing into the same allocation. */
typedef struct sample_component_s {
    i_logger_t logger_iface;
    i_clock_t  clock_iface;
    i_clock2_t clock2_iface;
    nano_refcnt_t ref_cnt;
} sample_component_t;

static uint32_t sample_add_ref(sample_component_t *impl)
{
    return nano_refcnt_inc(&impl->ref_cnt);
}

static uint32_t sample_release(sample_component_t *impl)
{
    uint32_t rc = nano_refcnt_dec(&impl->ref_cnt);
    if (rc == 0) {
        free(impl);
    }
    return rc;
}

/* --- i_unknown methods for logger view --- */
static status_t sample_logger_qi(i_unknown_t *self_u, nanoc_iid_t iid, void **out)
{
    sample_component_t *impl = (sample_component_t *)self_u; /* logger_iface is first -> same address */
    if (!out) return STATUS_E_INVALID_ARG;
    *out = NULL;

    if (IID_EQ(iid, IID_I_UNKNOWN) || IID_EQ(iid, IID_I_LOGGER)) {
        *out = &impl->logger_iface;
        (void)sample_add_ref(impl);
        return STATUS_OK;
    }
    if (NANOC_IID_EQ(iid, IID_I_CLOCK) || NANOC_IID_EQ(iid, IID_I_CLOCK2)) {
        *out = &impl->clock_iface;
        (void)sample_add_ref(impl);
        return STATUS_OK;
    }
    return STATUS_E_NOT_FOUND;
}

static uint32_t sample_logger_add_ref(i_unknown_t *self_u)
{
    sample_component_t *impl = (sample_component_t *)self_u;
    return sample_add_ref(impl);
}

static uint32_t sample_logger_release(i_unknown_t *self_u)
{
    sample_component_t *impl = (sample_component_t *)self_u;
    return sample_release(impl);
}

static void sample_log(i_logger_t *self, int level, const char *msg)
{
    (void)self;
    fprintf(stderr, "[sample_plugin][%d] %s\n", level, msg ? msg : "(null)");
}

/* --- i_unknown methods for clock view --- */
/* Note: clock_iface is not the first field, so we must compute impl from the interface pointer. */
static sample_component_t *impl_from_clock(i_clock_t *self)
{
    /* clock_iface is a member inside sample_component_t; compute base address safely. */
    return (sample_component_t *)((char *)self - offsetof(sample_component_t, clock_iface));
}

static status_t sample_clock_qi(i_unknown_t *self_u, nanoc_iid_t iid, void **out)
{
    i_clock_t *self = (i_clock_t *)self_u;
    sample_component_t *impl = impl_from_clock(self);
    if (!out) return STATUS_E_INVALID_ARG;
    *out = NULL;

    if (NANOC_IID_EQ(iid, IID_I_UNKNOWN) || NANOC_IID_EQ(iid, IID_I_CLOCK)) {
        *out = &impl->clock_iface;
        (void)sample_add_ref(impl);
        return STATUS_OK;
    }
    if (IID_EQ(iid, IID_I_LOGGER)) {
        *out = &impl->logger_iface;
        (void)sample_add_ref(impl);
        return STATUS_OK;
    }
    return STATUS_E_NOT_FOUND;
}

static uint32_t sample_clock_add_ref(i_unknown_t *self_u)
{
    i_clock_t *self = (i_clock_t *)self_u;
    return sample_add_ref(impl_from_clock(self));
}

static uint32_t sample_clock_release(i_unknown_t *self_u)
{
    i_clock_t *self = (i_clock_t *)self_u;
    return sample_release(impl_from_clock(self));
}

static status_t sample_now_unix_ms(i_clock_t *self, int64_t *out_ms)
{
    (void)self;
    if (!out_ms) return STATUS_E_INVALID_ARG;
    *out_ms = 0;

#if defined(_WIN32)
    /* timespec_get is available on MSVC in C11 mode; if not, replace with GetSystemTimeAsFileTime. */
#endif
    struct timespec ts;
#if defined(TIME_UTC)
    if (timespec_get(&ts, TIME_UTC) != TIME_UTC) return STATUS_E_FAIL;
#else
    /* POSIX fallback */
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) return STATUS_E_FAIL;
#endif
    *out_ms = (int64_t)ts.tv_sec * 1000 + (int64_t)ts.tv_nsec / 1000000;
    return STATUS_OK;
}

/* Vtables: base slots point to i_unknown-compatible functions. */
static const i_logger_vtbl_t SAMPLE_LOGGER_VTBL = {
    .base = {
        .query_interface = sample_logger_qi,
        .add_ref = sample_logger_add_ref,
        .release = sample_logger_release
    },
    .log = sample_log
};

static const i_clock_vtbl_t SAMPLE_CLOCK_VTBL = {
    .base = {
        .query_interface = sample_clock_qi,
        .add_ref = sample_clock_add_ref,
        .release = sample_clock_release
    },
    .now_unix_ms = sample_now_unix_ms
};


/* --- i_unknown methods for clock2 view --- */
static sample_component_t *impl_from_clock2(i_clock2_t *self)
{
    return (sample_component_t *)((char *)self - offsetof(sample_component_t, clock2_iface));
}

static status_t sample_clock2_qi(i_unknown_t *self_u, iid_t iid, void **out)
{
    i_clock2_t *self = (i_clock2_t *)self_u;
    sample_component_t *impl = impl_from_clock2(self);
    if (!out) return STATUS_E_INVALID_ARG;
    *out = NULL;

    if (IID_EQ(iid, IID_I_UNKNOWN) || IID_EQ(iid, IID_I_CLOCK2)) {
        *out = &impl->clock2_iface;
        (void)sample_add_ref(impl);
        return STATUS_OK;
    }
    if (IID_EQ(iid, IID_I_CLOCK) ) {
        *out = &impl->clock_iface;
        (void)sample_add_ref(impl);
        return STATUS_OK;
    }
    if (IID_EQ(iid, IID_I_LOGGER)) {
        *out = &impl->logger_iface;
        (void)sample_add_ref(impl);
        return STATUS_OK;
    }
    return STATUS_E_NOT_FOUND;
}

static uint32_t sample_clock2_add_ref(i_unknown_t *self_u)
{
    i_clock2_t *self = (i_clock2_t *)self_u;
    return sample_add_ref(impl_from_clock2(self));
}

static uint32_t sample_clock2_release(i_unknown_t *self_u)
{
    i_clock2_t *self = (i_clock2_t *)self_u;
    return sample_release(impl_from_clock2(self));
}

static status_t sample_clock2_now_unix_ms(i_clock2_t *self, int64_t *out_ms, i_error_info_t **out_err)
{
    status_t st = STATUS_OK;

    if (out_err) *out_err = NULL;
    if (!out_ms) {
        if (out_err) (void)sample_error_create(STATUS_E_INVALID_ARG, "out_ms is NULL", out_err);
        return STATUS_E_INVALID_ARG;
    }

    /* Reuse the v1 clock implementation. */
    st = sample_now_unix_ms((i_clock_t *)self, out_ms);
    if (STATUS_FAILED(st)) {
        if (out_err) (void)sample_error_create(st, "clock acquisition failed", out_err);
    }
    return st;
}

static const i_clock2_vtbl_t SAMPLE_CLOCK2_VTBL = {
    .base = {
        .query_interface = sample_clock2_qi,
        .add_ref = sample_clock2_add_ref,
        .release = sample_clock2_release
    },
    .now_unix_ms = sample_clock2_now_unix_ms
};


static status_t sample_component_create(nanoc_iid_t iid, void **out)
{
    status_t st = STATUS_OK;
    sample_component_t *impl = NULL;

    if (!out) return STATUS_E_INVALID_ARG;
    *out = NULL;

    impl = (sample_component_t *)calloc(1, sizeof(*impl));
    if (!impl) return STATUS_E_NO_MEM;

    impl->logger_iface.vtbl = &SAMPLE_LOGGER_VTBL;
    impl->clock_iface.vtbl  = &SAMPLE_CLOCK_VTBL;
    impl->clock2_iface.vtbl = &SAMPLE_CLOCK2_VTBL;
    nano_refcnt_init(&impl->ref_cnt, 1);

    /* Return requested interface. This also defines the "this" pointer shape. */
    if (IID_EQ(iid, IID_I_UNKNOWN) || IID_EQ(iid, IID_I_LOGGER)) {
        *out = &impl->logger_iface;
        return STATUS_OK;
    }
    if (IID_EQ(iid, IID_I_CLOCK) || IID_EQ(iid, IID_I_CLOCK2)) {
        *out = &impl->clock_iface;
        return STATUS_OK;
    }

    /* Requested interface not supported. */
    (void)sample_release(impl);
    st = STATUS_E_NOT_FOUND;

    return st;
}

/* Plugin API create_instance: CLSID + IID */
static status_t plugin_create_instance(const nanoc_clsid_t *clsid, const nanoc_iid_t *iid, void **out)
{
    if (!out) return STATUS_E_INVALID_ARG;
    *out = NULL;

    if (!NANOC_CLSID_EQ(clsid, &CLSID_SAMPLE_COMPONENT)) {
        return STATUS_E_NOT_FOUND;
    }
    return sample_component_create(iid, out); // sample_component_create muss auch const nanoc_iid_t* annehmen!
}

static void plugin_init_impl(void) { /* optional */ }
static void plugin_shutdown_impl(void) { /* optional */ }

static const plugin_api_v1_t PLUGIN_API_V1 = {
    .abi_version = 1,
    .create_instance = plugin_create_instance,
    .plugin_init = plugin_init_impl,
    .plugin_shutdown = plugin_shutdown_impl
};

/* Single exported symbol. */
NANO_EXPORT const plugin_api_v1_t *plugin_get_api_v1(void)
{
    return &PLUGIN_API_V1;
}
