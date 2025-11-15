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

enum optparse_flags
{
	OPT_OPERAND, /* option requires an operand, e.g. -opt thing */
	OPT_NUM,     /* option requires a max. 2 digit operand, e.g. vtXX */
	OPT_NOFLAG,  /* no operands at all, e.g. -opt */
};

struct optcmd
{
	const char *name;
	const char *description;
	const enum optparse_flags flag;
	const bool ignore;
};

#define IGNORE_OPT(s, f) { .name = s, .description = "", .flag = f, .ignore = true }

int optparse(int argc, char *argv[], const struct optcmd opts[], uint32_t optlen);

#endif
