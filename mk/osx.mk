.PHONY: all

FILTER_AUDIO :=	0

CODE_SIGN_IDENTITY ?=	-

FRAMEWORKS :=	AppKit ApplicationServices CoreGraphics OpenAL Foundation \
		CoreText CoreFoundation AVFoundation CoreVideo CoreMedia \
		OpenGL QuartzCore

OS_CFLAGS :=	-xobjective-c
OS_LDFLAGS :=	-shared -pthread -lm -lresolv
OS_LDFLAGS +=	$(foreach framework,${FRAMEWORKS},-framework ${framework})

PSEUDOSTATIC :=	1

DIRs :=		cocoa

include ${SRCDIR}/mk/common.mk

all: utox

utox-MainMenu.nib:
	ibtool --compile $@ ${SRCDIR}/src/cocoa/MainMenu.xib

utox-Info.plist: ${SRCDIR}/src/cocoa/Info.plist
	cp $< $@
	touch $@
	/usr/libexec/PlistBuddy -c "Set :CFBundleIdentifier 'future.utox'" $@
	/usr/libexec/PlistBuddy -c "Set :CFBundleExecutable 'utox'" $@
	/usr/libexec/PlistBuddy -c "Set :LSMinimumSystemVersion '10.7'" $@
	/usr/libexec/PlistBuddy -c "Set :CFBundleName 'uTox'" $@
	/usr/libexec/PlistBuddy -c "Set :CFBundleIconFile 'uTox'" $@

utox.icns: ${SRCDIR}/src/cocoa/utox.iconset
	iconutil --convert icns $< -o ./$@

uTox.app: ${BIN} utox-Info.plist utox-MainMenu.nib utox.icns
	${INSTALL_DATA_DIR} uTox.app/Contents/MacOS
	${INSTALL_DATA_DIR} uTox.app/Contents/Resources
	${INSTALL_PROGRAM} ${BIN} uTox.app/Contents/MacOS/utox
	${INSTALL_DATA} utox-Info.plist uTox.app/Contents/Info.plist
	${INSTALL_DATA} utox.icns uTox.app/Contents/Resources/uTox.icns
	${INSTALL_DATA} utox-MainMenu.nib uTox.app/Contents/Resources/MainMenu.nib

uTox.dmg: ${STATICBIN} uTox.app
	mkdir -p dmg
	tar -C dmg -xjf ${SRCDIR}/src/cocoa/frozen_dmg.tar.bz2
	rm dmg/uTox/uTox.app
	cp -r uTox.app dmg/uTox
	cp -r ${STATICBIN} dmg/uTox/uTox.app/Contents/MacOS/utox
	chmod +x dmg/uTox/uTox.app/Contents/MacOS/utox
	codesign -s "${CODE_SIGN_IDENTITY}" -fv dmg/uTox/uTox.app/Contents/MacOS/utox
	touch dmg/uTox/.Trash
	hdiutil create -megabytes 32 -srcfolder dmg/uTox \
		-format UDBZ -nospotlight -noanyowners "uTox.dmg"
