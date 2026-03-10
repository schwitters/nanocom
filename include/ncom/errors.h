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
 * @file errors.h
 * @brief ncom status codes and helpers (COM-style HRESULT analogue).
 */
#ifndef NCOM_ERRORS_H
#define NCOM_ERRORS_H

#include <ncom/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ncom_errors Errors
 * @brief Status codes returned by ncom functions.
 * @{
 */

/** Operation completed successfully. */
#define NCOM_OK            ((ncom_status_t)0)

/** Unspecified failure. */
#define NCOM_E_FAIL        ((ncom_status_t)-1)

/** One or more arguments are invalid. */
#define NCOM_E_INVALID_ARG ((ncom_status_t)-2)

/** Memory allocation failed. */
#define NCOM_E_NO_MEM      ((ncom_status_t)-3)

/** Requested object or entry was not found. */
#define NCOM_E_NOT_FOUND   ((ncom_status_t)-4)

/** Operation is not implemented. */
#define NCOM_E_NOT_IMPL    ((ncom_status_t)-5)

/**
 * @brief Output buffer too small; call again with a larger buffer.
 *
 * This code is returned by sizing-call APIs (e.g. get_message_buf()) when the
 * caller-provided buffer is too small. The required size is written to the
 * output length parameter even on this error.
 *
 * @warning NCOM_FAILED(NCOM_E_MORE_DATA) is true. Do NOT use NCOM_CHECK() around
 *          sizing calls — handle NCOM_E_MORE_DATA explicitly as a non-fatal condition.
 */
#define NCOM_E_MORE_DATA   ((ncom_status_t)-6)

/** Interface not found */
#define NCOM_E_NO_INTERFACE ((ncom_status_t)-7)

/** True if a status code indicates success (>= 0). */
#define NCOM_SUCCEEDED(ST) ((ncom_status_t)(ST) >= 0)

/** True if a status code indicates failure (< 0). */
#define NCOM_FAILED(ST)    ((ncom_status_t)(ST) < 0)

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NCOM_ERRORS_H */
