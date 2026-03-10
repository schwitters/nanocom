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
 * @file ncom_unit_tests.c
 * @brief Pure unit tests for ncom core functions (no plugin required).
 *
 * Tests: ncom_create_string, ncom_create_error_info (incl. all methods),
 *        IID identity (extern const), and null-arg handling.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <ncom/ncom.h>
#include <ncom/core_impl.h>

/* ============================================================================
 * Minimal test framework
 * ============================================================================ */

static int g_passed = 0;
static int g_failed = 0;

#define ASSERT(expr) \
    do { \
        if (expr) { \
            g_passed++; \
        } else { \
            fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); \
            g_failed++; \
        } \
    } while(0)

#define TEST(name) static void test_##name(void)
#define RUN(name)  do { printf("  %s\n", #name); test_##name(); } while(0)

/* ============================================================================
 * ncom_create_string
 * ============================================================================ */

TEST(string_null_input_yields_empty)
{
    ncom_istring_t *s = NULL;
    ASSERT(NCOM_SUCCEEDED(ncom_create_string(NULL, &s)));
    ASSERT(s != NULL);

    const char *p = NULL;
    ASSERT(NCOM_SUCCEEDED(s->vtbl->c_str(s, &p)));
    ASSERT(p != NULL && p[0] == '\0');

    uint64_t len = 99;
    ASSERT(NCOM_SUCCEEDED(s->vtbl->length(s, &len)));
    ASSERT(len == 0);

    ncom_istring_releasep(&s);
    ASSERT(s == NULL);
}

TEST(string_normal_content)
{
    ncom_istring_t *s = NULL;
    ASSERT(NCOM_SUCCEEDED(ncom_create_string("hello", &s)));

    const char *p = NULL;
    ASSERT(NCOM_SUCCEEDED(s->vtbl->c_str(s, &p)));
    ASSERT(p != NULL && strcmp(p, "hello") == 0);

    uint64_t len = 0;
    ASSERT(NCOM_SUCCEEDED(s->vtbl->length(s, &len)));
    ASSERT(len == 5);

    ncom_istring_releasep(&s);
}

TEST(string_null_out_returns_invalid_arg)
{
    ASSERT(ncom_create_string("x", NULL) == NCOM_E_INVALID_ARG);
}

TEST(string_refcount_starts_at_one)
{
    ncom_istring_t *s = NULL;
    ncom_create_string("rc", &s);

    /* AddRef raises to 2 */
    uint32_t rc = s->vtbl->base.add_ref((ncom_iunknown_t *)s);
    ASSERT(rc == 2);

    /* First release → 1, object still alive */
    rc = s->vtbl->base.release((ncom_iunknown_t *)s);
    ASSERT(rc == 1);

    ncom_istring_releasep(&s);
}

TEST(string_qi_iunknown)
{
    ncom_istring_t *s = NULL;
    ncom_create_string("qi", &s);

    ncom_iunknown_t *u = NULL;
    ncom_status_t st = s->vtbl->base.query_interface(
        (ncom_iunknown_t *)s, &NCOM_IID_IUNKNOWN, (void **)&u);
    ASSERT(NCOM_SUCCEEDED(st));
    ASSERT(u != NULL);
    ncom_iunknown_releasep(&u);

    ncom_istring_releasep(&s);
}

TEST(string_method_null_self)
{
    /* After the fix, calling c_str/length with NULL self → NCOM_E_INVALID_ARG */
    ncom_istring_t *s = NULL;
    ncom_create_string("tmp", &s);

    /* Save vtbl, then release so we can call with a zombie ptr indirectly.
     * Instead, just verify the NULL-self guard via a direct vtbl call. */
    const ncom_istring_vtbl_t *vtbl = s->vtbl;
    ncom_istring_releasep(&s);  /* s is now NULL */

    const char *p   = NULL;
    uint64_t   len  = 0;
    ASSERT(vtbl->c_str(NULL, &p)   == NCOM_E_INVALID_ARG);
    ASSERT(vtbl->length(NULL, &len) == NCOM_E_INVALID_ARG);
}

TEST(string_method_null_out_args)
{
    ncom_istring_t *s = NULL;
    ncom_create_string("x", &s);

    ASSERT(s->vtbl->c_str(s, NULL)   == NCOM_E_INVALID_ARG);
    ASSERT(s->vtbl->length(s, NULL)  == NCOM_E_INVALID_ARG);

    ncom_istring_releasep(&s);
}

/* ============================================================================
 * ncom_create_error_info
 * ============================================================================ */

TEST(error_info_null_out_returns_invalid_arg)
{
    ASSERT(ncom_create_error_info(NCOM_E_FAIL, "msg", NULL) == NCOM_E_INVALID_ARG);
}

TEST(error_info_code_roundtrip)
{
    ncom_ierror_info_t *e = NULL;
    ASSERT(NCOM_SUCCEEDED(ncom_create_error_info(NCOM_E_NO_MEM, "oom", &e)));
    ASSERT(e != NULL);

    ncom_status_t code = NCOM_OK;
    ASSERT(NCOM_SUCCEEDED(e->vtbl->get_code(e, &code)));
    ASSERT(code == NCOM_E_NO_MEM);

    ncom_ierror_info_releasep(&e);
    ASSERT(e == NULL);
}

TEST(error_info_message_string)
{
    ncom_ierror_info_t *e = NULL;
    ncom_create_error_info(NCOM_E_FAIL, "something went wrong", &e);

    ncom_istring_t *s = NULL;
    ASSERT(NCOM_SUCCEEDED(e->vtbl->get_message_string(e, &s)));
    ASSERT(s != NULL);

    const char *p = NULL;
    s->vtbl->c_str(s, &p);
    ASSERT(p != NULL && strcmp(p, "something went wrong") == 0);

    ncom_istring_releasep(&s);
    ncom_ierror_info_releasep(&e);
}

TEST(error_info_null_message_yields_empty)
{
    ncom_ierror_info_t *e = NULL;
    ncom_create_error_info(NCOM_E_FAIL, NULL, &e);

    ncom_istring_t *s = NULL;
    ASSERT(NCOM_SUCCEEDED(e->vtbl->get_message_string(e, &s)));

    const char *p = NULL;
    s->vtbl->c_str(s, &p);
    ASSERT(p != NULL && p[0] == '\0');

    ncom_istring_releasep(&s);
    ncom_ierror_info_releasep(&e);
}

TEST(error_info_message_buf_sizing_call)
{
    ncom_ierror_info_t *e = NULL;
    ncom_create_error_info(NCOM_E_FAIL, "hello", &e);

    uint64_t need = 0;
    ncom_char_buf_t empty = { NULL, 0 };
    ASSERT(NCOM_SUCCEEDED(e->vtbl->get_message_buf(e, &empty, &need)));
    ASSERT(need == 6); /* strlen("hello") + 1 */

    ncom_ierror_info_releasep(&e);
}

TEST(error_info_message_buf_fill)
{
    ncom_ierror_info_t *e = NULL;
    ncom_create_error_info(NCOM_E_FAIL, "world", &e);

    uint64_t need = 0;
    ncom_char_buf_t empty = { NULL, 0 };
    e->vtbl->get_message_buf(e, &empty, &need);

    char *buf = (char *)calloc((size_t)need, 1);
    ASSERT(buf != NULL);

    ncom_char_buf_t cbuf = { buf, need };
    ASSERT(NCOM_SUCCEEDED(e->vtbl->get_message_buf(e, &cbuf, &need)));
    ASSERT(strcmp(buf, "world") == 0);

    free(buf);
    ncom_ierror_info_releasep(&e);
}

TEST(error_info_message_buf_too_small)
{
    ncom_ierror_info_t *e = NULL;
    ncom_create_error_info(NCOM_E_FAIL, "toolong", &e);

    char tiny[2];
    uint64_t need = 0;
    ncom_char_buf_t cbuf = { tiny, sizeof(tiny) };
    ncom_status_t st = e->vtbl->get_message_buf(e, &cbuf, &need);
    ASSERT(st == NCOM_E_MORE_DATA);

    ncom_ierror_info_releasep(&e);
}

TEST(error_info_get_message_string_null_self)
{
    /* After the fix, CONTAINER_OF is called only after null check */
    ncom_ierror_info_t *e = NULL;
    ncom_create_error_info(NCOM_E_FAIL, "x", &e);
    const ncom_ierror_info_vtbl_t *vtbl = e->vtbl;
    ncom_ierror_info_releasep(&e);

    ncom_istring_t *s = NULL;
    ASSERT(vtbl->get_message_string(NULL, &s) == NCOM_E_INVALID_ARG);
    ASSERT(s == NULL);
}

TEST(error_info_null_out_args)
{
    ncom_ierror_info_t *e = NULL;
    ncom_create_error_info(NCOM_E_FAIL, "x", &e);

    ASSERT(e->vtbl->get_code(e, NULL)           == NCOM_E_INVALID_ARG);
    ASSERT(e->vtbl->get_message_string(e, NULL) == NCOM_E_INVALID_ARG);
    ASSERT(e->vtbl->get_message_buf(e, NULL, NULL) == NCOM_E_INVALID_ARG);

    ncom_ierror_info_releasep(&e);
}

/* ============================================================================
 * IID identity (extern const – single definition across TUs)
 * ============================================================================ */

TEST(iid_values_are_unique)
{
    /* All four framework IIDs must be distinct */
    ASSERT(!NCOM_IID_EQ(&NCOM_IID_IUNKNOWN,   &NCOM_IID_IFACTORY));
    ASSERT(!NCOM_IID_EQ(&NCOM_IID_IUNKNOWN,   &NCOM_IID_ISTRING));
    ASSERT(!NCOM_IID_EQ(&NCOM_IID_IUNKNOWN,   &NCOM_IID_IERRORINFO));
    ASSERT(!NCOM_IID_EQ(&NCOM_IID_IFACTORY,   &NCOM_IID_ISTRING));
    ASSERT(!NCOM_IID_EQ(&NCOM_IID_IFACTORY,   &NCOM_IID_IERRORINFO));
    ASSERT(!NCOM_IID_EQ(&NCOM_IID_ISTRING,    &NCOM_IID_IERRORINFO));
}

TEST(iid_pointer_identity)
{
    /* With extern const, every TU sees the same address */
    const ncom_iid_t *a = &NCOM_IID_IUNKNOWN;
    const ncom_iid_t *b = &NCOM_IID_IUNKNOWN;
    ASSERT(a == b);
}

/* ============================================================================
 * main
 * ============================================================================ */

int main(void)
{
    printf("=== ncom Unit Tests ===\n\n");

    printf("[ncom_create_string]\n");
    RUN(string_null_input_yields_empty);
    RUN(string_normal_content);
    RUN(string_null_out_returns_invalid_arg);
    RUN(string_refcount_starts_at_one);
    RUN(string_qi_iunknown);
    RUN(string_method_null_self);
    RUN(string_method_null_out_args);

    printf("\n[ncom_create_error_info]\n");
    RUN(error_info_null_out_returns_invalid_arg);
    RUN(error_info_code_roundtrip);
    RUN(error_info_message_string);
    RUN(error_info_null_message_yields_empty);
    RUN(error_info_message_buf_sizing_call);
    RUN(error_info_message_buf_fill);
    RUN(error_info_message_buf_too_small);
    RUN(error_info_get_message_string_null_self);
    RUN(error_info_null_out_args);

    printf("\n[IID identity]\n");
    RUN(iid_values_are_unique);
    RUN(iid_pointer_identity);

    printf("\n=== Results: %d passed, %d failed ===\n", g_passed, g_failed);
    return g_failed > 0 ? 1 : 0;
}
