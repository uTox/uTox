/******************************************************************************
 *** Russian i18l Strings                                                   ***
 ******************************************************************************/
msgid(LANG_NATIVE_NAME)
msgstr("Русский")

msgid(LANG_ENGLISH_NAME)
msgstr("RUSSIAN")


/******************************************************************************
 *** Splash Message & Change Log                                            ***
 ******************************************************************************/
msgid(SPLASH_TITLE)
msgstr("Добро пожаловать в новый uTox!")

msgid(SPLASH_TEXT)
msgstr("Вы только что обновились до версии 0.16.1! Название выпуска: No capes!")

msgid(CHANGE_LOG_TITLE)
msgstr("Изменения в v0.16.1")

msgid(CHANGE_LOG_TEXT)
msgstr(
"\n  ВАЖНОЕ ЗАМЕЧАНИЕ ДЛЯ ПОЛЬЗОВАТЕЛЕЙ WINDOWS\n"
"    В данный момент с автообновлением проблема, апдейтер не работает с .exe-файлами.\n"
"    Чтобы быть уверенным, что автообновление будет работать,\n"
"    вам нужно переименовать .exe в \"uTox.exe\" и включить его в настройках\n"
"\n"
"  Новые функции:\n"
"    Сохраняется язык интерфейса.\n"
"    Реализованы голосовые групповые звонки.\n"
"    Настройка FPS видео во время звонка.\n"
"  Исправления:\n"
"    Улучшена стабильность под OS X.\n"
"    Видео снова работает под Linux.\n"
"    Исправления и улучшения интерфейса. (Спасибо @redmanmale!)\n"
"    Исправлен заголовок окна предпросмотра видео под Windows. (Спасибо @thorpelawrence!)\n"
"    Исправлена часть сбоев в групповых чатах.\n"
"    Исправлено поведение двойного клика средней клавиши мыши в X11. (Спасибо @dkmoz!)\n"
"    Исправлен переход по ссылкам с Юникод-символами под Windows.\n"
"    При передаче файлов их имена очищаются от запрещённых символов под Windows.\n"
"    Сохраняются настройки подтверждения приёма файлов.\n"
"    Аудио уведомления теперь воспроизводятся только когда uTox не в фокусе.\n"
"    Сообщения помечаются прочитанными, если они пришли, когда чат был открыт.\n"
"    Выбор файла GTK теперь работает под OpenBSD и NetBSD.\n"
"    Исправлено поведение переключателя настройки автозапуска.\n"
"    Используется псевдоним контакта при уведомлении о наборе текста.\n"
"    Добавлены дата и время сообщения при экспорте чата.\n"
"  Разработка\n"
"    Прекращена поддержка ToxDNS.\n"
"    Добавлена инструкция по сборке под OS X (Спасибо @publicarray!)\n"
"    Прогон тестов под OS X (в дополнение к Linux).\n"
"    Добавлен скрипт на питоне для поиска отсутствующих переводов.\n"
"    Уменьшен размер MinSizeRel-сборок.\n"
"    Обновлены несколько переводов.\n"
"\n"
"    Знаешь другой язык? Приятнее читать список изменений на своём языке?\n"
"    Помоги нам перевести uTox!\n"
"\nv0.15.0\n"
"\nНовые функции:\n"
"    Переделан интерфейс. (Спасибо redmanmale!)\n"
"    Сворачивающаяся боковая панель под Android.\n"
"  Исправления:\n"
"    Видео снова работает под Windows.\n"
"    При автоматическом приёме файлов в портативном режиме они сохраняются в ту же папку.\n"
"    Обновлён украинский перевод. (Спасибо v2e!)\n"
"    Dbus уведомления должны работать в большем количестве систем.\n"
"  Разработка\n"
"    Добавлена поддержка NetBSD, OpenBSD и FreeBSD.\n"
"    Переделан интерфейс.\n"
"\nv0.9.7\n"
"    Некоторые незначительные исправления GUI\n"
"    Все выпадающие меню Да/Нет заменены переключателями\n"
"    Исправлено сохранение настроек прокси между запусками.\n"
"\nv0.9.5\n"
"    Встроенное видео для OSX\n"
"    Исправлена ошибка ведения лога\n"
"\nv0.9.3\n"
"    Исправлена ошибка сообщений, вызывающая случайные сбои в Windows\n"
"    Ограничено число сообщений для повторной отправки за раз\n"
"\nv0.9.2\n"
"    Чистка исходников/Изменения директорий\n"
"\nv0.9.1\n"
"    Исправлен сбой при отправке сообщения новому собеседнику\n"
"\nv0.9.0\n"
"   Очередь сообщений\n"
"   Улучшенные и раскрашенные групповые чаты\n"
"   Экспериментальная поддержка встроенного видео\n"
"   Компактный список контактов\n"
"   Переработана обработка сообщений\n"
"\n")

/******************************************************************************
 *** MISC & UNSORTED                                                        ***
 ******************************************************************************/

msgid(REQ_SENT)
msgstr("Запрос на добавление в контакты отправлен. Ваш собеседник станет доступен в сети, как только подтвердит запрос.")

msgid(REQ_INVALID_ID)
msgstr("Ошибка: неправильный Tox ID")

msgid(REQ_EMPTY_ID)
msgstr("Ошибка: не указан Tox ID")

msgid(REQ_LONG_MSG)
msgstr("Ошибка: слишком длинное сообщение")

msgid(REQ_NO_MSG)
msgstr("Ошибка: пустое сообщение")

msgid(REQ_SELF_ID)
msgstr("Ошибка: Tox ID совпадает с собственным")

msgid(REQ_ALREADY_FRIENDS)
msgstr("Ошибка: Tox ID уже есть в списке контактов")

msgid(REQ_UNKNOWN)
msgstr("Неизвестная ошибка")

msgid(REQ_BAD_CHECKSUM)
msgstr("Ошибка: неправильный Tox ID (контрольная сумма не совпадает)")

msgid(REQ_BAD_NOSPAM)
msgstr("Ошибка: неправильный Tox ID (неверное значение антиспама)")

msgid(REQ_NO_MEMORY)
msgstr("Ошибка: не хватает памяти")

msgid(SEND_FILE)
msgstr("Отправить файл")

msgid(SAVE_FILE)
msgstr("Сохранить файл")

msgid(WHERE_TO_SAVE_FILE_PROMPT)
msgstr("Где вы хотите сохранить \"%.*s\"?")

msgid(WHERE_TO_SAVE_FILE)
msgstr("Где вы хотите сохранить файл?")

msgid(SEND_FILE_PROMPT)
msgstr("Выберите один или несколько файлов для отправки.")

msgid(SCREEN_CAPTURE_PROMPT)
msgstr("Выделите область экрана, которую вы хотите отправить.")


/******************************************************************************
 *** File Transfer Strings                                                  ***
 ******************************************************************************/
msgid(TRANSFER_NEW)
msgstr("Передача нового файла")

msgid(TRANSFER_STARTED)
msgstr("Передача файла начата")

msgid(TRANSFER___)
msgstr("...")

msgid(TRANSFER_PAUSED)
msgstr("Передача файла приостановлена")

msgid(TRANSFER_BROKEN)
msgstr("Передача файла прервана")

msgid(TRANSFER_CANCELLED)
msgstr("Передача файла отменена")

msgid(TRANSFER_COMPLETE)
msgstr("Передача файла завершена")


/******************************************************************************
 *** Keyboard and Mouse Cursor Strings                                      ***
 ******************************************************************************/
msgid(CURSOR_CLICK_LEFT)
msgstr("Левый клик")

msgid(CURSOR_CLICK_RIGHT)
msgstr("Правый клик")


/******************************************************************************
 *** Audio / Video Call Strings                                             ***
 ******************************************************************************/
msgid(CALL_START_AUDIO)
msgstr("Позвонить")

msgid(CALL_START_VIDEO)
msgstr("Видео вызов")

msgid(CALL_DECLINE)
msgstr("Отклонить вызов")

msgid(CALL_CANCELLED)
msgstr("Вызов отменён")

msgid(CALL_INVITED)
msgstr("Входящий вызов")

msgid(CALL_RINGING)
msgstr("Исходящий вызов")

msgid(CALL_STARTED)
msgstr("Идёт вызов")

msgid(CALL_ENDED)
msgstr("Вызов завершён")

msgid(CALL_FRIEND_HAD_ENDED_CALL)
msgstr(" завершил вызов!")

msgid(CALL_VIDEO_SHOW_INLINE)
msgstr("Показывать видео встроенным")


/******************************************************************************
 *** Friend & Friend Settings Strings                                       ***
 ******************************************************************************/
msgid(FRIEND_ALIAS)
msgstr("Назначить псевдоним контакта")

msgid(FRIEND_PUBLIC_KEY)
msgstr("Открытый ключ контакта")

msgid(FRIEND_AUTOACCEPT)
msgstr("Принимать передаваемые файлы без подтверждения")

msgid(FRIEND_EXPORT_CHATLOG)
msgstr("Экспортировать историю переписки как текст")


/******************************************************************************
 *** Group Strings                                                          ***
 ******************************************************************************/
msgid(GROUPCHAT_JOIN_AUDIO)
msgstr("Присоединиться к голосовому чату")

msgid(GROUP_CREATE_TEXT)
msgstr("Создать текстовый групповой чат")

msgid(GROUP_CREATE_VOICE)
msgstr("Создать голосовой групповой чат")

msgid(CREATEGROUPCHAT)
msgstr("Создать групповой чат")

msgid(REMOVE_GROUP)
msgstr("Удалить группу")

msgid(LEAVE_GROUP)
msgstr("Покинуть группу")

msgid(GROUP_INVITE)
msgstr("Приглашение в групповой чат")

msgid(GROUP_INVITE_FRIEND)
msgstr("%s приглашает вас в групповой чат")

msgid(GROUP_MESSAGE_INVITE)
msgstr("<- пригласил %s")

msgid(GROUP_MESSAGE_JOIN)
msgstr("<- присоединился")

msgid(GROUP_MESSAGE_CHANGE_NAME)
msgstr("<- сменил имя с %.*s")

msgid(GROUP_MESSAGE_QUIT)
msgstr("<- вышел")


/******************************************************************************
 *** Group Settings                                                         ***
 ******************************************************************************/
msgid(GROUPCHAT_SETTINGS)
msgstr("Настройки группового чата")

msgid(GROUP_NOTIFICATIONS)
msgstr("Групповые уведомления")

msgid(GROUP_NOTIFICATIONS_ON)
msgstr("Вкл")

msgid(GROUP_NOTIFICATIONS_MENTION)
msgstr("Упомянутые")

msgid(GROUP_NOTIFICATIONS_OFF)
msgstr("Выкл")

msgid(GROUP_TOPIC)
msgstr("Тема группы")


/******************************************************************************
 *** Settings / Profile Strings                                             ***
 ******************************************************************************/
msgid(PROFILE_BUTTON)
msgstr("Профиль")

msgid(DEVICES_BUTTON)
msgstr("Устройства")

msgid(USER_INTERFACE_BUTTON)
msgstr("Интерфейс")

msgid(AUDIO_VIDEO_BUTTON)
msgstr("Аудио и видео")

msgid(ADVANCED_BUTTON)
msgstr("Дополнительно")

msgid(NOTIFICATIONS_BUTTON)
msgstr("Уведомления")

msgid(AUTO_UPDATE)
msgstr("Автоматически обновлять uTox")

msgid(PROFILE_SETTINGS)
msgstr("Настройки профиля")

msgid(PROFILE_PW_WARNING)
msgstr("ПРЕДУПРЕЖДЕНИЕ: µTox автоматически зашифрует профиль с использованием этого пароля.")

msgid(PROFILE_PW_NO_RECOVER)
msgstr("Не существует способов восстановления утраченных паролей.")



/***     	Profile                                                         ***/

/***     	Devices                                                         ***/
msgid(DEVICES_ADD_NEW)
msgstr("Добавить новое связанное устройство")

msgid(DEVICES_NUMBER)
msgstr("Число связанных устройств")
/*** 		Network                                                         ***/

/***    	User Interface                                                  ***/

/***		Audio/Video                                                     ***/



/******************************************************************************
 *** UNSORTED STRINGS                                                       ***
 *** TODO:                                                                  ***
 *** FIXME:                                                                 ***
 ******************************************************************************/
msgid(ADDFRIENDS)
msgstr("Добавить новый контакт")

msgid(TOXID)
msgstr("Tox ID")

msgid(MESSAGE)
msgstr("Сообщение")

msgid(FILTER_ONLINE)
msgstr("Контакты на связи")

msgid(FILTER_ALL)
msgstr("Все контакты")

msgid(FILTER_CONTACT_TOGGLE)
msgstr("Переключить видимость отсутствующих контактов.")

msgid(ADD)
msgstr("Добавить")

msgid(FRIENDREQUEST)
msgstr("Запрос на добавление в список контактов")

msgid(USERSETTINGS)
msgstr("Настройки пользователя")

msgid(FRIEND_SETTINGS)
msgstr("Настройки контакта")

msgid(NAME)
msgstr("Имя")

msgid(STATUSMESSAGE)
msgstr("Сообщение статуса")

msgid(PREVIEW)
msgstr("Предпросмотр")

msgid(AUDIOINPUTDEVICE)
msgstr("Устройство захвата звука")

msgid(AUDIOFILTERING)
msgstr("Фильтр звука")

msgid(AUDIOOUTPUTDEVICE)
msgstr("Устройство воспроизведения звука")

msgid(VIDEOINPUTDEVICE)
msgstr("Устройство захвата видео")

msgid(VIDEOFRAMERATE)
msgstr("Частота кадров видео (FPS)")

msgid(PUSH_TO_TALK)
msgstr("Нажать для разговора")

msgid(STATUS)
msgstr("Статус")

msgid(STATUS_ONLINE)
msgstr("в сети")

msgid(STATUS_AWAY)
msgstr("не на месте")

msgid(STATUS_BUSY)
msgstr("занят(а)")

msgid(STATUS_OFFLINE)
msgstr("не в сети")

msgid(STATUS_MESSAGE)
msgstr("uTox %.*s теперь %s.")

msgid(SETTINGS_UI_MINI_ROSTER)
msgstr("Компактный список контактов")

msgid(SETTINGS_UI_AUTO_HIDE_SIDEBAR)
msgstr("Скрывать боковую панель автоматически")

msgid(NOT_CONNECTED)
msgstr("Нет сети")

msgid(NOT_CONNECTED_SETTINGS)
msgstr("Изменить настройки сети")

msgid(OTHERSETTINGS)
msgstr("Прочие настройки")

msgid(UI)
msgstr("UI")

msgid(USER_INTERFACE)
msgstr("Интерфейс")

msgid(UTOX_SETTINGS)
msgstr("Настройки uTox")

msgid(NETWORK_SETTINGS)
msgstr("Настройки сети")

msgid(PROFILE_PASSWORD)
msgstr("Пароль профиля")

msgid(LOCK_UTOX)
msgstr("Отключиться от сети и заблокировать профиль")

msgid(SHOW_UI_PASSWORD)
msgstr("Показать поле пароля")

msgid(SHOW_UI_PASSWORD_TOOLTIP)
msgstr("Показать поле пароля профиля. Изменения, сделанные здесь, применяются сразу!")

msgid(HIDE_UI_PASSWORD)
msgstr("Скрыть поле пароля")

msgid(HIDE_UI_PASSWORD_TOOLTIP)
msgstr("Скрыть поле пароля профиля")

msgid(LOCK)
msgstr("Заблокировать")

msgid(AUDIO_VIDEO)
msgstr("Аудио/видео")

msgid(DPI)
msgstr("Разрешение, точек/дюйм")

msgid(SAVELOCATION)
msgstr("Сохранить расположение")

msgid(LANGUAGE)
msgstr("Язык")

msgid(NETWORK)
msgstr("Сеть")

msgid(IPV6)
msgstr("IPv6:")

msgid(UDP)
msgstr("UDP:")

msgid(PROXY)
msgstr("Прокси (SOCKS 5)")

msgid(PROXY_FORCE)
msgstr("Всегда использовать проки")

msgid(WARNING)
msgstr("Изменение сетевых настроек приведёт к временному отключению от сети Tox")

msgid(SAVE_CHAT_HISTORY)
msgstr("Сохранять историю чата")

msgid(AUDIONOTIFICATIONS)
msgstr("Включить звонок (звуковое уведомление)")

msgid(RINGTONE)
msgstr("Мелодия звонка")

msgid(IS_TYPING)
msgstr("печатает...")

msgid(CLOSE_TO_TRAY)
msgstr("Сворачивать вместо закрытия")

msgid(START_IN_TRAY)
msgstr("Запускать свёрнутым")

msgid(COPY)
msgstr("Копировать")

msgid(COPYWITHOUTNAMES)
msgstr("Копировать (без имён)")

msgid(COPY_WITH_NAMES)
msgstr("Копировать (с именами)")

msgid(CUT)
msgstr("Вырезать")

msgid(PASTE)
msgstr("Вставить")

msgid(DELETE)
msgstr("Удалить")

msgid(SELECTALL)
msgstr("Выделить всё")

msgid(REMOVE)
msgstr("Удалить")

msgid(REMOVE_FRIEND)
msgstr("Удалить контакт")

msgid(LEAVE)
msgstr("Выйти")

msgid(CTOPIC)
msgstr("Сменить тему")

msgid(ACCEPT)
msgstr("Принять")

msgid(IGNORE)
msgstr("Игнорировать")

msgid(SET_ALIAS)
msgstr("Назначить псевдоним")

msgid(ALIAS)
msgstr("Псевдоним")

msgid(SENDMESSAGE)
msgstr("Отправить сообщение")

msgid(SENDSCREENSHOT)
msgstr("Отправить снимок экрана")

msgid(CLICKTOSAVE)
msgstr("Нажмите чтобы сохранить")

msgid(CLICKTOOPEN)
msgstr("Нажмите чтобы открыть")

msgid(CANCELLED)
msgstr("Отменено")

msgid(DPI_060)
msgstr("Пользовательское 60%")

msgid(DPI_070)
msgstr("Пользовательское 70%")

msgid(DPI_080)
msgstr("Пользовательское 80%")

msgid(DPI_090)
msgstr("Пользовательское 90%")

msgid(DPI_100)
msgstr("Пользовательское 100%")

msgid(DPI_110)
msgstr("Пользовательское 110%")

msgid(DPI_120)
msgstr("Пользовательское 120%")

msgid(DPI_130)
msgstr("Пользовательское 130%")

msgid(DPI_140)
msgstr("Пользовательское 140%")

msgid(DPI_150)
msgstr("Пользовательское 150%")

msgid(DPI_160)
msgstr("Пользовательское 160%")

msgid(DPI_170)
msgstr("Пользовательское 170%")

msgid(DPI_180)
msgstr("Пользовательское 180%")

msgid(DPI_190)
msgstr("Пользовательское 190%")

msgid(DPI_200)
msgstr("Пользовательское 200%")

msgid(DPI_210)
msgstr("Пользовательское 210%")

msgid(DPI_220)
msgstr("Пользовательское 220%")

msgid(DPI_230)
msgstr("Пользовательское 230%")

msgid(DPI_240)
msgstr("Пользовательское 240%")

msgid(DPI_250)
msgstr("Пользовательское 250%")

msgid(DPI_260)
msgstr("Пользовательское 260%")

msgid(DPI_270)
msgstr("Пользовательское 270%")

msgid(DPI_280)
msgstr("Пользовательское 280%")

msgid(DPI_290)
msgstr("Пользовательское 290%")

msgid(DPI_300)
msgstr("Пользовательское 300%")

msgid(DPI_TINY)
msgstr("Мелкое (50%)")

msgid(DPI_NORMAL)
msgstr("Нормальное (100%)")

msgid(DPI_BIG)
msgstr("Крупное (150%)")

msgid(DPI_LARGE)
msgstr("Большое (200%)")

msgid(DPI_HUGE)
msgstr("Огромное (250%)")

msgid(PROXY_DISABLED)
msgstr("Отключено")

msgid(PROXY_FALLBACK)
msgstr("Если необходимо")

msgid(PROXY_ALWAYS_USE)
msgstr("Всегда")

msgid(NO)
msgstr("Нет")

msgid(YES)
msgstr("Да")

msgid(OFF)
msgstr("Выкл")

msgid(ON)
msgstr("Вкл")

msgid(SHOW)
msgstr("Показать")

msgid(HIDE)
msgstr("Скрыть")

msgid(VIDEO_IN_NONE)
msgstr("Не выбрано")

msgid(VIDEO_IN_DESKTOP)
msgstr("Рабочий стол")

msgid(AUDIO_IN_DEFAULT_LOOPBACK)
msgstr("По умолчанию")

msgid(AUDIO_IN_ANDROID)
msgstr("OpenSL")

msgid(DEFAULT_FRIEND_REQUEST_MESSAGE)
msgstr("Пожалуйста, добавьте меня в свой список контактов.")

msgid(CONTACT_SEARCH_ADD_HINT)
msgstr("Найти/добавить контакты")

msgid(PROXY_EDIT_HINT_IP)
msgstr("IP")

msgid(PROXY_EDIT_HINT_PORT)
msgstr("Порт")

msgid(WINDOW_TITLE_VIDEO_PREVIEW)
msgstr("Предпросмотр видео")

msgid(MUTE)
msgstr("Заглушить")

msgid(UNMUTE)
msgstr("Включить")

msgid(SELECT_AVATAR_TITLE)
msgstr("Выбрать изображение")

msgid(AVATAR_TOO_LARGE_MAX_SIZE_IS)
msgstr("Изображение слишком большое. Максимальный размер: ")

msgid(CANT_FIND_FILE_OR_EMPTY)
msgstr("Невозможно найти выбранный файл или выбранный файл пуст.")

msgid(CLEAR_HISTORY)
msgstr("Очистить историю")

msgid(AUTO_STARTUP)
msgstr("Запускать при загрузке системы")

msgid(THEME)
msgstr("Тема")

msgid(THEME_DEFAULT)
msgstr("По умолчанию")

msgid(THEME_LIGHT)
msgstr("Светлая")

msgid(THEME_DARK)
msgstr("Тёмная")

msgid(THEME_HIGHCONTRAST)
msgstr("Высококонтрастная")

msgid(THEME_CUSTOM)
msgstr("Своя (см. документацию)")

msgid(THEME_ZENBURN)
msgstr("Zenburn")

msgid(THEME_SOLARIZED_LIGHT)
msgstr("Solarized светлая")

msgid(THEME_SOLARIZED_DARK)
msgstr("Solarized тёмная")

msgid(SEND_TYPING_NOTIFICATIONS)
msgstr("Отправлять уведомления о наборе текста")

msgid(STATUS_NOTIFICATIONS)
msgstr("Уведомления о статусе")

msgid(RANDOMIZE_NOSPAM)
msgstr("Сгенерировать новый антиспам")

msgid(NOSPAM)
msgstr("Антиспам")

msgid(REVERT_NOSPAM)
msgstr("Отменить изменение антиспама")

msgid(NOSPAM_WARNING)
msgstr("Изменение антиспама приведёт к тому, что ваш старый Tox ID больше не будет работать. uTox не обновляет ваш Tox ID на серверах имён.")

msgid(BLOCK_FRIEND_REQUESTS)
msgstr("Блокировать запросы на добавление в контакты")

msgid(SHOW_NOSPAM)
msgstr("Показать настройки антиспама")

msgid(HIDE_NOSPAM)
msgstr("Скрыть настройки антиспама")

msgid(DELETE_FRIEND)
msgstr("Удалить из контактов")

msgid(DELETE_MESSAGE)
msgstr("Вы уверены, что хотите удалить ")

msgid(KEEP)
msgstr("Оставить")
