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
 * @file atomic.h
 * @brief Portable atomics used for reference counting.
 */
#ifndef NCOM_ATOMIC_H
#define NCOM_ATOMIC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ncom_atomic Atomics
 * @brief Small portability layer for thread-safe reference counting.
 *
 * On Windows (MSVC C), this header uses Interlocked operations.
 * On other platforms, it uses C11 `<stdatomic.h>`.
 *
 * @warning `ncom_refcnt_t` is platform-dependent in size and alignment.
 *          It MUST NOT be used in structs that cross ABI boundaries
 *          (e.g. as vtable parameters or in shared interface structs).
 *          It is intended exclusively for internal object implementations.
 * @{
 */

#ifdef _WIN32
#  include <windows.h>

/** Reference counter type (Windows, 32-bit). */
typedef volatile LONG ncom_refcnt_t;

/** Initialize reference counter. */
static inline void ncom_refcnt_init(ncom_refcnt_t *rc, uint32_t v) { *rc = (LONG)v; }

/** Increment reference counter; returns the new value. */
static inline uint32_t ncom_refcnt_inc(ncom_refcnt_t *rc) { return (uint32_t)InterlockedIncrement(rc); }

/** Decrement reference counter; returns the new value. */
static inline uint32_t ncom_refcnt_dec(ncom_refcnt_t *rc) { return (uint32_t)InterlockedDecrement(rc); }

#else
#  include <stdatomic.h>

/**
 * @brief Reference counter type (C11 atomics, explicit 32-bit).
 *
 * `_Atomic uint32_t` is used instead of `atomic_uint` to guarantee a
 * 32-bit counter on all platforms (C11 does not mandate that `unsigned int`
 * is 32 bits, though it is in practice).
 */
typedef _Atomic uint32_t ncom_refcnt_t;

/** Initialize reference counter. */
static inline void ncom_refcnt_init(ncom_refcnt_t *rc, uint32_t v) { atomic_init(rc, v); }

/** Increment reference counter; returns the new value. */
static inline uint32_t ncom_refcnt_inc(ncom_refcnt_t *rc)
{
    return atomic_fetch_add_explicit(rc, 1u, memory_order_relaxed) + 1u;
}

/** Decrement reference counter; returns the new value. */
static inline uint32_t ncom_refcnt_dec(ncom_refcnt_t *rc)
{
    return atomic_fetch_sub_explicit(rc, 1u, memory_order_acq_rel) - 1u;
}
#endif

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NCOM_ATOMIC_H */
