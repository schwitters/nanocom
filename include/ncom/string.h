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
 * @file string.h
 * @brief ABI-safe string interfaces.
 *
 * ncom avoids transferring heap-allocated strings across module boundaries.
 * Use @ref ncom_string_view_t for non-owning views and @ref ncom_istring_t when
 * an owning, reference-counted string object is required.
 */
#ifndef NCOM_STRING_H
#define NCOM_STRING_H

#include <ncom/base.h>
#include <ncom/errors.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ncom_string Strings
 * @brief ABI-safe string views and string interfaces.
 * @{
 */

/** IID of the ncom_istring_t interface. */
extern const ncom_iid_t NCOM_IID_ISTRING;

typedef struct ncom_istring_s ncom_istring_t;

/**
 * @brief Reference-counted owning string.
 *
 * The string is immutable. Callers can obtain a stable NUL-terminated view via
 * @ref ncom_istring_vtbl_s::c_str.
 */
typedef struct ncom_istring_vtbl_s {
    ncom_iunknown_vtbl_t base; /**< IUnknown base vtable (must be first). */

    /**
     * @brief Obtain a pointer to a NUL-terminated UTF-8 C string.
     *
     * @param self    String object.
     * @param out_ptr Receives a pointer to an internal NUL-terminated string.
     * @return Status code.
     *
     * @note The returned pointer remains valid as long as the string object is alive.
     */
    ncom_status_t (*c_str)(ncom_istring_t *self, const char **out_ptr);

    /**
     * @brief Get the string length in bytes (excluding the terminating NUL).
     */
    ncom_status_t (*length)(ncom_istring_t *self, uint64_t *out_len_bytes);
} ncom_istring_vtbl_t;

/** @brief IString interface (a pointer to a vtable). */
struct ncom_istring_s {
    const ncom_istring_vtbl_t *vtbl;
};

/** @brief Release an IString pointer and set it to NULL. */
static inline void ncom_istring_releasep(ncom_istring_t **p)
{
    if (p && *p) {
        (*p)->vtbl->base.release((ncom_iunknown_t *)*p);
        *p = NULL;
    }
}

/** @brief Query an IString interface from an IUnknown. */
static inline ncom_status_t qi_ncom_istring(ncom_iunknown_t *from, ncom_istring_t **out)
{
    if (out) *out = NULL;
    if (!from || !out) return NCOM_E_INVALID_ARG;
    return from->vtbl->query_interface(from, &NCOM_IID_ISTRING, (void **)out);
}

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NCOM_STRING_H */
