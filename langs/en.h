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
msgstr("You have just upgraded to version 0.16.1! Release name: No capes!")

msgid(CHANGE_LOG_TITLE)
msgstr("Changes in v0.16.1 (check out our new website, utox.io, we beg you)")

msgid(CHANGE_LOG_TEXT)
msgstr(
"\n  IMPORTANT NOTE FOR WINDOWS USERS\n"
"    There is currently an issue with the updater where it won't work with some .exe names.\n"
"    To be sure that it works and autoupdates, you have to rename the .exe to \"uTox.exe\"\n"
"    and enable it in the settings.\n"
"\n"
"  Features:\n"
"    Language is now saved.\n"
"    Added group audio calls.\n"
"    User-specified FPS during video call.\n"
"  Fixes:\n"
"    OS X stability improvements.\n"
"    Video now works on Linux again.\n"
"    UI bugfixes and improvements. (Thanks @redmanmale!)\n"
"    Fixed Windows video preview window title. (Thanks @thorpelawrence!)\n"
"    More groupchat crashes fixed.\n"
"    Middle/double click in X11 now have the correct functionality. (Thanks @dkmoz!)\n"
"    Unicode urls now work on Windows.\n"
"    File transfer names are now sanitized on Windows.\n"
"    Autoaccept file transfer setting now saves again.\n"
"    Audio notifications now only play when uTox isn't in focus.\n"
"    Messages aren't marked as unread if the chat is open when messages are received.\n"
"    The GTK file picker now works on OpenBSD and NetBSD.\n"
"    Fixed autostart toggle being a no-op.\n"
"    Fixed typing notifications not using the contact's alias.\n"
"    Added timestamps to exported chatlog.\n"
"  Development:\n"
"    Dropped support for ToxDNS.\n"
"    Build instructions for OS X now exist. (Thanks @publicarray!)\n"
"    Tests are now run against OS X in addition to Linux.\n"
"    Added Python script for finding missing translations.\n"
"    Decreased size of MinSizeRel builds.\n"
"    Updated lots of translations.\n"
"\n"
"  Do you know another language? Rather read the changelog in your language?\n"
"    Help us translate uTox!\n"
"\nv0.15.0\n"
"  Fixes:\n"
"    Video now works on Windows again.\n"
"    Autoaccepting files in portable mode now saves to the portable folder.\n"
"    Updated Ukrainian translation. (Thanks v2e!)\n"
"    Dbus notifications should now work on more different setups.\n"
"  Development\n"
"    Added support for NetBSD, OpenBSD, and FreeBSD.\n"
"    Refactored the UI.\n"
"\n  Do you know another language? Rather read the changelog in your language?\n"
"    Help us translate uTox!\n"
"\nv0.14.0\n"
"  Features:\n"
"    Real Updater.\n"
"       -- uTox now has an updater built in. Currently Windows only.\n"
"       -- But can be adapted to any platform if there's any interest.\n"
"    Added musl libc support.\n"
"    uTox is now compiled with stronger exploit protection.\n"
"    Significant reduction in memory usage for friend list.\n"
"  Fixes:\n"
"    New Nospams no longer start with 0000 on window.\n"
"    Fixed button alignment with Language selection.\n"
"    The tox save is no longer truncated at exit.\n"
"    The tray icon now always displays a square\n"
"       -- Even on broken display managers (uTox looks at gnome...)\n"
"    Bootstrap nodes list updated.\n"
"    Avatars can now be deleted.\n"
"    Notifications are no longer ignored when uTox is minimized.\n"
"    A few more file transfer fixes.\n"
"       -- Especially the last remaining one that would occasionally miss updates.\n"
"  Development\n"
"    All remaining warnings we fixed, -Werror was added\n"
"    Tests added:\n"
"                uTox Updater\n"
"                Chatlog reading and writing\n"
"    Refactored the UI a bit more. Which cleared out a few more of the UI glitches.\n"
"\nv0.13.0\n"
"  Features:\n"
"    You can now paste UTF8 characters in uTox.\n"
"    You can now manually set your nospam!\n"
"\n  Fixes:\n"
"    Mostly fix groupchats. (At least they no longer crash uTox instantly..)\n"
"    Chatlogs can now be saved on Windows again.\n"
"    Clearing chat backlog no longer crashes uTox.\n"
"    Plug a lot of memory leaks.\n"
"    Scrolling with pgup/pgdn now redraws the chat window.\n"
"    Avatars now load again.\n"
"    Custom themes now work and won't crash uTox if you try to switch to it when it doesn't exist.\n"
"    Lots of build system improvements.\n"
"    Lots of file transfer fixes.\n"
"\n  Development\n"
"    uTox now has a unit test implemented! (Hopefully this means we'll never have a bug ever again.)\n"
"    UI files separated into UI components and layout.\n"
"    We now have our own build server!\n"
"\nv0.12.2\n"
"    Fixed a Windows UI hang during file transfers.\n"
"    Fixed a Windows Crash during file transfers.\n"
"\nv0.11.1\n"
"    Fixed crash with some transfers.\n"
"    Bug Fixes\n"
"    Fixed building on Windows XP\n"
"    Friend request improvements\n"
"    Added the ability to change nospam\n"
"    Added notifications and advanced tab to settings\n"
"    New updater\n"
"    Started seperating the core and the UI\n"
"\nv0.11.1\n"
"    Feature: Export chatlog\n"
"    Feature: Added ability to decline an incoming call\n"
"    Various security fixes\n"
"    Fixed icon issues\n"
"    Better build system\n"
"    Switched to TokTok/c-toxcore\n"
"    Improvements to development process to allow for easier contribution of code\n"
"    New themes: Solarized light and Solarized dark\n"
"    Updated translations\n"
"    filter_audio optimisations\n"
"    Started using clang-format\n"
"    Stopped using clang-format\n"
"    New, shinier toggle buttons (thanks, tsudoko!)\n"
"    Better support for auto-accepting file transfers\n"
"    Fixed UI issues with the lock screen\n"
"\nv0.11.0\n"
"    Were sorry, but the changelog for this version of uTox\n"
"    is only available to uTox GOLD Members.\n"
"\nv0.9.8\n"
"    Fixed a crash when trying to create a new profile\n"
"\nv0.9.7\n"
"    Some minor GUI fixes\n"
"    Replaced all Yes/No dropdowns with Switches\n"
"    Fixed saving proxy host & port across restarts.\n"
"\nv0.9.5\n"
"    Inline video for OSX\n"
"    Fixed a logging bug\n"
"\nv0.9.3\n"
"    Fixed a message bug causing sporatic crashes in Windows\n"
"    Limited the number of Messages to resend at once\n"
"\nv0.9.2\n"
"    Source Cleanups/Dir changes\n"
"\nv0.9.1\n"
"    Fixed a crash when sending a message to a new fiend\n"
"\nv0.9.0\n"
"   Message queuing\n"
"   Better, and colorized groupchats\n"
"   Experimental Inline Video support\n"
"   Mini Contact list\n"
"   Refactored message handling\n"
"\n")

/******************************************************************************
 *** MISC & UNSORTED                                                        ***
 ******************************************************************************/

msgid(REQ_SENT)
msgstr("Friend request sent. Your friend will appear online after the request is accepted.")

msgid(REQ_INVALID_ID)
msgstr("Error: Invalid Tox ID")

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

msgid(GROUP_CREATE_TEXT)
msgstr("Create a text group chat")

msgid(GROUP_CREATE_VOICE)
msgstr("Create a group chat with voice")

msgid(CREATEGROUPCHAT)
msgstr("Create Groupchat")

msgid(REMOVE_GROUP)
msgstr("Remove Group")

msgid(LEAVE_GROUP)
msgstr("Leave Group")

msgid(GROUP_INVITE)
msgstr("Groupchat invite")

msgid(GROUP_INVITE_FRIEND)
msgstr("%s invites you to the groupchat")

msgid(GROUP_MESSAGE_INVITE)
msgstr("<- invited %s")

msgid(GROUP_MESSAGE_JOIN)
msgstr("<- has joined")

msgid(GROUP_MESSAGE_CHANGE_NAME)
msgstr("<- has changed name from %.*s")

msgid(GROUP_MESSAGE_QUIT)
msgstr("<- has quit")


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

msgid(AUTO_UPDATE)
msgstr("Automatically Update uTox")

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
 *** TODO:                                                                  ***
 *** FIXME:                                                                 ***
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
msgstr("IPv6:")

msgid(UDP)
msgstr("UDP:")

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
msgstr("IP")

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
