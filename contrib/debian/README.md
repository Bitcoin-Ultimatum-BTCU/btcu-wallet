
Debian
====================
This directory contains files used to package btcud/btcu-qt
for Debian-based Linux systems. If you compile btcud/btcu-qt yourself, there are some useful files here.

## btcu: URI support ##


btcu-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install btcu-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your btcu-qt binary to `/usr/bin`
and the `../../share/pixmaps/btcu128.png` to `/usr/share/pixmaps`

btcu-qt.protocol (KDE)

