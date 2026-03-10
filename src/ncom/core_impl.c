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
 * @file core_impl.c
 * @brief Core impl.
 */

#include <ncom/ncom.h>
#include <ncom/string.h>
#include <ncom/core_impl.h>

#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * IID Definitions (extern const declarations live in the public headers)
 * ============================================================================ */

const ncom_iid_t NCOM_IID_IUNKNOWN   = { 0x589dfb30790e4b07ULL, 0x952a7857d37828dcULL };
const ncom_iid_t NCOM_IID_IFACTORY   = { 0x74751d837fe74171ULL, 0xb280525960783e1bULL };
const ncom_iid_t NCOM_IID_ISTRING    = { 0x7dcad1ee32974171ULL, 0xb36302fc16e42721ULL };
const ncom_iid_t NCOM_IID_IERRORINFO = { 0x758cd93d790a49bfULL, 0xa7868baf9b6a9285ULL };

/* ============================================================================
 * Standard ncom_istring implementation
 * ============================================================================ */

typedef struct {
    ncom_istring_t iface;
    ncom_refcnt_t ref_cnt;
    uint64_t len_bytes;
    char *data; /* NUL-terminated UTF-8 */
} ncom_string_impl_t;

static ncom_status_t string_qi(ncom_iunknown_t *self_u, const ncom_iid_t *iid, void **out)
{
    ncom_string_impl_t *s = NCOM_CONTAINER_OF(self_u, ncom_string_impl_t, iface);
    if (!out) return NCOM_E_INVALID_ARG;
    *out = NULL;

    if (NCOM_IID_EQ(iid, &NCOM_IID_IUNKNOWN) || NCOM_IID_EQ(iid, &NCOM_IID_ISTRING)) {
        *out = &s->iface;
        ncom_refcnt_inc(&s->ref_cnt);
        return NCOM_OK;
    }
    return NCOM_E_NO_INTERFACE;
}

static uint32_t string_add_ref(ncom_iunknown_t *self_u)
{
    ncom_string_impl_t *s = NCOM_CONTAINER_OF(self_u, ncom_string_impl_t, iface);
    return ncom_refcnt_inc(&s->ref_cnt);
}

static uint32_t string_release(ncom_iunknown_t *self_u)
{
    ncom_string_impl_t *s = NCOM_CONTAINER_OF(self_u, ncom_string_impl_t, iface);
    uint32_t rc = ncom_refcnt_dec(&s->ref_cnt);
    if (rc == 0) {
        free(s->data);
        free(s);
    }
    return rc;
}

static ncom_status_t string_c_str(ncom_istring_t *self, const char **out_ptr)
{
    if (!out_ptr) return NCOM_E_INVALID_ARG;
    if (!self) return NCOM_E_INVALID_ARG;
    ncom_string_impl_t *s = NCOM_CONTAINER_OF(self, ncom_string_impl_t, iface);
    *out_ptr = s->data ? s->data : "";
    return NCOM_OK;
}

static ncom_status_t string_length(ncom_istring_t *self, uint64_t *out_len_bytes)
{
    if (!out_len_bytes) return NCOM_E_INVALID_ARG;
    if (!self) return NCOM_E_INVALID_ARG;
    ncom_string_impl_t *s = NCOM_CONTAINER_OF(self, ncom_string_impl_t, iface);
    *out_len_bytes = s->len_bytes;
    return NCOM_OK;
}

static const ncom_istring_vtbl_t STRING_VTBL = {
    .base = {
        .query_interface = string_qi,
        .add_ref = string_add_ref,
        .release = string_release
    },
    .c_str = string_c_str,
    .length = string_length
};

ncom_status_t ncom_create_string(const char *utf8, ncom_istring_t **out)
{
    ncom_status_t st = NCOM_OK;
    ncom_string_impl_t *s = NULL;
    size_t n = 0;

    if (out) *out = NULL;
    if (!out) return NCOM_E_INVALID_ARG;

    if (!utf8) utf8 = "";
    n = strlen(utf8);
    if (n > (SIZE_MAX - 1u)) { 
        st = NCOM_E_NO_MEM; 
        goto cleanup; 
    }
    s = (ncom_string_impl_t *)calloc(1, sizeof(*s));
    if (!s) { st = NCOM_E_NO_MEM; goto cleanup; }

    s->data = (char *)calloc(n + 1, 1);
    if (!s->data) { st = NCOM_E_NO_MEM; goto cleanup; }

    memcpy(s->data, utf8, n);
    s->len_bytes = n;
    s->iface.vtbl = &STRING_VTBL;
    ncom_refcnt_init(&s->ref_cnt, 1);

    *out = &s->iface;
    s = NULL;

cleanup:
    if (s) { free(s->data); free(s); }
    return st;
}


/* ============================================================================
 * Standard ncom_ierror_info implementation
 * ============================================================================ */

typedef struct {
    ncom_ierror_info_t iface;
    ncom_refcnt_t ref_cnt;
    ncom_status_t code;
    char *msg; /* NUL-terminated UTF-8 */
} ncom_error_info_impl_t;

static ncom_status_t error_qi(ncom_iunknown_t *self_u, const ncom_iid_t *iid, void **out)
{
    ncom_error_info_impl_t *e = NCOM_CONTAINER_OF(self_u, ncom_error_info_impl_t, iface);
    if (!out) return NCOM_E_INVALID_ARG;
    *out = NULL;

    if (NCOM_IID_EQ(iid, &NCOM_IID_IUNKNOWN) || NCOM_IID_EQ(iid, &NCOM_IID_IERRORINFO)) {
        *out = &e->iface;
        ncom_refcnt_inc(&e->ref_cnt);
        return NCOM_OK;
    }
    return NCOM_E_NO_INTERFACE;
}

static uint32_t error_add_ref(ncom_iunknown_t *self_u)
{
    ncom_error_info_impl_t *e = NCOM_CONTAINER_OF(self_u, ncom_error_info_impl_t, iface);
    return ncom_refcnt_inc(&e->ref_cnt);
}

static uint32_t error_release(ncom_iunknown_t *self_u)
{
    ncom_error_info_impl_t *e = NCOM_CONTAINER_OF(self_u, ncom_error_info_impl_t, iface);
    uint32_t rc = ncom_refcnt_dec(&e->ref_cnt);
    if (rc == 0) {
        free(e->msg);
        free(e);
    }
    return rc;
}

static ncom_status_t error_get_code(ncom_ierror_info_t *self, ncom_status_t *out_code)
{
    ncom_error_info_impl_t *e = NCOM_CONTAINER_OF(self, ncom_error_info_impl_t, iface);
    if (!out_code) return NCOM_E_INVALID_ARG;
    *out_code = e->code;
    return NCOM_OK;
}

static ncom_status_t error_get_message_string(ncom_ierror_info_t *self, ncom_istring_t **out_str)
{
    if (out_str) *out_str = NULL;
    if (!self || !out_str) return NCOM_E_INVALID_ARG;
    ncom_error_info_impl_t *e = NCOM_CONTAINER_OF(self, ncom_error_info_impl_t, iface);
    return ncom_create_string(e->msg ? e->msg : "", out_str);
}

static ncom_status_t error_get_message_buf(ncom_ierror_info_t *self, ncom_char_buf_t *buf, uint64_t *out_len_incl_nul)
{
    const ncom_error_info_impl_t *e = NCOM_CONTAINER_OF(self, ncom_error_info_impl_t, iface);
    uint64_t need = 1;
    if (!out_len_incl_nul) return NCOM_E_INVALID_ARG;

    if (e->msg) need = (uint64_t)strlen(e->msg) + 1;
    *out_len_incl_nul = need;

    /* sizing call: buf may be NULL, cap==0, or ptr==NULL */
    if (!buf || buf->cap == 0 || !buf->ptr) return NCOM_OK;
    if (buf->cap < need) return NCOM_E_MORE_DATA;

    if (need == 1) { buf->ptr[0] = '\0'; return NCOM_OK; }
    memcpy(buf->ptr, e->msg, (size_t)need);
    return NCOM_OK;
}

static const ncom_ierror_info_vtbl_t ERROR_VTBL = {
    .base = {
        .query_interface = error_qi,
        .add_ref = error_add_ref,
        .release = error_release
    },
    .get_code = error_get_code,
    .get_message_string = error_get_message_string,
    .get_message_buf = error_get_message_buf
};

ncom_status_t ncom_create_error_info(ncom_status_t code, const char *msg, ncom_ierror_info_t **out_err)
{
    ncom_status_t st = NCOM_OK;
    ncom_error_info_impl_t *e = NULL;
    size_t n = 0;

    if (out_err) *out_err = NULL;
    if (!out_err) return NCOM_E_INVALID_ARG;

    if (!msg) msg = "";
    n = strlen(msg);
    if (n > (SIZE_MAX - 1u)) { 
        st = NCOM_E_NO_MEM; 
        goto cleanup; 
    }
    e = (ncom_error_info_impl_t *)calloc(1, sizeof(*e));
    if (!e) { st = NCOM_E_NO_MEM; goto cleanup; }

    e->msg = (char *)calloc(n + 1, 1);
    if (!e->msg) { st = NCOM_E_NO_MEM; goto cleanup; }
    memcpy(e->msg, msg, n);

    e->code = code;
    e->iface.vtbl = &ERROR_VTBL;
    ncom_refcnt_init(&e->ref_cnt, 1);

    *out_err = &e->iface;
    e = NULL;

cleanup:
    if (e) { free(e->msg); free(e); }
    return st;
}