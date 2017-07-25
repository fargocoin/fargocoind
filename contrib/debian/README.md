
Debian
====================
This directory contains files used to package bitcoind/fargocoin-qt
for Debian-based Linux systems. If you compile bitcoind/fargocoin-qt yourself, there are some useful files here.

## fargocoin: URI support ##


fargocoin-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install fargocoin-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your fargocoin-qt binary to `/usr/bin`
and the `../../share/pixmaps/bitcoin128.png` to `/usr/share/pixmaps`

fargocoin-qt.protocol (KDE)

