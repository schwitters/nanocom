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
 * @file nidl_arena.c
 * @brief Nidl arena.
 */

#include "nidl_arena.h"
#include <stdlib.h>
#include <string.h>

struct arena_s {
    unsigned char *buf;
    size_t cap;
    size_t len;
};

arena_t *arena_create(void)
{
    arena_t *a = (arena_t *)calloc(1, sizeof(*a));
    if (!a) return NULL;
    a->cap = 64 * 1024;
    a->buf = (unsigned char *)malloc(a->cap);
    if (!a->buf) { free(a); return NULL; }
    a->len = 0;
    return a;
}

void arena_destroy(arena_t *a)
{
    if (!a) return;
    free(a->buf);
    free(a);
}

static int arena_grow(arena_t *a, size_t need)
{
    size_t new_cap = a->cap;
    while (new_cap < need) new_cap *= 2;
    unsigned char *nb = (unsigned char *)realloc(a->buf, new_cap);
    if (!nb) return 0;
    a->buf = nb;
    a->cap = new_cap;
    return 1;
}

void *arena_alloc(arena_t *a, size_t n)
{
    if (!a || n == 0) return NULL;
    size_t aligned = (n + 7u) & ~7u;
    size_t need = a->len + aligned;
    if (need > a->cap) {
        if (!arena_grow(a, need)) return NULL;
    }
    void *p = a->buf + a->len;
    memset(p, 0, aligned);
    a->len += aligned;
    return p;
}

char *arena_strdup(arena_t *a, const char *s, size_t n)
{
    char *p = (char *)arena_alloc(a, n + 1);
    if (!p) return NULL;
    memcpy(p, s, n);
    p[n] = '\0';
    return p;
}
