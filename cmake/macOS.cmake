# These variables are needed to generate the Info.plist
set(EXECUTABLE_NAME utox)
set(APPLE_ICON utox.icns)
set(APPLE_MENU MainMenu)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.6)

# Xcode needs these to find libs
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

# Make apple icon
add_custom_command(OUTPUT ${APPLE_ICON}
    COMMAND iconutil --convert icns ${uTox_SOURCE_DIR}/src/cocoa/utox.iconset -o ${APPLE_ICON}
    DEPENDS ${uTox_SOURCE_DIR}/src/cocoa/utox.iconset
)

# Make apple menu
add_custom_command(OUTPUT ${APPLE_MENU}.nib
    COMMAND ibtool --errors --warnings --notices --output-format human-readable-text
        --compile ${APPLE_MENU}.nib ${uTox_SOURCE_DIR}/src/cocoa/MainMenu.xib
    DEPENDS ${uTox_SOURCE_DIR}/src/cocoa/MainMenu.xib
)

# Copy files
set_source_files_properties(${APPLE_ICON} ${APPLE_MENU}.nib PROPERTIES
   MACOSX_PACKAGE_LOCATION "Resources")

set(APPLE_FILES
    ${APPLE_ICON}
    ${APPLE_MENU}.nib
)

if(UTOX_STATIC OR TOXCORE_STATIC)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a .dylib)
endif()

# create DMG
set(CPACK_GENERATOR "DragNDrop")
set(CPACK_PACKAGE_FILE_NAME "uTox ${PROJECT_VERSION}")

