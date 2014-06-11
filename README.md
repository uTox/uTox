# winTox

Lightweight [Tox](https://github.com/irungentoo/ProjectTox-Core) client.

* Some things are incomplete, but feel free to make any design suggestions (colors, fonts, whatever)

* Xlib support is experimental, many features may be missing

## Screenshots
![test](https://raw.github.com/notsecure/winTox/master/images/winTox.png "winTox early build")


## Building

Something like this:

Windows:

>windres icons/icon.rc -O coff -o icon.res

>gcc -o winTox.exe *.c icon.res -lgdi32 -lmsimg32 -ldnsapi -lcomdlg32 -lopenal32 -ltoxav

Xlib:

>gcc -o winTox.o *.c -lX11 -lXft -ltoxcore -ltoxav -lopenal -pthread -lresolv -ldl

or if you built toxcore statically:

>gcc -o winTox.o *.c -lX11 -lXft -ltoxcore -ltoxav -lopenal -lsodium -lopus lvpx -lm  -pthread -lresolv -ldl

## Downloads

[https://wiki.tox.im/Binaries](https://wiki.tox.im/Binaries)

## Info



