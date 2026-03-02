#pragma once
#include <stdint.h>
/* Cross-compiler tiny atomic abstraction for reference counting.
   - On GCC/Clang we use C11 <stdatomic.h>.
   - On MSVC C compiler, C11 atomics may be unavailable unless special flags are used.
     Therefore we use Windows Interlocked operations.
*/

#ifdef _WIN32
  #include <windows.h>
  typedef volatile LONG ncom_refcnt_t;

  static inline void ncom_refcnt_init(ncom_refcnt_t *rc, LONG v) { *rc = v; }
  static inline uint32_t ncom_refcnt_inc(ncom_refcnt_t *rc) { return (uint32_t)InterlockedIncrement(rc); }
  static inline uint32_t ncom_refcnt_dec(ncom_refcnt_t *rc) { return (uint32_t)InterlockedDecrement(rc); }

#else
  #include <stdatomic.h>
  typedef atomic_uint ncom_refcnt_t;

  static inline void ncom_refcnt_init(ncom_refcnt_t *rc, unsigned v) { atomic_init(rc, v); }
  static inline uint32_t ncom_refcnt_inc(ncom_refcnt_t *rc)
  {
      return atomic_fetch_add_explicit(rc, 1u, memory_order_relaxed) + 1u;
  }
  static inline uint32_t ncom_refcnt_dec(ncom_refcnt_t *rc)
  {
      return atomic_fetch_sub_explicit(rc, 1u, memory_order_acq_rel) - 1u;
  }
#endif
