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
 * @file ncom.h
 * @brief Umbrella header for the Nano COM (ncom) public API.
 *
 * Include this header to consume the ncom C API. When compiled as C++, this header
 * also includes optional header-only RAII wrappers.
 */
#ifndef NCOM_NCOM_H
#define NCOM_NCOM_H

#include <ncom/types.h>
#include <ncom/errors.h>
#include <ncom/style.h>
#include <ncom/atomic.h>
#include <ncom/base.h>
#include <ncom/string.h>
#include <ncom/error_info.h>
#include <ncom/plugin.h>
#include <ncom/plugin_loader.h>

#ifdef __cplusplus
#include <ncom/ncom_ptr.hpp>
#include <ncom/ncom_error.hpp>
#include <ncom/ncom_result.hpp>
#endif

#endif /* NCOM_NCOM_H */
