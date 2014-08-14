#define STR(x) { .str = (uint8_t*)x, .length = sizeof(x) - 1 }

STRING strings[][64] = {
//Deutsch  GERMAN
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
    STR("Kopieren (no names)"),
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
},

//English  ENGLISH
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
    STR("Copy (no names)"),
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
},

//Spanish SPANISH
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
    STR("Copy (no names)"),
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
},

//French FRENCH
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
    STR("Copy (no names)"),
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
},

//Polski POLISH
{
    //0
    STR("Zapytanie zostało wysłane. Znajomy pojawi się online kiedy zaakceptuje zapytanie."),
    STR("Próba rozwiązania nazwy DNS..."),
    STR("Błąd: Niepoprawny Tox ID"),
    STR("Błąd: Nie określono Tox ID"),
    STR("Błąd: Wiadomość jest zbyt długa"),
    STR("Błąd: Pusta wiadomość"),
    STR("Błąd: Tox ID to twój ID"),
    STR("Błąd: Tox ID jest już na liście znajomych"),
    STR("Błąd: Nieznany"),
    STR("Błąd: Niepoprawny Tox ID (zła suma kontrolna)"),
    STR("Błąd: Niepoprawny Tox ID (zła wartość nospam)"),
    STR("Błąd: Brak pamięci"),

    //12
    STR("Nowy transfer pliku"),
    STR("Transfer pliku rozpoczęty"),
    STR(".."),
    STR("Transfer pliku wstrzymany"),
    STR("Transfer pliku nieudany"),
    STR("Transfer pliku anulowany"),
    STR("Transfer pliku zakończony"),

    //19
    STR("Rozmowa anulowana"),
    STR("Rozmowa przychodząca"),
    STR("Nawiązywanie połączenia"),
    STR("Rozmowa rozpoczęta"),

    //23
    STR("Dodaj znajomych"),
    STR("Tox ID"),
    STR("Wiadomość"),
    STR("Znajdź znajomych"),
    STR("Dodaj"),
    STR("Zmień profil"),
    STR("Zapytania znajomych"),
    STR("Ustawienia użytkownika"),
    STR("Nazwa"),
    STR("Status"),
    STR("Podgląd"),
    STR("Wybór urządzenia"),
    STR("Urządzenie wejściowe audio"),
    STR("Urządzenie wyjściowe audio"),
    STR("Urządzenie wejściowe wideo"),
    STR("Inne ustawienia"),
    STR("DPI"),
    STR("Save Location"),
    STR("Język"),

    //41
    STR("Kopiuj"),
    STR("Kopiuj (bez nazw)"),
    STR("Wytnij"),
    STR("Wklej"),
    STR("Usuń"),
    STR("Zaznacz wszystko"),
    STR("Usuń"),
    STR("Opuść"),
    STR("Zaakceptuj"),
    STR("Ignoruj"),

    //50
    STR("Przyciśnij by zapisać"),
    STR("Przyciśnij by otworzyć"),
    STR("Anulowano"),
},

//Русский RUSSIAN
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
    STR("Копировать (no names)"),
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
},
};

#undef STR
