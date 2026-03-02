#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ncom/ncom.h>
#include <ncom/plugin_loader.h>

#include "demo.h"

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
    ncom_status_t st = NCOM_OK;

    ncom_plugin_handle_t *ph = NULL;
    const ncom_plugin_api_v1_t *api = NULL;

    ncom_iunknown_t *u = NULL;
    demo_ilogger_t *log = NULL;
    demo_iclock_t  *clk = NULL;
    demo_iclock2_t *clk2 = NULL;
    ncom_ierror_info_t *err = NULL;
    ncom_istring_t *err_str = NULL;

    char path[512];
    memset(path, 0, sizeof(path));
    if (argc >= 2) {
        snprintf(path, sizeof(path), "%s", argv[1]);
    } else {
        default_plugin_path(path, sizeof(path));
    }

    NCOM_CHECK(ncom_plugin_load(path, &ph, &api));

    /* Create component as IUnknown first, then query for specific interfaces. */
    NCOM_CHECK(api->create_instance(&DEMO_CLSID_SAMPLE_COMPONENT, &NCOM_IID_IUNKNOWN, (void **)&u));
    NCOM_CHECK(demo_ilogger_qi(u, &log));
    NCOM_CHECK(demo_iclock_qi(u, &clk));
    NCOM_CHECK(demo_iclock2_qi(u, &clk2));

    log->vtbl->log(log, 1, "Hello from host_app");

    int64_t now_ms = 0;
    /* Demonstrate rich error reporting: clock2 can return an error object. */
    st = clk2->vtbl->now_unix_ms(clk2, &now_ms, &err);
    if (NCOM_FAILED(st)) {
        const char *cmsg = NULL;
       

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
            ncom_char_buf_t buf;
            buf.cap = 0;
            (void)err->vtbl->get_message_buf(err, &buf, &need);
            if (need > 0 && need < 1024) {
                buf.ptr = (char *)calloc(need, 1);
                if (buf.ptr) {
                    (void)err->vtbl->get_message_buf(err, &buf, &need);
                    fprintf(stderr, "error message (caller buffer): %s\n", buf.ptr);
                    free(buf.ptr);
                }
            }
        }
        goto cleanup;
    }

    char msg[128];
    snprintf(msg, sizeof(msg), "clock.now_unix_ms = %lld", now_ms);
    log->vtbl->log(log, 1, msg);

cleanup:
    ncom_istring_releasep(&err_str);
    ncom_ierror_info_releasep(&err);
    demo_iclock2_releasep(&clk2);
    demo_iclock_releasep(&clk);
    demo_ilogger_releasep(&log);
    ncom_iunknown_releasep(&u);
    ncom_plugin_unload(ph);
    if (NCOM_FAILED(st)) {
        fprintf(stderr, "host_app failed: %d\n", st);
        return 1;
    }
    return 0;
}
