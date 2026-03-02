#pragma once
#include <ncom/types.h>

#define NCOM_OK                 ((ncom_status_t)0)
#define NCOM_E_FAIL             ((ncom_status_t)-1)
#define NCOM_E_INVALID_ARG      ((ncom_status_t)-2)
#define NCOM_E_NO_MEM           ((ncom_status_t)-3)
#define NCOM_E_NOT_FOUND        ((ncom_status_t)-4)
#define NCOM_E_NOT_IMPL         ((ncom_status_t)-5)
#define NCOM_E_MORE_DATA        ((ncom_status_t)-6) 

#define NCOM_SUCCEEDED(ST)      ((ncom_status_t)(ST) >= 0)
#define NCOM_FAILED(ST)         ((ncom_status_t)(ST) < 0)


#define NCOM_CHECK(EXPR) \
    do { \
        st = (EXPR); \
        if (NCOM_FAILED(st)) goto cleanup; \
    } while (0)
// ...