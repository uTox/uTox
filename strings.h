
#define STRLEN(x) (sizeof(x) - 1)

 char *addfriend_status_str[] = {
            "Friend request sent",
            "Error: Invalid ID format",
            "Error: Message too long",
            "Error: Empty message",
            "Error: ID is self",
            "Error: Friend request already sent",
            "Error: Unknown",
            "Error: Bad ID checksum",
            "Error: Bad ID nospam",
            "Error: No memory"};

uint16_t addfriend_status_length[] = {
            STRLEN("Friend request sent"),
            STRLEN("Error: Invalid ID format"),
            STRLEN("Error: Message too long"),
            STRLEN("Error: Empty message"),
            STRLEN("Error: ID is self"),
            STRLEN("Error: Friend request already sent"),
            STRLEN("Error: Unknown"),
            STRLEN("Error: Bad ID checksum"),
            STRLEN("Error: Bad ID nospam"),
            STRLEN("Error: No memory")};


#undef STRLEN
