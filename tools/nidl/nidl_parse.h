#pragma once
#include "nidl_ast.h"

/* Parse IDL source into an AST.
   Returns NULL on error. Errors are printed to stderr. */
idl_file_t *nidl_parse(const char *src);
