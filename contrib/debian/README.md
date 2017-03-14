
Debian
====================
This directory contains files used to package quantumd/quantum-qt
for Debian-based Linux systems. If you compile quantumd/quantum-qt yourself, there are some useful files here.

## quantum: URI support ##


quantum-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install quantum-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your quantum-qt binary to `/usr/bin`
and the `../../share/pixmaps/quantum128.png` to `/usr/share/pixmaps`

quantum-qt.protocol (KDE)

