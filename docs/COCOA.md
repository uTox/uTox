# uTox-cocoa specific notes

* 10.10 SDK is required to build, but the product should be
  compatible back to 10.7.
  * Support for Mavericks SDK is TODO (ifdef out 10.10 bits)
  * Please make sure your dependencies are built with the lowest
    `MACOSX_DEPLOYMENT_TARGET` possible
  * The gameplan for 10.6 is to just replace AVFoundation with QTKit
    equivalents.
    * 32-bit support would be nice too (PowerPC is probably too much)
* Build an OS X .app package by using `make uTox.app`.
  * Some features will not work unless you run from a `.app`. These
    include desktop notifications and the dock icon (???).
  * An Xcode project file is TODO
* It is recommended to define `UTOX_COCOA_BRAVE` while building for
  release. It will disable some basic sanity checks which should
  always pass if the code is correct.
* You can instruct uTox to use Yosemite blur as a UI element colour
  in utox_theme.ini by (not implemented currently)
* When filing issues directly related to uTox-cocoa, please @stal888 in
  your issue so I get notified.

## How to compile (for yourself)

```bash
brew install --HEAD libtoxcore
make -f cocoa/Makefile uTox.app
```

Done!

## How to compile (for your friend)

```bash
brew install --HEAD libtoxcore
make uTox.dmg
```

Done! (the DMG target builds a statically linked utox binary 
for you automatically. It does not depend on anything but system
libraries. Build just the binary with `make -f cocoa/Makefile utox-static`)

## Adding to uTox-cocoa

* Please keep your C straight
