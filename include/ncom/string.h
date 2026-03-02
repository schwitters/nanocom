#pragma once
#include <ncom/base.h>
#include <ncom/errors.h>
#ifdef __cplusplus
extern "C" {
#endif

// Framework IID für IString
static const ncom_iid_t NCOM_IID_ISTRING = { 0x7dcad1ee32974171ULL, 0xb36302fc16e42721ULL };

typedef struct ncom_istring_s ncom_istring_t;

typedef struct ncom_istring_vtbl_s {
    ncom_iunknown_vtbl_t base;

    /**
     * @brief Obtain a pointer to a NUL-terminated C string view.
     * * Lifetime: Remains valid as long as the ncom_istring_t object remains alive.
     */
    ncom_status_t (*c_str)(ncom_istring_t *self, const char* *out_ptr);

    /**
     * @brief Get the string length in bytes (excluding any terminating NUL).
     */
    ncom_status_t (*length)(ncom_istring_t *self, uint64_t *out_len_bytes);
} ncom_istring_vtbl_t;

struct ncom_istring_s { const ncom_istring_vtbl_t *vtbl; };

/** Releases and nulls the pointer (COM-style). */
static inline void ncom_istring_releasep(ncom_istring_t **p)
{
    if (p && *p) { (*p)->vtbl->base.release((ncom_iunknown_t *)*p); *p = NULL; }
}

/** Queries the requested interface from an ncom_iunknown_t. */
static inline ncom_status_t qi_ncom_istring(ncom_iunknown_t *from, ncom_istring_t **out)
{
    if (out) *out = NULL;
    if (!from || !out) return NCOM_E_INVALID_ARG;
    return from->vtbl->query_interface(from, &NCOM_IID_ISTRING, (void **)out);
}

#ifdef __cplusplus
}
#endif