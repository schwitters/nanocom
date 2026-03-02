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
 * @file base.h
 * @brief Core COM-like interfaces (IUnknown, IFactory) and ABI helper macros.
 */
#ifndef NCOM_BASE_H
#define NCOM_BASE_H

#include <stddef.h> /* NULL, offsetof */
#include <ncom/types.h>
#include <ncom/errors.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ncom_base Base interfaces
 * @brief COM-like base interfaces and helpers.
 * @{
 */

/**
 * @brief The canonical interface identifier for IUnknown.
 *
 * All ncom interfaces "inherit" from IUnknown, i.e. their vtable begins with the
 * IUnknown vtable layout.
 */
static const ncom_iid_t NCOM_IID_IUNKNOWN = { 0x589dfb30790e4b07ULL, 0x952a7857d37828dcULL };

/**
 * @brief The canonical interface identifier for IFactory.
 */
static const ncom_iid_t NCOM_IID_IFACTORY = { 0x74751d837fe74171ULL, 0xb280525960783e1bULL };

typedef struct ncom_iunknown_s ncom_iunknown_t;

/**
 * @brief IUnknown vtable (COM-style).
 *
 * **Rules**
 * - Slot order is ABI and must never change.
 * - `query_interface()` returns an AddRef'ed interface pointer on success.
 * - `add_ref()` / `release()` are lifetime-only thread-safe; method thread-safety
 *   is defined by each concrete object.
 */
typedef struct ncom_iunknown_vtbl_s {
    ncom_status_t (*query_interface)(ncom_iunknown_t *self, const ncom_iid_t *iid, void **out_iface);
    uint32_t      (*add_ref)(ncom_iunknown_t *self);
    uint32_t      (*release)(ncom_iunknown_t *self);
} ncom_iunknown_vtbl_t;

/** @brief IUnknown interface (a pointer to a vtable). */
struct ncom_iunknown_s {
    const ncom_iunknown_vtbl_t *vtbl;
};

/**
 * @brief Convenience wrapper for QueryInterface.
 *
 * This expands to a call to a generated helper function `qi_<iface_name>()`.
 */
#define NCOM_QI(obj, iface_name, out_ptr)     qi_##iface_name((ncom_iunknown_t *)(obj), (out_ptr))

/**
 * @brief Compute the address of the containing struct from an embedded member pointer.
 *
 * @param ptr    Pointer to an embedded member.
 * @param type   Type of the surrounding struct.
 * @param member Name of the embedded member within @p type.
 */
#define NCOM_CONTAINER_OF(ptr, type, member)     ((type *)((char *)(ptr) - offsetof(type, member)))

typedef struct ncom_ifactory_s ncom_ifactory_t;

/**
 * @brief Factory interface for creating component instances.
 *
 * The factory itself is an interface; it is typically owned by the host application.
 */
typedef struct ncom_ifactory_vtbl_s {
    ncom_iunknown_vtbl_t base; /**< IUnknown base vtable (must be first). */

    /**
     * @brief Create an instance of a component class and return a requested interface.
     *
     * @param self   The factory instance.
     * @param clsid  Class identifier of the component to instantiate.
     * @param iid    Interface identifier requested from the created instance.
     * @param outptr Receives the requested interface pointer on success (AddRef'ed).
     * @return Status code.
     */
    ncom_status_t (*create_instance)(
        ncom_ifactory_t      *self,
        const ncom_clsid_t   *clsid,
        const ncom_iid_t     *iid,
        void               **outptr
    );
} ncom_ifactory_vtbl_t;

/** @brief IFactory interface (a pointer to a vtable). */
struct ncom_ifactory_s {
    const ncom_ifactory_vtbl_t *vtbl;
};

/** @brief Release an IFactory pointer and set it to NULL (COM-style). */
static inline void ncom_ifactory_releasep(ncom_ifactory_t **p)
{
    if (p && *p) {
        (*p)->vtbl->base.release((ncom_iunknown_t *)*p);
        *p = NULL;
    }
}

/**
 * @brief Query an IFactory interface from an IUnknown.
 *
 * @param from Any interface pointer from the same object identity.
 * @param out  Receives the IFactory pointer (AddRef'ed) on success.
 */
static inline ncom_status_t qi_ncom_ifactory(ncom_iunknown_t *from, ncom_ifactory_t **out)
{
    if (out) *out = NULL;
    if (!from || !out) return NCOM_E_INVALID_ARG;
    return from->vtbl->query_interface(from, &NCOM_IID_IFACTORY, (void **)out);
}

/** @brief Release an IUnknown pointer and set it to NULL. */
static inline void ncom_iunknown_releasep(ncom_iunknown_t **p)
{
    if (p && *p) {
        (*p)->vtbl->release(*p);
        *p = NULL;
    }
}

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NCOM_BASE_H */
