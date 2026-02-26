#ifndef _NANO_BASE_H__
#define _NANO_BASE_H__

#include <nano_base.h>
#include <nano_status.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
/* 128-bit identifiers as two 64-bit words. */
typedef struct nanoc_iid_s { uint64_t hi; uint64_t lo; }   nanoc_iid_t;
typedef struct nanoc_clsid_s { uint64_t hi; uint64_t lo; } nanoc_clsid_t;

#define NANOC_IID_EQ(A, B)   ((A).hi == (B).hi && (A).lo == (B).lo)
#define NANOC_CLSID_EQ(A, B) ((A).hi == (B).hi && (A).lo == (B).lo)

/**
 * @brief Status/return code type used by this ABI.
 * 
 * Convention:
 * - 0 means success (STATUS_OK).
 * - Non-zero values indicate failure or a non-success condition.
 * 
 * The concrete numeric space and mapping is implementation-defined, but should
 * remain stable across versions of a plugin/host pair.
 */
typedef int32_t status_t;

/**
 * @brief Unsigned 32-bit integer type used in this IDL.
 * 
 * Note: Mapped to the platform's `unsigned long` in the ABI as specified here.
 * If you need strict-width types across platforms, prefer a fixed-width mapping
 * in the generator (e.g., `uint32_t`).
 */
typedef uint32_t uint32;

/**
 * @brief Signed 64-bit integer type used in this IDL.
 * 
 * Note: Mapped to the platform's `long long` in the ABI as specified here.
 */
typedef int64_t int64;

/**
 * @brief Borrowed UTF-8 slice (pointer + length).
 * 
 * This type does not own the memory it points to.
 * 
 * Typical usage:
 * - Input parameters that reference caller-owned memory.
 * - Strings that may contain embedded NUL bytes (length is explicit).
 */
typedef struct string_view_s {
    /**
     * @brief Pointer to the first byte of the UTF-8 data (not NUL-terminated). 
     */
    const uint8_t* ptr;
    /**
     * @brief Length in bytes of the UTF-8 data at @ref ptr. 
     */
    uint32 len;
} string_view_t;

/**
 * @brief Caller-provided character buffer (pointer + capacity).
 * 
 * This structure is typically used for "write into caller buffer" APIs.
 * 
 * Sizing convention:
 * - Call with `ptr = NULL` and `cap = 0` to query the required size.
 * - Then allocate a buffer of the required size and call again to fill it.
 * 
 * Encoding of written data is specified by the API using this buffer
 * (commonly UTF-8).
 */
typedef struct char_buf_s {
    /**
     * @brief Pointer to writable memory provided by the caller (may be NULL for sizing calls). 
     */
    char* ptr;
    /**
     * @brief Capacity of @ref ptr in bytes (0 for sizing calls). 
     */
    uint64_t cap;
} char_buf_t;


/* Interface IDs (IIDs) */
static const nanoc_iid_t IID_I_ERROR_INFO = { 0x758cd93d790a49bfULL, 0xa7868baf9b6a9285ULL };
static const nanoc_iid_t IID_I_STRING = { 0x7dcad1ee32974171ULL, 0xb36302fc16e42721ULL };
static const nanoc_iid_t IID_I_LOGGER = { 0x6be9117fd60f449eULL, 0xbc4979a93dd83529ULL };
static const nanoc_iid_t IID_I_UNKNOWN = { 0x589dfb30790e4b07ULL, 0x952a7857d37828dcULL };

/* Class IDs (CLSIDs) */
static const nanoc_clsid_t CLSID_NANO_COMPONENT = { 0x73bbc2c355194575ULL, 0x839ffa31da12bfabULL };

/**
 * @brief Base interface for all nano-com interfaces (COM-like).
 * 
 * Lifetime:
 * - Implementations are reference-counted.
 * - `add_ref()` increments the reference count.
 * - `release()` decrements the reference count; when it reaches 0, the object is destroyed.
 * 
 * Querying:
 * - `query_interface()` obtains other interfaces implemented by the same object.
 * - The returned interface pointer (if any) is AddRef'ed for the caller.
 */
typedef struct i_unknown_s i_unknown_t;
/**
 * @brief Logger interface.
 * 
 * Implementations may forward messages to console, files, syslog, Windows Event Log,
 * or a structured logging backend. Thread-safety and formatting rules are
 * implementation-defined unless specified by the host.
 */
typedef struct i_logger_s i_logger_t;
/**
 * @brief Read-only string object interface.
 * 
 * Provides access to a string owned by the implementing object.
 * The representation is typically UTF-8, but the exact encoding is
 * implementation-defined unless specified by the host.
 */
typedef struct i_string_s i_string_t;
/**
 * @brief Extended error information interface.
 * 
 * This is intended to carry rich error data beyond a simple status_t code,
 * similar to COM's IErrorInfo concept.
 */
typedef struct i_error_info_s i_error_info_t;
typedef struct i_factory_s i_factory_t;

typedef struct i_unknown_vtbl_s {
    i_unknown_vtbl_t base;
    /**
     * @brief Query for a supported interface on this object.
     * 
     * @param iid Interface identifier to query.
     * @param out_iface Receives the interface pointer on success.
     * 
     * Ownership:
     * - On success, `*out_iface` is a valid interface pointer with one reference
     *   owned by the caller (i.e., the callee has performed AddRef on it).
     * - On failure, `*out_iface` must be set to NULL.
     * 
     * @return status_t Success or failure code.
     */
    status_t (*query_interface)(i_unknown_t *self, nanoc_iid_t iid, void* *out_iface);
    /**
     * @brief Increment the reference count.
     * 
     * @return The new reference count (implementation-defined; may be used for diagnostics only).
     */
    uint32 (*add_ref)(i_unknown_t *self);
    /**
     * @brief Decrement the reference count.
     * 
     * When the reference count reaches 0, the object destroys itself and the pointer
     * becomes invalid.
     * 
     * @return The new reference count (implementation-defined; may be used for diagnostics only).
     */
    uint32 (*release)(i_unknown_t *self);
} i_unknown_vtbl_t;

struct i_unknown_s { const i_unknown_vtbl_t *vtbl; };

typedef struct i_logger_vtbl_s {
    i_unknown_vtbl_t base;
    /**
     * @brief Log a message.
     * 
     * @param level Severity level (implementation-defined; e.g., 0=debug, 1=info, 2=warn, 3=error).
     * @param msg UTF-8, NUL-terminated message string.
     * 
     * Notes:
     * - The callee must not assume the pointer remains valid after the call returns.
     */
    void (*log)(i_logger_t *self, int32_t level, const char* msg);
} i_logger_vtbl_t;

struct i_logger_s { const i_logger_vtbl_t *vtbl; };

typedef struct i_string_vtbl_s {
    i_unknown_vtbl_t base;
    /**
     * @brief Obtain a pointer to a NUL-terminated C string view.
     * 
     * @param out_ptr Receives a pointer to an internal NUL-terminated string.
     * 
     * Lifetime:
     * - The returned pointer remains valid as long as the `i_string` object remains alive.
     * - If the caller needs to keep the data longer, it must copy it.
     * 
     * @return status_t Success or failure code.
     */
    status_t (*c_str)(i_string_t *self, const char* *out_ptr);
    /**
     * @brief Get the string length in bytes (excluding any terminating NUL).
     * 
     * @param out_len_bytes Receives the byte length.
     * @return status_t Success or failure code.
     */
    status_t (*length)(i_string_t *self, uint64_t *out_len_bytes);
} i_string_vtbl_t;

struct i_string_s { const i_string_vtbl_t *vtbl; };

typedef struct i_error_info_vtbl_s {
    i_unknown_vtbl_t base;
    /**
     * @brief Get the associated status/error code.
     * 
     * @param out_code Receives the error code.
     * @return status_t Success or failure code.
     */
    status_t (*get_code)(i_error_info_t *self, status_t *out_code);
    /**
     * @brief Get the error message as an i_string object.
     * 
     * @param out_msg Receives an i_string object containing the message.
     * 
     * Ownership:
     * - On success, `out_msg` is AddRef'ed for the caller and must be released by the caller.
     * 
     * @return status_t Success or failure code.
     */
    status_t (*get_message_string)(i_error_info_t *self, i_string_t **out_msg);
    /**
     * @brief Get the error message into a caller-provided buffer.
     * 
     * Sizing convention:
     * - If `buf.ptr == NULL` and `buf.cap == 0`, the function returns the required size
     *   via `out_len_incl_nul` without writing any bytes.
     * - Otherwise, the function writes up to `buf.cap` bytes (including the terminating NUL if space permits).
     * 
     * @param buf In/out buffer descriptor.
     * @param out_len_incl_nul Receives required/actual length in bytes including terminating NUL.
     * 
     * @return status_t Success or failure code.
     */
    status_t (*get_message_buf)(i_error_info_t *self, nano_char_buf_t *buf, uint64_t *out_len_incl_nul);
} i_error_info_vtbl_t;

struct i_error_info_s { const i_error_info_vtbl_t *vtbl; };

typedef struct i_factory_vtbl_s {
    i_unknown_vtbl_t base;
    /**
     * Create an instance of a component (CLSID) and return a specific interface (IID).
     * 
     * @param clsid Class identifier of the component to instantiate.
     * @param iid Interface identifier requested from the created instance.
     * @param outptr Receives the requested interface pointer on success (AddRef'ed).
     * 
     * Ownership:
     * - On success, `outptr` receives a valid interface pointer owned by the caller.
     * - On failure, `outptr` must be set to NULL.
     * 
     * @return status_t Success or failure code.
     */
    status_t (*create_instance)(i_factory_t *self, clsid clsid, nanoc_iid_t iid, void* *outptr);
} i_factory_vtbl_t;

struct i_factory_s { const i_factory_vtbl_t *vtbl; };

/* Helper functions */
/** Releases and nulls the pointer (COM-style). */
static inline void i_unknown_releasep(i_unknown_t **p)
{
    if (p && *p) { (*p)->vtbl->base.release((i_unknown_t *)*p); *p = NULL; }
}

/** Queries the requested interface from an i_unknown. */
static inline status_t qi_i_unknown(i_unknown_t *from, i_unknown_t **out)
{
    if (out) *out = NULL;
    if (!from || !out) return STATUS_E_INVALID_ARG;
    return from->vtbl->query_interface(from, IID_I_UNKNOWN, (void **)out);
}

/** Releases and nulls the pointer (COM-style). */
static inline void i_logger_releasep(i_logger_t **p)
{
    if (p && *p) { (*p)->vtbl->base.release((i_unknown_t *)*p); *p = NULL; }
}

/** Queries the requested interface from an i_unknown. */
static inline status_t qi_i_logger(i_unknown_t *from, i_logger_t **out)
{
    if (out) *out = NULL;
    if (!from || !out) return STATUS_E_INVALID_ARG;
    return from->vtbl->query_interface(from, IID_I_LOGGER, (void **)out);
}

/** Releases and nulls the pointer (COM-style). */
static inline void i_string_releasep(i_string_t **p)
{
    if (p && *p) { (*p)->vtbl->base.release((i_unknown_t *)*p); *p = NULL; }
}

/** Queries the requested interface from an i_unknown. */
static inline status_t qi_i_string(i_unknown_t *from, i_string_t **out)
{
    if (out) *out = NULL;
    if (!from || !out) return STATUS_E_INVALID_ARG;
    return from->vtbl->query_interface(from, IID_I_STRING, (void **)out);
}

/** Releases and nulls the pointer (COM-style). */
static inline void i_error_info_releasep(i_error_info_t **p)
{
    if (p && *p) { (*p)->vtbl->base.release((i_unknown_t *)*p); *p = NULL; }
}

/** Queries the requested interface from an i_unknown. */
static inline status_t qi_i_error_info(i_unknown_t *from, i_error_info_t **out)
{
    if (out) *out = NULL;
    if (!from || !out) return STATUS_E_INVALID_ARG;
    return from->vtbl->query_interface(from, IID_I_ERROR_INFO, (void **)out);
}

/** Releases and nulls the pointer (COM-style). */
static inline void i_factory_releasep(i_factory_t **p)
{
    if (p && *p) { (*p)->vtbl->base.release((i_unknown_t *)*p); *p = NULL; }
}

/** Queries the requested interface from an i_unknown. */
static inline status_t qi_i_factory(i_unknown_t *from, i_factory_t **out)
{
    if (out) *out = NULL;
    if (!from || !out) return STATUS_E_INVALID_ARG;
    return from->vtbl->query_interface(from, IID_I_FACTORY, (void **)out);
}


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _NANO_BASE_H__ */
