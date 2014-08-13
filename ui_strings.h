#define STR(x) { .str = (uint8_t*)x, .length = sizeof(x) - 1 }

STRING strings[][64] = {
//ENGLISH
{
    //0
    STR("Friend request sent. Your friend will appear online when he accepts the request."),
    STR("Attempting to resolve DNS name..."),
    STR("Error: Invalid Tox ID"),
    STR("Error: No Tox ID specified"),
    STR("Error: Message is too long"),
    STR("Error: Empty message"),
    STR("Error: Tox ID is self ID"),
    STR("Error: Tox ID is already in friend list"),
    STR("Error: Unknown"),
    STR("Error: Invalid Tox ID (bad checksum)"),
    STR("Error: Invalid Tox ID (bad nospam value)"),
    STR("Error: No memory"),

    //12
    STR("New file transfer"),
    STR("File transfer started"),
    STR(".."),
    STR("File transfer paused"),
    STR("File transfer broken"),
    STR("File transfer cancelled"),
    STR("File transfer complete"),

    //19
    STR("Call cancelled"),
    STR("Call invited"),
    STR("Call ringing"),
    STR("Call started"),

    //23
    STR("Add Friends"),
    STR("Tox ID"),
    STR("Message"),
    STR("Search friends"),
    STR("Add"),
    STR("Switch Profile"),
    STR("Friend Request"),
    STR("User Settings"),
    STR("Name"),
    STR("Status Message"),
    STR("Preview"),
    STR("Device Selection"),
    STR("Audio Input Device"),
    STR("Audio Output Device"),
    STR("Video Input Device"),
    STR("Other Settings"),
    STR("DPI"),
    STR("Save Location"),
    STR("Language"),

    //41
    STR("Copy"),
    STR("Cut"),
    STR("Paste"),
    STR("Delete"),
    STR("Select All"),
    STR("Remove"),
    STR("Leave"),
    STR("Accept"),
    STR("Ignore"),

    //50
    STR("Click to save"),
    STR("Click to open"),
    STR("Cancelled"),

    //53
    STR("English"),
    STR("French"),
    STR("Russian"),
    STR("Spanish"),
    STR("German"),
},

//FRENCH
{
    //0
    STR("Demande envoyée. Votre ami apparaîtra en ligne quand il acceptera votre demande."),
    STR("Attempting to resolve DNS name..."),
    STR("Erreur: Invalid Tox ID"),
    STR("Erreur: Aucun Tox ID"),
    STR("Erreur: Message trop long"),
    STR("Erreur: Message vide"),
    STR("Erreur: Tox ID is self ID"),
    STR("Erreur: Tox ID is already in friend list"),
    STR("Erreur: Unknown"),
    STR("Erreur: Invalid Tox ID (bad checksum)"),
    STR("Erreur: Invalid Tox ID (bad nospam value)"),
    STR("Erreur: No memory"),
    //12
    STR("Nouveau transfert de fichier"),
    STR("File transfer started"),
    STR(".."),
    STR("File transfer paused"),
    STR("File transfer broken"),
    STR("File transfer cancelled"),
    STR("Transfert de fichier complété"),
    //19
    STR("Call cancelled"),
    STR("Call invited"),
    STR("Call ringing"),
    STR("Call started"),

    //23
    STR("Add Friends"),
    STR("Tox ID"),
    STR("Message"),
    STR("Search friends"),
    STR("Add"),
    STR("Switch Profile"),
    STR("Friend Request"),
    STR("User Settings"),
    STR("Nom"),
    STR("Status Message"),
    STR("Preview"),
    STR("Device Selection"),
    STR("Audio Input Device"),
    STR("Audio Output Device"),
    STR("Video Input Device"),
    STR("Other Settings"),
    STR("DPI"),
    STR("Save Location"),
    STR("Langue"),

    //41
    STR("Copy"),
    STR("Cut"),
    STR("Paste"),
    STR("Delete"),
    STR("Select All"),
    STR("Remove"),
    STR("Leave"),
    STR("Accept"),
    STR("Ignore"),

    //50
    STR("Click to save"),
    STR("Click to open"),
    STR("Cancelled"),

    //53
    STR("English"),
    STR("French"),
    STR("Russian"),
    STR("Spanish"),
    STR("German"),
},


//RUSSIAN
{
    //0
    STR("Запрос добавления в друзья отправлен. Ваш друг появится в сети, как только подтвердит запос."),
    STR("Попытка определения DNS-имени..."),
    STR("Ошибка: Некорректный Tox ID"),
    STR("Ошибка: Не указан Tox ID"),
    STR("Ошибка: Слишком длинное сообщение"),
    STR("Ошибка: Пустое сообщение"),
    STR("Ошибка: Tox ID совпадает с собственным"),
    STR("Ошибка: Tox ID уже в списке друзей"),
    STR("Ошибка: Неизвестная"),
    STR("Ошибка: Некорректный Tox ID (контрольная сумма не совпадает)"),
    STR("Ошибка: Некорректный Tox ID (плохое значение nospam)"),
    STR("Ошибка: Не хватает памяти"),

    //12
    STR("Передача нового файла"),
    STR("Передача файла начата"),
    STR(".."),
    STR("Передача файла приостановлена"),
    STR("Передача файла прервана"),
    STR("Передача файла отменена"),
    STR("Передача файла завершена"),

    //19
    STR("Звонок отменён"),
    STR("Входящий звонок"),
    STR("Звонок"),
    STR("Разговор начат"),

    //23
    STR("Добавить друзей"),
    STR("Tox ID"),
    STR("Сообщение"),
    STR("Поиск друзей"),
    STR("Добавить"),
    STR("Переключить профиль"),
    STR("Запрос добавления в список друзей"),
    STR("Настройки пользователя"),
    STR("Имя"),
    STR("Статусное сообщение"),
    STR("Предпросмотр"),
    STR("Выбор устройств"),
    STR("Устройство ввода звука"),
    STR("Устройство вывода звука"),
    STR("Устройство захвата видео"),
    STR("Другие настройки"),
    STR("Разрешение, точек/дюйм"),
    STR("Сохранить расположение"),
    STR("Язык"),

    //41
    STR("Копировать"),
    STR("Вырезать"),
    STR("Вставить"),
    STR("Удалить"),
    STR("Выбрать всё"),
    STR("Удалить"),
    STR("Покинуть"),
    STR("Принять"),
    STR("Игнорировать"),

    //50
    STR("Сохранить"),
    STR("Открыть"),
    STR("Отменено"),

    //53
    STR("Английский"),
    STR("Французский"),
    STR("Русский"),
    STR("Испанский"),
    STR("Немецкий"),
},

//SPANISH
{
    //0
    STR("Friend request sent. Your friend will appear online when he accepts the request."),
    STR("Attempting to resolve DNS name..."),
    STR("Error: Invalid Tox ID"),
    STR("Error: No Tox ID specified"),
    STR("Error: Message is too long"),
    STR("Error: Empty message"),
    STR("Error: Tox ID is self ID"),
    STR("Error: Tox ID is already in friend list"),
    STR("Error: Unknown"),
    STR("Error: Invalid Tox ID (bad checksum)"),
    STR("Error: Invalid Tox ID (bad nospam value)"),
    STR("Error: No memory"),

    //12
    STR("New file transfer"),
    STR("File transfer started"),
    STR(".."),
    STR("File transfer paused"),
    STR("File transfer broken"),
    STR("File transfer cancelled"),
    STR("File transfer complete"),

    //19
    STR("Call cancelled"),
    STR("Call invited"),
    STR("Call ringing"),
    STR("Call started"),

    //23
    STR("Add Friends"),
    STR("Tox ID"),
    STR("Message"),
    STR("Search friends"),
    STR("Add"),
    STR("Switch Profile"),
    STR("Friend Request"),
    STR("User Settings"),
    STR("Nombre"),
    STR("Status Message"),
    STR("Preview"),
    STR("Device Selection"),
    STR("Audio Input Device"),
    STR("Audio Output Device"),
    STR("Video Input Device"),
    STR("Other Settings"),
    STR("DPI"),
    STR("Save Location"),
    STR("Language"),

    //41
    STR("Copy"),
    STR("Cut"),
    STR("Paste"),
    STR("Delete"),
    STR("Select All"),
    STR("Remove"),
    STR("Leave"),
    STR("Accept"),
    STR("Ignore"),

    //50
    STR("Click to save"),
    STR("Click to open"),
    STR("Cancelled"),

    //53
    STR("English"),
    STR("French"),
    STR("Russian"),
    STR("Spanish"),
    STR("German"),
},

//GERMAN
{
    //0
    STR("Freundschaftsanfrage verschickt. Dein Freund wird online erscheinen, wenn er deine Anfrage akzeptiert."),
    STR("Versuche DNS Namen aufzulösen..."),
    STR("Fehler: Ungültige Tox ID"),
    STR("Fehler: Keine Tox ID angegeben"),
    STR("Fehler: Nachricht ist zu lang"),
    STR("Fehler: Leere Nachricht"),
    STR("Fehler: Tox ID ist eigene ID"),
    STR("Fehler: Tox ID wurde bereits in der Freundesliste"),
    STR("Fehler: Unbekannt"),
    STR("Fehler: Ungültige Tox ID (bad checksum)"),
    STR("Fehler: Ungültige Tox ID (bad nospam value)"),
    STR("Fehler: Kein Speicher"),

    //12
    STR("Neue Datenübertragung"),
    STR("Datenübertragung gestartet"),
    STR(".."),
    STR("Datenübertragung pausiert"),
    STR("Datenübertragung fehlerhaft"),
    STR("Datenübertragung abgebrochen"),
    STR("Datenübertragung fertig"),

    //19
    STR("Anruf abgebrochen"),
    STR("Anruf eingeladen"),
    STR("Anruf klingelt"),
    STR("Anruf gestartet"),

    //23
    STR("Freunde hinzufügen"),
    STR("Tox ID"),
    STR("Nachricht"),
    STR("Suche Freund"),
    STR("Hinzufügen"),
    STR("Profil wechseln"),
    STR("Freundesanfrage"),
    STR("Benutzereinstellungen"),
    STR("Name"),
    STR("Status Nachricht"),
    STR("Vorschau"),
    STR("Geräte Auswahl"),
    STR("Audio Quelle"),
    STR("Audio Ausgabegerät"),
    STR("Video Quelle"),
    STR("Andere Einstellungen"),
    STR("DPI"),
    STR("Speichere Standort Einstellung"),
    STR("Sprache"),

    //41
    STR("Kopieren"),
    STR("Ausschneiden"),
    STR("Einfügen"),
    STR("Löschen"),
    STR("Alle auswählen"),
    STR("Entfernen"),
    STR("Verlassen"),
    STR("Akzeptieren"),
    STR("Ignorieren"),

    //50
    STR("Klicke zum speichern"),
    STR("Klicke zum öffnen"),
    STR("Abgebrochen"),

    //53
    STR("Englisch"),
    STR("Französisch"),
    STR("Russisch"),
    STR("Spanisch"),
    STR("Deutsch"),
},

};

#undef STR
