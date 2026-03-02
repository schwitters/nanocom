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
 * @{
 */

#ifdef _WIN32
#  include <windows.h>

/** Reference counter type (Windows). */
typedef volatile LONG ncom_refcnt_t;

/** Initialize reference counter. */
static inline void ncom_refcnt_init(ncom_refcnt_t *rc, LONG v) { *rc = v; }

/** Increment reference counter; returns the new value. */
static inline uint32_t ncom_refcnt_inc(ncom_refcnt_t *rc) { return (uint32_t)InterlockedIncrement(rc); }

/** Decrement reference counter; returns the new value. */
static inline uint32_t ncom_refcnt_dec(ncom_refcnt_t *rc) { return (uint32_t)InterlockedDecrement(rc); }

#else
#  include <stdatomic.h>

/** Reference counter type (C11 atomics). */
typedef atomic_uint ncom_refcnt_t;

/** Initialize reference counter. */
static inline void ncom_refcnt_init(ncom_refcnt_t *rc, unsigned v) { atomic_init(rc, v); }

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
