/*
 * Minor common utilities
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef UTILS_IMPORTED
#define UTILS_IMPORTED

void asprintf_or_exit(char **restrict strp, const char *restrict fmt, ...);
char *strdup_or_exit(const char *s);

#endif
