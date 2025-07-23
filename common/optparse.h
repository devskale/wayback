/*
 * Option parsing
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef OPTPARSE_IMPORTED
#define OPTPARSE_IMPORTED
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct optcmd
{
	const char *name;
	const char *description;
	const bool req_operand;
	const bool ignore;
};

#define IGNORE_OPT(s, req_op)                                                                      \
	{ .name = s, .description = "", .req_operand = req_op, .ignore = true }

int optparse(int argc, char *argv[], const struct optcmd opts[], uint32_t optlen);

#endif
