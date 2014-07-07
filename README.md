# uTox

Lightweight [Tox](https://github.com/irungentoo/ProjectTox-Core) client.

* Some things are incomplete, but feel free to make any design suggestions (colors, fonts, whatever)

* Xlib support is experimental, many features may be missing

## Screenshots

uTox running on lubuntu:

![test](https://raw.github.com/notsecure/uTox/master/images/uTox.png "uTox running on lubuntu")


## Building

Something like this:

Windows:

> windres icons/icon.rc -O coff -o icon.res

> cc -o uTox.exe *.c icon.res -lgdi32 -lmsimg32 -ldnsapi -lcomdlg32 -lopenal32 -ltoxav

Xlib:

> cc -o uTox.o *.c -lX11 -lXft -lXrender -ltoxcore -ltoxav -ltoxdns -lopenal -pthread -lresolv -ldl -lm -lfontconfig -lv4lconvert -lvpx -I/usr/include/freetype2 -DV4L

or if you built toxcore statically:

> cc -o uTox.o *.c -lX11 -lXft -lXrender -ltoxcore -ltoxav -ltoxdns -lopenal -lsodium -lopus -lvpx -lm -pthread -lresolv -ldl -lfontconfig -lv4lconvert -DV4L -I/usr/include/freetype2

OS X (you need XQuartz on 10.8+, no video yet):

> cc -o uTox.o *.c -I/opt/X11/include -L/opt/X11/lib -lX11 -lXft -lXrender -lXext -ltoxcore -ltoxav -ltoxdns -framework OpenAL -pthread -lresolv -ldl -lm -lfontconfig -lvpx -I/opt/X11/include/freetype2




## Downloads

[https://wiki.tox.im/Binaries](https://wiki.tox.im/Binaries)

## Info



