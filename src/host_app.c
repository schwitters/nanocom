#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nano_style.h"
#include "plugin_loader.h"
#include "nano_base.h"



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

int main(int argc, char **argv)
{
    status_t st = STATUS_OK;

    plugin_handle_t *ph = NULL;
    const plugin_api_v1_t *api = NULL;

    i_unknown_t *u = NULL;
    i_logger_t *log = NULL;
    i_clock_t  *clk = NULL;
    i_clock2_t *clk2 = NULL;
    i_error_info_t *err = NULL;
    i_string_t *err_str = NULL;

    char path[512];
    memset(path, 0, sizeof(path));
    if (argc >= 2) {
        snprintf(path, sizeof(path), "%s", argv[1]);
    } else {
        default_plugin_path(path, sizeof(path));
    }

    CHECK(plugin_load(path, &ph, &api));

    /* Create component as IUnknown first, then query for specific interfaces. */
    CHECK(api->create_instance(CLSID_SAMPLE_COMPONENT, IID_I_UNKNOWN, (void **)&u));
    CHECK(qi_logger(u, &log));
    CHECK(qi_clock(u, &clk));
    CHECK(qi_clock2(u, &clk2));

    log->vtbl->log(log, 1, "Hello from host_app");

    int64_t now_ms = 0;
    /* Demonstrate rich error reporting: clock2 can return an error object. */
    st = clk2->vtbl->now_unix_ms(clk2, &now_ms, &err);
    if (STATUS_FAILED(st)) {
        const char *cmsg = NULL;
        if (err) (void)err->vtbl->get_message_cstr(err, &cmsg);
        fprintf(stderr, "clock2 failed: %d (%s)\n", (int)st, cmsg ? cmsg : "(no message)");

        /* Demonstrate i_string message retrieval. */
        if (err) {
            (void)err->vtbl->get_message_string(err, &err_str);
            if (err_str) {
                const char *s = NULL;
                (void)err_str->vtbl->c_str(err_str, &s);
                fprintf(stderr, "error message (i_string): %s\n", s ? s : "(null)");
            }
        }

        /* Demonstrate caller-provided buffer pattern (sizing call + copy). */
        if (err) {
            uint64_t need = 0;
            (void)err->vtbl->get_message_buf(err, NULL, 0, &need);
            if (need > 0 && need < 1024) {
                char *buf = (char *)calloc((size_t)need, 1);
                if (buf) {
                    (void)err->vtbl->get_message_buf(err, buf, need, &need);
                    fprintf(stderr, "error message (caller buffer): %s\n", buf);
                    free(buf);
                }
            }
        }
        goto cleanup;
    }

    char msg[128];
    snprintf(msg, sizeof(msg), "clock.now_unix_ms = %lld", (long long)now_ms);
    log->vtbl->log(log, 1, msg);

cleanup:
    i_string_releasep(&err_str);
    i_error_info_releasep(&err);
    i_clock2_releasep(&clk2);
    i_clock_releasep(&clk);
    i_logger_releasep(&log);
    i_unknown_releasep(&u);
    plugin_unload(ph);
    if (STATUS_FAILED(st)) {
        fprintf(stderr, "host_app failed: %d\n", (int)st);
        return 1;
    }
    return 0;
}
