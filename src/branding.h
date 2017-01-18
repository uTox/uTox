/**
 * uTox Versions and header information
 *
 * This file contains defines regarding uTox branding and version information
 * It is generated from branding.h.in which cmake will generate to branding.h
 */

#define TITLE "uTox"
#define SUB_TITLE "(Alpha)"

// The updater relies on these version numbers, and values greater than an octet were never tested
#define RELEASE_TITLE "GOLD MEMBERS"
#define VERSION "0.11.1"
#define VER_MAJOR 0
#define VER_MINOR 11
#define VER_PATCH 1
#define UTOX_VERSION_NUMBER (VER_MAJOR << 16 | VER_MINOR << 8 | VER_PATCH)

// Defaults
#define DEFAULT_NAME "uTox User"
#define DEFAULT_STATUS "Toxing on uTox, from the future!"
#define DEFAULT_SCALE 11
