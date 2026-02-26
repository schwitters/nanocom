#pragma once
#include "nidl_ast.h"

/* Generate Nano-COM C11 headers into out_dir/include.
   Returns 1 on success, 0 on failure (errors printed to stderr). */
int codegen_c_headers(const idl_file_t *f, const char *out_dir);
