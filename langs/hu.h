/******************************************************************************
 *** Hungarian i18l Strings                                                    ***
 ******************************************************************************/
msgid(LANG_NATIVE_NAME)
msgstr("Magyar")

msgid(LANG_ENGLISH_NAME)
msgstr("HUNGARIAN")


/******************************************************************************
 *** Splash Message & Change Log                                            ***
 ******************************************************************************/
msgid(SPLASH_TITLE)
msgstr("Üdvözöljük az újabb uToxban!")

msgid(SPLASH_TEXT)
msgstr("Ön a 0.17.0 verzióra frissített! A kiadás neve: bork bork")

msgid(CHANGE_LOG_TITLE)
msgstr("Változások a 0.17.0 verzióban")

msgid(CHANGE_LOG_TEXT)
msgstr(
"\n  FONTOS MEGJEGYZÉS WINDOWS FELHASZNÁLÓKNAK\n"
"    There is currently an issue with the updater where it won't work with some .exe names.\n"
"    To be sure that it works and autoupdates, you have to rename the .exe to \"uTox.exe\"\n"
"    and enable it in the settings.\n"
"\n"
"  Tulajdonságok:\n"
"    Toxcore 0.2.x. support!\n"
"    Settings are now stored in a human-readable .ini file.\n"
"    We now support Tox URIs.\n"
"    Dropdowns now stay open after being clicked.\n"
"    You can now get a QR code representing your ToxID.\n"
"    Estonian language support\n"
"  Javítások:\n"
"    The zenburn colour scheme now works.\n"
"    Calls are now added to the chatlogs.\n"
"    Saving inline images now works to paths with non-ascii characters.\n"
"    GTK should work on more platforms now.\n"
"    Exporting chatlogs on macOS doesn't crash anymore.\n"
"    File transfers are now supported on macOS.\n"
"    The force proxy setting is now saved.\n"
"    The tray icon should maybe now probably work on most (maybe) Linux systems.\n"
"    Lots of UI fixes involving the bounding boxes of things.\n"
"    We now handle signals on Linux systems.\n"
"    Filter audio can now be enabled again.\n"
"    The Windows tray icon will be reloaded if Windows explorer crashes.\n"
"    You no longer end up with invalid fps data when loading an older save.\n"
"    Datetime format is now saved.\n"
"    Non-ASCII should work better now.\n"
"    Tooltips will now always be drawn within the window boundaries.\n"
"  Fejlesztés:\n"
"    You can now set the library locations with CMAKE_PREFIX_PATH.\n"
"\n")

/******************************************************************************
 *** MISC & UNSORTED                                                        ***
 ******************************************************************************/

msgid(REQ_SENT)
msgstr("Partnerkérelem elküldve. A partner elérhető lesz, miután a kérelmet elfogadta.")

msgid(REQ_INVALID_ID)
msgstr("Hiba: Érvénytelen Tox ID")

msgid(REQ_ADDED_NO_FREQ_SENT)
msgstr("Megj.: Partner hozzáadva, de partnerkérelem nem volt küldve (nospam hiány)")

msgid(REQ_EMPTY_ID)
msgstr("Hiba: Nincs megadva Tox ID")

msgid(REQ_LONG_MSG)
msgstr("Hiba: Az üzenet túl hosszú")

msgid(REQ_NO_MSG)
msgstr("Hiba: Üres üzenet")

msgid(REQ_SELF_ID)
msgstr("Hiba: A Tox ID ugyanaz")

msgid(REQ_ALREADY_FRIENDS)
msgstr("Hiba: A Tox ID már a partnerlistában van")

msgid(REQ_UNKNOWN)
msgstr("Hiba: Ismeretlen")

msgid(REQ_BAD_CHECKSUM)
msgstr("Hiba: Érvénytelen Tox ID (hibás checksum)")

msgid(REQ_BAD_NOSPAM)
msgstr("Hiba: Érvénytelen Tox ID (hibás nospam érték)")

msgid(REQ_NO_MEMORY)
msgstr("Hiba: Nincs memória")

msgid(SEND_FILE)
msgstr("Fájl küldése")

msgid(SAVE_FILE)
msgstr("Fájl mentése")

msgid(WHERE_TO_SAVE_FILE_PROMPT)
msgstr("Hová szeretné menteni ezt:  \"%.*s\" ?")

msgid(WHERE_TO_SAVE_FILE)
msgstr("Hová szeretné menteni a fájlt?")

msgid(SEND_FILE_PROMPT)
msgstr("Válasszon ki egy vagy több fájlt a küldéshez!")

msgid(SCREEN_CAPTURE_PROMPT)
msgstr("Jelöljön ki egy területet a kijelzőn, amit el szeretne küldeni!")


/******************************************************************************
 *** File Transfer Strings                                                  ***
 ******************************************************************************/
msgid(TRANSFER_NEW)
msgstr("Új fájl átvitel")

msgid(TRANSFER_STARTED)
msgstr("Fájl átvitel elindult")

msgid(TRANSFER___)
msgstr("...")

msgid(TRANSFER_PAUSED)
msgstr("Fájl átvitel szünetelve")

msgid(TRANSFER_BROKEN)
msgstr("Fájl átvitel sikertelen")

msgid(TRANSFER_CANCELLED)
msgstr("Fájl átvitel megszakadt")

msgid(TRANSFER_COMPLETE)
msgstr("Fájl átvitel sikeres")


/******************************************************************************
 *** Keyboard and Mouse Cursor Strings                                      ***
 ******************************************************************************/
msgid(CURSOR_CLICK_LEFT)
msgstr("Bal klikk")

msgid(CURSOR_CLICK_RIGHT)
msgstr("Jobbklikk")


/******************************************************************************
 *** Audio / Video Call Strings                                             ***
 ******************************************************************************/
msgid(CALL_START_AUDIO)
msgstr("Hívás indítása")

msgid(CALL_START_VIDEO)
msgstr("Videohívás indítása")

msgid(CALL_DECLINE)
msgstr("Hívás elutasítása")

msgid(CALL_CANCELLED)
msgstr("Hívás megszakadt")

msgid(CALL_INVITED)
msgstr("Hívás fogadva")

msgid(CALL_RINGING)
msgstr("Hívás")

msgid(CALL_STARTED)
msgstr("Hívás elindult")

msgid(CALL_ENDED)
msgstr("Hívás befejezve")

msgid(CALL_FRIEND_HAD_ENDED_CALL)
msgstr(" befejezte a hívást!")

msgid(CALL_VIDEO_SHOW_INLINE)
msgstr("Video mutatása ")


/******************************************************************************
 *** Friend & Friend Settings Strings                                       ***
 ******************************************************************************/
msgid(FRIEND_ALIAS)
msgstr("Álnév beállítása")

msgid(FRIEND_PUBLIC_KEY)
msgstr("Publikus kulcs")

msgid(FRIEND_AUTOACCEPT)
msgstr("Bejövő fájlátvitel elfogadása megerősítés nélkül")

msgid(FRIEND_EXPORT_CHATLOG)
msgstr("Chatnapló exportálása szövegként")


/******************************************************************************
 *** Group Strings                                                          ***
 ******************************************************************************/
msgid(GROUPCHAT_JOIN_AUDIO)
msgstr("Belépés audiochatbe")

msgid(GROUP_CREATE_WITH_AUDIO)
msgstr("Hang engedélyezése")

msgid(GROUP_CREATE_TEXT)
msgstr("Szöveges chat csoport létrehozása")

msgid(GROUP_CREATE_VOICE)
msgstr("Audiochat csoport létrehozása")

msgid(CREATEGROUPCHAT)
msgstr("Csoportos chat létrehozása")

msgid(REMOVE_GROUP)
msgstr("Csoport eltávolítása")

msgid(LEAVE_GROUP)
msgstr("Kilépés a csoportból")


/******************************************************************************
 *** Group Settings                                                         ***
 ******************************************************************************/
msgid(GROUPCHAT_SETTINGS)
msgstr("Csoportos chat beállítások")

msgid(GROUP_NOTIFICATIONS)
msgstr("Csoport értesítések")

msgid(GROUP_NOTIFICATIONS_ON)
msgstr("Be")

msgid(GROUP_NOTIFICATIONS_MENTION)
msgstr("Megemlítés")

msgid(GROUP_NOTIFICATIONS_OFF)
msgstr("Ki")

msgid(GROUP_TOPIC)
msgstr("Csoporttéma beállítása")

/******************************************************************************
 *** Settings / Profile Strings                                             ***
 ******************************************************************************/
msgid(PROFILE_BUTTON)
msgstr("Profil")

msgid(DEVICES_BUTTON)
msgstr("Eszközök")

msgid(USER_INTERFACE_BUTTON)
msgstr("Felhasználói felület")

msgid(AUDIO_VIDEO_BUTTON)
msgstr("Audio & Video")

msgid(ADVANCED_BUTTON)
msgstr("Továbbiak")

msgid(NOTIFICATIONS_BUTTON)
msgstr("Értesítések")

msgid(PROFILE_SETTINGS)
msgstr("Profilbeállítások")

msgid(PROFILE_PW_WARNING)
msgstr("FIGYELEM: µTox automatikusan elindítja a titkosítást ezzel a jelszóval.")

msgid(PROFILE_PW_NO_RECOVER)
msgstr("Elveszett jelszó visszaállítása nem lehetséges.")



/***     	Profile                                                         ***/

/***     	Devices                                                       ***/
msgid(DEVICES_ADD_NEW)
msgstr("Új eszköz hozzáadása")

msgid(DEVICES_NUMBER)
msgstr("Csatlakoztatott eszközök száma")
/*** 		Network                                                         ***/

/***    User Interface                                                  ***/

/***		Audio/Video                                                     ***/



/******************************************************************************
 *** UNSORTED STRINGS                                                       ***
 *** TODO:                                                                  ***
 *** FIXME:                                                                 ***
 ******************************************************************************/
msgid(ADDFRIENDS)
msgstr("Új partner hozzáadása")

msgid(TOXID)
msgstr("Tox ID")

msgid(MESSAGE)
msgstr("Üzenet")

msgid(FILTER_ONLINE)
msgstr("Elérhető partnerek")

msgid(FILTER_ALL)
msgstr("Összes partner")

msgid(FILTER_CONTACT_TOGGLE)
msgstr("Nem elérhető partnerek szűrése")

msgid(ADD)
msgstr("Hozzáadás")

msgid(FRIENDREQUEST)
msgstr("Partnerkérelem")

msgid(USERSETTINGS)
msgstr("Felhasználói beállítások")

msgid(FRIEND_SETTINGS)
msgstr("Partnerbeállítások")

msgid(NAME)
msgstr("Név")

msgid(STATUSMESSAGE)
msgstr("Állapot üzenet")

msgid(PREVIEW)
msgstr("Előnézet")

msgid(AUDIOINPUTDEVICE)
msgstr("Bemeneti hangeszköz")

msgid(AUDIOFILTERING)
msgstr("Hangszűrés")

msgid(AUDIOOUTPUTDEVICE)
msgstr("Kimeneti hangeszköz")

msgid(VIDEOINPUTDEVICE)
msgstr("Bemeneti videoeszköz")

msgid(VIDEOFRAMERATE)
msgstr("Video képkocka (FPS)")

msgid(PUSH_TO_TALK)
msgstr("Nyomd És Beszélj")

msgid(STATUS)
msgstr("Állapot")

msgid(STATUS_ONLINE)
msgstr("Elérhető")

msgid(STATUS_AWAY)
msgstr("Távol")

msgid(STATUS_BUSY)
msgstr("Elfoglalt")

msgid(STATUS_OFFLINE)
msgstr("Nem elérhető")

/*
 * Leave %.*s. They are variables
 * The first one is your friend's name
 * The second one is the state your friend is now in
 */
msgid(STATUS_MESSAGE)
msgstr("uTox %.*s most már %s.")

msgid(SETTINGS_UI_MINI_ROSTER)
msgstr("Mini partnerlista használata")

msgid(SETTINGS_UI_AUTO_HIDE_SIDEBAR)
msgstr("Oldalsáv automatikus elrejtése")

msgid(NOT_CONNECTED)
msgstr("Nincs csatlakoztatva")

msgid(NOT_CONNECTED_SETTINGS)
msgstr("Hálózati beállítások beillesztése")

msgid(OTHERSETTINGS)
msgstr("Egyéb beállítások")

msgid(UI)
msgstr("UI")

msgid(USER_INTERFACE)
msgstr("Felhasználói felület")

msgid(UTOX_SETTINGS)
msgstr("uTox beállítások")

msgid(NETWORK_SETTINGS)
msgstr("Hálózati beállítások")

msgid(PROFILE_PASSWORD)
msgstr("Profiljelszó")

msgid(LOCK_UTOX)
msgstr("Lekapcsolódik a Toxról, és zárolja a profilt.")

msgid(SHOW_UI_PASSWORD)
msgstr("Jelszómező mutatása")

msgid(SHOW_UI_PASSWORD_TOOLTIP)
msgstr("Klikkeléskor a profiljelszó mező mutatása. A módosítás azonnal érvénybe lép!")

msgid(HIDE_UI_PASSWORD)
msgstr("Jelszómező elrejtése")

msgid(HIDE_UI_PASSWORD_TOOLTIP)
msgstr("Klikkeléskor a profiljelszó mező elrejtése.")

msgid(PASSWORD_TOO_SHORT)
msgstr("A jelszó legalább 4 karakter hosszú legyen")

msgid(LOCK)
msgstr("Zárolás")

msgid(AUDIO_VIDEO)
msgstr("Audio/Video")

msgid(DPI)
msgstr("DPI")

msgid(SAVELOCATION)
msgstr("Mentés helye")

msgid(LANGUAGE)
msgstr("Nyelv")

msgid(NETWORK)
msgstr("Hálózat")

msgid(IPV6)
msgstr("IPv6")

msgid(UDP)
msgstr("UDP")

msgid(PROXY)
msgstr("Proxy (SOCKS 5)")

msgid(PROXY_FORCE)
msgstr("uTox kényszerítése, hogy mindig proxy-t használjon")

msgid(WARNING)
msgstr("A hálózati-, vagy proxybeállítások változtatása miatt átmenetileg nem lesz elérhető a Tox hálózatán")

msgid(SAVE_CHAT_HISTORY)
msgstr("Chat előzmények mentése")

msgid(AUDIONOTIFICATIONS)
msgstr("Hallható jelzés (csengőhang) engedélyezése")

msgid(RINGTONE)
msgstr("Csengőhang")

msgid(IS_TYPING)
msgstr("éppen ír...")

msgid(CLOSE_TO_TRAY)
msgstr("Bezárás a tálcára")

msgid(START_IN_TRAY)
msgstr("Indítás a tálcán")

msgid(SHOW_QR)
msgstr("QR-kód mutatása")

msgid(HIDE_QR)
msgstr("QR-kód elrejtése")

msgid(SAVE_QR)
msgstr("QR-kód mentése")

msgid(COPY_TOX_ID)
msgstr("Másolás szövegként")

msgid(COPY)
msgstr("Másolás")

msgid(COPYWITHOUTNAMES)
msgstr("Másolás (nevek nélkül)")

msgid(COPY_WITH_NAMES)
msgstr("Másolás (nevekkel együtt)")

msgid(CUT)
msgstr("Kivágás")

msgid(PASTE)
msgstr("Beillesztés")

msgid(DELETE)
msgstr("Törlés")

msgid(SELECTALL)
msgstr("Mindet kiválaszt")

msgid(REMOVE)
msgstr("Eltávolítás")

msgid(REMOVE_FRIEND)
msgstr("Partner eltávolítása")

msgid(LEAVE)
msgstr("Kilépés")

msgid(CTOPIC)
msgstr("Téma változtatás")

msgid(ACCEPT)
msgstr("Elfogadás")

msgid(IGNORE)
msgstr("Mellőzés")

msgid(SET_ALIAS)
msgstr("Álnév beállítása")

msgid(ALIAS)
msgstr("Álnév")

msgid(SENDMESSAGE)
msgstr("Üzenet küldése")

msgid(SENDSCREENSHOT)
msgstr("Képernyőkép küldése")

msgid(CLICKTOSAVE)
msgstr("Kattintson a mentéshez")

msgid(CLICKTOOPEN)
msgstr("Kattintson a megnyitáshoz")

msgid(CANCELLED)
msgstr("Megszakítva")

msgid(DPI_060)
msgstr("Egyedi DPI 60%")

msgid(DPI_070)
msgstr("Egyedi DPI 70%")

msgid(DPI_080)
msgstr("Egyedi DPI 80%")

msgid(DPI_090)
msgstr("Egyedi DPI 90%")

msgid(DPI_100)
msgstr("Egyedi DPI 100%")

msgid(DPI_110)
msgstr("Egyedi DPI 110%")

msgid(DPI_120)
msgstr("Egyedi DPI 120%")

msgid(DPI_130)
msgstr("Egyedi DPI 130%")

msgid(DPI_140)
msgstr("Egyedi DPI 140%")

msgid(DPI_150)
msgstr("Egyedi DPI 150%")

msgid(DPI_160)
msgstr("Egyedi DPI 160%")

msgid(DPI_170)
msgstr("Egyedi DPI 170%")

msgid(DPI_180)
msgstr("Egyedi DPI 180%")

msgid(DPI_190)
msgstr("Egyedi DPI 190%")

msgid(DPI_200)
msgstr("Egyedi DPI 200%")

msgid(DPI_210)
msgstr("Egyedi DPI 210%")

msgid(DPI_220)
msgstr("Egyedi DPI 220%")

msgid(DPI_230)
msgstr("Egyedi DPI 230%")

msgid(DPI_240)
msgstr("Egyedi DPI 240%")

msgid(DPI_250)
msgstr("Egyedi DPI 250%")

msgid(DPI_260)
msgstr("Egyedi DPI 260%")

msgid(DPI_270)
msgstr("Egyedi DPI 270%")

msgid(DPI_280)
msgstr("Egyedi DPI 280%")

msgid(DPI_290)
msgstr("Egyedi DPI 290%")

msgid(DPI_300)
msgstr("Egyedi DPI 300%")

msgid(DPI_TINY)
msgstr("Kicsi (50%)")

msgid(DPI_NORMAL)
msgstr("Normál (100%)")

msgid(DPI_BIG)
msgstr("Nagy (150%)")

msgid(DPI_LARGE)
msgstr("Nagyobb (200%)")

msgid(DPI_HUGE)
msgstr("Legnagyobb (250%)")

msgid(PROXY_DISABLED)
msgstr("Letiltva")

msgid(PROXY_FALLBACK)
msgstr("Fallback")

msgid(PROXY_ALWAYS_USE)
msgstr("Mindig használja")

msgid(NO)
msgstr("Nem")

msgid(YES)
msgstr("Igen")

msgid(OFF)
msgstr("Ki")

msgid(ON)
msgstr("Be")

msgid(SHOW)
msgstr("Mutat")

msgid(HIDE)
msgstr("Elrejt")

msgid(VIDEO_IN_NONE)
msgstr("Nincs")

msgid(VIDEO_IN_DESKTOP)
msgstr("Asztal")

msgid(AUDIO_IN_DEFAULT_LOOPBACK)
msgstr("Alapértelmezett Loopback")

msgid(AUDIO_IN_ANDROID)
msgstr("OpenSL bevitel")

msgid(DEFAULT_FRIEND_REQUEST_MESSAGE)
msgstr("Fogadja el a partnerkérelmet!")

msgid(CONTACT_SEARCH_ADD_HINT)
msgstr("Partner keresése/hozzáadása")

msgid(PROXY_EDIT_HINT_IP)
msgstr("IP-cím")

msgid(PROXY_EDIT_HINT_PORT)
msgstr("Port")

msgid(WINDOW_TITLE_VIDEO_PREVIEW)
msgstr("Videó előnézet")

msgid(MUTE)
msgstr("Némítás")

msgid(UNMUTE)
msgstr("Némítás visszavonása")

msgid(SELECT_AVATAR_TITLE)
msgstr("Avatarkép választás")

msgid(AVATAR_TOO_LARGE_MAX_SIZE_IS)
msgstr("Az avatarkép túl nagy! Maximális méret: ")

msgid(CANT_FIND_FILE_OR_EMPTY)
msgstr("A kiválasztott kép nem található, vagy üres.")

msgid(CLEAR_HISTORY)
msgstr("Előzmények törlése")

msgid(AUTO_STARTUP)
msgstr("uTox indítása rendszerinduláskor")

msgid(THEME)
msgstr("Téma")

msgid(THEME_DEFAULT)
msgstr("Alapértelmezett")

msgid(THEME_LIGHT)
msgstr("Világos téma")

msgid(THEME_DARK)
msgstr("Sötét téma")

msgid(THEME_HIGHCONTRAST)
msgstr("Feltűnő téma")

msgid(THEME_CUSTOM)
msgstr("Egyedi (lásd a dokumentációt)")

msgid(THEME_ZENBURN)
msgstr("Zenburn")

msgid(THEME_SOLARIZED_LIGHT)
msgstr("Solarized-light")

msgid(THEME_SOLARIZED_DARK)
msgstr("Solarized-dark")

msgid(SEND_TYPING_NOTIFICATIONS)
msgstr("Írási értesítés küldése")

msgid(STATUS_NOTIFICATIONS)
msgstr("Állapot értesítések")

msgid(RANDOMIZE_NOSPAM)
msgstr("Véletlenszerű Nospam")

msgid(NOSPAM)
msgstr("Nospam")

msgid(REVERT_NOSPAM)
msgstr("Nospam visszaállítása")

msgid(NOSPAM_WARNING)
msgstr("A nospam megváltoztatása a régi Tox ID működésképtelenségéhez vezet. Az uTox nem képes frissíteni az ID-t a névszervereken.")

msgid(BLOCK_FRIEND_REQUESTS)
msgstr("Partnerkérelmek blokkolása")

msgid(SHOW_NOSPAM)
msgstr("Nospam beállítások mutatása")

msgid(HIDE_NOSPAM)
msgstr("Nospam beállítások elrejtése")

msgid(DELETE_FRIEND)
msgstr("Partner törlése")

msgid(DELETE_MESSAGE)
msgstr("Biztosan törölni szeretné ")

msgid(KEEP)
msgstr("Megtartás")
