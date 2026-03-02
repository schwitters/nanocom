# nano_com — a tiny, COM-inspired C11 component model

**nano_com** is a small, pragmatic framework for building **binary-stable**, **plugin-friendly** components in **C11** — inspired by the best ideas from **MS COM** (IUnknown, vtables, reference counting, QueryInterface), but designed to stay **portable** (Linux/macOS/Windows) and **C-first**.

If you’ve ever wanted:

* a clean way to expose interfaces from shared libraries (`.so` / `.dll`)
* stable ABI boundaries without C++
* a disciplined “who owns what memory?” story
* safe error reporting across module boundaries

…then nano_com is for you.

This repository contains:

* the **nano_com runtime** (`ncom_*`)
* a **minimal demo plugin** and host
* a small **IDL-ish generator tool** (`nidlgen`) that can generate interface boilerplate

Licensed under **Apache-2.0**.

---

## Why this exists

C is excellent for systems work, but plugin systems often degenerate into:

* ad-hoc function pointers
* unclear ownership rules
* versioning pain
* “works on my compiler” ABI bugs

nano_com provides a small set of conventions that keep things boring (in a good way):

* **Interfaces are vtables**
* **Every object supports IUnknown**
* **QueryInterface is the only cast**
* **Reference counting controls lifetime**
* **Strings/errors cross boundaries via caller-provided buffers**

No magic. No global registries. No exceptions. Just a clear contract.

---

## Concepts (30 seconds)

### Interface = vtable

An interface is a struct of function pointers (a vtable), plus an “instance pointer”.

### IUnknown (COM-style)

Every object supports:

* `query_interface(self, iid, out_iface)`
* `add_ref(self)`
* `release(self)`

### ABI stability

**Never reorder** vtable slots.
To extend an interface, either:

* append methods at the end (same ABI *if you control both sides*), or
* create a **new IID** and a new interface version (recommended for public/plugin APIs)

### Memory safety across modules

Avoid passing ownership of heap allocations across module boundaries unless you *also* pass an allocator interface.
nano_com defaults to safer patterns:

* caller-provided buffers
* views / spans
* explicit sizing calls

---

## Quick start (calm, step-by-step)

You don’t need to understand everything to try it.
Just build it and run the demo once.

### 1) Build

From the repository root:

```bash
cmake -S . -B build
cmake --build build
```

If you prefer Release:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### 2) Run the demo host

The demo host loads the demo plugin and calls an interface exposed by it.

```bash
./build/demo/ncom_demo_host
```

(Exact binary names may differ slightly depending on your generator/build options, but the build output will show you what was produced.)

If something goes wrong, don’t worry — common causes are:

* missing execute bit on Linux/macOS (`chmod +x …`)
* `LD_LIBRARY_PATH` / runtime loader can’t find the plugin
* wrong working directory when launching the host

The demo is intentionally small so you can debug it without stress.

---

## A tiny tutorial: “Hello interface”

This is the mental model you’ll use again and again.

### Step A: define an interface (header)

Interfaces are described by:

* an IID (interface identifier)
* a vtable struct
* a “typed” interface struct containing a vtable pointer

Pseudo-example:

```c
typedef struct my_hello_vtbl_s {
    ncom_iunknown_vtbl_t base;    /* must start with IUnknown */
    ncom_status_t (*say_hello)(void *self, char *buf, size_t cap);
} my_hello_vtbl_t;

typedef struct my_hello_s {
    const my_hello_vtbl_t *vtbl;
} my_hello_t;
```

### Step B: implement it (plugin)

Your object typically embeds one or more interface “views”.
It implements `query_interface/add_ref/release` plus your methods.

### Step C: use it (host)

The host:

1. loads the plugin (`ncom_plugin_load`)
2. asks the plugin to create an instance (CLSIDs/classes)
3. calls `query_interface` for the interface it needs
4. calls methods through the vtable
5. releases everything

This pattern scales from tiny utilities to full systems.

---

## Plugin model in nano_com

A plugin exports a single symbol:

* `ncom_plugin_get_api_v1`

The host:

* opens the shared library (`dlopen` / `LoadLibrary`)
* resolves that symbol
* verifies the ABI version
* uses function pointers in the returned API table (e.g. `create_instance`)

No registry, no installers, no COM runtime required.

---

## Error handling

nano_com uses `ncom_status_t` status codes (think HRESULT-like).
Optional detail is carried via an error interface (COM-style `IErrorInfo` concept) so you can pass rich error text without throwing exceptions and without allocating across boundaries.

Common style:

* return status codes
* fill output parameters only on success
* provide a “sizing call” pattern for strings/buffers

---

## Directory overview

Typical layout:

* `include/ncom/` — public API headers
* `src/ncom/` — runtime implementation
* `demo/` — demo host and plugin
* `tools/` — utilities (e.g. UUID tool, nidl generator)
* `cmake/` — helper modules (if present)

---

## Design goals

* **Portable C11**
* **Stable ABI at module boundaries**
* **No hidden allocations across boundaries by default**
* **Explicit lifetimes (refcount)**
* **Readable codebase (clean, boring, testable)**
* **Small surface area**

Non-goals:

* a full COM clone (registry, apartments, marshaling, etc.)
* automatic distribution / package management
* hiding complexity with macros

---

## License

Apache License 2.0 — see `LICENSE` and `NOTICE`.

---

## Next steps

If you want to explore deeper, a good order is:

1. read `include/ncom/base.h` (IUnknown + core rules)
2. look at the demo plugin implementation
3. follow how the host loads the plugin and queries interfaces
4. optionally inspect `nidlgen` to see how boilerplate generation fits in

If you want, paste the exact demo file names (host + plugin) from your build output and I’ll tailor the “Run” section so it matches your binary names 1:1.
