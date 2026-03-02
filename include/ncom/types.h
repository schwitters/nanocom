#pragma once
#include <stdint.h>

typedef int32_t  ncom_status_t;
typedef struct { uint64_t hi; uint64_t lo; } ncom_iid_t;
typedef struct { uint64_t hi; uint64_t lo; } ncom_clsid_t;

typedef struct {
    const uint8_t* ptr;
    uint32_t len;
} ncom_string_view_t;

typedef struct {
    char* ptr;
    uint64_t cap;
} ncom_char_buf_t;

#define NCOM_IID_EQ(A, B)   ((A)->hi == (B)->hi && (A)->lo == (B)->lo)
#define NCOM_CLSID_EQ(A, B) ((A)->hi == (B)->hi && (A)->lo == (B)->lo)