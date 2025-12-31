/*
 * wayback-session launches the wayback compositor, XWayback/XWayland
 * and the given session executable
 *
 * SPDX-License-Identifier: MIT
 */

#include "optparse.h"
#include "utils.h"
#include "wayback_log.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

pid_t xwayback_pid;
pid_t session_pid;

char *get_xinitrc_path()
{
	char *home = getenv("HOME");
	if (home) {
		char *xinitrc;
		asprintf_or_exit(&xinitrc, "%s/.xinitrc", home);
		if (access(xinitrc, R_OK) == 0)
			return xinitrc;
		free(xinitrc);
	}

	if (access("/etc/X11/xinit/xinitrc", R_OK) == 0)
		return strdup_or_exit("/etc/X11/xinit/xinitrc");

	wayback_log(LOG_ERROR, "Unable to find xinitrc file");
	exit(EXIT_FAILURE);
}

void handle_child_exit(int sig)
{
	pid_t pid = waitpid(-1, NULL, WNOHANG);
	if (pid == session_pid || pid == xwayback_pid) {
		if (pid == session_pid && xwayback_pid > 0)
			kill(xwayback_pid, SIGTERM);
		if (pid == xwayback_pid && session_pid > 0)
			kill(session_pid, SIGTERM);
		exit(EXIT_SUCCESS);
	}
}

int main(int argc, char *argv[])
{
	wayback_log_init("wayback-session", LOG_INFO, NULL);

	char **session_cmd = NULL;
	char *xinitrc_path = NULL;
	const struct optcmd opts[] = {
		{ .name = "-sesscmd",
		  .description = "run custom session command",
		  .flag = OPT_OPERAND,
		  .ignore = false },
		{ .name = "-showconfig",
		  .description = "alias to -version",
		  .flag = OPT_NOFLAG,
		  .ignore = false },
		{ .name = "-version",
		  .description = "show wayback-session version",
		  .flag = OPT_NOFLAG,
		  .ignore = false },
	};
	signal(SIGCHLD, handle_child_exit);

	int cur_opt = 0;
	while (cur_opt = optparse(argc, argv, opts, ARRAY_SIZE(opts)), cur_opt != -1) {
		if (strcmp(argv[cur_opt], "-version") == 0 || strcmp(argv[cur_opt], "-showconfig") == 0) {
			wayback_log(LOG_INFO,
			            "Wayback <https://wayback.freedesktop.org/> X.Org compatibility layer");
			wayback_log(LOG_INFO, "Version %s", WAYBACK_VERSION);
			exit(EXIT_SUCCESS);
		} else if (strcmp(argv[cur_opt], "-sesscmd") == 0) {
			session_cmd = &argv[cur_opt + 1];
		} else {
			wayback_log(LOG_ERROR, "Unknown option %s", argv[cur_opt]);
			exit(EXIT_FAILURE);
		}
	}

	if ((argc - optind) <= 0) {
		wayback_log(LOG_ERROR, "Argument count is <= 0");
	}
	if (!session_cmd) {
		xinitrc_path = get_xinitrc_path();
	}

	char *xwayback_path = getenv("XWAYBACK_PATH");
	if (xwayback_path != NULL) {
		if (access(xwayback_path, X_OK) == -1) {
			wayback_log(
				LOG_ERROR, "Xwayback executable %s not found or not executable", xwayback_path);
			exit(EXIT_FAILURE);
		}
	} else {
		xwayback_path = "Xwayback";
	}

	int fd[2];
	if (pipe(fd) == -1) {
		wayback_log(LOG_ERROR, "Failed to create pipe");
		exit(EXIT_FAILURE);
	}

	xwayback_pid = fork();
	if (xwayback_pid == 0) {
		close(fd[0]);
		wayback_log(LOG_INFO, "Launching with fd %d", fd[1]);
		char *fd_str;
		asprintf_or_exit(&fd_str, "%d", fd[1]);
		execlp(xwayback_path, xwayback_path, "-displayfd", fd_str, (void *)NULL);
		wayback_log(LOG_ERROR, "Failed to launch Xwayback: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	char buffer[4095];
	close(fd[1]);
	ssize_t n = read(fd[0], buffer, sizeof(buffer) - 1);
	if (n > 0) {
		buffer[n - 1] = '\0'; // Convert from newline-terminated to null-terminated string
		wayback_log(LOG_INFO, "Received display %s", buffer);
	}

	char *x_display;
	asprintf_or_exit(&x_display, ":%s", buffer);

	session_pid = fork();
	if (session_pid == 0) {
		unsetenv("WAYLAND_DISPLAY");
		setenv("XDG_SESSION_TYPE", "x11", true);
		setenv("DISPLAY", x_display, true);
		if (xinitrc_path != NULL) {
			execlp("sh", "sh", xinitrc_path, (void *)NULL);
		} else if (session_cmd != NULL) {
			execvp(session_cmd[0], session_cmd);
		}
		wayback_log(LOG_ERROR, "Failed to launch session: %s", strerror(errno));
		free(xinitrc_path);
		exit(EXIT_FAILURE);
	}

	while (1)
		pause();
	return 0;
}
