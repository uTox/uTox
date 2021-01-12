/******************************************************************************
 *** English i18l Strings                                                   ***
 ******************************************************************************/
msgid(LANG_NATIVE_NAME)
msgstr("English")

msgid(LANG_ENGLISH_NAME)
msgstr("ENGLISH")


/******************************************************************************
 *** Splash Message & Change Log                                            ***
 ******************************************************************************/
msgid(SPLASH_TITLE)
msgstr("Welcome to the newer uTox!")

msgid(SPLASH_TEXT)
msgstr("You have just upgraded to version 0.18.0! Release name: 👑🎉")

msgid(CHANGE_LOG_TITLE)
msgstr("Changes in v0.18.0")

msgid(CHANGE_LOG_TEXT)
msgstr(
"\n"
"  Features:\n"
"    More stability, less code!\n"
"\n"
"  Fixes:\n"
"    **Fix crash when changing toxcore-related settings**\n"
"    fix dragging friend list (#1470) (Thanks, w23rd!)\n"
"    utox_settings struct: give proxy_ip[] a size (fix UB)\n"
"    Update and refactor settings loading and saving:\n"
"      - **Drop obsolete save and load settings format** (Thanks, redmanmale!)\n"
"      - Rename (and fix usages) SETTINGS fields to match UTOXSAVE struct to be able to load it from config directly (Thanks, redmanmale!)\n"
"      - Remove unused function config_osdefaults() (Thanks, redmanmale!)\n"
"      - Use settings instead of separate variables (Thanks, redmanmale!)\n"
"      - Refactoring save and load uTox settings (Thanks, redmanmale!)\n"
"      - Save settings before killing Tox threads (Thanks, redmanmale!)\n"
"      - Style fixes (Thanks, redmanmale!)\n"
"      - Update mock settings (Thanks, redmanmale!)\n"
"      - Try create Tox folder before saving settings (Thanks, redmanmale!)\n"
"      - settings.c: Write macros for write_config_value_*()\n"
"    getopt: Exit on wrong usage\n"
"    Fix offset of the typing notification (Thanks, redmanmale!)\n"
"    Fix file size formatting in chatlog (Thanks, redmanmale!)\n"
"    Fix handling large files (Thanks, redmanmale!)\n"
"\n"
"  Documentation:\n"
"    Various README updates:\n"
"      - README.md: replace link to changelog with release notes\n"
"      - README.md: Team: replace GitLab with GitHub links\n"
"      - README.md: offer direct email contact\n"
"      - README.md: rearrange Features\n"
"      - README.md: move Features before Downloads\n"
"      - README.md: move screenshots to separate page\n"
"      - README.md: reorder links in the header\n"
"      - README.md: add introduction text\n"
"      - README: Update my email address\n"
"    INSTALL: Fix table of contents; clean up a bit; update link for Windows (Thanks, redmanmale!)\n"
"    README: Update link to win64; drop obsolete or dead links to win32 and winXP (Thanks, redmanmale!)\n"
"    README: Drop frighteningly red coverage badge leading nowhere (Thanks, redmanmale!)\n"
"    README: aTox is love, aTox is life! (Thanks, grayhatter!)\n"
"    Document and simplify filter_audio_check()\n"
"    Reflect in --help text, that --debug requires an argument\n"
"    Add missing themes to --help text\n"
"    Various man page updates:\n"
"      - utox.1.in: Don't list every theme in synopsis\n"
"      - utox.1.in: Add --debug option\n"
"      - utox.1.in: Correct max verbosity increment from 3 to 6\n"
"      - utox.1.in: Remove reference to website utox.io\n"
"      - Move utox.1.in from src/ to man/\n"
"    README: Update fingerprint (Thanks, endoffile78!)\n"
"    CHANGELOG.md: redirect to release_notes/\n"
"    BUILD: Update build docs for x86 (Thanks, redmanmale!)\n"
"    README: Update Appveyor badge to point to official uTox project (Thanks, redmanmale!)\n"
"    BUILD: Update build docs for Windows (Thanks, redmanmale!)\n"
"\n"
"  Languages:\n"
"    Update Japanese (Thanks, xfm00mm!)\n"
"    Update German (Thanks, Markus V!)\n"
"\n"
"  Development:\n"
"    Minor settings changes\n"
"    **/CMakeLists.txt: remove header files from add_library() and add_executable()\n"
"    src/xlib/CMakeLists.txt: compile mmenu.{c,h} conditionally\n"
"    src/xlib/CMakeLists.txt: compile dbus.{c,h} conditionally\n"
"    Return the new groupchat pointer from group_create instead of a bool (Thanks, endoffile78!)\n"
"    CMake: Simplify regexes that match MacOS, Linux, NetBSD and FreeBSD (Thanks, mazocomp!)\n"
"    CI: Use Openal fork with fix for CMake > 3.12 (Thanks, redmanmale!)\n"
"    Update Travis CI config: remove obsolete parameters, update linux dist (Thanks, redmanmale!)\n"
"    Rename friend list functions\n"
"    Various Windows cleanups (Thanks, redmanmale!)\n"
"    Use the old stb commit (Thanks, robinlinden!)\n"
"    CMake: Mark targets as being C only (Thanks, robinlinden!)\n"
"    CI: Don't depend on cmdline.org artifacts on Travis (Thanks, robinlinden!)\n"
"    CMake: Fix cross-compilation from Linux to Windows (Thanks, robinlinden!)\n"
"    CI: Update Appveyor build (Thanks, redmanmale!)\n"
"    CMake: Give third-party libraries their own targets (Thanks, robinlinden!)\n"
"    Allow building Windows w/ -fno-common (Thanks, robinlinden!)\n"
"    Fix implicit fallthrough warnings in Windows (Thanks, robinlinden!)\n"
"\n")


/******************************************************************************
 *** MISC & UNSORTED                                                        ***
 ******************************************************************************/
msgid(REQ_SENT)
msgstr("Friend request sent. Your friend will appear online after the request is accepted.")

msgid(REQ_INVALID_ID)
msgstr("Error: Invalid Tox ID")

msgid(REQ_ADDED_NO_FREQ_SENT)
msgstr("Note: Friend added but friend request was not sent (nospam missing)")

msgid(REQ_EMPTY_ID)
msgstr("Error: No Tox ID specified")

msgid(REQ_LONG_MSG)
msgstr("Error: Message is too long")

msgid(REQ_NO_MSG)
msgstr("Error: Empty message")

msgid(REQ_SELF_ID)
msgstr("Error: Tox ID is self ID")

msgid(REQ_ALREADY_FRIENDS)
msgstr("Error: Tox ID is already in friend list")

msgid(REQ_UNKNOWN)
msgstr("Error: Unknown")

msgid(REQ_BAD_CHECKSUM)
msgstr("Error: Invalid Tox ID (bad checksum)")

msgid(REQ_BAD_NOSPAM)
msgstr("Error: Invalid Tox ID (bad nospam value)")

msgid(REQ_NO_MEMORY)
msgstr("Error: No memory")

msgid(SEND_FILE)
msgstr("Send File")

msgid(SAVE_FILE)
msgstr("Save File")

msgid(WHERE_TO_SAVE_FILE_PROMPT)
msgstr("Where do you want to save \"%.*s\"?")

msgid(WHERE_TO_SAVE_FILE)
msgstr("Where do you want to save the file?")

msgid(SEND_FILE_PROMPT)
msgstr("Select one or more files to send.")

msgid(SCREEN_CAPTURE_PROMPT)
msgstr("Drag a box around the area of the screen you want to send.")


/******************************************************************************
 *** File Transfer Strings                                                  ***
 ******************************************************************************/
msgid(TRANSFER_NEW)
msgstr("New file transfer")

msgid(TRANSFER_STARTED)
msgstr("File transfer started")

msgid(TRANSFER___)
msgstr("...")

msgid(TRANSFER_PAUSED)
msgstr("File transfer paused")

msgid(TRANSFER_BROKEN)
msgstr("File transfer broken")

msgid(TRANSFER_CANCELLED)
msgstr("File transfer canceled")

msgid(TRANSFER_COMPLETE)
msgstr("File transfer complete")


/******************************************************************************
 *** Keyboard and Mouse Cursor Strings                                      ***
 ******************************************************************************/
msgid(CURSOR_CLICK_LEFT)
msgstr("Left click")

msgid(CURSOR_CLICK_RIGHT)
msgstr("Right click")


/******************************************************************************
 *** Audio / Video Call Strings                                             ***
 ******************************************************************************/
msgid(CALL_START_AUDIO)
msgstr("Start call")

msgid(CALL_START_VIDEO)
msgstr("Start video call")

msgid(CALL_DECLINE)
msgstr("Decline the call")

msgid(CALL_CANCELLED)
msgstr("Call canceled")

msgid(CALL_INVITED)
msgstr("Call invited")

msgid(CALL_RINGING)
msgstr("Call ringing")

msgid(CALL_STARTED)
msgstr("Call started")

msgid(CALL_ENDED)
msgstr("Call ended")

msgid(CALL_FRIEND_HAD_ENDED_CALL)
msgstr(" has ended the call!")

msgid(CALL_VIDEO_SHOW_INLINE)
msgstr("Show video inline")


/******************************************************************************
 *** Friend & Friend Settings Strings                                       ***
 ******************************************************************************/
msgid(FRIEND_ALIAS)
msgstr("Set Friend Alias")

msgid(FRIEND_PUBLIC_KEY)
msgstr("Friend's Public Key")

msgid(FRIEND_AUTOACCEPT)
msgstr("Accept incoming file transfers without confirmation")

msgid(FRIEND_EXPORT_CHATLOG)
msgstr("Export Chatlog as plain text")


/******************************************************************************
 *** Group Strings                                                          ***
 ******************************************************************************/
msgid(GROUPCHAT_JOIN_AUDIO)
msgstr("Join audio chat")

msgid(GROUP_CREATE_WITH_AUDIO)
msgstr("Enable Audio")

msgid(GROUP_CREATE_TEXT)
msgstr("Create a text-chat group")

msgid(GROUP_CREATE_VOICE)
msgstr("Create a voice-chat group")

msgid(CREATEGROUPCHAT)
msgstr("Create Groupchat")

msgid(REMOVE_GROUP)
msgstr("Remove Group")

msgid(LEAVE_GROUP)
msgstr("Leave Group")


/******************************************************************************
 *** Group Settings                                                         ***
 ******************************************************************************/
msgid(GROUPCHAT_SETTINGS)
msgstr("Groupchat Settings")

msgid(GROUP_NOTIFICATIONS)
msgstr("Group Notifications")

msgid(GROUP_NOTIFICATIONS_ON)
msgstr("On")

msgid(GROUP_NOTIFICATIONS_MENTION)
msgstr("Mentioned")

msgid(GROUP_NOTIFICATIONS_OFF)
msgstr("Off")

msgid(GROUP_TOPIC)
msgstr("Set Group's Topic")


/******************************************************************************
 *** Settings / Profile Strings                                             ***
 ******************************************************************************/
msgid(PROFILE_BUTTON)
msgstr("Profile")

msgid(DEVICES_BUTTON)
msgstr("Devices")

msgid(USER_INTERFACE_BUTTON)
msgstr("User Interface")

msgid(AUDIO_VIDEO_BUTTON)
msgstr("Audio & Video")

msgid(ADVANCED_BUTTON)
msgstr("Advanced")

msgid(NOTIFICATIONS_BUTTON)
msgstr("Notifications")

msgid(PROFILE_SETTINGS)
msgstr("Profile Settings")

msgid(PROFILE_PW_WARNING)
msgstr("WARNING: µTox will automatically start encrypting with this password.")

msgid(PROFILE_PW_NO_RECOVER)
msgstr("There is no way to recover lost passwords.")



/***     	Profile                                                         ***/

/***     	Devices                                                         ***/
msgid(DEVICES_ADD_NEW)
msgstr("Add New Device to network")

msgid(DEVICES_NUMBER)
msgstr("Number of linked devices")
/*** 		Network                                                         ***/

/***    	User Interface                                                  ***/

/***		Audio/Video                                                     ***/



/******************************************************************************
 *** UNSORTED STRINGS                                                       ***
 ******************************************************************************/
msgid(ADDFRIENDS)
msgstr("Add New Contact")

msgid(TOXID)
msgstr("Tox ID")

msgid(MESSAGE)
msgstr("Message")

msgid(FILTER_ONLINE)
msgstr("Online Contacts")

msgid(FILTER_ALL)
msgstr("All Contacts")

msgid(FILTER_CONTACT_TOGGLE)
msgstr("Toggle filtering of offline contacts.")

msgid(ADD)
msgstr("Add")

msgid(FRIENDREQUEST)
msgstr("Friend Request")

msgid(USERSETTINGS)
msgstr("User Settings")

msgid(FRIEND_SETTINGS)
msgstr("Friend Settings")

msgid(NAME)
msgstr("Name")

msgid(STATUSMESSAGE)
msgstr("Status Message")

msgid(PREVIEW)
msgstr("Preview")

msgid(AUDIOINPUTDEVICE)
msgstr("Audio Input Device")

msgid(AUDIOFILTERING)
msgstr("Audio Filtering")

msgid(AUDIOOUTPUTDEVICE)
msgstr("Audio Output Device")

msgid(VIDEOINPUTDEVICE)
msgstr("Video Input Device")

msgid(VIDEOFRAMERATE)
msgstr("Video Frame Rate (FPS)")

msgid(PUSH_TO_TALK)
msgstr("Push To Talk")

msgid(STATUS)
msgstr("Status")

msgid(STATUS_ONLINE)
msgstr("Online")

msgid(STATUS_AWAY)
msgstr("Away")

msgid(STATUS_BUSY)
msgstr("Busy")

msgid(STATUS_OFFLINE)
msgstr("Offline")

/*
 * Leave %.*s. They are variables
 * The first one is your friend's name
 * The second one is the state your friend is now in
 */
msgid(STATUS_MESSAGE)
msgstr("uTox %.*s is now %s.")

msgid(SETTINGS_UI_MINI_ROSTER)
msgstr("Use mini contact list")

msgid(SETTINGS_UI_AUTO_HIDE_SIDEBAR)
msgstr("Auto hide sidebar")

msgid(NOT_CONNECTED)
msgstr("Not Connected")

msgid(NOT_CONNECTED_SETTINGS)
msgstr("Adjust network settings")

msgid(OTHERSETTINGS)
msgstr("Other Settings")

msgid(UI)
msgstr("UI")

msgid(USER_INTERFACE)
msgstr("User Interface")

msgid(UTOX_SETTINGS)
msgstr("uTox Settings")

msgid(NETWORK_SETTINGS)
msgstr("Network Settings")

msgid(PROFILE_PASSWORD)
msgstr("Profile Password")

msgid(LOCK_UTOX)
msgstr("Disconnects from Tox and locks this profile.")

msgid(SHOW_UI_PASSWORD)
msgstr("Show password field")

msgid(SHOW_UI_PASSWORD_TOOLTIP)
msgstr("Click to show profile password field. Changes made here will be instant!")

msgid(HIDE_UI_PASSWORD)
msgstr("Hide password field")

msgid(HIDE_UI_PASSWORD_TOOLTIP)
msgstr("Click to hide profile password field.")

msgid(PASSWORD_TOO_SHORT)
msgstr("Password must be at least 4 characters long")

msgid(LOCK)
msgstr("Lock")

msgid(AUDIO_VIDEO)
msgstr("Audio/Video")

msgid(DPI)
msgstr("DPI")

msgid(SAVELOCATION)
msgstr("Save Location")

msgid(LANGUAGE)
msgstr("Language")

msgid(NETWORK)
msgstr("Network")

msgid(IPV6)
msgstr("IPv6")

msgid(UDP)
msgstr("UDP")

msgid(PROXY)
msgstr("Proxy (SOCKS 5)")

msgid(PROXY_FORCE)
msgstr("Force uTox to always use proxy")

msgid(WARNING)
msgstr("Changing Network/Proxy settings will temporarily disconnect you from the Tox network")

msgid(SAVE_CHAT_HISTORY)
msgstr("Save Chat History")

msgid(AUDIONOTIFICATIONS)
msgstr("Enable Audible Notification (Ringtone)")

msgid(RINGTONE)
msgstr("Ringtone")

msgid(IS_TYPING)
msgstr("is typing...")

msgid(CLOSE_TO_TRAY)
msgstr("Close To Tray")

msgid(START_IN_TRAY)
msgstr("Start In Tray")

msgid(SHOW_QR)
msgstr("Show QR code")

msgid(HIDE_QR)
msgstr("Hide QR code")

msgid(SAVE_QR)
msgstr("Save QR code")

msgid(COPY_TOX_ID)
msgstr("Copy as text")

msgid(COPY)
msgstr("Copy")

msgid(COPYWITHOUTNAMES)
msgstr("Copy (Without Names)")

msgid(COPY_WITH_NAMES)
msgstr("Copy (Include Names)")

msgid(CUT)
msgstr("Cut")

msgid(PASTE)
msgstr("Paste")

msgid(DELETE)
msgstr("Delete")

msgid(SELECTALL)
msgstr("Select All")

msgid(REMOVE)
msgstr("Remove")

msgid(REMOVE_FRIEND)
msgstr("Remove Friend")

msgid(LEAVE)
msgstr("Leave")

msgid(CTOPIC)
msgstr("Change Topic")

msgid(ACCEPT)
msgstr("Accept")

msgid(IGNORE)
msgstr("Ignore")

msgid(SET_ALIAS)
msgstr("Set alias")

msgid(ALIAS)
msgstr("Alias")

msgid(SENDMESSAGE)
msgstr("Send message")

msgid(SENDSCREENSHOT)
msgstr("Send a screenshot")

msgid(CLICKTOSAVE)
msgstr("Click to save")

msgid(CLICKTOOPEN)
msgstr("Click to open")

msgid(CANCELLED)
msgstr("Cancelled")

msgid(DPI_060)
msgstr("Custom DPI 60%")

msgid(DPI_070)
msgstr("Custom DPI 70%")

msgid(DPI_080)
msgstr("Custom DPI 80%")

msgid(DPI_090)
msgstr("Custom DPI 90%")

msgid(DPI_100)
msgstr("Custom DPI 100%")

msgid(DPI_110)
msgstr("Custom DPI 110%")

msgid(DPI_120)
msgstr("Custom DPI 120%")

msgid(DPI_130)
msgstr("Custom DPI 130%")

msgid(DPI_140)
msgstr("Custom DPI 140%")

msgid(DPI_150)
msgstr("Custom DPI 150%")

msgid(DPI_160)
msgstr("Custom DPI 160%")

msgid(DPI_170)
msgstr("Custom DPI 170%")

msgid(DPI_180)
msgstr("Custom DPI 180%")

msgid(DPI_190)
msgstr("Custom DPI 190%")

msgid(DPI_200)
msgstr("Custom DPI 200%")

msgid(DPI_210)
msgstr("Custom DPI 210%")

msgid(DPI_220)
msgstr("Custom DPI 220%")

msgid(DPI_230)
msgstr("Custom DPI 230%")

msgid(DPI_240)
msgstr("Custom DPI 240%")

msgid(DPI_250)
msgstr("Custom DPI 250%")

msgid(DPI_260)
msgstr("Custom DPI 260%")

msgid(DPI_270)
msgstr("Custom DPI 270%")

msgid(DPI_280)
msgstr("Custom DPI 280%")

msgid(DPI_290)
msgstr("Custom DPI 290%")

msgid(DPI_300)
msgstr("Custom DPI 300%")

msgid(DPI_TINY)
msgstr("Tiny (50%)")

msgid(DPI_NORMAL)
msgstr("Normal (100%)")

msgid(DPI_BIG)
msgstr("Big (150%)")

msgid(DPI_LARGE)
msgstr("Large (200%)")

msgid(DPI_HUGE)
msgstr("Huge (250%)")

msgid(PROXY_DISABLED)
msgstr("Disabled")

msgid(PROXY_FALLBACK)
msgstr("Fallback")

msgid(PROXY_ALWAYS_USE)
msgstr("Always use")

msgid(NO)
msgstr("No")

msgid(YES)
msgstr("Yes")

msgid(OFF)
msgstr("Off")

msgid(ON)
msgstr("On")

msgid(SHOW)
msgstr("Show")

msgid(HIDE)
msgstr("Hide")

msgid(EXIT)
msgstr("Exit")

msgid(VIDEO_IN_NONE)
msgstr("None")

msgid(VIDEO_IN_DESKTOP)
msgstr("Desktop")

msgid(AUDIO_IN_DEFAULT_LOOPBACK)
msgstr("Default Loopback")

msgid(AUDIO_IN_ANDROID)
msgstr("OpenSL Input")

msgid(DEFAULT_FRIEND_REQUEST_MESSAGE)
msgstr("Please accept this friend request.")

msgid(CONTACT_SEARCH_ADD_HINT)
msgstr("Search/Add Friends")

msgid(PROXY_EDIT_HINT_IP)
msgstr("IP address")

msgid(PROXY_EDIT_HINT_PORT)
msgstr("Port")

msgid(WINDOW_TITLE_VIDEO_PREVIEW)
msgstr("Video Preview")

msgid(MUTE)
msgstr("Mute")

msgid(UNMUTE)
msgstr("Unmute")

msgid(SELECT_AVATAR_TITLE)
msgstr("Select Avatar")

msgid(AVATAR_TOO_LARGE_MAX_SIZE_IS)
msgstr("Avatar too large. Maximum size: ")

msgid(CANT_FIND_FILE_OR_EMPTY)
msgstr("Cannot find selected file or selected file is empty.")

msgid(CLEAR_HISTORY)
msgstr("Clear history")

msgid(AUTO_STARTUP)
msgstr("Launch at system startup")

msgid(THEME)
msgstr("Theme")

msgid(THEME_DEFAULT)
msgstr("Default")

msgid(THEME_LIGHT)
msgstr("Light theme")

msgid(THEME_DARK)
msgstr("Dark theme")

msgid(THEME_HIGHCONTRAST)
msgstr("High contrast")

msgid(THEME_CUSTOM)
msgstr("Custom (see docs)")

msgid(THEME_ZENBURN)
msgstr("Zenburn")

msgid(THEME_SOLARIZED_LIGHT)
msgstr("Solarized-light")

msgid(THEME_SOLARIZED_DARK)
msgstr("Solarized-dark")

msgid(SEND_TYPING_NOTIFICATIONS)
msgstr("Send typing notifications")

msgid(STATUS_NOTIFICATIONS)
msgstr("Status Notifications")

msgid(RANDOMIZE_NOSPAM)
msgstr("Randomize Nospam")

msgid(NOSPAM)
msgstr("Nospam")

msgid(REVERT_NOSPAM)
msgstr("Revert Nospam")

msgid(NOSPAM_WARNING)
msgstr("Changing your nospam will cause your old tox ID to no longer work. uTox does not update your ID on name servers.")

msgid(BLOCK_FRIEND_REQUESTS)
msgstr("Block Friend Requests")

msgid(SHOW_NOSPAM)
msgstr("Show Nospam Settings")

msgid(HIDE_NOSPAM)
msgstr("Hide Nospam Settings")

msgid(DELETE_FRIEND)
msgstr("Delete Friend")

msgid(DELETE_MESSAGE)
msgstr("Are you sure you want to delete ")

msgid(KEEP)
msgstr("Keep")
