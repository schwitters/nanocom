#pragma once

#include <ncom/string.h>
#include <ncom/error_info.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates a standard ncom_istring_t implementation.
 * * Allocates memory and copies the provided UTF-8 string. The returned
 * object has an initial reference count of 1.
 * * @param utf8_str Null-terminated string (may be NULL, resulting in empty string).
 * @param out_str  Receives the new string object.
 * @return ncom_status_t NCOM_OK on success.
 */
ncom_status_t ncom_create_string(const char *utf8_str, ncom_istring_t **out_str);

/**
 * @brief Creates a standard ncom_ierror_info_t implementation.
 * * @param code     The error code to encapsulate.
 * @param msg      The detailed error message (copied internally).
 * @param out_err  Receives the new error object.
 * @return ncom_status_t NCOM_OK on success.
 */
ncom_status_t ncom_create_error_info(ncom_status_t code, const char *msg, ncom_ierror_info_t **out_err);

#ifdef __cplusplus
}
#endif