#ifndef NATIVE_CLIPBOARD_H
#define NATIVE_CLIPBOARD_H

/* value 0: copy
 * value 1: copy with names */
void copy(int value);
void paste(void);
void paste_as_quote(void);

#endif
