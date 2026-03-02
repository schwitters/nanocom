#ifndef NCOM_TOOLS_NIDL_NIDL_AST_H
#define NCOM_TOOLS_NIDL_NIDL_AST_H

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
#include <stdint.h>

/* Minimal AST structures for the IDL subset used in this project.
   This is intentionally tiny and can be extended over time. */

typedef struct idl_param_s {
    const char *doc;
    const char *dir;   /* "in" | "out" | "inout" */
    const char *type;  /* identifier (builtin or user) */
    const char *name;  /* identifier */
    int optional;      /* [optional] */
    struct idl_param_s *next;
} idl_param_t;

typedef struct idl_method_s {
    const char *doc;
    const char *ret_type;   /* e.g., "status_t" or "void" */
    const char *name;
    idl_param_t *params;
    struct idl_method_s *next;
} idl_method_t;

typedef struct idl_interface_s {
    const char *doc;
    const char *name;
    const char *base;       /* optional base interface */
    const char *uuid;       /* uuid string literal */
    idl_method_t *methods;
    struct idl_interface_s *next;
} idl_interface_t;

typedef struct idl_struct_field_s {
    const char *doc;
    const char *type;
    const char *name;
    struct idl_struct_field_s *next;
} idl_struct_field_t;

typedef struct idl_struct_s {
    const char *doc;
    const char *name;
    idl_struct_field_t *fields;
    struct idl_struct_s *next;
} idl_struct_t;

typedef struct idl_typedef_s {
    const char *doc;
    const char *alias;
    const char *target;
    struct idl_typedef_s *next;
} idl_typedef_t;

typedef struct idl_coclass_s {
    const char *doc;
    const char *name;
    const char *uuid;
    struct idl_coclass_s *next;
} idl_coclass_t;

typedef struct idl_file_s {
    const char *module_name;
    idl_typedef_t *typedefs;
    idl_struct_t *structs;
    idl_interface_t *interfaces;
    idl_coclass_t *coclasses;
} idl_file_t;

/* Allocation is done from a simple bump arena (see nidl_arena.c). */

#endif /* NCOM_TOOLS_NIDL_NIDL_AST_H */
