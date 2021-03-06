# qTorrent

## Overview

qTorrent is an open-source, cross-platform BitTorrent client currently being written in C++ with Qt.
The project is still at an early stage of development.

![Alt text](screenshots/screenshot.png?raw=true "qTorrent screenshot")

## Supported platforms

The goal for qTorrent is to be able to work on all major desktop operating systems - Windows, MacOS and Linux on both Xorg and Wayland.

## Installing

In the [releases](https://github.com/pgeorgiev98/qTorrent/releases) section of the project's github repository, you can find installers for Windows and Debian packages. If you're not using Microsoft Windows or a Debian-based operating system, you will have to compile the application (sorry about that).

## Compiling

To compile the application, you must have the Qt framework installed (Qt5.5 and later are the recommended versions) with QtGui and QtNetwork, Make and gcc or MinGW. The application compiles successfully with gcc-4.8 and MinGW-gcc-4.8.

You can build qTorrent with:

	# Create build directory (name and path don't matter)
	mkdir qTorrent-build
	cd qTorrent-build

	# To create a release build
	qmake /path/to/qTorrent/qTorrent.pro -config release

	# ... or debug build
	qmake /path/to/qTorrent/qTorrent.pro -config debug

	make -j4
	# The '-j4' means "Use up to 4 threads to build"
	# which speeds up things quite a bit
	# Ignore the argument or set it according to your CPU

	# To install the application (Works only on Linux)
	sudo make install

	# ... and to uninstall it (Works only on Linux)
	sudo make uninstall

	# Make sure you're using the propper (Qt5) version of qmake
	# On some systems that have both Qt4 and Qt5 installed you
	# may need to add the '-qt=5' argument to the qmake command
	# or you may need to call qmake-qt5 instead of just qmake.

## Usage

Just call

	qTorrent

Or start it via your application launcher or .exe file

## Current state

Currently, qTorrent:
* Can fully download torrents
* Can seed torrents
* Has a very basic GUI
* Can pause and resume torrents
* Supports Local Service Discovery (LSD)
* Does not support DHT, PEX, Magnet Links, UDP trackers or encryption

## Code style

qTorrent is written with the Linux Kernel code style with tab indentation.
The style is verified with 'astyle' with the following command:

	find -name '*.cpp' -or -name '*.h' | xargs astyle --style=linux --indent=force-tab=4
