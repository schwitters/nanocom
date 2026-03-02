// In include/ncom/style.h

#pragma once
#include <ncom/errors.h>
#include <ncom/error_info.h> // Für ncom_ierror_info_releasep

/**
 * Goto-cleanup style helpers for ncom.
 * * REQUIRES: A local variable `ncom_status_t st = NCOM_OK;` in the calling scope,
 * and a label `cleanup:` at the end of the function.
 */

/** * @brief Evaluates an expression, assigns it to 'st', and jumps to cleanup on failure. 
 */
#define NCOM_CHECK(EXPR)                                  \
    do {                                                  \
        st = (EXPR);                                      \
        if (NCOM_FAILED(st)) goto cleanup;                \
    } while (0)

/** * @brief Checks if a pointer is NULL. If so, sets 'st' to INVALID_ARG and jumps to cleanup. 
 */
#define NCOM_CHECK_NULL(PTR)                              \
    do {                                                  \
        if ((PTR) == NULL) {                              \
            st = NCOM_E_INVALID_ARG;                      \
            goto cleanup;                                 \
        }                                                 \
    } while (0)

/** * @brief Safely transfers ownership of a temporary pointer to an out-parameter.
 * * This prevents memory leaks if the function fails halfway through and jumps
 * to cleanup, because the temporary pointer is only handed over right before 
 * a successful return.
 */
#define NCOM_COMMIT_OUT(OUT_PTR, TMP_PTR)                 \
    do {                                                  \
        *(OUT_PTR) = (TMP_PTR);                           \
        (TMP_PTR) = NULL;                                 \
    } while (0)


/* ===== Rich error handling helpers =====
 * Convention:
 * - Functions supporting rich errors accept an OUT parameter: ncom_ierror_info_t **out_err
 * - Callee may set *out_err to a new error object (caller must release).
 */

/** * @brief Like NCOM_CHECK(), but meant for calls where an error object might be populated.
 * Jumps to cleanup on failure. 
 */
#define NCOM_CHECK_ERR(EXPR, ERR)                         \
    do {                                                  \
        (void)(ERR); /* Suppress unused warning */        \
        st = (EXPR);                                      \
        if (NCOM_FAILED(st)) goto cleanup;                \
    } while (0)

/** * @brief Prepares an error pointer by releasing any old error, then runs EXPR.
 * Jumps to cleanup on failure.
 * * REQUIRES: A local variable `ncom_ierror_info_t *ERR = NULL;`
 */
#define NCOM_CHECK_SET_ERR(EXPR, ERR)                     \
    do {                                                  \
        ncom_ierror_info_releasep(&(ERR));                \
        (ERR) = NULL;                                     \
        st = (EXPR);                                      \
        if (NCOM_FAILED(st)) goto cleanup;                \
    } while (0)