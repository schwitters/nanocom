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

#include "nidl_codegen_wit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void emit_wit_doc(FILE *fp, const char *doc, int indent) {
    if (!doc || !doc[0]) return;
    const char *p = doc;
    while (*p) {
        const char *line = p;
        while (*p && *p != '\n') p++;
        const char *line_end = p;
        
        while (line < line_end && (*line == ' ' || *line == '\t')) line++;
        if (line < line_end && *line == '*') {
            line++;
            if (line < line_end && *line == ' ') line++;
        }
        
        for (int i = 0; i < indent; i++) fputc(' ', fp);
        fputs("/// ", fp);
        fwrite(line, 1, (size_t)(line_end - line), fp);
        fputc('\n', fp);
        
        if (*p == '\n') p++;
    }
}

static void to_wit_ident(const char *in, char *out, size_t cap) {
    if (!in || cap == 0) return;
    size_t len = strlen(in);
    const char *p = in;
    if (len >= 2 && in[0] == 'i' && in[1] == '_') {
        p = in + 2;
    }
    size_t idx = 0;
    for (size_t i = 0; p[i] && idx < cap - 1; i++) {
        char c = p[i];
        if (c == '_') {
            out[idx++] = '-';
        } else if (c >= 'A' && c <= 'Z') {
            out[idx++] = (char)(c - 'A' + 'a');
        } else {
            out[idx++] = c;
        }
    }
    out[idx] = '\0';
}

static const char *map_wit_type(const char *t, char out[256]) {
    if (!t) { strcpy(out, "void"); return out; }
    
    const char *p = t;
    while (*p == ' ' || *p == '\t') p++;
    
    if (strncmp(p, "const ", 6) == 0) {
        p += 6;
    }
    
    char base[256];
    size_t bn = 0;
    int ptrs = 0;
    while (*p && bn < 255) {
        char c = *p;
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            p++;
            continue;
        }
        if (c == '*') {
            ptrs++;
            p++;
            continue;
        }
        base[bn++] = c;
        p++;
    }
    base[bn] = '\0';
    
    const char *mapped = NULL;
    if (strcmp(base, "octet") == 0) mapped = "u8";
    else if (strcmp(base, "char") == 0) {
        if (ptrs > 0) mapped = "string";
        else mapped = "char";
    }
    else if (strcmp(base, "int") == 0) mapped = "s32";
    else if (strcmp(base, "short") == 0) mapped = "s16";
    else if (strcmp(base, "long") == 0) mapped = "s32";
    else if (strcmp(base, "longlong") == 0) mapped = "s64";
    else if (strcmp(base, "unsignedint") == 0) mapped = "u32";
    else if (strcmp(base, "unsignedshort") == 0) mapped = "u16";
    else if (strcmp(base, "unsignedlong") == 0) mapped = "u32";
    else if (strcmp(base, "unsignedlonglong") == 0) mapped = "u64";
    else if (strcmp(base, "int8_t") == 0) mapped = "s8";
    else if (strcmp(base, "uint8_t") == 0) mapped = "u8";
    else if (strcmp(base, "int16_t") == 0) mapped = "s16";
    else if (strcmp(base, "uint16_t") == 0) mapped = "u16";
    else if (strcmp(base, "int32_t") == 0) mapped = "s32";
    else if (strcmp(base, "uint32_t") == 0) mapped = "u32";
    else if (strcmp(base, "int64_t") == 0) mapped = "s64";
    else if (strcmp(base, "uint64_t") == 0) mapped = "u64";
    else if (strcmp(base, "string_view") == 0) mapped = "string";
    else if (strcmp(base, "const_char_ptr") == 0) mapped = "string";
    else if (strcmp(base, "constcharptr") == 0) mapped = "string";
    
    else if (strcmp(base, "const_iid_ptr") == 0) mapped = "string";
    else if (strcmp(base, "constiidptr") == 0) mapped = "string";
    else if (strcmp(base, "i_unknown") == 0 || strcmp(base, "iunknown") == 0 || strcmp(base, "ncom_iunknown") == 0) mapped = "unknown";
    else if (strcmp(base, "i_factory") == 0 || strcmp(base, "ifactory") == 0 || strcmp(base, "ncom_ifactory") == 0) mapped = "factory";
    else if (strcmp(base, "i_string") == 0 || strcmp(base, "istring") == 0 || strcmp(base, "ncom_istring") == 0) mapped = "string-resource";
    else if (strcmp(base, "i_error_info") == 0 || strcmp(base, "ierrorinfo") == 0 || strcmp(base, "ncom_ierror_info") == 0) mapped = "error-info";
    
    if (!mapped) {
        to_wit_ident(base, out, 256);
        return out;
    }
    
    strcpy(out, mapped);
    return out;
}

int codegen_wit(arena_t *arena, const idl_file_t *f, const char *out_dir) {
    if (!f || !out_dir) return 0;
    
    char wit_dir[1024];
    snprintf(wit_dir, sizeof(wit_dir), "%s/wit", out_dir);
    if (!mkdir_p(out_dir)) return 0;
    if (!mkdir_p(wit_dir)) return 0;
    
    const char *mn = f->module_name ? f->module_name : "module";
    char path[2048];
    snprintf(path, sizeof(path), "%s/%s.wit", wit_dir, mn);
    fprintf(stderr, "nidlgen: Writing file %s\n", path);
    
    FILE *fp = fopen(path, "wb");
    if (!fp) return 0;
    
    char pkg[256];
    to_wit_ident(mn, pkg, sizeof(pkg));
    
    fprintf(fp, "// Generated by nidlgen from %s.idl. DO NOT EDIT.\n", mn);
    fprintf(fp, "package %s:%s;\n\n", pkg, pkg);
    fprintf(fp, "interface %s-api {\n", pkg);
    
    fprintf(fp, "    resource unknown;\n");
    fprintf(fp, "    resource factory;\n");
    fprintf(fp, "    resource string-resource;\n");
    fprintf(fp, "    resource error-info;\n\n");
    
    int td_count = 0;
    const idl_typedef_t *td_curr = f->typedefs;
    while (td_curr) { td_count++; td_curr = td_curr->next; }
    if (td_count > 0) {
        const idl_typedef_t **td_arr = arena_calloc(arena, (size_t)td_count, sizeof(*td_arr));
        int idx = td_count - 1;
        for (const idl_typedef_t *x = f->typedefs; x; x = x->next) td_arr[idx--] = x;
        for (int i = 0; i < td_count; i++) {
            const idl_typedef_t *td = td_arr[i];
            if (!td->alias || !td->target) continue;
            emit_wit_doc(fp, td->doc, 4);
            char target_wit[256];
            map_wit_type(td->target, target_wit);
            char alias_wit[256];
            to_wit_ident(td->alias, alias_wit, sizeof(alias_wit));
            fprintf(fp, "    type %s = %s;\n\n", alias_wit, target_wit);
        }
    }
    
    int st_count = 0;
    const idl_struct_t *st_curr = f->structs;
    while (st_curr) { st_count++; st_curr = st_curr->next; }
    if (st_count > 0) {
        const idl_struct_t **st_arr = arena_calloc(arena, (size_t)st_count, sizeof(*st_arr));
        int idx = st_count - 1;
        for (const idl_struct_t *x = f->structs; x; x = x->next) st_arr[idx--] = x;
        for (int i = 0; i < st_count; i++) {
            const idl_struct_t *st = st_arr[i];
            if (!st->name) continue;
            emit_wit_doc(fp, st->doc, 4);
            char st_name_wit[256];
            to_wit_ident(st->name, st_name_wit, sizeof(st_name_wit));
            fprintf(fp, "    record %s {\n", st_name_wit);
            
            int fld_count = 0;
            for (const idl_struct_field_t *x = st->fields; x; x = x->next) fld_count++;
            if (fld_count > 0) {
                const idl_struct_field_t **fld_arr = arena_calloc(arena, (size_t)fld_count, sizeof(*fld_arr));
                int fidx = fld_count - 1;
                for (const idl_struct_field_t *x = st->fields; x; x = x->next) fld_arr[fidx--] = x;
                for (int j = 0; j < fld_count; j++) {
                    const idl_struct_field_t *fld = fld_arr[j];
                    if (!fld->name || !fld->type) continue;
                    emit_wit_doc(fp, fld->doc, 8);
                    char fld_name_wit[256];
                    to_wit_ident(fld->name, fld_name_wit, sizeof(fld_name_wit));
                    char fld_type_wit[256];
                    map_wit_type(fld->type, fld_type_wit);
                    fprintf(fp, "        %s: %s%s\n", fld_name_wit, fld_type_wit, (j == fld_count - 1) ? "" : ",");
                }
            }
            fprintf(fp, "    }\n\n");
        }
    }
    
    int it_count = 0;
    for (const idl_interface_t *x = f->interfaces; x; x = x->next) {
        if (!x->is_imported) it_count++;
    }
    if (it_count > 0) {
        const idl_interface_t **it_arr = arena_calloc(arena, (size_t)it_count, sizeof(*it_arr));
        int idx = it_count - 1;
        for (const idl_interface_t *x = f->interfaces; x; x = x->next) {
            if (!x->is_imported) it_arr[idx--] = x;
        }
        for (int i = 0; i < it_count; i++) {
            const idl_interface_t *it = it_arr[i];
            if (!it->name) continue;
            
            emit_wit_doc(fp, it->doc, 4);
            char it_name_wit[256];
            to_wit_ident(it->name, it_name_wit, sizeof(it_name_wit));
            fprintf(fp, "    resource %s {\n", it_name_wit);
            
            int m_count = 0;
            for (const idl_method_t *x = it->methods; x; x = x->next) m_count++;
            if (m_count > 0) {
                const idl_method_t **m_arr = arena_calloc(arena, (size_t)m_count, sizeof(*m_arr));
                int midx = m_count - 1;
                for (const idl_method_t *x = it->methods; x; x = x->next) m_arr[midx--] = x;
                for (int j = 0; j < m_count; j++) {
                    const idl_method_t *m = m_arr[j];
                    if (!m->name) continue;
                    
                    emit_wit_doc(fp, m->doc, 8);
                    char m_name_wit[256];
                    to_wit_ident(m->name, m_name_wit, sizeof(m_name_wit));
                    
                    int p_count = 0;
                    for (const idl_param_t *x = m->params; x; x = x->next) p_count++;
                    
                    const idl_param_t **p_arr = NULL;
                    if (p_count > 0) {
                        p_arr = arena_calloc(arena, (size_t)p_count, sizeof(*p_arr));
                        int pidx = p_count - 1;
                        for (const idl_param_t *x = m->params; x; x = x->next) p_arr[pidx--] = x;
                    }
                    
                    int in_count = 0;
                    int out_count = 0;
                    for (int k = 0; k < p_count; k++) {
                        const idl_param_t *p = p_arr[k];
                        if (strcmp(p->dir, "in") == 0 || strcmp(p->dir, "inout") == 0) in_count++;
                        if (strcmp(p->dir, "out") == 0 || strcmp(p->dir, "inout") == 0) out_count++;
                    }
                    
                    fprintf(fp, "        %s: func(", m_name_wit);
                    int first_in = 1;
                    for (int k = 0; k < p_count; k++) {
                        const idl_param_t *p = p_arr[k];
                        if (strcmp(p->dir, "in") == 0 || strcmp(p->dir, "inout") == 0) {
                            if (!first_in) fprintf(fp, ", ");
                            first_in = 0;
                            char p_name_wit[256];
                            to_wit_ident(p->name, p_name_wit, sizeof(p_name_wit));
                            char p_type_wit[256];
                            map_wit_type(p->type, p_type_wit);
                            fprintf(fp, "%s: %s", p_name_wit, p_type_wit);
                        }
                    }
                    fprintf(fp, ")");
                    
                    int is_status = (strcmp(m->ret_type, "status_t") == 0 || strcmp(m->ret_type, "ncom_status_t") == 0);
                    
                    if (is_status) {
                        if (out_count == 0) {
                            fprintf(fp, " -> result<_, s32>;\n");
                        } else if (out_count == 1) {
                            const idl_param_t *out_p = NULL;
                            for (int k = 0; k < p_count; k++) {
                                if (strcmp(p_arr[k]->dir, "out") == 0 || strcmp(p_arr[k]->dir, "inout") == 0) {
                                    out_p = p_arr[k];
                                    break;
                                }
                            }
                            char out_type_wit[256];
                            map_wit_type(out_p->type, out_type_wit);
                            fprintf(fp, " -> result<%s, s32>;\n", out_type_wit);
                        } else {
                            fprintf(fp, " -> result<tuple<");
                            int first_out = 1;
                            for (int k = 0; k < p_count; k++) {
                                const idl_param_t *p = p_arr[k];
                                if (strcmp(p->dir, "out") == 0 || strcmp(p->dir, "inout") == 0) {
                                    if (!first_out) fprintf(fp, ", ");
                                    first_out = 0;
                                    char out_type_wit[256];
                                    map_wit_type(p->type, out_type_wit);
                                    fprintf(fp, "%s", out_type_wit);
                                }
                            }
                            fprintf(fp, ">, s32>;\n");
                        }
                    } else {
                        int has_c_ret = (strcmp(m->ret_type, "void") != 0);
                        int total_rets = out_count + (has_c_ret ? 1 : 0);
                        
                        if (total_rets > 0) {
                            fprintf(fp, " -> ");
                            if (total_rets == 1) {
                                if (has_c_ret) {
                                    char ret_type_wit[256];
                                    map_wit_type(m->ret_type, ret_type_wit);
                                    fprintf(fp, "%s;\n", ret_type_wit);
                                } else {
                                    const idl_param_t *out_p = NULL;
                                    for (int k = 0; k < p_count; k++) {
                                        if (strcmp(p_arr[k]->dir, "out") == 0 || strcmp(p_arr[k]->dir, "inout") == 0) {
                                            out_p = p_arr[k];
                                            break;
                                        }
                                    }
                                    char out_type_wit[256];
                                    map_wit_type(out_p->type, out_type_wit);
                                    fprintf(fp, "%s;\n", out_type_wit);
                                }
                            } else {
                                fprintf(fp, "tuple<");
                                int first_out = 1;
                                if (has_c_ret) {
                                    char ret_type_wit[256];
                                    map_wit_type(m->ret_type, ret_type_wit);
                                    fprintf(fp, "%s", ret_type_wit);
                                    first_out = 0;
                                }
                                for (int k = 0; k < p_count; k++) {
                                    const idl_param_t *p = p_arr[k];
                                    if (strcmp(p->dir, "out") == 0 || strcmp(p->dir, "inout") == 0) {
                                        if (!first_out) fprintf(fp, ", ");
                                        first_out = 0;
                                        char out_type_wit[256];
                                        map_wit_type(p->type, out_type_wit);
                                        fprintf(fp, "%s", out_type_wit);
                                    }
                                }
                                fprintf(fp, ">;\n");
                            }
                        } else {
                            fprintf(fp, ";\n");
                        }
                    }
                }
            }
            fprintf(fp, "    }\n\n");
        }
    }
    
    fprintf(fp, "}\n");
    fclose(fp);
    return 1;
}
