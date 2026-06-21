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
 * @file host_app.c
 * @brief Host app.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

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
    demo_ilogger2_t *log2 = NULL;
    demo_iclock_t  *clk = NULL;
    demo_iclock2_t *clk2 = NULL;
    demo_icapabilities_t *caps = NULL;
    demo_icomponent_factory_t *factory = NULL;
    ncom_iunknown_t *clock_from_factory_u = NULL;
    demo_iclock_t *clock_from_factory = NULL;
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
    NCOM_CHECK(demo_ilogger2_qi(u, &log2));
    NCOM_CHECK(demo_iclock_qi(u, &clk));
    NCOM_CHECK(demo_iclock2_qi(u, &clk2));
    NCOM_CHECK(demo_icapabilities_qi(u, &caps));

    log->vtbl->log(log, 1, "Hello from host_app");
    {
        const char msg2[] = "Hello from i_logger2::log_view";
        ncom_string_view_t v = { (const uint8_t *)msg2, (uint32_t)(sizeof(msg2) - 1u) };
        NCOM_CHECK(log2->vtbl->log_view(log2, 1, v));
    }

    {
        uint32_t api_major = 0;
        uint32_t api_minor = 0;
        uint32_t supports_logger2 = 0;
        uint32_t supports_clock2 = 0;
        uint32_t supports_capabilities = 0;
        uint32_t supports_factory = 0;
        NCOM_CHECK(caps->vtbl->get_api_version(caps, &api_major, &api_minor));
        NCOM_CHECK(caps->vtbl->get_component_capabilities(caps, &supports_logger2,
                                                          &supports_clock2,
                                                          &supports_capabilities,
                                                          &supports_factory));
        fprintf(stderr,
                "capabilities: api=%u.%u logger2=%u clock2=%u capabilities=%u factory=%u\n",
                (unsigned)api_major, (unsigned)api_minor,
                (unsigned)supports_logger2, (unsigned)supports_clock2,
                (unsigned)supports_capabilities, (unsigned)supports_factory);
    }

    int64_t now_ms = 0;
    /* Demonstrate rich error reporting: clock2 can return an error object. */
    st = clk2->vtbl->now_unix_ms(clk2, &now_ms, &err);
    if (NCOM_FAILED(st)) {
        ncom_status_t msg_st = NCOM_OK;

        /* Demonstrate i_string message retrieval. */
        if (err) {
            msg_st = err->vtbl->get_message_string(err, &err_str);
            if (NCOM_FAILED(msg_st)) {
                fprintf(stderr, "get_message_string failed: %d\n", (int)msg_st);
            } else if (err_str) {
                const char *s = NULL;
                msg_st = err_str->vtbl->c_str(err_str, &s);
                if (NCOM_FAILED(msg_st)) {
                    fprintf(stderr, "c_str failed: %d\n", (int)msg_st);
                } else {
                    fprintf(stderr, "error message (i_string): %s\n", s ? s : "(null)");
                }
            }
        }

        /* Demonstrate caller-provided buffer pattern (sizing call + copy). */
        if (err) {
            uint64_t need = 0;
            ncom_char_buf_t buf = {0};
            msg_st = err->vtbl->get_message_buf(err, &buf, &need);
            if (NCOM_FAILED(msg_st)) {
                fprintf(stderr, "get_message_buf sizing call failed: %d\n", (int)msg_st);
            } else if (need > 0 && need < 1024) {
                buf.ptr = (char *)calloc(need, 1);
                if (buf.ptr) {
                    msg_st = err->vtbl->get_message_buf(err, &buf, &need);
                    if (NCOM_FAILED(msg_st)) {
                        fprintf(stderr, "get_message_buf copy call failed: %d\n", (int)msg_st);
                    } else {
                        fprintf(stderr, "error message (caller buffer): %s\n", buf.ptr);
                    }
                    free(buf.ptr);
                }
            }
        }
        goto cleanup;
    }

    NCOM_CHECK(api->create_instance(&DEMO_CLSID_SAMPLE_COMPONENT_FACTORY,
                                    &DEMO_IID_ICOMPONENT_FACTORY,
                                    (void **)&factory));
    NCOM_CHECK(factory->vtbl->create_sample_component(factory, &DEMO_IID_ICLOCK,
                                                      &clock_from_factory_u, &err));
    NCOM_CHECK(demo_iclock_qi(clock_from_factory_u, &clock_from_factory));
    NCOM_CHECK(clock_from_factory->vtbl->now_unix_ms(clock_from_factory, &now_ms));

    char msg[128];
    snprintf(msg, sizeof(msg), "clock.now_unix_ms = %" PRId64, now_ms);
    log->vtbl->log(log, 1, msg);

cleanup:
    ncom_istring_releasep(&err_str);
    ncom_ierror_info_releasep(&err);
    demo_iclock_releasep(&clock_from_factory);
    ncom_iunknown_releasep(&clock_from_factory_u);
    demo_icomponent_factory_releasep(&factory);
    demo_icapabilities_releasep(&caps);
    demo_iclock2_releasep(&clk2);
    demo_iclock_releasep(&clk);
    demo_ilogger2_releasep(&log2);
    demo_ilogger_releasep(&log);
    ncom_iunknown_releasep(&u);
    ncom_plugin_unload(ph);
    if (NCOM_FAILED(st)) {
        fprintf(stderr, "host_app failed: %d\n", st);
        return 1;
    }
    return 0;
}
