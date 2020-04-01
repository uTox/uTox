/******************************************************************************
 *** English i18l Strings                                                   ***
 ******************************************************************************/
msgid(LANG_NATIVE_NAME)
msgstr("Български")

msgid(LANG_ENGLISH_NAME)
msgstr("BULGARIAN")


/******************************************************************************
 *** Splash Message & Change Log                                            ***
 ******************************************************************************/
msgid(SPLASH_TITLE)
msgstr("Добри дошли в по-новия uTox!")

msgid(SPLASH_TEXT)
msgstr("Вече сте с версия 0.16.1! Кодово име: без шапки!")

msgid(CHANGE_LOG_TITLE)
msgstr("Промени в 0.16.1 (моля проверете нашият уебсайт, utox.io)")

msgid(CHANGE_LOG_TEXT)
msgstr(
"\n  ВАЖНО съобщение за WINDOWS потребителите\n"
"    Съществува проблем с актуализатора, при което той не работи с някои имена на .exe файла.\n"
"    За да сте сигурни, че ще работи автоматичното актуализиране преименувайте .exe файла на \"uTox.exe\"\n"
"    и активирайте функцията в настройките.\n"
"\n"
"  Нововъведения:\n"
"    Вече се запазва езика.\n"
"    Добавени са групови аудио разговори.\n"
"    Потребителски зададени РВС във видео разговорите.\n"
"  Поправки:\n"
"    OS X подобрения в стабилността.\n"
"    Видеото отново работи под Linuх.\n"
"    Поправки и подобрения в потребителския изглед. (Благодарности на @redmanmale!)\n"
"    Поправено е заглавието на видео прозореца под Windows. (Благодарности на @thorpelawrence!)\n"
"    Поправени са причините за сриване при групов чат.\n"
"    Натискането със среден бутон и двоен клик вече работят коректно в X11. (Благодарности на @dkmoz!)\n"
"    Уникод линковете вече работят под Windows.\n"
"    Имената на прехвърляните файлове вече се запазват под Windows.\n"
"    Автоматичното приемане на прехвърлянията на файлове вече пак се запаметяват.\n"
"    Вече аудиоизвестията се изпълняват, когато uTox не е на фокус.\n"
"    Съобщенията не се маркират като непрочетени, ако чата е отворен, в момента на получаване.\n"
"    Избирачът на файлове пад GTK вече работи и за OpenBSD и NetBSD.\n"
"    Поправена опция за автостарт.\n"
"    Поправени известия при писане.\n"
"    Добавени времеви точки при изнасяне на дискусията.\n"
"  В разработка:\n"
"    Отхвърлена поддръжка на ToxDNS.\n"
"    Инструкции за компилиране под OS X вече съществуват. (Благодарности на @publicarray!)\n"
"    Тестовете вече се извършват за OS X както и под Linux.\n"
"    Добавен скрипт на Python за откриване на липсващи преводи.\n"
"    Намален размер на MinSizeRel компилации.\n"
"    Обновени множество преводи.\n"
"\n"
"  Знаете ли друг език? Желаете ли да четете промените по версията на друг език?\n"
"    Помогнете ни да преведем uTox!\n"
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
msgstr("Поканата за приятелство е изпратена! Приятелят Ви ще се появи онлайн, когато поканата бъде приета.")


msgid(REQ_INVALID_ID)
msgstr("Грешка: Невалиден Tox идентификатор")

msgid(REQ_ADDED_NO_FREQ_SENT)
msgstr("Бележка: Приятелят е добавен, но поканата не е изпратена (липсва антиспам код)")

msgid(REQ_EMPTY_ID)
msgstr("Грешка: Не е зададен Tox идентификатор")

msgid(REQ_LONG_MSG)
msgstr("Грешка: Съобщението е прекалено дълго")

msgid(REQ_NO_MSG)
msgstr("Грешка: Празно съобщение")

msgid(REQ_SELF_ID)
msgstr("Грешка: Tox идентификацията е вашата идентификация")

msgid(REQ_ALREADY_FRIENDS)
msgstr("Грешка: Тох идентификацията е вече в списъка с приятели")

msgid(REQ_UNKNOWN)
msgstr("Грешка: Неизвестно")

msgid(REQ_BAD_CHECKSUM)
msgstr("Грешка: Невалидна Tox идентификация (грешна сума)")

msgid(REQ_BAD_NOSPAM)
msgstr("Грешка: Невалидна Tox идентификация (грешна антиспам стойност)")

msgid(REQ_NO_MEMORY)
msgstr("Грешка: Недостатъчно памет")

msgid(SEND_FILE)
msgstr("Изпрати файл")

msgid(SAVE_FILE)
msgstr("Запази файл")

msgid(WHERE_TO_SAVE_FILE_PROMPT)
msgstr("Къде желаете да запазите \"%.*s\"?")

msgid(WHERE_TO_SAVE_FILE)
msgstr("Къде желаете да запазите файла?")

msgid(SEND_FILE_PROMPT)
msgstr("Изберете един или повече файлове за изпращане.")

msgid(SCREEN_CAPTURE_PROMPT)
msgstr("Начертайте кутия около зоната от екрана, която искате да изпратите.")


/******************************************************************************
 *** File Transfer Strings                                                  ***
 ******************************************************************************/
msgid(TRANSFER_NEW)
msgstr("Ново прехвърляне на файл")

msgid(TRANSFER_STARTED)
msgstr("Начало на прехвърлянето на файл")

msgid(TRANSFER___)
msgstr("...")

msgid(TRANSFER_PAUSED)
msgstr("Прехвърлянето е паузирано")

msgid(TRANSFER_BROKEN)
msgstr("Прехвърлянето се провали")

msgid(TRANSFER_CANCELLED)
msgstr("Прехвърлянето е отказано")

msgid(TRANSFER_COMPLETE)
msgstr("Прехвърлянето приключи")


/******************************************************************************
 *** Keyboard and Mouse Cursor Strings                                      ***
 ******************************************************************************/
msgid(CURSOR_CLICK_LEFT)
msgstr("Натискане с ляв бутон")

msgid(CURSOR_CLICK_RIGHT)
msgstr("Натискане с десен бутон")


/******************************************************************************
 *** Audio / Video Call Strings                                             ***
 ******************************************************************************/
msgid(CALL_START_AUDIO)
msgstr("Започни разговор")

msgid(CALL_START_VIDEO)
msgstr("Започни видео разговор")

msgid(CALL_DECLINE)
msgstr("Отхвърли повикването")

msgid(CALL_CANCELLED)
msgstr("Повикването отказано")

msgid(CALL_INVITED)
msgstr("Поканен в разговор")

msgid(CALL_RINGING)
msgstr("Позвъняване")

msgid(CALL_STARTED)
msgstr("Начало на разговора")

msgid(CALL_ENDED)
msgstr("Край на разговора")

msgid(CALL_FRIEND_HAD_ENDED_CALL)
msgstr(" прекрати разговора!")

msgid(CALL_VIDEO_SHOW_INLINE)
msgstr("Покажи видеото в полето")


/******************************************************************************
 *** Friend & Friend Settings Strings                                       ***
 ******************************************************************************/
msgid(FRIEND_ALIAS)
msgstr("Задай псевдоним на приятел")

msgid(FRIEND_PUBLIC_KEY)
msgstr("Публичен ключ на приятел")

msgid(FRIEND_AUTOACCEPT)
msgstr("Приемай файловете без потвърждение")

msgid(FRIEND_EXPORT_CHATLOG)
msgstr("Изнеси дневника на чата като неформатиран текст")


/******************************************************************************
 *** Group Strings                                                          ***
 ******************************************************************************/
msgid(GROUPCHAT_JOIN_AUDIO)
msgstr("Присъедини се в разговора")

msgid(GROUP_CREATE_TEXT)
msgstr("Създай групов чат")

msgid(GROUP_CREATE_VOICE)
msgstr("Създай групов гласов чат")

msgid(CREATEGROUPCHAT)
msgstr("Създай груповчат")

msgid(REMOVE_GROUP)
msgstr("Премахни групата")

msgid(LEAVE_GROUP)
msgstr("Напусни групата")


/******************************************************************************
 *** Group Settings                                                         ***
 ******************************************************************************/
msgid(GROUPCHAT_SETTINGS)
msgstr("Настройки на груповия чат")

msgid(GROUP_NOTIFICATIONS)
msgstr("Известия на груповия чат")

msgid(GROUP_NOTIFICATIONS_ON)
msgstr("Вкл.")

msgid(GROUP_NOTIFICATIONS_MENTION)
msgstr("Споменаване")

msgid(GROUP_NOTIFICATIONS_OFF)
msgstr("Изкл.")

msgid(GROUP_TOPIC)
msgstr("Задай тема на чата")

/******************************************************************************
 *** Settings / Profile Strings                                             ***
 ******************************************************************************/
msgid(PROFILE_BUTTON)
msgstr("Профил")

msgid(DEVICES_BUTTON)
msgstr("Устройства")

msgid(USER_INTERFACE_BUTTON)
msgstr("Потребителски изглед")

msgid(AUDIO_VIDEO_BUTTON)
msgstr("Звук и Видео")

msgid(ADVANCED_BUTTON)
msgstr("Допълнителни")

msgid(NOTIFICATIONS_BUTTON)
msgstr("Известия")

msgid(PROFILE_SETTINGS)
msgstr("Настройки на профила")

msgid(PROFILE_PW_WARNING)
msgstr("Внимание: µTox ще започне да криптира автоматично с тази парола.")

msgid(PROFILE_PW_NO_RECOVER)
msgstr("Не могат да се възстановяват загубени пароли.")



/***     	Profile                                                         ***/

/***     	Devices                                                         ***/
msgid(DEVICES_ADD_NEW)
msgstr("Добави ново устройстов в мрежата")

msgid(DEVICES_NUMBER)
msgstr("Брой на свързаните устройства")
/*** 		Network                                                         ***/

/***    	User Interface                                                  ***/

/***		Audio/Video                                                     ***/



/******************************************************************************
 *** UNSORTED STRINGS                                                       ***
 *** TODO:                                                                  ***
 *** FIXME:                                                                 ***
 ******************************************************************************/
msgid(ADDFRIENDS)
msgstr("Добави нов контакт")

msgid(TOXID)
msgstr("Тох идентификатор")

msgid(MESSAGE)
msgstr("Съобщение")

msgid(FILTER_ONLINE)
msgstr("Контакти на линия")

msgid(FILTER_ALL)
msgstr("Всички контакти")

msgid(FILTER_CONTACT_TOGGLE)
msgstr("Превключване филтрирането на контактите извън линия.")

msgid(ADD)
msgstr("Добави")

msgid(FRIENDREQUEST)
msgstr("Покана за приятелство")

msgid(USERSETTINGS)
msgstr("Потребителски настройки")

msgid(FRIEND_SETTINGS)
msgstr("Настройки за приятелите")

msgid(NAME)
msgstr("Име")

msgid(STATUSMESSAGE)
msgstr("Статус съобщение")

msgid(PREVIEW)
msgstr("Преглед")

msgid(AUDIOINPUTDEVICE)
msgstr("Входно звуково устройство")

msgid(AUDIOFILTERING)
msgstr("Филтриране на звука")

msgid(AUDIOOUTPUTDEVICE)
msgstr("Изходно звуково устройство")

msgid(VIDEOINPUTDEVICE)
msgstr("Входно видео устройство")

msgid(VIDEOFRAMERATE)
msgstr("Рамки в секунда за видеото (РВС)")

msgid(PUSH_TO_TALK)
msgstr("Говори при натиснат бутон")

msgid(STATUS)
msgstr("Статус")

msgid(STATUS_ONLINE)
msgstr("На линия")

msgid(STATUS_AWAY)
msgstr("Отсъстващ")

msgid(STATUS_BUSY)
msgstr("Зает")

msgid(STATUS_OFFLINE)
msgstr("Извън линия")

/*
 * Leave %.*s. They are variables
 * The first one is your friend's name
 * The second one is the state your friend is now in
 */
msgid(STATUS_MESSAGE)
msgstr("uTox %.*s е вече %s.")

msgid(SETTINGS_UI_MINI_ROSTER)
msgstr("Използвай малък списък с контакти")

msgid(SETTINGS_UI_AUTO_HIDE_SIDEBAR)
msgstr("Автоматично скривай страничния панел")

msgid(NOT_CONNECTED)
msgstr("Не сте свързан")

msgid(NOT_CONNECTED_SETTINGS)
msgstr("Нагласяне на интернет настройките")

msgid(OTHERSETTINGS)
msgstr("Други настройки")

msgid(UI)
msgstr("ПИ")

msgid(USER_INTERFACE)
msgstr("Потребителски изглед")

msgid(UTOX_SETTINGS)
msgstr("uTox настройки")

msgid(NETWORK_SETTINGS)
msgstr("Интернет настройки")

msgid(PROFILE_PASSWORD)
msgstr("Парола на профила")

msgid(LOCK_UTOX)
msgstr("Разкача се от Tox мрежата и се заключва профила.")

msgid(SHOW_UI_PASSWORD)
msgstr("Покажи полето за парола")

msgid(SHOW_UI_PASSWORD_TOOLTIP)
msgstr("Натиснете, за да се покаже полето за парола на профила. Промените тук са незабавни!")

msgid(HIDE_UI_PASSWORD)
msgstr("Скрий полето за парола")

msgid(HIDE_UI_PASSWORD_TOOLTIP)
msgstr("Натиснете, за да се скрие полето за парола на профила.")

msgid(PASSWORD_TOO_SHORT)
msgstr("Паролата, трябва да е с дължина поне 4 символа")

msgid(LOCK)
msgstr("Заключи")

msgid(AUDIO_VIDEO)
msgstr("Звук/Видео")

msgid(DPI)
msgstr("Точки на инч")

msgid(SAVELOCATION)
msgstr("Запази местоположението")

msgid(LANGUAGE)
msgstr("Език")

msgid(NETWORK)
msgstr("Мрежа")

msgid(IPV6)
msgstr("IPv6:")

msgid(UDP)
msgstr("UDP:")

msgid(PROXY)
msgstr("Прокси (SOCKS 5)")

msgid(PROXY_FORCE)
msgstr("Задължи uTox винаги да използва прокси")

msgid(WARNING)
msgstr("Промяната на Мрежови/Прокси настройки ще Ви разкачи временно от мрежта на Tox")

msgid(SAVE_CHAT_HISTORY)
msgstr("Запазвай чат хронология")

msgid(AUDIONOTIFICATIONS)
msgstr("Активирай звукови известия")

msgid(RINGTONE)
msgstr("Звуково известие")

msgid(IS_TYPING)
msgstr("въвежда...")

msgid(CLOSE_TO_TRAY)
msgstr("Затвори в таблата")

msgid(START_IN_TRAY)
msgstr("Стартирай в таблата")

msgid(SHOW_QR)
msgstr("Покажи QR")

msgid(HIDE_QR)
msgstr("Скрий QR")

msgid(SAVE_QR)
msgstr("Запази QR")

msgid(COPY_TOX_ID)
msgstr("Копирай като текст")

msgid(COPY)
msgstr("Копирай")

msgid(COPYWITHOUTNAMES)
msgstr("Копирай (без имена)")

msgid(COPY_WITH_NAMES)
msgstr("Копирай (включително и имената)")

msgid(CUT)
msgstr("Изрежи")

msgid(PASTE)
msgstr("Постави")

msgid(DELETE)
msgstr("Изтрий")

msgid(SELECTALL)
msgstr("Избери всички")

msgid(REMOVE)
msgstr("Премахни")

msgid(REMOVE_FRIEND)
msgstr("Премахни приятел")

msgid(LEAVE)
msgstr("Напусни")

msgid(CTOPIC)
msgstr("Промени темата")

msgid(ACCEPT)
msgstr("Приеми")

msgid(IGNORE)
msgstr("Отхвърли")

msgid(SET_ALIAS)
msgstr("Задай псевдоним")

msgid(ALIAS)
msgstr("Псевдоним")

msgid(SENDMESSAGE)
msgstr("Изпрати съобщение")

msgid(SENDSCREENSHOT)
msgstr("Изпрати снимка на екрана")

msgid(CLICKTOSAVE)
msgstr("Натисни за запазване")

msgid(CLICKTOOPEN)
msgstr("Натисни за отваряне")

msgid(CANCELLED)
msgstr("Отказан")

msgid(DPI_060)
msgstr("Потребителски ТНИ 60%")

msgid(DPI_070)
msgstr("Потребителски ТНИ 70%")

msgid(DPI_080)
msgstr("Потребителски ТНИ 80%")

msgid(DPI_090)
msgstr("Потребителски ТНИ 90%")

msgid(DPI_100)
msgstr("Потребителски ТНИ 100%")

msgid(DPI_110)
msgstr("Потребителски ТНИ 110%")

msgid(DPI_120)
msgstr("Потребителски ТНИ 120%")

msgid(DPI_130)
msgstr("Потребителски ТНИ 130%")

msgid(DPI_140)
msgstr("Потребителски ТНИ 140%")

msgid(DPI_150)
msgstr("Потребителски ТНИ 150%")

msgid(DPI_160)
msgstr("Потребителски ТНИ 160%")

msgid(DPI_170)
msgstr("Потребителски ТНИ 170%")

msgid(DPI_180)
msgstr("Потребителски ТНИ 180%")

msgid(DPI_190)
msgstr("Потребителски ТНИ 190%")

msgid(DPI_200)
msgstr("Потребителски ТНИ 200%")

msgid(DPI_210)
msgstr("Потребителски ТНИ 210%")

msgid(DPI_220)
msgstr("Потребителски ТНИ 220%")

msgid(DPI_230)
msgstr("Потребителски ТНИ 230%")

msgid(DPI_240)
msgstr("Потребителски ТНИ 240%")

msgid(DPI_250)
msgstr("Потребителски ТНИ 250%")

msgid(DPI_260)
msgstr("Потребителски ТНИ 260%")

msgid(DPI_270)
msgstr("Потребителски ТНИ 270%")

msgid(DPI_280)
msgstr("Потребителски ТНИ 280%")

msgid(DPI_290)
msgstr("Потребителски ТНИ 290%")

msgid(DPI_300)
msgstr("Потребителски ТНИ 300%")

msgid(DPI_TINY)
msgstr("Миниатюрно (50%)")

msgid(DPI_NORMAL)
msgstr("Нормално (100%)")

msgid(DPI_BIG)
msgstr("Голямо (150%)")

msgid(DPI_LARGE)
msgstr("По-голямо (200%)")

msgid(DPI_HUGE)
msgstr("Огромно (250%)")

msgid(PROXY_DISABLED)
msgstr("Деактивирано")

msgid(PROXY_FALLBACK)
msgstr("Безопасен режим")

msgid(PROXY_ALWAYS_USE)
msgstr("Винаги използвай")

msgid(NO)
msgstr("Не")

msgid(YES)
msgstr("Да")

msgid(OFF)
msgstr("Изкл.")

msgid(ON)
msgstr("Вкл.")

msgid(SHOW)
msgstr("Покажи")

msgid(HIDE)
msgstr("Скрий")

msgid(VIDEO_IN_NONE)
msgstr("Без")

msgid(VIDEO_IN_DESKTOP)
msgstr("Работен плот")

msgid(AUDIO_IN_DEFAULT_LOOPBACK)
msgstr("Стандартно повторение")

msgid(AUDIO_IN_ANDROID)
msgstr("OpenSL Вход")

msgid(DEFAULT_FRIEND_REQUEST_MESSAGE)
msgstr("Моля, приемете тази покана за приятелство.")

msgid(CONTACT_SEARCH_ADD_HINT)
msgstr("Намери/Добави приятели")

msgid(PROXY_EDIT_HINT_IP)
msgstr("IP")

msgid(PROXY_EDIT_HINT_PORT)
msgstr("Порт")

msgid(WINDOW_TITLE_VIDEO_PREVIEW)
msgstr("Преглед на видео")

msgid(MUTE)
msgstr("Заглуши")

msgid(UNMUTE)
msgstr("Отглуши")

msgid(SELECT_AVATAR_TITLE)
msgstr("Избери аватар")

msgid(AVATAR_TOO_LARGE_MAX_SIZE_IS)
msgstr("Аватарът е прекалено голям. Максимален размер: ")

msgid(CANT_FIND_FILE_OR_EMPTY)
msgstr("Не може да се намери избраният файл или той е празен.")

msgid(CLEAR_HISTORY)
msgstr("Изчисти хронология")

msgid(AUTO_STARTUP)
msgstr("Стартирай със системата")

msgid(THEME)
msgstr("Тема")

msgid(THEME_DEFAULT)
msgstr("Стандартна")

msgid(THEME_LIGHT)
msgstr("Светла тема")

msgid(THEME_DARK)
msgstr("Тъмна тема")

msgid(THEME_HIGHCONTRAST)
msgstr("Висок контраст")

msgid(THEME_CUSTOM)
msgstr("Потребителска (Вижте документите)")

msgid(THEME_ZENBURN)
msgstr("Треска за злато")

msgid(THEME_SOLARIZED_LIGHT)
msgstr("Слънчевосветло")

msgid(THEME_SOLARIZED_DARK)
msgstr("Слънчевотъмно")

msgid(SEND_TYPING_NOTIFICATIONS)
msgstr("Изпращай известия при писане")

msgid(STATUS_NOTIFICATIONS)
msgstr("Известия за състоянието")

msgid(RANDOMIZE_NOSPAM)
msgstr("Случаен антиспам")

msgid(NOSPAM)
msgstr("Антиспам")

msgid(REVERT_NOSPAM)
msgstr("Върни антиспама")

msgid(NOSPAM_WARNING)
msgstr("Смяната на антиспама ще прекрати ползването на стария Ви идентификатор. uTox не обновява Вашият идентификатор в именните сървъри.")

msgid(BLOCK_FRIEND_REQUESTS)
msgstr("Блокирай поканите за приятелство")

msgid(SHOW_NOSPAM)
msgstr("Покажи антиспам настройки")

msgid(HIDE_NOSPAM)
msgstr("Скрий антиспам настройки")

msgid(DELETE_FRIEND)
msgstr("Изтрий приятел")

msgid(DELETE_MESSAGE)
msgstr("Сигурни ли сте, че искате да изтриете ")

msgid(KEEP)
msgstr("Задръж")
