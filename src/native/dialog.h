#ifndef NATIVE_DIALOG_H
#define NATIVE_DIALOG_H

// Linux, OS X, and Windows.
void openfilesend(void);

// Use the file picker to select an avatar and set it as the user's.
// Linux, OS X, and Windows.
void openfileavatar(void);

/**
 * @brief Show a platform-independent message box.
 */
void show_messagebox(const char *caption, uint16_t caption_length, const char *message, uint16_t message_length);

#endif
