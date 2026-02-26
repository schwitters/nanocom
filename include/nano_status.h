#pragma once
#include <stdint.h>

/* status_t is a lightweight HRESULT-like code:
   >= 0 success, < 0 failure. */
typedef int32_t status_t;

#define STATUS_OK                ((status_t)0)
#define STATUS_E_FAIL            ((status_t)-1)
#define STATUS_E_INVALID_ARG     ((status_t)-2)
#define STATUS_E_NO_MEM          ((status_t)-3)
#define STATUS_E_NOT_FOUND       ((status_t)-4)
#define STATUS_E_NOT_IMPL        ((status_t)-5)
#define STATUS_E_MORE_DATA       ((status_t)-6)

#define STATUS_SUCCEEDED(ST)     ((status_t)(ST) >= 0)
#define STATUS_FAILED(ST)        ((status_t)(ST) < 0)
