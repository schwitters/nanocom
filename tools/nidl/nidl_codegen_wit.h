#ifndef NCOM_TOOLS_NIDL_NIDL_CODEGEN_WIT_H
#define NCOM_TOOLS_NIDL_NIDL_CODEGEN_WIT_H

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

#include "nidl_ast.h"
#include "nidl_arena.h"

int codegen_wit(arena_t *arena, const idl_file_t *f, const char *out_dir);

#endif /* NCOM_TOOLS_NIDL_NIDL_CODEGEN_WIT_H */
