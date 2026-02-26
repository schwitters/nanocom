# nidlgen (Nano IDL Generator)

This folder contains a minimal IDL toolchain integrated into the project.

## Goals
- CORBA-like IDL syntax as authoring format (`idl/*.idl`)
- Generate Nano-COM C11 headers (vtables + interface structs)
- Future: generate C++ wrappers and Rust `repr(C)` bindings

## Parser / Lexer tooling (requested)
- Lexer spec: `nidl_lexer.re` (re2c)
- Parser grammar: `nidl_parser.y` (Lemon from SQLite)

### Important note
To keep this repository self-contained and buildable without external tools,
the current `nidlgen` binary uses a small handwritten parser (`nidl_parse.c`)
that supports the subset used by `idl/nano.idl`.

If you want regeneration via re2c + lemon:
- Install re2c and build SQLite's `lemon` (or vendor `lemon.c` + `lempar.c` from SQLite).
- Then replace the handwritten parser with generated sources and wire them in CMake.

References:
- re2c: https://github.com/skvadrik/re2c
- Lemon (SQLite): https://sqlite.org/lemon.html


## re2c + lemon switch (auto-fetch)

If you enable `NANO_USE_RE2C_LEMON=ON`, the build can automatically fetch:

- re2c from https://github.com/skvadrik/re2c.git
- sqlite from https://github.com/sqlite/sqlite.git (only `tool/lemon.c` + `tool/lempar.c` are used)

Configure:

```bash
cmake -S . -B build \
  -DNANO_USE_RE2C_LEMON=ON \
  -DNANO_FETCH_DEPS=ON
cmake --build build --config Release
```

This will:
- build `re2c` from sources (CMake project)
- build `lemon_tool` from `sqlite/tool/lemon.c`
- generate `nidl_lex_gen.c` and `nidl_parser.c` during the build.

### External tools mode (no fetching)

```bash
cmake -S . -B build \
  -DNANO_USE_RE2C_LEMON=ON \
  -DNANO_FETCH_DEPS=OFF \
  -DLEMON_EXECUTABLE=/path/to/lemon \
  -DLEMPAR_TEMPLATE=/path/to/lempar.c
cmake --build build --config Release
```

Notes:
- The grammar (`nidl_parser.y`) is complete for the subset used by `idl/nano.idl`.


Note: the build passes `-T` to lemon to point to the copied `lempar.c` template in the generated folder.
