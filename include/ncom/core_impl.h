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
 * @file core_impl.h
 * @brief Internal helpers for implementing ncom components.
 * @internal
 *
 * This header is intentionally *not* part of the stable public ABI. It provides
 * small helper constructors used by the reference implementation and samples.
 */
#ifndef NCOM_CORE_IMPL_H
#define NCOM_CORE_IMPL_H

#include <ncom/string.h>
#include <ncom/error_info.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create the reference implementation of @ref ncom_istring_t.
 *
 * The returned object starts with a reference count of 1.
 *
 * @param utf8_str Optional NUL-terminated UTF-8 string (NULL yields empty string).
 * @param out_str  Receives the new string object (owned by caller).
 */
ncom_status_t ncom_create_string(const char *utf8_str, ncom_istring_t **out_str);

/**
 * @brief Create the reference implementation of @ref ncom_ierror_info_t.
 *
 * The returned object starts with a reference count of 1.
 *
 * @param code    Status code to encapsulate.
 * @param msg     Optional NUL-terminated UTF-8 message (copied internally).
 * @param out_err Receives the new error object (owned by caller).
 */
ncom_status_t ncom_create_error_info(ncom_status_t code, const char *msg, ncom_ierror_info_t **out_err);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NCOM_CORE_IMPL_H */
