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
    STR("Имя"),
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

};

#undef STR
