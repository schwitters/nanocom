#include <stdio.h>
#include <stdlib.h>

#include "nano_style.h"
#include "helpers.h"
#include "plugin_loader.h"
#include "nano_ids.h"

/* Example 1: Use CHECK_SET_ERR with an interface method that returns i_error_info_t. */
static status_t example_clock2(plugin_api_v1_t const *api)
{
    status_t st = STATUS_OK;
    i_unknown_t *u = NULL;
    i_clock2_t *clk2 = NULL;
    i_error_info_t *err = NULL;

    CHECK_NULL(api);

    CHECK(api->create_instance(CLSID_SAMPLE_COMPONENT, IID_I_UNKNOWN, (void **)&u));
    CHECK(qi_clock2(u, &clk2));

    int64_t now_ms = 0;

    /* CHECK_SET_ERR releases previous err, NULLs it, then calls expression. */
    CHECK_SET_ERR(clk2->vtbl->now_unix_ms(clk2, &now_ms, &err), err);

    printf("example_clock2: now_ms=%lld\n", (long long)now_ms);

cleanup:
    if (STATUS_FAILED(st)) {
        const char *msg = NULL;
        if (err) (void)err->vtbl->get_message_cstr(err, &msg);
        fprintf(stderr, "example_clock2 failed: %d (%s)\n", (int)st, msg ? msg : "(no message)");
    }
    i_error_info_releasep(&err);
    i_clock2_releasep(&clk2);
    i_unknown_releasep(&u);
    return st;
}

/* Example 2: Use CHECK_ERR when you want to keep ERR untouched and handle it manually. */
static status_t example_manual_err(plugin_api_v1_t const *api)
{
    status_t st = STATUS_OK;
    i_unknown_t *u = NULL;
    i_clock2_t *clk2 = NULL;
    i_error_info_t *err = NULL;

    CHECK_NULL(api);
    CHECK(api->create_instance(CLSID_SAMPLE_COMPONENT, IID_I_UNKNOWN, (void **)&u));
    CHECK(qi_clock2(u, &clk2));

    int64_t now_ms = 0;
    err = NULL;
    st = clk2->vtbl->now_unix_ms(clk2, &now_ms, &err);

    /* Here we use CHECK_ERR on the status 'st' or just inline it: */
    CHECK_ERR(st, err);

    printf("example_manual_err: now_ms=%lld\n", (long long)now_ms);

cleanup:
    if (STATUS_FAILED(st)) {
        /* Show message via i_string as well. */
        i_string_t *s = NULL;
        const char *c = NULL;

        if (err) {
            (void)err->vtbl->get_message_string(err, &s);
            if (s) (void)s->vtbl->c_str(s, &c);
        }
        fprintf(stderr, "example_manual_err failed: %d (%s)\n", (int)st, c ? c : "(no message)");
        i_string_releasep(&s);
    }
    i_error_info_releasep(&err);
    i_clock2_releasep(&clk2);
    i_unknown_releasep(&u);
    return st;
}

int main(int argc, char **argv)
{
    status_t st = STATUS_OK;
    plugin_handle_t *ph = NULL;
    const plugin_api_v1_t *api = NULL;

    const char *path = NULL;
    if (argc >= 2) path = argv[1];
#ifdef _WIN32
    if (!path) path = "sample_plugin.dll";
#elif __APPLE__
    if (!path) path = "libsample_plugin.dylib";
#else
    if (!path) path = "libsample_plugin.so";
#endif

    CHECK(plugin_load(path, &ph, &api));

    st = example_clock2(api);
    if (STATUS_FAILED(st)) goto cleanup;

    st = example_manual_err(api);
    if (STATUS_FAILED(st)) goto cleanup;

cleanup:
    plugin_unload(ph);
    if (STATUS_FAILED(st)) return 1;
    return 0;
}
