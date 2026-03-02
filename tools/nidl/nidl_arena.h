#ifndef NCOM_TOOLS_NIDL_NIDL_ARENA_H
#define NCOM_TOOLS_NIDL_NIDL_ARENA_H

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
#include <stddef.h>

typedef struct arena_s arena_t;

arena_t *arena_create(void);
void arena_destroy(arena_t *a);

void *arena_alloc(arena_t *a, size_t n);
char *arena_strdup(arena_t *a, const char *s, size_t n);

#endif /* NCOM_TOOLS_NIDL_NIDL_ARENA_H */
