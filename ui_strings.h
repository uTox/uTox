#define AFS(x) { .str = (uint8_t*)x, .length = sizeof(x) - 1 }

static STRING addstatus[] = {
    AFS("Friend request sent. Your friend will appear online when he accepts the request."),
    AFS("Attempting to resolve DNS name..."),
    AFS("Error: Invalid Tox ID"),
    AFS("Error: No Tox ID specified"),
    AFS("Error: Message is too long"),
    AFS("Error: Empty message"),
    AFS("Error: Tox ID is self ID"),
    AFS("Error: Tox ID is already in friend list"),
    AFS("Error: Unknown"),
    AFS("Error: Invalid Tox ID (bad checksum)"),
    AFS("Error: Invalid Tox ID (bad nospam value)"),
    AFS("Error: No memory")
};

STRING filestatus[] = {
    AFS("New file transfer"),
    AFS("File transfer started"),
    AFS("File transfer paused"),
    AFS("File transfer broken"),
    AFS("File transfer cancelled"),
    AFS("File transfer complete")
};

STRING callstatus[] = {
    AFS("Call cancelled"),
    AFS("Call invited"),
    AFS(".."),
    AFS("Call started")
};

#undef AFS
