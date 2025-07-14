/*
 * XWayback is a wrapper around wayback-compositor
 * and XWayland, it launches both in succession and
 * functions similiar to a traditional X server.
 *
 * SPDX-License-Identifier: MIT
 */

#include "wayback_log.h"
#include "xdg-output-unstable-v1-client-protocol.h"

#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wayland-client.h>

struct xwayback
{
	struct wl_display *display;

	char *wayland_socket;
	char *X_display;

	struct zxdg_output_manager_v1 *xdg_output_manager;

	struct xway_output *first_output;
	struct wl_list outputs;
};

struct xway_output
{
	struct wl_list link;
	struct wl_output *output;
	struct zxdg_output_v1 *xdg_output;
	struct xwayback *xwayback;
	uint32_t wl_name;

	char *name;
	char *description;

	char *make;
	char *model;

	uint32_t height, width, x, y;
	int32_t physical_height, physical_width;
	int32_t subpixel;
	int32_t transform;
	int32_t scale;
	float refresh;
};

static pid_t comp_pid;
static pid_t xway_pid;

static void output_geometry(void *data,
                            struct wl_output *wl_output,
                            int32_t x,
                            int32_t y,
                            int32_t width_mm,
                            int32_t height_mm,
                            int32_t subpixel,
                            const char *make,
                            const char *model,
                            int32_t transform)
{
	struct xway_output *output = data;

	free(output->make);
	free(output->model);

	output->physical_height = height_mm;
	output->physical_width = width_mm;
	output->make = make != NULL ? strdup(make) : NULL;
	output->model = model != NULL ? strdup(model) : NULL;
	output->subpixel = subpixel;
	output->transform = transform;
	return;
}

static void output_mode(void *data,
                        struct wl_output *wl_output,
                        uint32_t flags,
                        int32_t width,
                        int32_t height,
                        int32_t refresh)
{
	struct xway_output *output = data;
	output->refresh = (float)refresh / 1000;
	output->width = width;
	output->height = height;
	return;
}

static void output_done(void *data, struct wl_output *wl_output)
{
	// No extra output processing needed
	return;
}

static void output_scale(void *data, struct wl_output *wl_output, int32_t factor)
{
	struct xway_output *output = data;
	output->scale = factor;
	return;
}

static const struct wl_output_listener output_listener = {
	.geometry = output_geometry,
	.mode = output_mode,
	.done = output_done,
	.scale = output_scale,
};

static void xdg_output_handle_logical_position(void *data,
                                               struct zxdg_output_v1 *xdg_output,
                                               int32_t x,
                                               int32_t y)
{
	struct xway_output *output = data;
	output->x = x;
	output->y = y;
}

static void xdg_output_handle_logical_size(void *data,
                                           struct zxdg_output_v1 *xdg_output,
                                           int32_t width,
                                           int32_t height)
{
	struct xway_output *output = data;
	output->height = height;
	output->width = width;
}

static void xdg_output_handle_done(void *data, struct zxdg_output_v1 *xdg_output)
{
	// No extra processing is currently required.
	return;
}

static void xdg_output_handle_name(void *data, struct zxdg_output_v1 *xdg_output, const char *name)
{
	struct xway_output *output = data;
	free(output->name);
	output->name = strdup(name);
}

static void xdg_output_handle_description(void *data,
                                          struct zxdg_output_v1 *xdg_output,
                                          const char *description)
{
	struct xway_output *output = data;
	free(output->description);
	output->description = strdup(description);
}

static const struct zxdg_output_v1_listener xdg_output_listener = {
	.logical_position = xdg_output_handle_logical_position,
	.logical_size = xdg_output_handle_logical_size,
	.done = xdg_output_handle_done,
	.name = xdg_output_handle_name,
	.description = xdg_output_handle_description,
};

static void add_xdg_output(struct xway_output *output)
{
	if (output->xdg_output != NULL) {
		return;
	}

	output->xdg_output =
		zxdg_output_manager_v1_get_xdg_output(output->xwayback->xdg_output_manager, output->output);
	zxdg_output_v1_add_listener(output->xdg_output, &xdg_output_listener, output);
}

static void handle_global(void *data,
                          struct wl_registry *registry,
                          uint32_t name,
                          const char *interface,
                          uint32_t version)
{
	struct xwayback *xwayback = data;
	if (strcmp(interface, wl_output_interface.name) == 0) {
		struct xway_output *output = calloc(1, sizeof(struct xway_output));
		output->xwayback = xwayback;
		output->output = wl_registry_bind(registry, name, &wl_output_interface, 3);
		wl_output_add_listener(output->output, &output_listener, output);
		output->wl_name = name;
		wl_list_init(&output->link);
		wl_list_insert(&xwayback->outputs, &output->link);

		if (xwayback->first_output == NULL)
			xwayback->first_output = output;
		if (xwayback->xdg_output_manager != NULL)
			add_xdg_output(output);
	} else if (strcmp(interface, zxdg_output_manager_v1_interface.name) == 0) {
		xwayback->xdg_output_manager =
			wl_registry_bind(registry, name, &zxdg_output_manager_v1_interface, 2);
	}
}

static const struct wl_registry_listener registry_listener = {
	.global = handle_global,
	.global_remove = NULL, // TODO: handle_global_remove
};

static void handle_child_exit(int sig)
{
	pid_t pid = waitpid(-1, NULL, WNOHANG);
	if (pid == comp_pid || pid == xway_pid) {
		if (pid == comp_pid && xway_pid > 0)
			kill(xway_pid, SIGTERM);
		if (pid == xway_pid && comp_pid > 0)
			kill(comp_pid, SIGTERM);
		exit(EXIT_SUCCESS);
	}
}

static void handle_exit(int sig)
{
	kill(xway_pid, SIGTERM);
	kill(comp_pid, SIGTERM);
}

static void handle_segv(int sig)
{
	const char *errormsg =
		"[ERROR] (Xwayback): Received SIGSEGV (Segmentation fault)!\n"
		"This is a bug!\nPlease visit https://gitlab.freedesktop.org/wayback/wayback/-/issues/ to "
		"check\nif this bug has already been reported.  If not, fill a new bug report with "
		"steps\nto reproduce this error.  If you need assistance, join #wayback on Libera.Chat\nor "
		"#wayback:catircservices.org on Matrix.\n";
	write(STDERR_FILENO, errormsg, strlen(errormsg));
	handle_exit(sig);
}

char **merge_args(char **args1, int args1_len, char **args2, int args2_len)
{
	char **result = malloc(sizeof(char *) * (args1_len + args2_len + 1));
	if (!result)
		return NULL;

	for (int i = 0; i < args1_len; i++) {
		result[i] = args1[i];
	}

	for (int i = 0; i < args2_len; i++) {
		result[args1_len + i] = args2[i];
	}

	result[args1_len + args2_len] = NULL;
	return result;
}

static void usage(char *binname)
{
	fprintf(stderr, "usage: %s [:display] [option]\n", binname);
	fprintf(
		stderr,
		"-displayfd fd		file descriptor to write display number to when ready to connect\n");
}

int main(int argc, char *argv[])
{
	struct xwayback *xwayback = malloc(sizeof(struct xwayback));
	int socket_xwayback[2];
	int socket_xwayland[2];
	const char *x_display = ":0";
	char *displayfd = NULL;

	signal(SIGCHLD, handle_child_exit);
	signal(SIGSEGV, handle_segv);
	signal(SIGTERM, handle_exit);

	wayback_log_init("Xwayback", LOG_INFO, NULL);

	// TODO: Implement all options from Xserver(1) and Xorg(1)
	int j = 0;
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == ':') {
			x_display = argv[i];
		} else if (strcmp(argv[i], "-displayfd") == 0) {
			if (++i < argc) {
				displayfd = argv[i];
			}
		} else if (strcmp(argv[i], "-help") == 0) {
			usage(argv[0]);
			exit(EXIT_SUCCESS);
		} else {
			argv[j++] = argv[i];
		}
	}
	argc = j;

	// displayfd takes priority
	// TODO: Check if this is also the case in Xserver(7)
	if (displayfd != NULL) {
		wayback_log(LOG_INFO, "Using displayfd");
		x_display = NULL;
	}

	// check if the compositor/Xwayland binaries are accessible before doing anything else
	char *wayback_compositor_path = getenv("WAYBACK_COMPOSITOR_PATH");
	char *xwayland_path = getenv("XWAYLAND_PATH");
	if (wayback_compositor_path == NULL)
		wayback_compositor_path = WAYBACK_COMPOSITOR_EXEC_PATH;
	if (xwayland_path == NULL)
		xwayland_path = XWAYLAND_EXEC_PATH;

	if (access(wayback_compositor_path, X_OK) == -1) {
		wayback_log(LOG_ERROR,
		            "wayback-compositor executable %s not found or not executable",
		            wayback_compositor_path);
		exit(EXIT_FAILURE);
	}
	if (access(xwayland_path, X_OK) == -1) {
		wayback_log(LOG_ERROR, "Xwayland executable %s not found or not executable", xwayland_path);
		exit(EXIT_FAILURE);
	}

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, socket_xwayback) == -1) {
		wayback_log(LOG_ERROR, "Unable to create Xwayback socket");
		exit(EXIT_FAILURE);
	}

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, socket_xwayland) == -1) {
		wayback_log(LOG_ERROR, "Unable to create Xwayland socket");
		exit(EXIT_FAILURE);
	}

	comp_pid = fork();
	if (comp_pid == 0) {
		char fd_xwayback[64];
		char fd_xwayland[64];
		snprintf(fd_xwayback, sizeof(fd_xwayback), "%d", socket_xwayback[0]);
		snprintf(fd_xwayland, sizeof(fd_xwayland), "%d", socket_xwayland[0]);

		wayback_log(
			LOG_INFO, "Passed descriptor Xwayback: %s; Xwayland %s", fd_xwayback, fd_xwayland);
		close(socket_xwayback[1]);
		close(socket_xwayland[1]);
		execlp(wayback_compositor_path,
		       wayback_compositor_path,
		       fd_xwayback,
		       fd_xwayland,
		       (void *)NULL);
		wayback_log(LOG_ERROR, "Failed to launch wayback-compositor: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	close(socket_xwayback[0]);
	close(socket_xwayland[0]);

	if (unsetenv("WAYLAND_DISPLAY") == -1)
		// not the end of the world if it couldn't be unset
		wayback_log(LOG_WARN, "Could not unset WAYLAND_DISPLAY");

	char way_display[1024];
	snprintf(way_display, sizeof(way_display), "%d", socket_xwayback[1]);
	setenv("WAYLAND_SOCKET", way_display, true);

	if (x_display)
		xwayback->X_display = strdup(x_display);

	xwayback->display = wl_display_connect(NULL);
	if (!xwayback->display) {
		wayback_log(LOG_ERROR, "Unable to connect to wayback-compositor");
		exit(EXIT_FAILURE);
	}

	wl_list_init(&xwayback->outputs);
	xwayback->first_output = NULL;
	struct wl_registry *registry = wl_display_get_registry(xwayback->display);
	wl_registry_add_listener(registry, &registry_listener, xwayback);
	wl_display_roundtrip(xwayback->display);
	wl_display_roundtrip(xwayback->display); // xdg-output requires two roundtrips

	char *output = getenv("WAYBACK_OUTPUT");
	if (output != NULL) {
		struct xway_output *out;
		wl_list_for_each(out, &xwayback->outputs, link) {
			char *output_make_model;
			asprintf(&output_make_model, "%s %s", out->make, out->model);
			if (strcmp(output_make_model, output) == 0 || strcmp(out->make, output) == 0)
				xwayback->first_output = out;
		}
	}

	if (xwayback->first_output == NULL) {
		wayback_log(LOG_ERROR, "Unable to get outputs");
		exit(EXIT_FAILURE);
	}

	xway_pid = fork();
	if (xway_pid == 0) {
		char way_display[64];
		snprintf(way_display, sizeof(way_display), "%d", socket_xwayland[1]);
		setenv("WAYLAND_SOCKET", way_display, true);

		char geometry[4096];
		snprintf(geometry,
		         sizeof(geometry),
		         "%dx%d",
		         xwayback->first_output->width,
		         xwayback->first_output->height);

		if (x_display) {
			char *base_args[] = { xwayland_path, x_display,   "-fullscreen",
				                  "-terminate",  "-geometry", geometry };
			int base_argc = 6;
			char **xwayland_argv = merge_args(base_args, base_argc, argv, argc);
			execv(xwayland_path, xwayland_argv);
		} else {
			char *base_args[] = { xwayland_path, "-displayfd", displayfd,   "-fullscreen",
				                  "-retro",      "-terminate", "-geometry", geometry };
			int base_argc = 8;
			char **xwayland_argv = merge_args(base_args, base_argc, argv, argc);
			execv(xwayland_path, xwayland_argv);
		}
		wayback_log(LOG_ERROR, "Failed to launch Xwayland");
		exit(EXIT_FAILURE);
	}

	while (1)
		pause();

	return 0;
}
