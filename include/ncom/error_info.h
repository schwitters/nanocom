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
 * @file error_info.h
 * @brief ABI-safe error object for rich error reporting across module boundaries.
 *
 * In addition to a plain status code, ncom APIs may optionally return an
 * @ref ncom_ierror_info_t object describing the failure in more detail.
 */
#ifndef NCOM_ERROR_INFO_H
#define NCOM_ERROR_INFO_H

#include <ncom/base.h>
#include <ncom/string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ncom_error_info Error info
 * @brief Rich error reporting via a reference-counted error object.
 * @{
 */

/** IID of the ncom_ierror_info_t interface. */
extern const ncom_iid_t NCOM_IID_IERRORINFO;

typedef struct ncom_ierror_info_s ncom_ierror_info_t;

/**
 * @brief Reference-counted error information.
 */
typedef struct ncom_ierror_info_vtbl_s {
    ncom_iunknown_vtbl_t base; /**< IUnknown base vtable (must be first). */

    /** @brief Get the associated status/error code. */
    ncom_status_t (*get_code)(ncom_ierror_info_t *self, ncom_status_t *out_code);

    /**
     * @brief Get the error message as an owning string object.
     *
     * @param self    Error object.
     * @param out_msg Receives an AddRef'ed @ref ncom_istring_t on success.
     */
    ncom_status_t (*get_message_string)(ncom_ierror_info_t *self, ncom_istring_t **out_msg);

    /**
     * @brief Get the error message into a caller-provided buffer (NUL-terminated).
     *
     * This method follows the sizing-call convention:
     * - Call with `buf == NULL` or `buf->ptr == NULL` or `buf->cap == 0` to obtain the required size.
     * - Otherwise, provide a buffer and receive the message.
     *
     * @param self              Error object.
     * @param buf               Output buffer descriptor.
     * @param out_len_incl_nul  Receives required/written length including NUL.
     * @return @ref NCOM_OK on success, @ref NCOM_E_MORE_DATA if buffer too small.
     */
    ncom_status_t (*get_message_buf)(
        ncom_ierror_info_t *self,
        ncom_char_buf_t    *buf,
        uint64_t           *out_len_incl_nul
    );
} ncom_ierror_info_vtbl_t;

/** @brief IErrorInfo interface (a pointer to a vtable). */
struct ncom_ierror_info_s {
    const ncom_ierror_info_vtbl_t *vtbl;
};

/** @brief Release an IErrorInfo pointer and set it to NULL. */
static inline void ncom_ierror_info_releasep(ncom_ierror_info_t **p)
{
    if (p && *p) {
        (*p)->vtbl->base.release((ncom_iunknown_t *)*p);
        *p = NULL;
    }
}

/** @brief Query an IErrorInfo interface from an IUnknown. */
static inline ncom_status_t qi_ncom_ierror_info(ncom_iunknown_t *from, ncom_ierror_info_t **out)
{
    if (out) *out = NULL;
    if (!from || !out) return NCOM_E_INVALID_ARG;
    return from->vtbl->query_interface(from, &NCOM_IID_IERRORINFO, (void **)out);
}

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NCOM_ERROR_INFO_H */
