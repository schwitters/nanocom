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
 * @file types.h
 * @brief Fundamental fixed-width types and identifiers used across the ncom ABI.
 *
 * The ncom ABI is designed to be stable across compilers and shared-library boundaries.
 * Therefore, this header uses fixed-width integer types and POD structs only.
 */
#ifndef NCOM_TYPES_H
#define NCOM_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ncom_types Types
 * @brief Fundamental ABI types used by all ncom interfaces.
 * @{
 */

/** Status code returned by ncom API functions (COM-style HRESULT analogue). */
typedef int32_t ncom_status_t;

/**
 * @brief 128-bit interface identifier.
 *
 * An IID identifies a specific interface (vtable layout). The IID must change
 * if the vtable changes in a non-backwards-compatible way (e.g., reordering slots).
 */
typedef struct ncom_iid_s {
    uint64_t hi;
    uint64_t lo;
} ncom_iid_t;

/**
 * @brief 128-bit class identifier.
 *
 * A CLSID identifies a concrete component class that can be instantiated by a factory.
 */
typedef struct ncom_clsid_s {
    uint64_t hi;
    uint64_t lo;
} ncom_clsid_t;

/**
 * @brief Non-owning UTF-8 string view.
 *
 * The memory is owned by the producer. Unless otherwise documented, the view is only
 * guaranteed to be valid until the next call on the same object (or until Release()).
 */
typedef struct ncom_string_view_s {
    const uint8_t *ptr; /**< Pointer to UTF-8 bytes (may be NULL if len==0). */
    uint32_t len;       /**< Number of bytes (not including any NUL terminator). */
} ncom_string_view_t;

/**
 * @brief Caller-provided output buffer for NUL-terminated UTF-8 strings.
 *
 * Many ncom APIs follow a "sizing call" convention:
 * - If @c ptr is NULL or @c cap is 0, the function returns the required size in bytes.
 * - Otherwise, the function writes at most @c cap bytes and guarantees NUL-termination
 *   when the buffer is large enough.
 */
typedef struct ncom_char_buf_s {
    char    *ptr; /**< Output buffer pointer (may be NULL for sizing calls). */
    uint64_t cap; /**< Capacity in bytes of @c ptr. */
} ncom_char_buf_t;

#ifdef __cplusplus
#define NCOM_STATIC_ASSERT(COND, MSG) static_assert((COND), MSG)
#else
#define NCOM_STATIC_ASSERT(COND, MSG) _Static_assert((COND), MSG)
#endif

NCOM_STATIC_ASSERT(sizeof(ncom_status_t) == 4, "ncom_status_t must be 32-bit");
NCOM_STATIC_ASSERT(sizeof(ncom_iid_t) == 16, "ncom_iid_t must be 128-bit");
NCOM_STATIC_ASSERT(sizeof(ncom_clsid_t) == 16, "ncom_clsid_t must be 128-bit");

/** Compare two IIDs for equality. */
#define NCOM_IID_EQ(A, B)   (((A)->hi == (B)->hi) && ((A)->lo == (B)->lo))

/** Compare two CLSIDs for equality. */
#define NCOM_CLSID_EQ(A, B) (((A)->hi == (B)->hi) && ((A)->lo == (B)->lo))

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NCOM_TYPES_H */
