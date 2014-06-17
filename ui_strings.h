#define AFS(x) { .str = (uint8_t*)x, .length = sizeof(x) - 1 }

static struct {
    uint8_t *str;
    uint16_t length;
} addstatus[] = {
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
