#pragma once
#include "nano_status.h"

/* Goto-cleanup style helpers.
   Requires a local variable: status_t st = STATUS_OK; */

#define CHECK(EXPR)                               \
    do {                                          \
        st = (EXPR);                              \
        if (STATUS_FAILED(st)) goto cleanup;      \
    } while (0)

#define CHECK_NULL(PTR)                           \
    do {                                          \
        if ((PTR) == NULL) {                      \
            st = STATUS_E_INVALID_ARG;            \
            goto cleanup;                         \
        }                                         \
    } while (0)

/* Commit ownership transfer for out-parameters. */
#define COMMIT_OUT(OUT_PTR, TMP_PTR)              \
    do {                                          \
        *(OUT_PTR) = (TMP_PTR);                   \
        (TMP_PTR) = NULL;                         \
    } while (0)


/* Like CHECK(), but captures a rich error object (if provided by the callee).
   Requires local variables:
     status_t st = STATUS_OK;
     i_error_info_t *err = NULL;  // optional
*/
#define CHECK_WITH_ERROR(EXPR, ERR_PTR)           \
    do {                                          \
        st = (EXPR);                              \
        if (STATUS_FAILED(st)) {                  \
            /* ERR_PTR is expected to be a pointer to i_error_info_t* */ \
            goto cleanup;                         \
        }                                         \
    } while (0)


/* ===== Rich error handling helpers =====
   Convention:
   - Functions that support rich errors accept an OUT parameter: i_error_info_t **out_err
   - Callee may set *out_err to a new error object (caller must release).
*/

/* CHECK_ERR: evaluate EXPR and jump to cleanup on failure.
   ERR is not modified; use this when EXPR already manages ERR. */
#define CHECK_ERR(EXPR, ERR)                      \
    do {                                          \
        (void)(ERR);                              \
        st = (EXPR);                              \
        if (STATUS_FAILED(st)) goto cleanup;      \
    } while (0)

/* CHECK_SET_ERR: release any previous error object, set ERR to NULL,
   run EXPR (which is expected to potentially set ERR), and jump to cleanup on failure. */
#define CHECK_SET_ERR(EXPR, ERR)                  \
    do {                                          \
        i_error_info_releasep(&(ERR));            \
        (ERR) = NULL;                             \
        st = (EXPR);                              \
        if (STATUS_FAILED(st)) goto cleanup;      \
    } while (0)
