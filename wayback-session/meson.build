executable(
	'wayback-session',
	['wayback-session.c', '../common/wayback_log.c'],
	dependencies: [wayland_server, wayland_client, wayland_cursor, wayland_egl, wayland_protos, wlroots, xkbcommon, server_protos],
	include_directories: common,
	install: true,
)
