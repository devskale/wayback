# wayback

Wayback is a X11 compatibility layer which allows for running full X11
desktop environments using Wayland components.  It is essentially
a stub compositor which provides just enough Wayland capabilities to host
a rootful Xwayland server.

It is intended to eventually replace the classic X.Org server, thus reducing
maintenance burden of X11 applications, but a lot of work needs to be done first.

Wayback is an experimental state: expect breaking changes, and lots of
bugs.  Please submit pull requests fixing bugs instead of bug reports
if you are able.

## Installation

Dependencies:
- wayland (wayland-server, wayland-client, wayland-cursor, wayland-egl)
- wayland-protocol >=1.14
- xkbcommon
- wlroots-0.19
- xwayland >= 24.1

Building:
```
meson setup _build
cd _build
meson compile
```

Installing:
```
meson install
```

## Discussion

Matrix: [`#wayback:catircservices.org`](https://matrix.to/#/#wayback:catircservices.org)
IRC: irc.libera.chat #wayback

## Code of Conduct

As a project hosted on freedesktop.org, Wayback follows its [Code of Conduct][fdo-coc],
based on the Contributor Covenant. Please conduct yourself in a respectful
and civilized manner when using the above venues.

[fdo-coc]: https://www.freedesktop.org/wiki/CodeOfConduct
