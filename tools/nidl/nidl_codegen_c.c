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
 * @file nidl_codegen_c.c
 * @brief Nidl codegen c.
 */

#include "nidl_codegen_c.h"
#include "nidl_arena.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#if defined(_WIN32)
  #include <direct.h>
  #include <errno.h>
  static int mkdir_one(const char *p) { return _mkdir(p) == 0 || errno == EEXIST; }
#else
  #include <sys/stat.h>
  #include <sys/types.h>
  #include <errno.h>
  static int mkdir_one(const char *p) { return mkdir(p, 0755) == 0 || errno == EEXIST; }
#endif

static int mkdir_p(const char *path)
{
    char tmp[1024];
    size_t n = strlen(path);
    if (n >= sizeof(tmp)) return 0;
    memcpy(tmp, path, n + 1);

    for (size_t i = 1; i < n; i++) {
        if (tmp[i] == '/' || tmp[i] == '\\') {
            char c = tmp[i];
            tmp[i] = '\0';
            if (tmp[0] && !mkdir_one(tmp)) return 0;
            tmp[i] = c;
        }
    }
    if (tmp[0] && !mkdir_one(tmp)) return 0;
    return 1;
}
static void format_ident(const char* in, char* out, size_t cap) {
    /* 1. Kugelsichere Pointer- und Kapazitätsprüfung */
    if (!out || cap == 0) {
        return;
    }

    if (!in) {
        out[0] = '\0';
        return;
    }

    /* 2. Explizite Längenprüfung (Clean Code, leicht zu lesen) */
    size_t len = strlen(in);
    if (len >= 2 && in[0] == 'i' && in[1] == '_') {
        snprintf(out, cap, "i%s", in + 2);
    }
    else {
        snprintf(out, cap, "%s", in);
    }
}
static void ln(FILE *fp, const char *s) { fputs(s, fp); fputc('\n', fp); }

static int is_space(char c) { return c==' ' || c=='\t' || c=='\n'; }

static void emit_doc(arena_t* arena,FILE *fp, const char *doc, int indent)
{
    (void)arena;
    if (!doc || !doc[0]) return;

    /* Trim leading blank lines */
    const char *start = doc;
    while (*start) {
        const char *line = start;
        while (*start && *start != '\n') start++;
        const char *end = start;
        int blank = 1;
        for (const char *p=line; p<end; p++) {
            if (!is_space(*p)) { blank = 0; break; }
        }
        if (!blank) { start = line; break; }
        if (*start == '\n') start++;
    }

    /* Trim trailing blank lines */
    const char *enddoc = doc + strlen(doc);
    while (enddoc > start) {
        const char *p = enddoc;
        while (p > start && p[-1] != '\n') p--;
        const char *line = p;
        const char *line_end = enddoc;
        int blank = 1;
        for (const char *q=line; q<line_end; q++) {
            if (!is_space(*q)) { blank = 0; break; }
        }
        if (!blank) break;
        enddoc = (p > start) ? (p - 1) : start;
    }

    for (int i=0;i<indent;i++) fputc(' ', fp);
    fputs("/**\n", fp);

    const char *p = start;
    while (p < enddoc && *p) {
        const char *line = p;
        while (p < enddoc && *p && *p != '\n') p++;
        const char *line_end = p;

        /* Strip leading whitespace and optional leading '*' (common in block docs) */
        const char *q = line;
        while (q < line_end && is_space(*q)) q++;
        if (q < line_end && *q == '*') {
            q++;
            if (q < line_end && *q == ' ') q++;
        }

        for (int i=0;i<indent;i++) fputc(' ', fp);
        fputs(" * ", fp);
        fwrite(q, 1, (size_t)(line_end - q), fp);
        fputc('\n', fp);

        if (p < enddoc && *p == '\n') p++;
    }

    for (int i=0;i<indent;i++) fputc(' ', fp);
    fputs(" */\n", fp);
}

static int hexval(int c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

static int parse_uuid_16(const char *s, uint8_t out[16])
{
    if (!s) return 0;
    int idx = 0;
    for (int i = 0; s[i] && idx < 16; ) {
        if (s[i] == '-') { i++; continue; }
        int a = hexval((unsigned char)s[i++]);
        int b = hexval((unsigned char)s[i++]);
        if (a < 0 || b < 0) return 0;
        out[idx++] = (uint8_t)((a << 4) | b);
    }
    return idx == 16;
}

static void uuid_to_u64_pair(const uint8_t u[16], uint64_t *hi, uint64_t *lo)
{
    uint64_t h = 0, l = 0;
    for (int i = 0; i < 8; i++) h = (h << 8) | u[i];
    for (int i = 8; i < 16; i++) l = (l << 8) | u[i];
    *hi = h; *lo = l;
}

static int is_interface_type(const idl_file_t *f, const char *t)
{
    for (const idl_interface_t *it = f->interfaces; it; it = it->next) {
        if (it->name && strcmp(it->name, t) == 0) return 1;
    }
    return 0;
}


static void trim_spaces(const char *in, char out[256])
{
    size_t n = 0;
    int was_space = 0;
    for (size_t i=0; in && in[i] && n < 255; i++) {
        char c = in[i];
        if (c==' ' || c=='\t' || c=='\n' || c=='\r') { was_space = 1; continue; }
        if (was_space && n>0) out[n++] = ' ';
        was_space = 0;
        out[n++] = c;
    }
    out[n] = 0;
}

/* Returns a mapped C type string for a primitive/framework IDL type.
 * Result is arena-allocated (no static buffer) so multiple results can coexist. */
static const char *map_prim(arena_t *a, const char *t)
{
    if (!t) return "void";

    char norm[256];
    trim_spaces(t, norm);

    int ptrs = 0;
    for (size_t i=0; norm[i]; i++) if (norm[i]=='*') ptrs++;

    char base[256];
    size_t bn=0;
    for (size_t i=0; norm[i] && bn<255; i++) {
        if (norm[i]=='*') continue;
        base[bn++] = norm[i];
    }
    base[bn]=0;
    trim_spaces(base, base);

    int is_const = (strncmp(base, "const ", 6)==0);
    const char *p = base;
    if (is_const) p += 6;

    int is_unsigned = 0;
    if (strncmp(p, "unsigned ", 9)==0) { is_unsigned=1; p += 9; }
    else if (strncmp(p, "signed ", 7)==0) { p += 7; }

    const char *mapped = NULL;
    /* Basic C types */
    if (strcmp(p, "octet")==0) mapped = "uint8_t";
    else if (strcmp(p, "char")==0) mapped = "char";
    else if (strcmp(p, "int")==0) mapped = is_unsigned ? "uint32_t" : "int32_t";
    else if (strcmp(p, "short")==0) mapped = is_unsigned ? "uint16_t" : "int16_t";
    else if (strcmp(p, "long")==0) mapped = is_unsigned ? "uint32_t" : "int32_t";
    else if (strcmp(p, "long long")==0) mapped = is_unsigned ? "uint64_t" : "int64_t";
    else if (strcmp(p, "void_ptr")==0) mapped = "void*";
    else if (strcmp(p, "const_char_ptr")==0) mapped = "const char*";

    /* ncom Framework Base Types */
    else if (strcmp(p, "status_t")==0) mapped = "ncom_status_t";
    else if (strcmp(p, "uuid")==0) mapped = "ncom_iid_t";
    else if (strcmp(p, "clsid")==0) mapped = "ncom_clsid_t";
    else if (strcmp(p, "string_view")==0) mapped = "ncom_string_view_t";
    else if (strcmp(p, "char_buf")==0) mapped = "ncom_char_buf_t";
    else if (strcmp(p, "const_iid_ptr") == 0) mapped = "const ncom_iid_t*";
    /* ncom Framework Core Interfaces */
    else if (strcmp(p, "i_unknown")==0) mapped = "ncom_iunknown_t";
    else if (strcmp(p, "ncom_iunknown") == 0) mapped = "ncom_iunknown_t";
    else if (strcmp(p, "ncom_iunknown_ptr") == 0) mapped = "ncom_iunknown_t*";
    else if (strcmp(p, "i_factory")==0) mapped = "ncom_ifactory_t";
    else if (strcmp(p, "ncom_ifactory") == 0) mapped = "ncom_ifactory_t";
    else if (strcmp(p, "i_string")==0) mapped = "ncom_istring_t";
    else if (strcmp(p, "ncom_istring") == 0) mapped = "ncom_istring_t";
    else if (strcmp(p, "i_error_info")==0) mapped = "ncom_ierror_info_t";
    else if (strcmp(p, "ncom_ierror_info") == 0) mapped = "ncom_ierror_info_t";

    if (!mapped) mapped = p;

    char buf[512];
    {
        char ptrbuf[32] = {0};
        for (int i = 0; i < ptrs && i < 16; i++) ptrbuf[i] = '*';
        snprintf(buf, sizeof(buf), "%s%s%s",
                 is_const ? "const " : "", mapped, ptrbuf);
    }
    return arena_strdup(a, buf, strlen(buf));
}

static void to_upper_ident(const char *in, char out[256])
{
    size_t n = strlen(in);
    if (n >= 255) n = 255;
    for (size_t i=0;i<n;i++) {
        char c = in[i];
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        out[i] = c;
    }
    out[n] = '\0';
}


static void to_upper_guard(const char *in, char out[256])
{
    size_t n = strlen(in);
    if (n >= 255) n = 255;
    for (size_t i=0;i<n;i++) {
        char c = in[i];
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        /* normalize non-identifier chars to '_' */
        if (!((c>='A'&&c<='Z') || (c>='0'&&c<='9') || c=='_')) c = '_';
        out[i] = c;
    }
    out[n] = '\0';
}

static const idl_typedef_t **typedefs_to_array(arena_t *a, const idl_typedef_t *t, int *out_n)
{
    int n=0; for (const idl_typedef_t *x=t;x;x=x->next) n++;
    *out_n = n;
    if (n==0) return NULL;
    const idl_typedef_t **arr = (const idl_typedef_t **)arena_calloc(a, (size_t)n, sizeof(*arr));
    if (!arr) { *out_n = 0; return NULL; }
    int i=n-1;
    for (const idl_typedef_t *x=t;x;x=x->next) arr[i--]=x;
    return arr;
}

static const idl_struct_t **structs_to_array(arena_t *a, const idl_struct_t *s, int *out_n)
{
    int n=0; for (const idl_struct_t *x=s;x;x=x->next) n++;
    *out_n = n;
    if (n==0) return NULL;
    const idl_struct_t **arr = (const idl_struct_t **)arena_calloc(a, (size_t)n, sizeof(*arr));
    if (!arr) { *out_n = 0; return NULL; }
    int i=n-1;
    for (const idl_struct_t *x=s;x;x=x->next) arr[i--]=x;
    return arr;
}


static const idl_interface_t *find_interface(const idl_file_t *f, const char *name)
{
    for (const idl_interface_t *it = f->interfaces; it; it = it->next) {
        if (it->name && name && strcmp(it->name, name) == 0) return it;
    }
    return NULL;
}

static int iface_depth(const idl_file_t *f, const idl_interface_t *it, int guard)
{
    if (!it || !it->base || !it->base[0]) return 0;
    if (guard > 32) return 0; /* cycle guard */
    const idl_interface_t *b = find_interface(f, it->base);
    if (!b) return 0;
    return 1 + iface_depth(f, b, guard + 1);
}

static void sort_interfaces_by_base(const idl_file_t *f, const idl_interface_t **arr, int n)
{
    /* Stable insertion sort by depth (base-first). */
    for (int i=1; i<n; i++) {
        const idl_interface_t *key = arr[i];
        int kd = iface_depth(f, key, 0);
        int j = i - 1;
        while (j >= 0) {
            int jd = iface_depth(f, arr[j], 0);
            if (jd <= kd) break;
            arr[j+1] = arr[j];
            j--;
        }
        arr[j+1] = key;
    }
}


static const idl_method_t **methods_to_array(arena_t *a, const idl_method_t *m, int *out_n)
{
    int n=0;
    for (const idl_method_t *x=m; x; x=x->next) n++;
    *out_n = n;
    if (n==0) return NULL;

    const idl_method_t **arr = (const idl_method_t **)arena_calloc(a, (size_t)n, sizeof(*arr));
    if (!arr) { *out_n = 0; return NULL; }
    int i = n-1;
    for (const idl_method_t *x=m; x; x=x->next) arr[i--] = x;
    return arr;
}

static const idl_param_t **params_to_array(arena_t *a, const idl_param_t *p, int *out_n)
{
    int n=0;
    for (const idl_param_t *x=p; x; x=x->next) n++;
    *out_n = n;
    if (n==0) return NULL;

    const idl_param_t **arr = (const idl_param_t **)arena_calloc(a, (size_t)n, sizeof(*arr));
    if (!arr) { *out_n = 0; return NULL; }
    int i = n-1;
    for (const idl_param_t *x=p; x; x=x->next) arr[i--] = x;
    return arr;
}


/* Returns the C type string for any IDL type (primitive, framework, or user-defined).
 * Result is arena-allocated; multiple results can coexist safely. */
static const char *map_type(arena_t *a, const idl_file_t *f, const char *t)
{
    /* Map primitives and framework types first */
    const char *m = map_prim(a, t);
    if (m && strcmp(m, t) != 0) return m;

    /* User-defined interfaces: use source_module if imported */
    for (const idl_interface_t *it = f->interfaces; it; it = it->next) {
        if (it->name && strcmp(it->name, t) == 0) {
            const char *mod = (it->source_module && it->source_module[0])
                              ? it->source_module
                              : (f->module_name ? f->module_name : "mod");
            char clean_type[256];
            format_ident(t, clean_type, sizeof(clean_type));
            char buf[512];
            snprintf(buf, sizeof(buf), "%s_%s_t", mod, clean_type);
            return arena_strdup(a, buf, strlen(buf));
        }
    }

    /* User-defined structs: use source_module if imported */
    for (const idl_struct_t *st = f->structs; st; st = st->next) {
        if (st->name && strcmp(st->name, t) == 0) {
            const char *mod = (st->source_module && st->source_module[0])
                              ? st->source_module
                              : (f->module_name ? f->module_name : "mod");
            char clean_type[256];
            format_ident(t, clean_type, sizeof(clean_type));
            char buf[512];
            snprintf(buf, sizeof(buf), "%s_%s_t", mod, clean_type);
            return arena_strdup(a, buf, strlen(buf));
        }
    }

    return t;
}

/* Returns the vtable base type name for an interface's vtbl struct.
 * Result for user-defined bases is arena-allocated. */
static const char *vtbl_base_type(arena_t *a, const idl_file_t *f, const idl_interface_t *it)
{
    /* 1. No base specified -> IUnknown */
    if (!it->base || it->base[0] == '\0') {
        return "ncom_iunknown_vtbl_t";
    }

    /* 2. Framework base interfaces (fixed vtable names) */
    if (strcmp(it->base, "i_unknown") == 0 || strcmp(it->base, "ncom_iunknown") == 0)
        return "ncom_iunknown_vtbl_t";
    if (strcmp(it->base, "i_string") == 0 || strcmp(it->base, "ncom_istring") == 0)
        return "ncom_istring_vtbl_t";
    if (strcmp(it->base, "i_error_info") == 0 || strcmp(it->base, "ncom_ierror_info") == 0)
        return "ncom_ierror_info_vtbl_t";
    if (strcmp(it->base, "i_factory") == 0 || strcmp(it->base, "ncom_ifactory") == 0)
        return "ncom_ifactory_vtbl_t";

    /* 3. User-defined base interface (may be from an imported module) */
    const idl_interface_t *base_it = find_interface(f, it->base);
    const char *mod = (base_it && base_it->source_module && base_it->source_module[0])
                      ? base_it->source_module
                      : (f->module_name ? f->module_name : "mod");
    char clean_base[256];
    format_ident(it->base, clean_base, sizeof(clean_base));
    char buf[512];
    snprintf(buf, sizeof(buf), "%s_%s_vtbl_t", mod, clean_base);
    return arena_strdup(a, buf, strlen(buf));
}

static void emit_module_preamble(arena_t* arena, FILE *fp, const idl_file_t *f)
{
    (void)arena;
    const char *module_name = f->module_name ? f->module_name : "module";
    char up[256];
    to_upper_guard(module_name, up);

    fprintf(fp, "#ifndef NCOM_GENERATED_%s_H\n", up);
    fprintf(fp, "#define NCOM_GENERATED_%s_H\n\n", up);

    /* Include the core framework umbrella header */
    ln(fp, "#include <ncom/ncom.h>");

    /* Include generated headers for imported modules */
    for (const idl_import_t *imp = f->imports; imp; imp = imp->next) {
        if (imp->module_name && imp->module_name[0])
            fprintf(fp, "#include \"%s.h\"\n", imp->module_name);
    }
    fputc('\n', fp);

    ln(fp, "#ifdef __cplusplus");
    ln(fp, "extern \"C\" {");
    ln(fp, "#endif");
    fputc('\n', fp);
}

static void emit_module_epilogue(arena_t* arena,FILE* fp, const idl_file_t* f)
{
    (void)arena;
    const char* module_name = f->module_name ? f->module_name : "module";

    char mod_up[256];
    to_upper_ident(module_name, mod_up);

    fputc('\n', fp);
    ln(fp, "#ifdef __cplusplus");
    ln(fp, "} /* extern \"C\" */");
    fputc('\n', fp);
    /* ncom_ptr.hpp defines ncom::iid_traits<T>; include explicitly so this
     * header does not rely on ncom.h being included first in C++ TUs. */
    ln(fp, "#include <ncom/ncom_ptr.hpp>");
    fputc('\n', fp);

    /* Generate C++ traits for type-safe query_interface (ncom::ptr<T>) */
    ln(fp, "namespace ncom {");

    for (const idl_interface_t* it = f->interfaces; it; it = it->next) {
        if (!it->name || it->is_imported) continue;

        /* 1. Das 'i_' Präfix entfernen (i_clock2 -> iclock2) */
        char clean_name[256];
        format_ident(it->name, clean_name, sizeof(clean_name));

        /* 2. In Großbuchstaben für die IID umwandeln (iclock2 -> ICLOCK2) */
        char up[256];
        to_upper_ident(clean_name, up);

        /* 3. Sauber formatiert ausgeben! */
        fprintf(fp, "    template<> struct iid_traits<%s_%s_t> {\n", module_name, clean_name);
        fprintf(fp, "        static const ncom_iid_t* get() { return &%s_IID_%s; }\n", mod_up, up);
        fprintf(fp, "    };\n");
    }

    ln(fp, "} /* namespace ncom */");
    ln(fp, "#endif /* __cplusplus */\n");

    /* Close the include guard */
    char guard_up[256];
    to_upper_guard(module_name, guard_up);
    fprintf(fp, "#endif /* NCOM_GENERATED_%s_H */\n", guard_up);
}

static void emit_typedefs(arena_t* arena,FILE *fp, const idl_file_t *f)
{
    int n=0;
    const idl_typedef_t **arr = typedefs_to_array(arena, f->typedefs, &n);
    for (int i=0;i<n;i++) {
        const idl_typedef_t *td = arr[i];
        if (!td->alias || !td->target) continue;
        emit_doc(arena,fp, td->doc, 0);
        fprintf(fp, "typedef %s %s;\n\n", map_type(arena, f, td->target), td->alias);
    }
}

static void emit_structs(arena_t* arena,FILE *fp, const idl_file_t *f)
{
    int n=0;
    const idl_struct_t **arr = structs_to_array(arena, f->structs, &n);
    for (int i=0;i<n;i++) {
        const idl_struct_t *st = arr[i];
        if (!st->name) continue;

        emit_doc(arena,fp, st->doc, 0);
        fprintf(fp, "typedef struct %s_s {\n", st->name);

        /* fields are stored reversed, normalize */
        int fn=0;
        const idl_struct_field_t *fld = st->fields;
        for (const idl_struct_field_t *x=fld;x;x=x->next) fn++;
        const idl_struct_field_t **farr = NULL;
        if (fn>0) {
            farr = (const idl_struct_field_t **)arena_calloc(arena, (size_t)fn, sizeof(*farr));
            if (farr) {
                int k=fn-1;
                for (const idl_struct_field_t *x=fld;x;x=x->next) farr[k--]=x;
            }
        }
        for (int j=0;j<fn;j++) {
            const idl_struct_field_t *fl = farr[j];
            if (!fl->name || !fl->type) continue;
            emit_doc(arena,fp, fl->doc, 4);
            fprintf(fp, "    %s %s;\n", map_type(arena, f, fl->type), fl->name);
        }

        fprintf(fp, "} %s_t;\n\n", st->name);
    }
}

static void emit_ids(arena_t* arena,FILE *fp, const idl_file_t *f)
{
    (void)arena;
    char mod_up[256];
    to_upper_ident(f->module_name ? f->module_name : "MOD", mod_up);

    for (const idl_interface_t *it = f->interfaces; it; it = it->next) {
        if (!it->name || !it->uuid || it->is_imported) continue;
        uint8_t u[16];
        if (!parse_uuid_16(it->uuid, u)) continue;
        uint64_t hi, lo;
        uuid_to_u64_pair(u, &hi, &lo);

        /* 1. Bereinige den Namen: "i_logger" -> "ilogger" */
        char clean_name[256];
        format_ident(it->name, clean_name, sizeof(clean_name));

        /* 2. In Großbuchstaben umwandeln: "ilogger" -> "ILOGGER" */
        char up[256];
        to_upper_ident(clean_name, up);

        /* 3. Generiert: static const ncom_iid_t DEMO_IID_ILOGGER = ... */
        fprintf(fp, "static const ncom_iid_t %s_IID_%s = { 0x%016llxULL, 0x%016llxULL };\n",
                mod_up, up, (unsigned long long)hi, (unsigned long long)lo);
    }
    
    /* Optional: Das Gleiche für CLSIDs, falls du coclasses in der IDL hast */
    for (const idl_coclass_t *cc = f->coclasses; cc; cc = cc->next) {
        if (!cc->name || !cc->uuid) continue;
        uint8_t u[16];
        if (!parse_uuid_16(cc->uuid, u)) continue;
        uint64_t hi, lo;
        uuid_to_u64_pair(u, &hi, &lo);

        char up[256];
        to_upper_ident(cc->name, up);

        fprintf(fp, "static const ncom_clsid_t %s_CLSID_%s = { 0x%016llxULL, 0x%016llxULL };\n",
                mod_up, up, (unsigned long long)hi, (unsigned long long)lo);
    }
    fputc('\n', fp);
}
static int is_framework_iface(const char* t) {
    if (!t) return 0;
    return strcmp(t, "i_unknown") == 0 || 
           strcmp(t, "i_factory") == 0 ||
           strcmp(t, "i_string") == 0 || 
           strcmp(t, "i_error_info") == 0;
}
static void emit_interfaces(arena_t* arena,FILE *fp, const idl_file_t *f)
{
    /* Build array of local (non-imported) interfaces only. */
    int n = 0;
    for (const idl_interface_t *x = f->interfaces; x; x = x->next)
        if (!x->is_imported) n++;

    const idl_interface_t **arr = NULL;
    if (n > 0) {
        arr = (const idl_interface_t **)calloc((size_t)n, sizeof(*arr));
        int idx = n - 1;
        for (const idl_interface_t *x = f->interfaces; x; x = x->next)
            if (!x->is_imported) arr[idx--] = x;
    }
    if (arr && n > 1) sort_interfaces_by_base(f, arr, n);

    const char *mod = f->module_name ? f->module_name : "mod";

    /* 1. Forward declarations */
    for (int i = 0; i < n; i++) {
        const idl_interface_t *it = arr[i];
        if (!it->name) continue;
        
        /* Namen bereinigen: i_logger -> ilogger */
        char clean_name[256];
        format_ident(it->name, clean_name, sizeof(clean_name));

        emit_doc(arena,fp, it->doc, 0);
        fprintf(fp, "typedef struct %s_%s_s %s_%s_t;\n", mod, clean_name, mod, clean_name);
    }
    fputc('\n', fp);

    /* 2. VTable + object layout */
    for (int i = 0; i < n; i++) {
        const idl_interface_t *it = arr[i];
        if (!it->name) continue;

        /* Namen bereinigen */
        char clean_name[256];
        format_ident(it->name, clean_name, sizeof(clean_name));

        fprintf(fp, "typedef struct %s_%s_vtbl_s {\n", mod, clean_name);
        
        /* Base interface vtbl */
        fprintf(fp, "    %s base;\n", vtbl_base_type(arena, f, it));

        int mcount = 0;
        const idl_method_t **marr = methods_to_array(arena, it->methods, &mcount);
        for (int mi = 0; mi < mcount; mi++) {
            const idl_method_t *m = marr[mi];

            const char *ret = map_type(arena, f, m->ret_type);

            emit_doc(arena,fp, m->doc, 4);
            fprintf(fp, "    %s (*%s)(%s_%s_t *self", ret, m->name, mod, clean_name);

            int pcount = 0;
            const idl_param_t **parr = params_to_array(arena, m->params, &pcount);
            for (int pi = 0; pi < pcount; pi++) {
                const idl_param_t *pa = parr[pi];
                const char *ct = map_type(arena, f, pa->type);

                int is_iface = is_interface_type(f, pa->type) || is_framework_iface(pa->type);

                if (is_iface && strcmp(pa->dir, "out") == 0) {
                    fprintf(fp, ", %s **%s", ct, pa->name);
                    continue;
                }

                int is_128bit = (pa->type != NULL) &&
                                (strcmp(pa->type, "uuid") == 0 || strcmp(pa->type, "clsid") == 0);

                if (strcmp(pa->dir, "out") == 0 || strcmp(pa->dir, "inout") == 0) {
                    fprintf(fp, ", %s *%s", ct, pa->name);
                } else if (is_128bit) {
                    fprintf(fp, ", const %s *%s", ct, pa->name);
                } else if (is_iface) {
                    fprintf(fp, ", %s *%s", ct, pa->name);
                } else {
                    fprintf(fp, ", %s %s", ct, pa->name);
                }
            }

            fprintf(fp, ");\n");
        }

        fprintf(fp, "} %s_%s_vtbl_t;\n\n", mod, clean_name);
        
        /* WICHTIG: Das eigentliche Objekt-Struct */
        fprintf(fp, "struct %s_%s_s { const %s_%s_vtbl_t *vtbl; };\n\n", mod, clean_name, mod, clean_name);
    }

    /* 3. Helper functions */
    ln(fp, "/* Helper functions */");
    
    char mod_up[256];
    to_upper_ident(mod, mod_up);

    for (int i = 0; i < n; i++) {
        const idl_interface_t *it = arr[i];
        if (!it->name) continue;

        /* Namen bereinigen */
        char clean_name[256];
        format_ident(it->name, clean_name, sizeof(clean_name));

        fprintf(fp,
            "/** Releases and nulls the pointer (COM-style). */\n"
            "static inline void %s_%s_releasep(%s_%s_t **p)\n"
            "{\n"
            "    if (p && *p) {\n"
            "        ncom_iunknown_t *unk = (ncom_iunknown_t *)(*p);\n"
            "        unk->vtbl->release(unk);\n"
            "        *p = NULL;\n"
            "    }\n"
            "}\n\n", mod, clean_name, mod, clean_name);
        char up[256];
        to_upper_ident(clean_name, up);

        fprintf(fp,
            "/** Queries the requested interface from an ncom_iunknown_t. */\n"
            "static inline ncom_status_t %s_%s_qi(ncom_iunknown_t *from, %s_%s_t **out)\n"
            "{\n"
            "    if (out) *out = NULL;\n"
            "    if (!from || !out) return NCOM_E_INVALID_ARG;\n"
            "    return from->vtbl->query_interface(from, &%s_IID_%s, (void **)out);\n"
            "}\n\n", mod, clean_name, mod, clean_name, mod_up, up);
    }
}

static int emit_module_header(arena_t* arena,const idl_file_t *f, const char *inc_dir)
{
    const char *mn = (f && f->module_name) ? f->module_name : "module";
    size_t need = strlen(inc_dir) + 1 + strlen(mn) + 3; /* "/" + ".h" + NUL */
    char *path = arena_alloc(arena, need);
    if (!path) return -1;
    snprintf(path, need, "%s/%s.h", inc_dir, mn);
    fprintf(stderr,"nidlgen: Writing file %s\n",path);
    FILE *fp = fopen(path, "wb");
    if (!fp) return 0;

    emit_module_preamble(arena, fp, f);

    emit_typedefs(arena,fp, f);
    emit_structs(arena,fp, f);
    emit_ids(arena,fp, f);
    emit_interfaces(arena,fp, f);
    
    /* FIX: Pass the AST object 'f', not the string 'mn'! */
    emit_module_epilogue(arena,fp, f); 

    fclose(fp);
    return 1;
}

int codegen_c_headers(arena_t* arena,const idl_file_t *f, const char *out_dir)
{
    if (!f || !out_dir) return 0;

    char inc_dir[1024];
    snprintf(inc_dir, sizeof(inc_dir), "%s/include", out_dir);

    if (!mkdir_p(out_dir)) return 0;
    if (!mkdir_p(inc_dir)) return 0;

    /* Single header per module */
    if (!emit_module_header(arena,f, inc_dir)) return 0;

    return 1;
}
