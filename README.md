# uTox

Lightweight [Tox](https://github.com/irungentoo/ProjectTox-Core) client.

* Some things are incomplete and there may be some bugs: feel free to make issues/suggestions

* Xlib support is mostly complete, but may have some small bugs (no right click menus, lags)

## Screenshots

uTox running on Windows 8

![test](https://raw.github.com/notsecure/uTox/master/images/uTox-win32.png "uTox running on Windows 8")

uTox running on lubuntu:

![test](https://raw.github.com/notsecure/uTox/master/images/uTox-xlib.png "uTox running on lubuntu")


## Building

Something like this (these commands may sometimes be outdated, try the makefile or make an issue if they do not work):

Windows, using prebuilt toxav dll from Jenkins [32-bit](https://jenkins.libtoxcore.so/job/toxcore_win32_dll/) | [64-bit](https://jenkins.libtoxcore.so/job/toxcore_win64_dll/):

Note: building for Windows requires mingw-w64 (mingw lacks some header files), other compilers (not tested) may work with some tweaks

> windres icons/icon.rc -O coff -o icon.res

> gcc -o uTox.exe *.c ./png/png.c icon.res -lgdi32 -lmsimg32 -ldnsapi -lcomdlg32 -lopenal32 -lole32 -lstrmiids -loleaut32 -lvpx -ltoxav

Xlib:

> cc -o uTox.o *.c ./png/png.c -lX11 -lXft -lXrender -lXext -ltoxcore -ltoxav -ltoxdns -lopenal -pthread -lresolv -ldl -lm -lfontconfig -lv4lconvert -lvpx -I/usr/include/freetype2 -ldbus-1

or if you built toxcore statically:

> cc -o uTox.o *.c ./png/png.c -lX11 -lXft -lXrender -lXext -ltoxcore -ltoxav -ltoxdns -lopenal -lsodium -lopus -lvpx -lm -pthread -lresolv -ldl -lfontconfig -lv4lconvert -I/usr/include/freetype2 -ldbus-1

OS X (you need XQuartz on 10.8+, no video yet):

> cc -o uTox.o *.c -I/opt/X11/include -L/opt/X11/lib -lX11 -lXft -lXrender -lXext -ltoxcore -ltoxav -ltoxdns -framework OpenAL -pthread -lresolv -ldl -lm -lfontconfig -lvpx -I/opt/X11/include/freetype2

## Downloads

[Up to date download links on utox.org](http://utox.org)

## Info



