#pragma once
#include <ncom/types.h>
#include <ncom/errors.h>
#ifdef __cplusplus
extern "C" {
#endif

// Framework IIDs
static const ncom_iid_t NCOM_IID_IUNKNOWN = { 0x589dfb30790e4b07ULL, 0x952a7857d37828dcULL };
static const ncom_iid_t NCOM_IID_IFACTORY = { 0x74751d837fe74171ULL, 0xb280525960783e1bULL };

typedef struct ncom_iunknown_s ncom_iunknown_t;

typedef struct ncom_iunknown_vtbl_s {
    ncom_status_t (*query_interface)(ncom_iunknown_t *self, const ncom_iid_t *iid, void* *out_iface);
    uint32_t      (*add_ref)(ncom_iunknown_t *self);
    uint32_t      (*release)(ncom_iunknown_t *self);
} ncom_iunknown_vtbl_t;

struct ncom_iunknown_s { const ncom_iunknown_vtbl_t *vtbl; };

#define NCOM_QI(obj, iface_name, out_ptr) \
    qi_##iface_name((ncom_iunknown_t*)(obj), (out_ptr))

/**
 * @brief Computes the pointer to the surrounding implementation struct.
 */
#define NCOM_CONTAINER_OF(ptr, type, member) \
    ((type *)( (char *)(ptr) - offsetof(type, member) ))
	
	
typedef struct ncom_ifactory_s ncom_ifactory_t;

typedef struct ncom_ifactory_vtbl_s {
    // Erbt von IUnknown
    ncom_iunknown_vtbl_t base;
    
    /**
     * @brief Create an instance of a component (CLSID) and return a specific interface (IID).
     * * @param clsid Class identifier of the component to instantiate (by reference!).
     * @param iid   Interface identifier requested from the created instance (by reference!).
     * @param outptr Receives the requested interface pointer on success (AddRef'ed).
     * * @return ncom_status_t Success or failure code.
     */
    ncom_status_t (*create_instance)(
        ncom_ifactory_t *self, 
        const ncom_clsid_t *clsid, // <-- WICHTIG: Pointer statt Call-by-Value
        const ncom_iid_t *iid,     // <-- WICHTIG: Pointer statt Call-by-Value
        void **outptr
    );
} ncom_ifactory_vtbl_t;

struct ncom_ifactory_s { const ncom_ifactory_vtbl_t *vtbl; };

/** Releases and nulls the pointer (COM-style). */
static inline void ncom_ifactory_releasep(ncom_ifactory_t **p)
{
    if (p && *p) { (*p)->vtbl->base.release((ncom_iunknown_t *)*p); *p = NULL; }
}

/** Queries the requested interface from an ncom_iunknown_t. */
static inline ncom_status_t qi_ncom_ifactory(ncom_iunknown_t *from, ncom_ifactory_t **out)
{
    if (out) *out = NULL;
    if (!from || !out) return NCOM_E_INVALID_ARG;
    return from->vtbl->query_interface(from, &NCOM_IID_IFACTORY, (void **)out);
}

/** Releases and nulls the pointer (COM-style). */
static inline void ncom_iunknown_releasep(ncom_iunknown_t** p)
{
    if (p && *p) { (*p)->vtbl->release(*p); *p = NULL; }
}
#ifdef __cplusplus
}
#endif