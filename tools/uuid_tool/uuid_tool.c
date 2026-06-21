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
 * @file uuid_tool.c
 * @brief Uuid tool.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <bcrypt.h>
  #pragma comment(lib, "bcrypt.lib")
#else
  #include <sys/time.h> /* gettimeofday */
  #include <fcntl.h>
  #include <unistd.h>
#endif

#define NCOM_UUID_COUNT_ENV   "NCOM_UUID_COUNT"
#define NCOM_UUID_PREFIX_ENV  "NCOM_UUID_PREFIX"
#define NCOM_UUID_NAME_ENV    "NCOM_UUID_NAME"
#define NCOM_UUID_VERSION_ENV "NCOM_UUID_VERSION"

static int parse_positive_long(const char *s, long *out)
{
    char *end = NULL;
    long v = 0;

    if (!s || !out) {
        return 0;
    }

    errno = 0;
    v = strtol(s, &end, 10);
    if (errno != 0 || !end || *end != '\0' || v <= 0 || v > 100000) {
        return 0;
    }

    *out = v;
    return 1;
}

static int parse_uuid_version(const char *s, int *out_version)
{
    if (!s || !out_version) {
        return 0;
    }
    if (strcmp(s, "4") == 0) {
        *out_version = 4;
        return 1;
    }
    if (strcmp(s, "7") == 0) {
        *out_version = 7;
        return 1;
    }
    return 0;
}

static uint64_t get_time_ms(void)
{
#ifdef _WIN32
    FILETIME ft;
    uint64_t time;
    GetSystemTimeAsFileTime(&ft);
    time = (((uint64_t)ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    return (time - 116444736000000000ULL) / 10000;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
#endif
}
static int get_random_bytes(uint8_t *buf, size_t n)
{
#if defined(_WIN32)
    return BCryptGenRandom(NULL, (PUCHAR)buf, (ULONG)n, BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
#else
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) return 0;
    size_t off = 0;
    while (off < n) {
        ssize_t r = read(fd, buf + off, n - off);
        if (r <= 0) { close(fd); return 0; }
        off += (size_t)r;
    }
    close(fd);
    return 1;
#endif
}

static int uuid_v4(uint8_t out[16])
{
    if (!get_random_bytes(out, 16)) return 0;
    out[6] = (uint8_t)((out[6] & 0x0F) | 0x40);
    out[8] = (uint8_t)((out[8] & 0x3F) | 0x80);
    return 1;
}

static int uuid_v7(uint8_t out[16])
{
    if (!get_random_bytes(out, 16)) return 0;
    uint64_t ms = get_time_ms();
    out[0] = (uint8_t)(ms >> 40);
    out[1] = (uint8_t)(ms >> 32);
    out[2] = (uint8_t)(ms >> 24);
    out[3] = (uint8_t)(ms >> 16);
    out[4] = (uint8_t)(ms >> 8);
    out[5] = (uint8_t)ms;
    out[6] = (uint8_t)((out[6] & 0x0F) | 0x70);
    out[8] = (uint8_t)((out[8] & 0x3F) | 0x80);
    return 1;
}
static void uuid_to_string(const uint8_t u[16], char out[37])
{
    static const char *hex = "0123456789abcdef";
    int p = 0;
    for (int i = 0; i < 16; i++) {
        if (i==4 || i==6 || i==8 || i==10) out[p++] = '-';
        out[p++] = hex[(u[i] >> 4) & 0xF];
        out[p++] = hex[u[i] & 0xF];
    }
    out[p] = '\0';
}

static void uuid_to_u64_pair_be(const uint8_t u[16], uint64_t *hi, uint64_t *lo)
{
    uint64_t h = 0, l = 0;
    for (int i = 0; i < 8; i++) h = (h << 8) | u[i];
    for (int i = 8; i < 16; i++) l = (l << 8) | u[i];
    *hi = h;
    *lo = l;
}

static void usage(const char *exe)
{
    fprintf(stderr,
        "usage: %s [<count>] [--prefix IID_] [--name I_UNKNOWN] [--version 4|7]\n"
        "\n"
        "Environment variables:\n"
        "  %s     count fallback when <count> is omitted\n"
        "  %s    fallback for --prefix\n"
        "  %s      fallback for --name\n"
        "  %s   fallback for --version (4 or 7)\n"
        "\n"
        "For each UUID prints two lines:\n"
        "  [uuid(\"...\")]\n"
        "  static const ncom_iid_t <PREFIX><NAME_OR_INDEX> = { 0x..ULL, 0x..ULL };\n",
        exe,
        NCOM_UUID_COUNT_ENV,
        NCOM_UUID_PREFIX_ENV,
        NCOM_UUID_NAME_ENV,
        NCOM_UUID_VERSION_ENV);
}

int main(int argc, char **argv)
{
    int argi = 1;
    long count = 0;
    const char *prefix = "IID_";
    const char *fixed_name = NULL;
    int uuid_version = 4;

    if (argc >= 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        usage(argv[0]);
        return 0;
    }

    {
        const char *env_prefix = getenv(NCOM_UUID_PREFIX_ENV);
        const char *env_name = getenv(NCOM_UUID_NAME_ENV);
        const char *env_version = getenv(NCOM_UUID_VERSION_ENV);

        if (env_prefix && env_prefix[0] != '\0') {
            prefix = env_prefix;
        }
        if (env_name && env_name[0] != '\0') {
            fixed_name = env_name;
        }
        if (env_version && env_version[0] != '\0') {
            if (!parse_uuid_version(env_version, &uuid_version)) {
                fprintf(stderr, "invalid %s value: %s\n", NCOM_UUID_VERSION_ENV, env_version);
                return 2;
            }
        }
    }

    if (argi < argc && argv[argi][0] != '-') {
        if (!parse_positive_long(argv[argi], &count)) {
            fprintf(stderr, "invalid count: %s\n", argv[argi]);
            return 2;
        }
        argi++;
    } else {
        const char *env_count = getenv(NCOM_UUID_COUNT_ENV);
        if (!parse_positive_long(env_count, &count)) {
            fprintf(stderr, "missing or invalid count; pass <count> or set %s\n", NCOM_UUID_COUNT_ENV);
            usage(argv[0]);
            return 2;
        }
    }

    for (int i = argi; i < argc; i++) {
        if (strcmp(argv[i], "--prefix") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "--prefix requires a value\n");
                return 2;
            }
            prefix = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "--name") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "--name requires a value\n");
                return 2;
            }
            fixed_name = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "--version") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "--version requires a value (4 or 7)\n");
                return 2;
            }
            if (!parse_uuid_version(argv[++i], &uuid_version)) {
                fprintf(stderr, "invalid --version value: %s\n", argv[i]);
                return 2;
            }
            continue;
        }
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        }
        fprintf(stderr, "unknown argument: %s\n", argv[i]);
        return 2;
    }

    for (long i = 1; i <= count; i++) {
        uint8_t u[16];
        int ok = (uuid_version == 7) ? uuid_v7(u) : uuid_v4(u);
        if (!ok) {
            fprintf(stderr, "failed to generate random bytes\n");
            return 1;
        }

        char s[37];
        uuid_to_string(u, s);

        uint64_t hi = 0, lo = 0;
        uuid_to_u64_pair_be(u, &hi, &lo);

        printf("[uuid(\"%s\")]\n", s);

        if (fixed_name) {
            printf("static const ncom_iid_t   %s%s = { 0x%016llxULL, 0x%016llxULL };\n",
                   prefix, fixed_name,
                   (unsigned long long)hi, (unsigned long long)lo);
        } else {
            printf("static const ncom_iid_t   %s%04ld = { 0x%016llxULL, 0x%016llxULL };\n",
                   prefix, i,
                   (unsigned long long)hi, (unsigned long long)lo);
        }
    }
    return 0;
}
