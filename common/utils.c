/*
 * Minor common utilities
 *
 * SPDX-License-Identifier: MIT
 */

#include "utils.h"

#include "wayback_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void asprintf_or_exit(char **restrict strp, const char *restrict fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int ret = vasprintf(strp, fmt, args);
	va_end(args);

	if (ret == -1) {
		wayback_log(LOG_ERROR, "Failed to allocate memory for string");
		exit(EXIT_FAILURE);
	}
}

char *strdup_or_exit(const char *s)
{
	char *result = strdup(s);
	if (!result) {
		wayback_log(LOG_ERROR, "Failed to allocate memory for string");
		exit(EXIT_FAILURE);
	}
	return result;
}
