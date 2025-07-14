/*
 * wayback-session launches the wayback compositor, XWayback/XWayland
 * and the given session executable
 *
 * SPDX-License-Identifier: MIT
 */

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
		if (asprintf(&xinitrc, "%s/.xinitrc", home) == -1) {
			wayback_log(LOG_ERROR, "Unable to get xinitrc");
			exit(EXIT_FAILURE);
		}
		if (access(xinitrc, R_OK) == 0)
			return xinitrc;
		free(xinitrc);
	}

	if (access("/etc/X11/xinit/xinitrc", R_OK) == 0)
		return strdup("/etc/X11/xinit/xinitrc");
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
	signal(SIGCHLD, handle_child_exit);

	if (argc == 1) {
		xinitrc_path = get_xinitrc_path();
	} else {
		session_cmd = &argv[optind];
	}

	char *xwayback_path = getenv("XWAYBACK_PATH");
	if (xwayback_path != NULL && access(xwayback_path, X_OK) == -1) {
		wayback_log(LOG_ERROR, "Xwayback executable %s not found or not executable", xwayback_path);
		exit(EXIT_FAILURE);
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
		asprintf(&fd_str, "%d", fd[1]);
		execlp(xwayback_path, xwayback_path, "--displayfd", fd_str, (void *)NULL);
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
	asprintf(&x_display, ":%s", buffer);

	session_pid = fork();
	if (session_pid == 0) {
		setenv("XDG_SESSION_TYPE", "x11", true);
		setenv("DISPLAY", x_display, true);
		if (xinitrc_path != NULL) {
			execlp("sh", "sh", xinitrc_path, (void *)NULL);
		} else if (session_cmd != NULL) {
			execvp(session_cmd[0], session_cmd);
		}
		wayback_log(LOG_ERROR, "Failed to launch session");
		free(xinitrc_path);
		exit(EXIT_FAILURE);
	}

	while (1)
		pause();
	return 0;
}
