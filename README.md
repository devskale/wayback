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
- C compiler supporting the C23 standard (e.g. gcc >= 14 or clang >= 18)
- meson >= 1.4.0
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

## Distribution packages

While Wayback is still alpha-quality software as of now there are packages in 
various popular distributions maintained by folks in the Wayback community.

***NOTE: These packages are NOT supported by Wayback developers, and as Wayback
development continues they may become out of date.  Wayback is still
alpha-quality software and development is rapidly moving, so before reporting any 
issues we ask you to replicate them on the git `main` branch to make sure they
are still applicable to the latest code.***

- Alpine Linux edge: [`wayback` and `wayback-doc`][alpine-pkg] in the `testing` 
  repository
- ALT Linux: [`wayback`][alt-pkg]
- Arch Linux: [`wayback-x11-git`][aur-pkg] in the AUR
- Fedora, RHEL: [`ngompa/wayback`][copr-pkg] Copr repository
- T2 SDE: [`wayback`][t2sde-pkg]

[alpine-pkg]: https://pkgs.alpinelinux.org/packages?name=wayback*&branch=edge&repo=&arch=&origin=&flagged=&maintainer=
[alt-pkg]:    https://packages.altlinux.org/en/sisyphus/srpms/wayback/
[aur-pkg]:    https://aur.archlinux.org/packages/wayback-x11-git
[copr-pkg]:   https://copr.fedorainfracloud.org/coprs/ngompa/wayback/
[t2sde-pkg]:  https://t2sde.org/packages/wayback

## Discussion

- Matrix: [`#wayback:catircservices.org`](https://matrix.to/#/#wayback:catircservices.org)
- IRC: irc.libera.chat `#wayback`

## Code of Conduct

As a project hosted on freedesktop.org, Wayback follows its [Code of Conduct][fdo-coc],
based on the Contributor Covenant.

Please conduct yourself in a respectful and civilized manner when using the above venues.

[fdo-coc]: https://www.freedesktop.org/wiki/CodeOfConduct
