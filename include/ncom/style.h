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
 * @file style.h
 * @brief Convenience macros for "fail-fast" error handling in C (goto-cleanup pattern).
 *
 * These macros are optional but recommended for consistent control flow in ncom code.
 * They follow the "happy path left" style: check, jump to cleanup, release resources.
 */
#ifndef NCOM_STYLE_H
#define NCOM_STYLE_H

#include <ncom/errors.h>
#include <ncom/error_info.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ncom_style Style helpers
 * @brief Goto-cleanup macros for consistent, leak-free error handling.
 *
 * **Convention**
 * - The calling function defines: `ncom_status_t st = NCOM_OK;`
 * - The calling function has a `cleanup:` label near the end.
 * - Resources are released in the `cleanup:` section.
 * @{
 */

/**
 * @brief Evaluate @p EXPR, assign it to `st`, and jump to `cleanup` on failure.
 */
#define NCOM_CHECK(EXPR)                                      do {                                                         st = (EXPR);                                              if (NCOM_FAILED(st)) goto cleanup;                    } while (0)

/**
 * @brief If @p PTR is NULL, set `st` to @ref NCOM_E_INVALID_ARG and jump to `cleanup`.
 */
#define NCOM_CHECK_NULL(PTR)                                  do {                                                         if ((PTR) == NULL) {                                         st = NCOM_E_INVALID_ARG;                                  goto cleanup;                                         }                                                    } while (0)

/**
 * @brief Commit a temporary pointer into an out parameter and NULL the temporary.
 *
 * Typical usage:
 * @code
 * ncom_foo_t *tmp = create_foo();
 * ...
 * NCOM_COMMIT_OUT(out_foo, tmp);
 * @endcode
 */
#define NCOM_COMMIT_OUT(OUT_PTR, TMP_PTR)                     do {                                                         *(OUT_PTR) = (TMP_PTR);                                   (TMP_PTR) = NULL;                                     } while (0)

/**
 * @brief Like @ref NCOM_CHECK but intended for APIs that may populate an error object.
 *
 * The macro does not modify @p ERR; it only prevents "unused parameter" warnings in
 * call sites where an error object is optional.
 */
#define NCOM_CHECK_ERR(EXPR, ERR)                             do {                                                         (void)(ERR);                                              st = (EXPR);                                              if (NCOM_FAILED(st)) goto cleanup;                    } while (0)

/**
 * @brief Release any existing error object in @p ERR, then evaluate @p EXPR.
 *
 * On failure, the function jumps to `cleanup`. This macro is useful when a function
 * performs multiple operations and you only want to keep the most recent rich error.
 */
#define NCOM_CHECK_SET_ERR(EXPR, ERR)                         do {                                                         ncom_ierror_info_releasep(&(ERR));                        st = (EXPR);                                              if (NCOM_FAILED(st)) goto cleanup;                    } while (0)

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NCOM_STYLE_H */
