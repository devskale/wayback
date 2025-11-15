/*
 * Option parsing
 *
 * SPDX-License-Identifier: MIT
 */

#include "optparse.h"

#include "wayback_log.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int optind = 0, optpos = 0, optoper = 0;

int optparse(int argc, char *argv[], const struct optcmd opts[], uint32_t optlen)
{
	optpos++;
	optpos += optoper;
	optoper = 0;
	if (optpos >= argc || !argv[optpos]) {
		return -1;
	}
	for (uint32_t i = 0; i < optlen; i++) {
		/* help message */
		if (strcmp(argv[optpos], "-help") == 0) {
			wayback_log(LOG_INFO,
			            "Wayback <https://wayback.freedesktop.org/> X.Org compatibility layer");
			wayback_log(
				LOG_INFO,
				"Report bugs to <https://gitlab.freedesktop.org/wayback/wayback/-/issues>.");
			wayback_log(LOG_INFO, "Usage: %s [option]", argv[0]);
			wayback_log(LOG_INFO, "\t-help\t\t show this help message");
			for (size_t j = 0; j < optlen; j++) {
				if (!opts[j].ignore) {
					wayback_log(LOG_INFO,
					            "\t%s%s\t\t %s",
					            opts[j].name,
					            opts[j].flag == OPT_OPERAND ? " opt" : "",
					            opts[j].description);
				}
			}
			exit(EXIT_SUCCESS);
		} else if (strcmp(argv[optpos], opts[i].name) == 0) {
			if (((opts[i].flag == OPT_OPERAND) && (((optpos + 1) >= argc || !argv[optpos + 1]))) ||
			    opts[i].flag == OPT_NUM) {
				wayback_log(LOG_ERROR, "Option %s requires operand", argv[optpos]);
				exit(EXIT_FAILURE);
			} else if (opts[i].ignore == true) {
				if (strcmp(opts[i].description, "") != 0)
					wayback_log(LOG_WARN, "%s: %s", argv[optpos], opts[i].description);
				else
					wayback_log(LOG_WARN, "Option %s ignored", argv[optpos]);
			}

			if (opts[i].flag == OPT_OPERAND) {
				optind++;
				optoper++;
			}
			optind++;
			break;
		} else if (opts[i].flag == OPT_NUM &&
		           strncmp(argv[optpos], opts[i].name, strlen(opts[i].name)) == 0) {
			if (opts[i].ignore == true) {
				if (strcmp(opts[i].description, "") != 0)
					wayback_log(LOG_WARN, "%s: %s", argv[optpos], opts[i].description);
				else
					wayback_log(LOG_WARN, "Option %s ignored", argv[optpos]);
			}
			optind++;
			break;
		}
	}
	return optpos;
}
