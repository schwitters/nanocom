#pragma once
#include <ncom/base.h>
#include <ncom/string.h>

#ifdef __cplusplus
extern "C" {
#endif

// Framework IID für IErrorInfo
static const ncom_iid_t NCOM_IID_IERRORINFO = { 0x758cd93d790a49bfULL, 0xa7868baf9b6a9285ULL };

typedef struct ncom_ierror_info_s ncom_ierror_info_t;

typedef struct ncom_ierror_info_vtbl_s {
    ncom_iunknown_vtbl_t base;

    /**
     * @brief Get the associated status/error code.
     */
    ncom_status_t (*get_code)(ncom_ierror_info_t *self, ncom_status_t *out_code);

    /**
     * @brief Get the error message as an ncom_istring_t object.
     * Ownership: On success, `*out_msg` is AddRef'ed for the caller.
     */
    ncom_status_t (*get_message_string)(ncom_ierror_info_t *self, ncom_istring_t **out_msg);

    /**
     * @brief Get the error message into a caller-provided buffer.
     * Sizing call: Call with buf->ptr = NULL, buf->cap = 0 to get required length.
     */
    ncom_status_t (*get_message_buf)(ncom_ierror_info_t *self, ncom_char_buf_t *buf, uint64_t *out_len_incl_nul);
} ncom_ierror_info_vtbl_t;

struct ncom_ierror_info_s { const ncom_ierror_info_vtbl_t *vtbl; };

/** Releases and nulls the pointer (COM-style). */
static inline void ncom_ierror_info_releasep(ncom_ierror_info_t **p)
{
    if (p && *p) { (*p)->vtbl->base.release((ncom_iunknown_t *)*p); *p = NULL; }
}

/** Queries the requested interface from an ncom_iunknown_t. */
static inline ncom_status_t qi_ncom_ierror_info(ncom_iunknown_t *from, ncom_ierror_info_t **out)
{
    if (out) *out = NULL;
    if (!from || !out) return NCOM_E_INVALID_ARG;
    return from->vtbl->query_interface(from, &NCOM_IID_IERRORINFO, (void **)out);
}

#ifdef __cplusplus
}
#endif