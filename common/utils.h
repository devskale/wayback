/*
 * Minor common utilities
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef UTILS_IMPORTED
#define UTILS_IMPORTED

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*x))

void asprintf_or_exit(char **restrict strp, const char *restrict fmt, ...);
char *strdup_or_exit(const char *s);

#endif
