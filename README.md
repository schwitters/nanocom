# nano_com_minimal

Minimal Nano-COM style component system in C11 with:
- ABI-stable interfaces (vtables)
- Multi-interface object (`i_logger` + `i_clock`)
- Plugin loaded at runtime (`.so` / `.dll`) using a single exported entry point
- `goto cleanup` error handling (portable, analyzer-friendly)

## Build (Linux/macOS)
```bash
cmake -S . -B build
cmake --build build --config Release
```
Binaries:
- `build/bin/host_app`
- `build/bin/libsample_plugin.so` (Linux) / `libsample_plugin.dylib` (macOS)

Run:
```bash
cd build/bin
./host_app ./libsample_plugin.so
```

## Build (Windows)
```powershell
cmake -S . -B build
cmake --build build --config Release
```
Binaries (typically in `build/bin/Release` with multi-config generators):
- `host_app.exe`
- `sample_plugin.dll`

Run:
```powershell
cd build\bin\Release
.\host_app.exe .\sample_plugin.dll
```

## Notes
- Interfaces are immutable: never reorder/modify vtable slots; create a new IID for new versions.
- Strings cross module boundaries only as `const char*` literals here; for dynamic strings, prefer:
  - caller-provided buffers, or
  - an `i_string` object with `release()`.


### MSVC note
On Windows, MSVC's C compiler may not enable C11 `<stdatomic.h>` by default.
This project uses `include/nano_atomic.h` which maps refcounting to `InterlockedIncrement/Decrement` on Windows.


## Added features
- `i_error_info`: rich error object with message retrieval as `const char*`, `i_string`, and caller-provided buffer.
- `i_clock2`: versioned clock interface that can return `i_error_info`.
- `i_string`: immutable UTF-8 string object safe across shared library boundaries.


## Tests
After building:
```bash
cd build
ctest --output-on-failure
```


## IDL generator (CORBA-like syntax)

This project includes a minimal IDL tool `nidlgen` under `tools/nidl`.
The authoring file is `idl/nano.idl`.

By default, the project builds using the hand-written headers in `include/`.

To additionally run the generator at build time (and emit `build/generated/include/nidl_generated.h`):
```bash
cmake -S . -B build -DNANO_GENERATE_FROM_IDL=ON
cmake --build build --config Release
```

Note: the current generator supports only the subset used by `idl/nano.idl`.
The folder includes placeholder specs for re2c (`nidl_lexer.re`) and Lemon (`nidl_parser.y`)
for a future transition to generated lexer/parser.


## UUID tool

This repo includes a small CLI `uuid_tool` that generates UUIDv4 values and prints both:
- an IDL attribute line: `[uuid("...")]`
- a C constant line: `static const iid_t   ... = { 0x..ULL, 0x..ULL };`

Examples:

```bash
./uuid_tool 3
./uuid_tool 1 --prefix IID_ --name I_UNKNOWN
```


## Documentation (Doxygen)

The IDL supports doc-comments:
- Line doc: `/// ...`
- Block doc: `/** ... */`

`nidlgen` propagates these into generated C headers as Doxygen comments.

To build HTML docs (requires `doxygen` installed):

```bash
cmake -S . -B build -DNANO_BUILD_DOCS=ON
cmake --build build --target docs
```
