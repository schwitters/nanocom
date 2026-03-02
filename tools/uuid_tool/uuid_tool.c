#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#if defined(_WIN32)
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <bcrypt.h>
  #pragma comment(lib, "bcrypt.lib")
#else
  #include <fcntl.h>
  #include <unistd.h>
#endif
static uint64_t get_time_ms(void) {
#ifdef _WIN32
    FILETIME ft;
    uint64_t time;
    // Holt die Zeit in 100-Nanosekunden-Intervallen seit 1. Jan 1601
    GetSystemTimeAsFileTime(&ft);
    time = (((uint64_t)ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    // Umrechnung in Unix-Epoche (1. Jan 1970) und Millisekunden
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

    // 4. Version setzen: 0111 (7) in die oberen 4 Bits von Byte 6
    out[6] = (uint8_t)((out[6] & 0x0F) | 0x70);

    // 5. Variante setzen: 10 (RFC 4122) in die oberen 2 Bits von Byte 8
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
        "usage: %s <count> [--prefix IID_] [--name I_UNKNOWN]\n"
        "\n"
        "For each UUID prints two lines:\n"
        "  [uuid(\"...\")]\n"
        "  static const iid_t   <PREFIX><NAME_OR_INDEX> = { 0x..ULL, 0x..ULL };\n",
        exe);
}

int main(int argc, char **argv)
{
    if (argc < 2) { usage(argv[0]); return 2; }

    char *end = NULL;
    long count = strtol(argv[1], &end, 10);
    if (!end || *end != '\0' || count <= 0 || count > 100000) {
        fprintf(stderr, "invalid count: %s\n", argv[1]);
        return 2;
    }

    const char *prefix = "IID_";
    const char *fixed_name = NULL;

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--prefix") == 0 && i + 1 < argc) {
            prefix = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "--name") == 0 && i + 1 < argc) {
            fixed_name = argv[++i];
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
        if (!uuid_v4(u)) {
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
