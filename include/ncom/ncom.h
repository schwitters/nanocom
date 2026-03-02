#pragma once

/**
 * @file ncom.h
 * @brief Umbrella header for the ncom (Nano COM) component framework.
 * * Includes all core definitions, types, error codes, and base interfaces 
 * required to build or consume ncom components in C11 or C++.
 */

/* 1. Primitive types and constants */
#include <ncom/types.h>
#include <ncom/errors.h>

/* 2. Control flow macros (Goto-cleanup pattern) */
#include <ncom/style.h>

/* 3. Thread-safe reference counting */
#include <ncom/atomic.h>

/* 4. Core interfaces (IUnknown, IFactory, Container-Of macro) */
#include <ncom/base.h>

/* 5. Standard data types for the ABI boundary */
#include <ncom/string.h>
#include <ncom/error_info.h>

/* 6. Plugin loader signatures (Boundary between Host and DLL/SO) */
#include <ncom/plugin.h>

/* 7. Optional C++ Wrappers (Smart Pointers & Result Types)
 * If compiled as C++, we directly pull in our type-safe, 
 * RAII-based wrappers. For pure C compilers, this is ignored.
 */
#ifdef __cplusplus
#include <ncom/ncom_ptr.hpp>
#include <ncom/ncom_error.hpp>
#include <ncom/ncom_result.hpp>
#endif