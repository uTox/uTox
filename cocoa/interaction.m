#include "../main.h"

int getbuf(char_t *ptr, size_t len, int value);

@implementation uToxView (UserInteraction)

+ (NSSpeechSynthesizer *)sharedSpeechSynthesizer {
    static NSSpeechSynthesizer *ss;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        ss = [[NSSpeechSynthesizer alloc] initWithVoice:[NSSpeechSynthesizer defaultVoice]];
    });
    return ss;
}

- (void)mouseDown:(NSEvent *)theEvent {
    NSLog(@"mouse down");
    panel_mdown(&panel_main);
}

- (void)mouseUp:(NSEvent *)theEvent {
    panel_mup(&panel_main);
}

- (void)rightMouseDown:(NSEvent *)theEvent {
    panel_mright(&panel_main);
}

- (void)rightMouseUp:(NSEvent *)theEvent {
    panel_mup(&panel_main);
}

- (void)mouseMoved:(NSEvent *)theEvent {
    panel_mmove(&panel_main, 0, 0, utox_window_width, utox_window_height, theEvent.locationInWindow.x, self.frame.size.height - theEvent.locationInWindow.y, theEvent.deltaX, theEvent.deltaY);
}

- (void)mouseDragged:(NSEvent *)theEvent {
    panel_mmove(&panel_main, 0, 0, utox_window_width, utox_window_height, theEvent.locationInWindow.x, self.frame.size.height - theEvent.locationInWindow.y, theEvent.deltaX, theEvent.deltaY);
}

- (void)rightMouseDragged:(NSEvent *)theEvent {
    panel_mmove(&panel_main, 0, 0, utox_window_width, utox_window_height, theEvent.locationInWindow.x, self.frame.size.height - theEvent.locationInWindow.y, theEvent.deltaX, theEvent.deltaY);
}

- (void)mouseEntered:(NSEvent *)theEvent {
    
}

- (void)mouseExited:(NSEvent *)theEvent {
    panel_mleave(&panel_main);
}

- (void)scrollWheel:(NSEvent *)theEvent {
    panel_mwheel(&panel_main, 0, 0, utox_window_width, utox_window_height, theEvent.deltaY);
}

- (void)updateTrackingAreas {
    [self removeTrackingArea:[self.trackingAreas firstObject]];
    NSTrackingArea *track = [[NSTrackingArea alloc] initWithRect:self.bounds options:NSTrackingMouseMoved | NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways owner:self userInfo:nil];
    [self addTrackingArea:track];
    [track release];
}

- (void)keyDown:(NSEvent *)theEvent {
    // option + [0-9] will jump to the n-th chat
    char n = 0;
    if (theEvent.charactersIgnoringModifiers.length == 1 && (theEvent.modifierFlags & NSDeviceIndependentModifierFlagsMask) == NSAlternateKeyMask) {
        switch (n = [theEvent.charactersIgnoringModifiers characterAtIndex:0]) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                list_selectchat(n - '0');
            default:
                break;
        }
    } else {
        // easier to let MacOS interpret
        [self interpretKeyEvents:@[theEvent]];
    }
}

#define BEEP_IF_EDIT_NOT_ACTIVE() if (!edit_active()) { \
    NSBeep(); \
    return; \
}

- (void)insertText:(id)insertString {
    BEEP_IF_EDIT_NOT_ACTIVE() 

    if ([insertString isKindOfClass:NSAttributedString.class])
        insertString = [insertString string];
    edit_paste((char_t *)[insertString UTF8String], [insertString lengthOfBytesUsingEncoding:NSUTF8StringEncoding], NO);
}

// TODO: NSTextInputClient
#define FLAGS() (([NSEvent modifierFlags] & NSCommandKeyMask)? 4 : 0) | (([NSEvent modifierFlags] & NSShiftKeyMask)? 1 : 0)
- (void)insertTab:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()

    edit_char(KEY_TAB, YES, FLAGS());
}

- (void)insertNewline:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()

    edit_char(KEY_RETURN, YES, FLAGS());
}

- (void)deleteBackward:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()

    edit_char(KEY_BACK, YES, FLAGS());
}

- (void)deleteForward:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()

    edit_char(KEY_DEL, YES, FLAGS());
}

- (void)moveLeft:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()

    edit_char(KEY_LEFT, YES, FLAGS());
}

- (void)moveRight:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()

    edit_char(KEY_RIGHT, YES, FLAGS());
}

- (void)moveUp:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()

    edit_char(KEY_UP, YES, FLAGS());
}

- (void)moveDown:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()

    edit_char(KEY_DOWN, YES, FLAGS());
}

- (void)selectAll:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()

    edit_char('A', YES, FLAGS());
}

- (void)cut:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()

    copy(0);
    [self delete:sender];
}

- (void)delete:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()

    /* TODO: no delete unless stuff is selected */
    if (edit_copy(NULL, 0))
        edit_char(KEY_DEL, YES, FLAGS());
}

#undef FLAGS
#undef BEEP_IF_EDIT_NOT_ACTIVE

- (void)paste:(id)sender {
    paste();
}

- (void)copy:(id)sender {
    copy(1);
}

- (void)focusPaneAddFriend:(id)sender {
    list_selectaddfriend();
}

- (void)focusPanePreferences:(id)sender {
    list_selectsettings();
}

- (void)createGroupchat:(id)sender {
    tox_postmessage(TOX_NEWGROUP, 1, 0, NULL);
}

- (void)startSpeaking:(id)sender {
    char_t *buf = malloc(65536);
    int len = getbuf(buf, 65536, 0);

    if (len != 0) {
        NSString *strtospk = [[NSString alloc] initWithBytes:buf length:len encoding:NSUTF8StringEncoding];

        [[uToxView sharedSpeechSynthesizer] startSpeakingString:strtospk];
        [strtospk release];
    }

    free(buf);
}

- (void)stopSpeaking:(id)sender {
    [[uToxView sharedSpeechSynthesizer] stopSpeaking];
}

@end

// FIXME: asda
static char_t clip_data[65536];

void setselection(char_t *data, STRING_IDX length) {
}

int getbuf(char_t *ptr, size_t len, int value) {
    int ret = 0;
    if (edit_active()) {
        // FIXME: asfasg
        ret = edit_copy(ptr, len);
    } else if(sitem->item == ITEM_FRIEND) {
        ret = messages_selection(&messages_friend, ptr, len, value);
    } else {
        ret = messages_selection(&messages_group, ptr, len, value);
    }
    
    return ret;
}

void copy(int value) {
    int len;
    NSString *strtocopy;

    len = getbuf(clip_data, sizeof(clip_data), value);

    strtocopy = [[NSString alloc] initWithBytes:clip_data length:len encoding:NSUTF8StringEncoding];
    if(len) {
        [[NSPasteboard generalPasteboard] clearContents];
        [[NSPasteboard generalPasteboard] writeObjects:@[strtocopy]];
    }
    [strtocopy release];
}

void paste(void) {
    NSPasteboard *pb = [NSPasteboard generalPasteboard];
    NSArray *arr = [pb readObjectsForClasses:@[NSImage.class, NSString.class] options:nil];
    id string_or_img = arr.firstObject;

    if ([string_or_img isKindOfClass:NSString.class]) {
        NSString *str = string_or_img;
        if (edit_active()) {
            edit_paste((char_t *)str.UTF8String, [str lengthOfBytesUsingEncoding:NSUTF8StringEncoding], 0);
        } else {
            NSBeep();
        }
    } else /* NSImage */ {
        // FIXME: implement
    }
}

void showkeyboard(_Bool show) {

}

@interface NSUserNotification (uToxAdditions)
- (void)set_identityImage:(id)arg1;
- (void)set_identityImageHasBorder:(BOOL)arg1;
@end

void notify(char_t *title, STRING_IDX title_length, char_t *msg, STRING_IDX msg_length, FRIEND *f) {
    if ([NSUserNotification class]) {
        NSUserNotification *usernotification = [[NSUserNotification alloc] init];
        NSString *t = [[NSString alloc] initWithBytes:title length:title_length encoding:NSUTF8StringEncoding];
        usernotification.title = t;

        NSString *msg_ = [[NSString alloc] initWithBytes:msg length:msg_length encoding:NSUTF8StringEncoding];
        usernotification.informativeText = msg_;

        if (friend_has_avatar(f)) {
            UTOX_NATIVE_IMAGE *im = f->avatar.image;
            size_t w = CGImageGetWidth(im->image) / im->scale,
                   h = CGImageGetHeight(im->image) / im->scale;
            NSImage *i = [[NSImage alloc] initWithCGImage:im->image size:(CGSize){w, h}];
            [usernotification set_identityImage:i];
            [usernotification set_identityImageHasBorder:YES];
            [i release];
        }

        [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:usernotification];

        [t release];
        [msg_ release];
        [usernotification release];
    }
}

void update_tray(void) {
    uToxAppDelegate *ad = (uToxAppDelegate *)[NSApplication sharedApplication].delegate;
    ad.nameMenuItem.title = [NSString stringWithCString:(char *)self.name encoding:NSUTF8StringEncoding];
    ad.statusMenuItem.title = [NSString stringWithCString:(char *)self.statusmsg encoding:NSUTF8StringEncoding];
}

/* file utils */

void savefilerecv(uint32_t fid, MSG_FILE *file) {
    NSSavePanel *picker = [NSSavePanel savePanel];
    picker.message = @"Where do you want to save \"%s\"?";
    int ret = [picker runModal];

    if (ret == NSFileHandlingPanelOKButton) {
        NSURL *destination = picker.URL;
        // FIXME: might be leaking
        tox_postmessage(TOX_ACCEPTFILE, fid, file->filenumber, strdup(destination.path.UTF8String));
    }
}

void savefiledata(MSG_FILE *file) {
    NSSavePanel *picker = [NSSavePanel savePanel];
    NSString *fname = [[NSString alloc] initWithBytesNoCopy:file->name length:file->name_length encoding:NSUTF8StringEncoding freeWhenDone:NO];
    picker.message = [NSString stringWithFormat:@"Where do you want to save \"%@\"?", fname];
    picker.nameFieldStringValue = fname;
    [fname release];
    int ret = [picker runModal];

    if (ret == NSFileHandlingPanelOKButton) {
        NSURL *destination = picker.URL;
        NSData *d = [NSData dataWithBytesNoCopy:file->path length:file->size freeWhenDone:NO];
        [d writeToURL:destination atomically:YES];

        free(file->path);
        file->path = (uint8_t*)strdup("inline.png");
        file->inline_png = 0;
    }
}

void openfilesend(void) {
    NSOpenPanel *picker = [NSOpenPanel openPanel];
    picker.title = @"Select one or more files to send.";
    picker.allowsMultipleSelection = YES;
    int ret = [picker runModal];

    if (ret == NSFileHandlingPanelOKButton) {
        NSArray *urls = picker.URLs;
        NSMutableString *s = [NSMutableString string];
        for (NSURL *url in urls) {
            [s appendFormat:@"%@\n", url.path];
        }
        tox_postmessage(TOX_SEND_NEW_FILE, (FRIEND*)sitem->data - friend, 0xFFFF, strdup(s.UTF8String));
    }
}

void openfileavatar(void) {
    NSOpenPanel *picker = [NSOpenPanel openPanel];
    picker.title = @"Select New Avatar";
    picker.allowedFileTypes = @[@"png"];
    int ret = [picker runModal];

    if (ret == NSFileHandlingPanelOKButton) {
        NSURL *url = picker.URL;
        uint32_t fsize = 0;
        void *file_data = file_raw((char *)url.path.UTF8String, &fsize);
        if (fsize > UTOX_AVATAR_MAX_DATA_LENGTH) {
            debug("avatar too big, show an error here i guess");
            free(file_data);
        } else {
            postmessage(SET_AVATAR, fsize, 0, file_data);
        }
    }
}