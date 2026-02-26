#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "nano_style.h"
#include "plugin_loader.h"
#include "helpers.h"
#include "nano_ids.h"
#include "i_error_info.h"
#include "i_string.h"
#include "i_clock2.h"

/* Tiny test helpers (no external framework). */
static int g_failed = 0;
static int g_argc = 0;
static char **g_argv = NULL;

#define TEST_CASE(NAME) static void NAME(void)

#define ASSERT_TRUE(COND) do { \
    if (!(COND)) { \
        fprintf(stderr, "ASSERT_TRUE failed at %s:%d: %s\n", __FILE__, __LINE__, #COND); \
        g_failed = 1; \
        return; \
    } \
} while (0)

#define ASSERT_EQ_I32(A,B) do { \
    int32_t _a = (int32_t)(A); \
    int32_t _b = (int32_t)(B); \
    if (_a != _b) { \
        fprintf(stderr, "ASSERT_EQ_I32 failed at %s:%d: %s=%d %s=%d\n", __FILE__, __LINE__, #A, (int)_a, #B, (int)_b); \
        g_failed = 1; \
        return; \
    } \
} while (0)

#define ASSERT_NOT_NULL(P) ASSERT_TRUE((P) != NULL)

static void default_plugin_path(char *buf, size_t cap)
{
#ifdef _WIN32
    snprintf(buf, cap, "sample_plugin.dll");
#elif __APPLE__
    snprintf(buf, cap, "libsample_plugin.dylib");
#else
    snprintf(buf, cap, "libsample_plugin.so");
#endif
}

static const char *get_plugin_path(int argc, char **argv, char *tmp, size_t cap)
{
    if (argc >= 2 && argv[1] && argv[1][0]) return argv[1];
    default_plugin_path(tmp, cap);
    return tmp;
}

TEST_CASE(test_plugin_load_and_qi)
{
    status_t st = STATUS_OK;
    plugin_handle_t *ph = NULL;
    const plugin_api_v1_t *api = NULL;

    i_unknown_t *u = NULL;
    i_logger_t *log = NULL;
    i_clock_t *clk = NULL;
    i_clock2_t *clk2 = NULL;

    char path[512];
    const char *p = get_plugin_path(g_argc, g_argv, path, sizeof(path));

    CHECK(plugin_load(p, &ph, &api));
    ASSERT_NOT_NULL(api);

    CHECK(api->create_instance(CLSID_SAMPLE_COMPONENT, IID_I_UNKNOWN, (void **)&u));
    ASSERT_NOT_NULL(u);

    CHECK(qi_logger(u, &log));
    CHECK(qi_clock(u, &clk));
    CHECK(qi_clock2(u, &clk2));

    /* Smoke calls */
    log->vtbl->log(log, 1, "test_plugin_load_and_qi");

    int64_t now_ms = 0;
    CHECK(clk->vtbl->now_unix_ms(clk, &now_ms));
    ASSERT_TRUE(now_ms > 0);

cleanup:
    i_clock2_releasep(&clk2);
    i_clock_releasep(&clk);
    i_logger_releasep(&log);
    i_unknown_releasep(&u);
    plugin_unload(ph);
    ASSERT_EQ_I32(st, STATUS_OK);
}

TEST_CASE(test_error_info_and_string_patterns)
{
    status_t st = STATUS_OK;
    plugin_handle_t *ph = NULL;
    const plugin_api_v1_t *api = NULL;

    i_unknown_t *u = NULL;
    i_clock2_t *clk2 = NULL;
    i_error_info_t *err = NULL;
    i_string_t *s = NULL;

    char path[512];
    const char *p = get_plugin_path(g_argc, g_argv, path, sizeof(path));

    CHECK(plugin_load(p, &ph, &api));
    CHECK(api->create_instance(CLSID_SAMPLE_COMPONENT, IID_I_UNKNOWN, (void **)&u));
    CHECK(qi_clock2(u, &clk2));

    /* Force an error: pass NULL out_ms. */
    st = clk2->vtbl->now_unix_ms(clk2, NULL, &err);
    ASSERT_EQ_I32(st, STATUS_E_INVALID_ARG);
    ASSERT_NOT_NULL(err);

    /* cstr */
    const char *cmsg = NULL;
    CHECK(err->vtbl->get_message_cstr(err, &cmsg));
    ASSERT_TRUE(cmsg != NULL && strlen(cmsg) > 0);

    /* i_string */
    CHECK(err->vtbl->get_message_string(err, &s));
    ASSERT_NOT_NULL(s);
    const char *sptr = NULL;
    CHECK(s->vtbl->c_str(s, &sptr));
    ASSERT_TRUE(sptr != NULL && strlen(sptr) > 0);

    /* caller-buffer sizing call + copy */
    uint64_t need = 0;
    CHECK(err->vtbl->get_message_buf(err, NULL, 0, &need));
    ASSERT_TRUE(need >= 2 && need < 1024);

    char *buf = (char *)calloc((size_t)need, 1);
    ASSERT_NOT_NULL(buf);

    st = err->vtbl->get_message_buf(err, buf, need, &need);
    ASSERT_EQ_I32(st, STATUS_OK);
    ASSERT_TRUE(strlen(buf) > 0);

    free(buf);
    st = STATUS_OK;

cleanup:
    i_string_releasep(&s);
    i_error_info_releasep(&err);
    i_clock2_releasep(&clk2);
    i_unknown_releasep(&u);
    plugin_unload(ph);
    ASSERT_EQ_I32(st, STATUS_OK);
}

TEST_CASE(test_refcount_basic)
{
    status_t st = STATUS_OK;
    plugin_handle_t *ph = NULL;
    const plugin_api_v1_t *api = NULL;

    i_logger_t *log = NULL;
    i_clock_t *clk = NULL;

    char path[512];
    const char *p = get_plugin_path(g_argc, g_argv, path, sizeof(path));

    CHECK(plugin_load(p, &ph, &api));

    /* Create as ILogger directly: initial refcount should be 1. */
    CHECK(api->create_instance(CLSID_SAMPLE_COMPONENT, IID_I_LOGGER, (void **)&log));
    ASSERT_NOT_NULL(log);

    uint32_t rc = log->vtbl->base.add_ref((i_unknown_t *)log);
    ASSERT_TRUE(rc >= 2);

    /* Query another view increments the same underlying refcount. */
    CHECK(log->vtbl->base.query_interface((i_unknown_t *)log, IID_I_CLOCK, (void **)&clk));
    ASSERT_NOT_NULL(clk);

    /* Release clock view, then release logger view twice (for add_ref + initial). */
    rc = clk->vtbl->base.release((i_unknown_t *)clk);
    ASSERT_TRUE(rc >= 1);

    rc = log->vtbl->base.release((i_unknown_t *)log); /* undo add_ref */
    ASSERT_TRUE(rc >= 1);

    rc = log->vtbl->base.release((i_unknown_t *)log); /* final release */
    (void)rc;
    log = NULL;

cleanup:
    i_clock_releasep(&clk);
    i_logger_releasep(&log);
    plugin_unload(ph);
    ASSERT_EQ_I32(st, STATUS_OK);
}

TEST_CASE(test_plugin_load_missing_file)
{
    status_t st = STATUS_OK;
    plugin_handle_t *ph = NULL;
    const plugin_api_v1_t *api = NULL;

#ifdef _WIN32
    st = plugin_load("this_file_does_not_exist_12345.dll", &ph, &api);
#else
    st = plugin_load("this_file_does_not_exist_12345.so", &ph, &api);
#endif
    ASSERT_TRUE(STATUS_FAILED(st));
    ASSERT_TRUE(ph == NULL);
    ASSERT_TRUE(api == NULL);
}

int main(int argc, char **argv)
{
    g_argc = argc;
    g_argv = argv;

    test_plugin_load_and_qi();
    test_error_info_and_string_patterns();
    test_refcount_basic();
    test_plugin_load_missing_file();

    if (g_failed) {
        fprintf(stderr, "nano_tests: FAILED\n");
        return 1;
    }
    fprintf(stderr, "nano_tests: OK\n");
    return 0;
}
