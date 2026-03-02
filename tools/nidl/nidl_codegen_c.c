#include "nidl_codegen_c.h"
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

static void ln(FILE *fp, const char *s) { fputs(s, fp); fputc('\n', fp); }

static int is_space(char c) { return c==' ' || c=='\t' || c=='\n'; }

static void emit_doc(FILE *fp, const char *doc, int indent)
{
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

static const char *map_prim(const char *t)
{
    static char buf[256];
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
    if (strcmp(p, "octet")==0) mapped = "uint8_t";
    else if (strcmp(p, "char")==0) mapped = "char";
    else if (strcmp(p, "int")==0) mapped = is_unsigned ? "uint32_t" : "int32_t";
    else if (strcmp(p, "short")==0) mapped = is_unsigned ? "uint16_t" : "int16_t";
    else if (strcmp(p, "long")==0) mapped = is_unsigned ? "uint32_t" : "int32_t";
    else if (strcmp(p, "long long")==0) mapped = is_unsigned ? "uint64_t" : "int64_t";
    else if (strcmp(p, "uuid")==0) mapped = "nanoc_iid_t";
	else if (strcmp(p, "clsid")==0) mapped = "nanoc_clsid_t";
    else if (strcmp(p, "void_ptr")==0) mapped = "void*";
    else if (strcmp(p, "const_char_ptr")==0) mapped = "const char*";
    else if (strcmp(p, "string_view")==0) mapped = "nano_string_view_t";
    else if (strcmp(p, "char_buf")==0) mapped = "nano_char_buf_t";

    if (!mapped) mapped = p;

    buf[0] = 0;
    if (is_const) strncat(buf, "const ", sizeof(buf)-1);
    strncat(buf, mapped, sizeof(buf)-1);
    for (int i=0;i<ptrs;i++) strncat(buf, "*", sizeof(buf)-1);
    return buf;
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

static const idl_typedef_t **typedefs_to_array(const idl_typedef_t *t, int *out_n)
{
    int n=0; for (const idl_typedef_t *x=t;x;x=x->next) n++;
    *out_n = n;
    if (n==0) return NULL;
    const idl_typedef_t **arr = (const idl_typedef_t **)calloc((size_t)n, sizeof(*arr));
    int i=n-1;
    for (const idl_typedef_t *x=t;x;x=x->next) arr[i--]=x;
    return arr;
}

static const idl_struct_t **structs_to_array(const idl_struct_t *s, int *out_n)
{
    int n=0; for (const idl_struct_t *x=s;x;x=x->next) n++;
    *out_n = n;
    if (n==0) return NULL;
    const idl_struct_t **arr = (const idl_struct_t **)calloc((size_t)n, sizeof(*arr));
    int i=n-1;
    for (const idl_struct_t *x=s;x;x=x->next) arr[i--]=x;
    return arr;
}

static const idl_interface_t **interfaces_to_array(const idl_interface_t *it, int *out_n)
{
    int n=0; for (const idl_interface_t *x=it;x;x=x->next) n++;
    *out_n = n;
    if (n==0) return NULL;
    const idl_interface_t **arr = (const idl_interface_t **)calloc((size_t)n, sizeof(*arr));
    int i=n-1;
    for (const idl_interface_t *x=it;x;x=x->next) arr[i--]=x;
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


static const idl_coclass_t **coclasses_to_array(const idl_coclass_t *cc, int *out_n)
{
    int n=0; for (const idl_coclass_t *x=cc;x;x=x->next) n++;
    *out_n = n;
    if (n==0) return NULL;
    const idl_coclass_t **arr = (const idl_coclass_t **)calloc((size_t)n, sizeof(*arr));
    int i=n-1;
    for (const idl_coclass_t *x=cc;x;x=x->next) arr[i--]=x;
    return arr;
}
static const idl_method_t **methods_to_array(const idl_method_t *m, int *out_n)
{
    int n=0;
    for (const idl_method_t *x=m; x; x=x->next) n++;
    *out_n = n;
    if (n==0) return NULL;

    const idl_method_t **arr = (const idl_method_t **)calloc((size_t)n, sizeof(*arr));
    int i = n-1;
    for (const idl_method_t *x=m; x; x=x->next) arr[i--] = x;
    return arr;
}

static const idl_param_t **params_to_array(const idl_param_t *p, int *out_n)
{
    int n=0;
    for (const idl_param_t *x=p; x; x=x->next) n++;
    *out_n = n;
    if (n==0) return NULL;

    const idl_param_t **arr = (const idl_param_t **)calloc((size_t)n, sizeof(*arr));
    int i = n-1;
    for (const idl_param_t *x=p; x; x=x->next) arr[i--] = x;
    return arr;
}


static int is_struct_type(const idl_file_t *f, const char *t)
{
    for (const idl_struct_t *st = f->structs; st; st = st->next) {
        if (st->name && strcmp(st->name, t) == 0) return 1;
    }
    return 0;
}

static const char *map_type(const idl_file_t *f, const char *t)
{
    /* Map primitives/typedef-builtin names first */
    const char *m = map_prim(t);
    if (m && strcmp(m, t) != 0) return m;

    /* Known structs: <name>_t */
    if (is_struct_type(f, t)) {
        static char buf[256];
        snprintf(buf, sizeof(buf), "%s_t", t);
        return buf;
    }

    /* Unknown user-defined types: keep as-is */
    return t;
}

static const char *vtbl_base_type(const idl_interface_t *it)
{
    if (!it->base || it->base[0] == 0) return "i_unknown_vtbl_t";
    /* base interface vtbl type */
    static char buf[256];
    snprintf(buf, sizeof(buf), "%s_vtbl_t", it->base);
    return buf;
}

static void emit_module_preamble(FILE *fp, const char *module_name)
{
    char up[256];
    to_upper_guard(module_name ? module_name : "MODULE", up);

    fprintf(fp, "#ifndef _NANO_COMPONENT_%s_H__\n", up);
    fprintf(fp, "#define _NANO_COMPONENT_%s_H__\n\n", up);

    ln(fp, "#include <nano_base.h>");
    ln(fp, "#include <nano_status.h>");
    ln(fp, "#include <stdint.h>");
    fputc('\n', fp);

    ln(fp, "#ifdef __cplusplus");
    ln(fp, "extern \"C\" {");
    ln(fp, "#endif");
    fputc('\n', fp);
}

static void emit_module_epilogue(FILE *fp, const char *module_name)
{
    (void)module_name;
    fputc('\n', fp);
    ln(fp, "#ifdef __cplusplus");
    ln(fp, "} /* extern \"C\" */");
    ln(fp, "#endif");
    fputc('\n', fp);

    /* close guard */
    char up[256];
    to_upper_guard(module_name ? module_name : "MODULE", up);
    fprintf(fp, "#endif /* _NANO_COMPONENT_%s_H__ */\n", up);
}

static void emit_typedefs(FILE *fp, const idl_file_t *f)
{
    int n=0;
    const idl_typedef_t **arr = typedefs_to_array(f->typedefs, &n);
    for (int i=0;i<n;i++) {
        const idl_typedef_t *td = arr[i];
        if (!td->alias || !td->target) continue;
        emit_doc(fp, td->doc, 0);
        fprintf(fp, "typedef %s %s;\n\n", map_type(f, td->target), td->alias);
    }
    free((void*)arr);
}

static void emit_structs(FILE *fp, const idl_file_t *f)
{
    int n=0;
    const idl_struct_t **arr = structs_to_array(f->structs, &n);
    for (int i=0;i<n;i++) {
        const idl_struct_t *st = arr[i];
        if (!st->name) continue;

        emit_doc(fp, st->doc, 0);
        fprintf(fp, "typedef struct %s_s {\n", st->name);

        /* fields are stored reversed, normalize */
        int fn=0;
        const idl_struct_field_t *fld = st->fields;
        for (const idl_struct_field_t *x=fld;x;x=x->next) fn++;
        const idl_struct_field_t **farr = NULL;
        if (fn>0) {
            farr = (const idl_struct_field_t **)calloc((size_t)fn, sizeof(*farr));
            int k=fn-1;
            for (const idl_struct_field_t *x=fld;x;x=x->next) farr[k--]=x;
        }
        for (int j=0;j<fn;j++) {
            const idl_struct_field_t *fl = farr[j];
            if (!fl->name || !fl->type) continue;
            emit_doc(fp, fl->doc, 4);
            fprintf(fp, "    %s %s;\n", map_type(f, fl->type), fl->name);
        }
        free((void*)farr);

        fprintf(fp, "} %s_t;\n\n", st->name);
    }
    free((void*)arr);
}

static void emit_ids(FILE *fp, const idl_file_t *f)
{
    fputc('\n', fp);
    ln(fp, "/* Interface IDs (IIDs) */");
    for (const idl_interface_t *it = f->interfaces; it; it = it->next) {
        if (!it->name || !it->uuid) continue;
        uint8_t u[16];
        if (!parse_uuid_16(it->uuid, u)) continue;
        uint64_t hi, lo;
        uuid_to_u64_pair(u, &hi, &lo);

        char up[256];
        to_upper_ident(it->name, up);

        fprintf(fp, "static const nanoc_iid_t IID_%s = { 0x%016llxULL, 0x%016llxULL };\n",
                up, (unsigned long long)hi, (unsigned long long)lo);
    }

    fputc('\n', fp);
    ln(fp, "/* Class IDs (CLSIDs) */");
    for (const idl_coclass_t *cc = f->coclasses; cc; cc = cc->next) {
        if (!cc->name || !cc->uuid) continue;
        uint8_t u[16];
        if (!parse_uuid_16(cc->uuid, u)) continue;
        uint64_t hi, lo;
        uuid_to_u64_pair(u, &hi, &lo);

        char up[256];
        to_upper_ident(cc->name, up);

        fprintf(fp, "static const nanoc_clsid_t CLSID_%s = { 0x%016llxULL, 0x%016llxULL };\n",
                up, (unsigned long long)hi, (unsigned long long)lo);
    }
    fputc('\n', fp);
}

static void emit_interfaces(FILE *fp, const idl_file_t *f)
{
    int n=0;
    const idl_interface_t **arr = interfaces_to_array(f->interfaces, &n);
    if (arr && n > 1) sort_interfaces_by_base(f, arr, n);

    /* Forward decls */
    for (int i=0;i<n;i++) {
        const idl_interface_t *it = arr[i];
        if (!it->name) continue;
        emit_doc(fp, it->doc, 0);
        fprintf(fp, "typedef struct %s_s %s_t;\n", it->name, it->name);
    }
    fputc('\n', fp);

    /* vtbl + object layout */
    for (int i=0;i<n;i++) {
        const idl_interface_t *it = arr[i];
        if (!it->name) continue;

        fprintf(fp, "typedef struct %s_vtbl_s {\n", it->name);
        fprintf(fp, "    %s base;\n", vtbl_base_type(it));

        int mcount=0;
        const idl_method_t **marr = methods_to_array(it->methods, &mcount);
        for (int mi=0; mi<mcount; mi++) {
            const idl_method_t *m = marr[mi];
            const char *ret = map_type(f, m->ret_type);

            emit_doc(fp, m->doc, 4);
            fprintf(fp, "    %s (*%s)(%s_t *self", ret, m->name, it->name);

            int pcount=0;
            const idl_param_t **parr = params_to_array(m->params, &pcount);
            for (int pi=0; pi<pcount; pi++) {
                const idl_param_t *pa = parr[pi];

                if (is_interface_type(f, pa->type) && strcmp(pa->dir, "out")==0) {
                    fprintf(fp, ", %s_t **%s", pa->type, pa->name);
                    continue;
                }

                const char *ct = map_type(f, pa->type);
                
                //128-Bit ID-Types
                int is_128bit = (strcmp(pa->type, "uuid") == 0 || strcmp(pa->type, "clsid") == 0);

                if (strcmp(pa->dir, "out")==0 || strcmp(pa->dir, "inout")==0) {
                    fprintf(fp, ", %s *%s", ct, pa->name);
                } else if (is_128bit) {
                    // NEU: ABI Fix - 128-Bit IDs immer by const reference
                    fprintf(fp, ", const %s *%s", ct, pa->name);
                } else {
                    fprintf(fp, ", %s %s", ct, pa->name);
                }
            }
            free((void*)parr);

            fprintf(fp, ");\n");
        }
        free((void*)marr);

        fprintf(fp, "} %s_vtbl_t;\n\n", it->name);
        fprintf(fp, "struct %s_s { const %s_vtbl_t *vtbl; };\n\n", it->name, it->name);
    }

    /* Helpers */
    ln(fp, "/* Helper functions */");
    for (int i=0;i<n;i++) {
        const idl_interface_t *it = arr[i];
        if (!it->name) continue;

        fprintf(fp,
            "/** Releases and nulls the pointer (COM-style). */\n"
            "static inline void %s_releasep(%s_t **p)\n"
            "{\n"
            "    if (p && *p) { (*p)->vtbl->base.release((i_unknown_t *)*p); *p = NULL; }\n"
            "}\n\n", it->name, it->name);

        char up[256];
        to_upper_ident(it->name, up);

        fprintf(fp,
            "/** Queries the requested interface from an i_unknown. */\n"
            "static inline status_t qi_%s(i_unknown_t *from, %s_t **out)\n"
            "{\n"
            "    if (out) *out = NULL;\n"
            "    if (!from || !out) return STATUS_E_INVALID_ARG;\n"
            "    return from->vtbl->query_interface(from, &IID_%s, (void **)out);\n"
            "}\n\n", it->name, it->name, up);
    }

    free((void*)arr);
}

static int emit_module_header(const idl_file_t *f, const char *inc_dir)
{
    const char *mn = (f && f->module_name) ? f->module_name : "module";
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s.h", inc_dir, mn);

    FILE *fp = fopen(path, "wb");
    if (!fp) return 0;

    emit_module_preamble(fp, mn);

    emit_typedefs(fp, f);
    emit_structs(fp, f);
    emit_ids(fp, f);
    emit_interfaces(fp, f);

    emit_module_epilogue(fp, mn);
    fclose(fp);
    return 1;
}

int codegen_c_headers(const idl_file_t *f, const char *out_dir)
{
    if (!f || !out_dir) return 0;

    char inc_dir[1024];
    snprintf(inc_dir, sizeof(inc_dir), "%s/include", out_dir);

    if (!mkdir_p(out_dir)) return 0;
    if (!mkdir_p(inc_dir)) return 0;

    /* Single header per module */
    if (!emit_module_header(f, inc_dir)) return 0;

    return 1;
}
