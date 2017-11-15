#include "main.h"
#include "cursor.h"

#include "../avatar.h"
#include "../chatlog.h"
#include "../file_transfers.h"
#include "../filesys.h"
#include "../flist.h"
#include "../friend.h"
#include "../groups.h"
#include "../main.h"
#include "../messages.h"
#include "../self.h"
#include "../settings.h"
#include "../stb.h"
#include "../text.h"
#include "../tox.h"
#include "../ui.h"
#include "../utox.h"

#include "../av/utox_av.h"

#include "../native/clipboard.h"
#include "../native/keyboard.h"
#include "../native/notify.h"
#include "../native/ui.h"

#include "../ui/edit.h"
#include "../ui/panel.h"

#include "../layout/background.h"
#include "../layout/friend.h"
#include "../layout/group.h"

NSCursor *cursors[8];
bool have_focus = false;

void setup_cursors(void) {
    cursors[CURSOR_NONE]     = [NSCursor arrowCursor];
    cursors[CURSOR_HAND]     = [NSCursor pointingHandCursor];
    cursors[CURSOR_SELECT]   = [NSCursor crosshairCursor];
    cursors[CURSOR_TEXT]     = [NSCursor IBeamCursor];
    cursors[CURSOR_ZOOM_IN]  = create_zoom_in_cursor();
    cursors[CURSOR_ZOOM_OUT] = create_zoom_out_cursor();
}

int getbuf(char *ptr, size_t len, int value);

// below comment applies too
static inline NSRange uToxRangeFromNSRange(NSRange utf16, NSString *s) {
    NSInteger start = [[s substringToIndex:utf16.location] lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    NSInteger len   = [[s substringWithRange:utf16] lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    return NSMakeRange(start, len);
}

static inline void utf8_correct(NSRange *r, char *s, size_t len) {
    while ((s[r->location] >> 6) == 2 && r->location > 0) {
        --r->location;
    }

    int ind = r->location + r->length - 1;
    while ((s[ind] >> 6) == 2 && ind < len) {
        ++ind;
    }
    r->length = ind - r->location;
}

// find a PANEL by bruteforcing the UI tree.
static int find_ui_object_recursive(const PANEL *root, const PANEL *target, int **outarrayptr, int n) {
    // if root == target and n == 0 (i.e. you initially called with the same ptr for root and target)
    // outarrayptr will be undefined, but the return will be 1
    // root and target being the same is a bug in the caller though, but i thought
    // you should know this.
    if (root == target) {
        // alloc the path at the end so we don't have to know the length first.
        // we're going to fill in this array backwards
        *outarrayptr = calloc(n + 1, sizeof(int));
        // NSLog(@"ALLOCATED IT AT %p %d", *outarrayptr, n);
        *outarrayptr += n;
        **outarrayptr = -1; // sentinel
        return 1;
    }

    PANEL **child = root->child;
    if (!child) {
        return 0;
    }

    do {
        if (!*child) {
            break;
        }

        int ret = find_ui_object_recursive(*child, target, outarrayptr, n + 1);
        if (ret) {
            (*outarrayptr)--;
            // when the initial call returns, *outarrayptr should be the same as the value
            // we got from calloc above. consider this comment a "soft assertion".
            **outarrayptr = child - root->child;
            return ret;
        }
    } while (++child);

    return 0;
}

#define _apply_generic_transform(type, a)                                     \
    {                                                                         \
        type *__   = (type *)a;                                               \
        int   relx = (a->x < 0) ? width + a->x : a->x;                        \
        int   rely = (a->y < 0) ? height + a->y : a->y;                       \
        x += relx;                                                            \
        y += rely;                                                            \
        width  = (__->width <= 0) ? width + __->width - relx : __->width;     \
        height = (__->height <= 0) ? height + __->height - rely : __->height; \
    }

static CGRect find_ui_object_in_window(const PANEL *ui) {
    int *  path     = NULL;
    CGRect ret      = CGRectZero;
    int    did_find = find_ui_object_recursive(&panel_root, ui, &path, 0);

    PANEL *ui_element = &panel_root;
    if (did_find) {
        int x = ui_element->x, y = ui_element->y, width = settings.window_width, height = settings.window_height;

        for (int i = 0; path[i] != -1; i++) {
            // LOG_TRACE("Interaction", "@: %d %d %d %d", x, y, width, height);
            // LOG_TRACE("Interaction", "%d %d %d %p", i, path[i], ui_element->child[path[i]]->type,
            // ui_element->child[path[i]]->content_scroll);

            ui_element = ui_element->child[path[i]];
            switch (ui_element->type) {
                case 6:
                    _apply_generic_transform(EDIT, ui_element);
                    height += SCALE(8); // seems to be the magic number
                    break;
                default: _apply_generic_transform(PANEL, ui_element); break;
            }
        }

        ret = CGRectMake(x, settings.window_height - height - y, width, height);
    }

    free(path);
    return ret;
}

static inline void move_left_to_char(char c) {
    EDIT *edit = edit_get_active();
    int   loc  = edit_getcursorpos();

    if (loc == 0) {
        return;
    }

    if (edit->data[loc - 1] == c) {
        loc--;
    }

    while (loc != 0 && edit->data[loc - 1] != c) {
        int move = utf8_unlen(edit->data + loc);
        loc -= move;
    }

    edit_setselectedrange(loc, 0);
    redraw();
}

static inline void select_left_to_char(char c) {
    EDIT *edit = edit_get_active();
    int   loc = edit_getcursorpos(), len = edit_selection(edit, NULL, 0);

    if (loc == 0) {
        return;
    }

    if (edit->data[loc - 1] == c) {
        loc--;
        len++;
    }

    while (loc != 0 && edit->data[loc - 1] != c) {
        int move = utf8_unlen(edit->data + loc);
        loc -= move;
        len += move;
    }

    edit_setselectedrange(loc, len);
    redraw();
}

static inline void move_right_to_char(char c) {
    EDIT *edit = edit_get_active();
    int   loc  = edit_getcursorpos();

    if (loc > edit->length) {
        return;
    }

    if (edit->data[loc] == c) {
        loc += 1;
    }

    while (loc != edit->length && edit->data[loc] != c) {
        loc += utf8_len(&edit->data[loc]);
    }

    edit_setselectedrange(loc, 0);
    redraw();
}

static inline void select_right_to_char(char c) {
    EDIT *edit = edit_get_active();
    int   loc = edit_getcursorpos(), end = loc + edit_selection(edit, NULL, 0);

    if (end > edit->length) {
        return;
    }

    if (edit->data[end] == c) {
        end += 1;
    }

    while (end != edit->length && edit->data[end] != c) {
        int move = utf8_len(edit->data + end);
        end += move;
    }

    edit_setselectedrange(loc, end - loc);
    redraw();
}

@implementation uToxView (UserInteraction)

+ (NSSpeechSynthesizer *)sharedSpeechSynthesizer {
    static NSSpeechSynthesizer *ss;
    static dispatch_once_t      onceToken;
    dispatch_once(&onceToken, ^{
      ss = [[NSSpeechSynthesizer alloc] initWithVoice:[NSSpeechSynthesizer defaultVoice]];
    });
    return ss;
}

- (void)mouseDown:(NSEvent *)theEvent {
    // NSLog(@"mouse down");
    panel_mdown(&panel_root);
    int tclk = 0;
    switch (theEvent.clickCount) {
        case 3: tclk = 1;
        case 2: panel_dclick(&panel_root, tclk);
        default: break;
    }
}

- (void)mouseUp:(NSEvent *)theEvent {
    panel_mup(&panel_root);
}

- (void)rightMouseDown:(NSEvent *)theEvent {
    panel_mright(&panel_root);
}

- (void)rightMouseUp:(NSEvent *)theEvent {
    panel_mup(&panel_root);
}

- (void)mouseMoved:(NSEvent *)theEvent {
    cursor = 0;
    panel_mmove(&panel_root, 0, 0, settings.window_width, settings.window_height, theEvent.locationInWindow.x,
                self.frame.size.height - theEvent.locationInWindow.y, theEvent.deltaX, theEvent.deltaY);
    [cursors[cursor] set];
}

- (void)mouseDragged:(NSEvent *)theEvent {
    cursor = 0;
    panel_mmove(&panel_root, 0, 0, settings.window_width, settings.window_height, theEvent.locationInWindow.x,
                self.frame.size.height - theEvent.locationInWindow.y, theEvent.deltaX, theEvent.deltaY);
    [cursors[cursor] set];
}

- (void)rightMouseDragged:(NSEvent *)theEvent {
    cursor = 0;
    panel_mmove(&panel_root, 0, 0, settings.window_width, settings.window_height, theEvent.locationInWindow.x,
                self.frame.size.height - theEvent.locationInWindow.y, theEvent.deltaX, theEvent.deltaY);
    [cursors[cursor] set];
}

- (void)mouseEntered:(NSEvent *)theEvent {
    [cursors[0] push];
}

- (void)mouseExited:(NSEvent *)theEvent {
    panel_mleave(&panel_root);
    [NSCursor pop];
}

- (void)scrollWheel:(NSEvent *)theEvent {
    panel_mwheel(&panel_root, 0, 0, settings.window_width, settings.window_height, theEvent.deltaY,
                 theEvent.hasPreciseScrollingDeltas);
}

- (void)updateTrackingAreas {
    [self removeTrackingArea:[self.trackingAreas firstObject]];
    NSTrackingArea *track = [[NSTrackingArea alloc]
        initWithRect:self.bounds
             options:NSTrackingMouseMoved | NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways
               owner:self
            userInfo:nil];
    [self addTrackingArea:track];
    [track release];
}

- (void)keyDown:(NSEvent *)theEvent {
    // option + [0-9] will jump to the n-th chat
    char n = 0;
    if (theEvent.charactersIgnoringModifiers.length == 1
        && (theEvent.modifierFlags & NSDeviceIndependentModifierFlagsMask) == NSControlKeyMask) {
        switch (n = [theEvent.charactersIgnoringModifiers characterAtIndex:0]) {
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                flist_selectchat(n - '1');
                redraw();
                break;
            case '0':
                flist_selectchat(9);
                redraw();
                break;
            default: goto defaultaction;
        }
    } else if (stardust_context.window && theEvent.keyCode == kVK_Escape) {
        stardust_context.finished_callback(stardust_context.view.isVideo, 1, stardust_context.window);
    } else {
    defaultaction:
        // easier to let MacOS interpret
        [self interpretKeyEvents:@[ theEvent ]];
    }
}

#define BEEP_IF_EDIT_NOT_ACTIVE() \
    if (!edit_active()) {         \
        NSBeep();                 \
        return;                   \
    }

- (void)insertText:(id)insertString {
    BEEP_IF_EDIT_NOT_ACTIVE()

    if ([insertString isKindOfClass:NSAttributedString.class]) {
        insertString = [insertString string];
    }

    edit_paste((char *)[insertString UTF8String], [insertString lengthOfBytesUsingEncoding:NSUTF8StringEncoding], NO);
}

// TODO: NSTextInputClient
#define FLAGS() \
    (([NSEvent modifierFlags] & NSCommandKeyMask) ? 4 : 0) | (([NSEvent modifierFlags] & NSShiftKeyMask) ? 1 : 0)
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

- (void) delete:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()

    if (edit_copy(NULL, 0)) {
        edit_char(KEY_DEL, YES, FLAGS());
    }
}

- (void)moveToBeginningOfDocument:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()

    edit_setselectedrange(0, 0);
    redraw();
}

- (void)moveToBeginningOfDocumentAndModifySelection:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()

    int loc = edit_getcursorpos(), len = edit_copy(NULL, 0);
    edit_setselectedrange(0, loc + len);
    redraw();
}

- (void)moveToEndOfDocument:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()

    edit_setselectedrange(edit_get_active()->length, 0);
    redraw();
}

- (void)moveToEndOfDocumentAndModifySelection:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()

    int loc = edit_getcursorpos(), len = edit_get_active()->length - loc;
    edit_setselectedrange(loc, len);
    redraw();
}

- (void)moveToBeginningOfLine:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()
    move_left_to_char('\n');
}

- (void)moveToBeginningOfLineAndModifySelection:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()
    select_left_to_char('\n');
}

- (void)moveToEndOfLine:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()
    move_right_to_char('\n');
}

- (void)moveToEndOfLineAndModifySelection:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()
    select_right_to_char('\n');
}

- (void)moveWordLeft:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()

    // FIXME: words are not always separated by a space
    move_left_to_char(' ');
}

- (void)moveWordLeftAndModifySelection:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()

    // FIXME: words are not always separated by a space
    select_left_to_char(' ');
}

- (void)moveWordRight:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()

    // FIXME: words are not always separated by a space
    move_right_to_char(' ');
}

- (void)moveWordRightAndModifySelection:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()

    // FIXME: words are not always separated by a space
    select_right_to_char(' ');
}

- (void)moveLeftAndModifySelection:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()

    if (edit_getcursorpos() == 0) {
        return;
    }

    int loc = edit_getcursorpos() - 1, len = edit_copy(NULL, 0) + 1;
    while (edit_get_active()->data[loc] >> 6 == 2) {
        loc -= 1;
        len += 1;
    }
    edit_setselectedrange(loc, len);
    redraw();
}

- (void)moveRightAndModifySelection:(id)sender {
    BEEP_IF_EDIT_NOT_ACTIVE()

    int loc = edit_getcursorpos(), len = edit_copy(NULL, 0);
    if (loc + len > edit_get_active()->length) {
        return;
    }

    int l = utf8_len(&edit_get_active()->data[loc + len]);
    len += l;
    edit_setselectedrange(loc, len);
    redraw();
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
    flist_selectaddfriend();
    redraw();
}

- (void)focusPanePreferences:(id)sender {
    flist_selectsettings();
    redraw();
}

- (void)createGroupchat:(id)sender {
    postmessage_toxcore(TOX_GROUP_CREATE, 1, 0, NULL);
}

- (void)tabPrevFriend:(id)sender {
    flist_previous_tab();
    redraw();
}

- (void)tabNextFriend:(id)sender {
    flist_next_tab();
    redraw();
}

- (void)startSpeaking:(id)sender {
    char *buf = malloc(65536);
    int   len = getbuf(buf, 65536, 0);

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

- (NSString *)copyEditContents {
    NSString *strtocopy;
    char *    buf = malloc(65536);
    int       len = getbuf(buf, 65536, 0);
    strtocopy     = [[NSString alloc] initWithBytes:buf length:len encoding:NSUTF8StringEncoding];
    free(buf);
    return strtocopy;
}

#pragma mark - NSTextInputClient

- (void)insertText:(id)aString replacementRange:(NSRange)replacementRange {
    // Get a valid range
    if (replacementRange.location == NSNotFound) {
        NSRange markedRange = self.markedRange;
        if (markedRange.location != NSNotFound) {
            replacementRange = markedRange;
        } else {
            replacementRange = self.selectedRange;
        }
    }

    if ([aString isKindOfClass:[NSAttributedString class]]) {
        aString = [aString string];
    }

    edit_setselectedrange(replacementRange.location, replacementRange.length);
    uint16_t insl = [aString lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    edit_paste((char *)[aString UTF8String], insl, 0);

    [self unmarkText];
}

- (NSArray *)validAttributesForMarkedText {
    return @[];
}

- (BOOL)hasMarkedText {
    return edit_getmark(NULL, NULL);
}

- (NSRange)markedRange {
    uint16_t loc, len;
    BOOL     valid = edit_getmark(&loc, &len);
    if (!valid) {
        return (NSRange){ NSNotFound, 0 };
    } else {
        return (NSRange){ loc, len };
    }
}

- (void)unmarkText {
    edit_setmark(0, 0);
}

- (NSRange)selectedRange {
    if (!edit_active()) {
        return (NSRange){ NSNotFound, 0 };
    } else {
        return (NSRange){ edit_getcursorpos(), edit_selection(edit_get_active(), NULL, 0) };
    }
}

- (void)setMarkedText:(id)aString selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange {
    NSLog(@"%@", NSStringFromRange(replacementRange));
    uint16_t loc, len;
    BOOL     valid;
    if ((valid = edit_getmark(&loc, &len)) && replacementRange.location != NSNotFound) {
        replacementRange.location += loc;
    } else if (valid) {
        replacementRange = NSMakeRange(loc, len);
        NSLog(@"valid=1 replace %d %d", loc, len);
    } else {
        replacementRange = NSMakeRange(edit_getcursorpos(), edit_selection(edit_get_active(), NULL, 0));
    }

    if ([aString isKindOfClass:[NSAttributedString class]]) {
        aString = [aString string];
    }

    edit_setselectedrange(replacementRange.location, replacementRange.length);
    uint16_t insl = [aString lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
    if (!insl) {
        edit_char(KEY_DEL, YES, 0);
    } else {
        edit_paste((char *)[aString UTF8String], insl, 0);
    }

    if ([aString length] == 0) {
        [self unmarkText];
    } else {
        edit_setmark(replacementRange.location, insl);
    }

    NSRange selRanged = uToxRangeFromNSRange(selectedRange, aString);
    edit_setselectedrange(replacementRange.location + selRanged.location, selRanged.length);

    NSLog(@"%@", NSStringFromRange(selRanged));
}

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange {
    char *buf = malloc(65536);
    int   len = getbuf(buf, 65536, 0);

    if (aRange.location >= len) {
        free(buf);
        return nil;
    }

    if (aRange.location + aRange.length > len) {
        aRange.length = len - aRange.location;
    }

    utf8_correct(&aRange, buf, len);

    if (len == 0) {
        free(buf);
        return nil;
    }

    NSString *s =
        [[NSString alloc] initWithBytes:buf + aRange.location length:aRange.length encoding:NSUTF8StringEncoding];
    NSAttributedString *a = [[NSAttributedString alloc] initWithString:s attributes:nil];

    free(buf);
    [s release];
    return [a autorelease];
}

- (NSUInteger)characterIndexForPoint:(NSPoint)aPoint {
    NSLog(@"WARNING: unimplemented characterIndexForPoint:%@", NSStringFromPoint(aPoint));
    NSLog(@"If you see this message, file a bug about \"characterIndexForPoint:\" !");
    return 0;
}

- (NSRect)firstRectForCharacterRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange {
    CGRect loc = find_ui_object_in_window(&edit_get_active()->panel);
    // NSLog(@"%@", NSStringFromRect(loc));
    return [self.window convertRectToScreen:loc];
}

@end

// FIXME: asda
static char clip_data[65536];

void setselection(char *data, uint16_t length) {}

int getbuf(char *ptr, size_t len, int value) {
    int ret = 0;
    if (edit_active()) {
        // FIXME: asfasg
        ret = edit_copy(ptr, len);
    } else if (flist_get_type() == ITEM_FRIEND) {
        ret = messages_selection(&messages_friend, ptr, len, value);
    } else {
        ret = messages_selection(&messages_group, ptr, len, value);
    }

    return ret;
}

void copy(int value) {
    int       len;
    NSString *strtocopy;

    len = getbuf(clip_data, sizeof(clip_data), value);

    strtocopy = [[NSString alloc] initWithBytes:clip_data length:len encoding:NSUTF8StringEncoding];
    if (len) {
        [[NSPasteboard generalPasteboard] clearContents];
        [[NSPasteboard generalPasteboard] writeObjects:@[ strtocopy ]];
    }
    [strtocopy release];
}

void paste(void) {
    NSPasteboard *pb       = [NSPasteboard generalPasteboard];
    NSArray *arr           = [pb readObjectsForClasses:@[ NSImage.class, NSString.class ] options:nil];
    id       string_or_img = arr.firstObject;

    if ([string_or_img isKindOfClass:NSString.class]) {
        NSString *str = string_or_img;
        if (edit_active()) {
            edit_paste((char *)str.UTF8String, [str lengthOfBytesUsingEncoding:NSUTF8StringEncoding], 0);
        } else {
            NSBeep();
        }
    } else /* NSImage */ {
        [string_or_img lockFocus];
        NSBitmapImageRep *bmp =
            [[NSBitmapImageRep alloc] initWithFocusedViewRect:(CGRect){ CGPointZero, [string_or_img size] }];
        [string_or_img unlockFocus];

        CGImageRef    img = CGImageRetain(bmp.CGImage);
        NATIVE_IMAGE *i   = malloc(sizeof(NATIVE_IMAGE));
        i->scale          = 1.0;
        i->image          = img;

        CFMutableDataRef      dat  = CFDataCreateMutable(kCFAllocatorDefault, 0);
        CGImageDestinationRef dest = CGImageDestinationCreateWithData(dat, kUTTypePNG, 1, NULL);
        CGImageDestinationAddImage(dest, img, NULL);
        CGImageDestinationFinalize(dest);
        CFRelease(dest);

        size_t   size      = CFDataGetLength(dat);
        uint8_t *owned_ptr = malloc(size);

        if (owned_ptr) {
            memcpy(owned_ptr, CFDataGetBytePtr(dat), size);
            friend_sendimage(flist_get_friend(), i, CGImageGetWidth(img), CGImageGetHeight(img), (UTOX_IMAGE)owned_ptr,
                             size);
        } else {
            free(i);
            NSLog(@"ran out of memory, we will just do nothing and hope user doesn't notice because we're probably not "
                  @"the only process being screwy");
        }

        CFRelease(dat);
        [bmp release];
    }
}

void showkeyboard(bool show) {}

void edit_will_deactivate(void) {
    uToxAppDelegate *ad = (uToxAppDelegate *)[NSApplication sharedApplication].delegate;
    [ad.mainView.inputContext discardMarkedText];
    [ad.mainView unmarkText];
}

@interface NSUserNotification (uToxAdditions)
- (void)set_identityImage:(id)arg1;
- (void)set_identityImageHasBorder:(BOOL)arg1;
@end

void notify(char *title, uint16_t title_length, const char *msg, uint16_t msg_length, void *object, bool is_group) {
    if ([NSUserNotification class]) {
        NSUserNotification *usernotification = [[NSUserNotification alloc] init];
        NSString *t = [[NSString alloc] initWithBytes:title length:title_length encoding:NSUTF8StringEncoding];
        usernotification.title = t;

        NSString *msg_ = [[NSString alloc] initWithBytes:msg length:msg_length encoding:NSUTF8StringEncoding];
        usernotification.informativeText = msg_;

        if (!is_group) {
            FRIEND *f = object;
            if (friend_has_avatar(f)) {
                NATIVE_IMAGE *im = f->avatar->img;
                size_t        w = CGImageGetWidth(im->image) / im->scale, h = CGImageGetHeight(im->image) / im->scale;
                NSImage *i = [[NSImage alloc] initWithCGImage:im->image size:(CGSize){ w, h }];
                if ([usernotification respondsToSelector:@selector(set_identityImage:)]) {
                    [usernotification set_identityImage:i];
                } else {
                    NSLog(@"WARNING: OS X has broken the private api I use to set notification avatars. "
                           "If you see this message please update uTox (if you're on latest, file a bug)");
                }
                if ([usernotification respondsToSelector:@selector(set_identityImageHasBorder:)]) {
                    [usernotification set_identityImageHasBorder:YES];
                } else {
                    NSLog(@"WARNING: OS X has broken the private api I use to set notification avatars. "
                           "If you see this message please update uTox (if you're on latest, file a bug)");
                }
                [i release];
            }
        }

        [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:usernotification];

        [t release];
        [msg_ release];
        [usernotification release];
    }

    if ([NSApplication sharedApplication].isActive) {
        have_focus = true;
    } else {
        // Bounce icon.
        [[NSApplication sharedApplication] requestUserAttention:NSInformationalRequest];
        [NSApplication sharedApplication].dockTile.badgeLabel = @"!";
        have_focus = false;
    }
}

void update_tray(void) {
    uToxAppDelegate *ad = (uToxAppDelegate *)[NSApplication sharedApplication].delegate;
    ad.nameMenuItem.title =
        [[[NSString alloc] initWithBytes:self.name length:self.name_length encoding:NSUTF8StringEncoding] autorelease];
    ad.statusMenuItem.title =
        [[[NSString alloc] initWithBytes:self.statusmsg length:self.statusmsg_length encoding:NSUTF8StringEncoding]
            autorelease];
}

/* file utils */

void native_export_chatlog_init(uint32_t fid) {

    FRIEND *f = get_friend(fid);
    if (!f) {
        LOG_ERR("Cocoa", "Could not get friend with number: %u", fid);
        return;
    }

    NSSavePanel *picker = [NSSavePanel savePanel];
    NSString *fname = [[NSString alloc]
        initWithBytesNoCopy:f->name
        length:f->name_length
        encoding:NSUTF8StringEncoding
        freeWhenDone:NO];

    picker.message = [NSString stringWithFormat:NSSTRING_FROM_LOCALIZED(WHERE_TO_SAVE_FILE_PROMPT),
            f->name_length,
            f->name];

    picker.nameFieldStringValue = fname;
    [fname release];
    int ret = [picker runModal];

    if (ret == NSFileHandlingPanelOKButton) {
        NSURL *destination = picker.URL;
        FILE *file = utox_get_file_simple(destination.path.UTF8String, UTOX_FILE_OPTS_WRITE | UTOX_FILE_OPTS_MKDIR);
        if (!file) {
            LOG_ERR("Cocoa", "Could not write to file: %s", destination.path.UTF8String);
            return;
        }
        utox_export_chatlog(f->id_str, file);
    }
}

void native_select_dir_ft(uint32_t fid, uint32_t num, FILE_TRANSFER *file) {
    NSSavePanel *picker = [NSSavePanel savePanel];
    NSString    *fname  = [[NSString alloc] initWithBytesNoCopy:file->name
                                                         length:file->name_length
                                                       encoding:NSUTF8StringEncoding
                                                   freeWhenDone:NO];
    picker.message = [NSString stringWithFormat:NSSTRING_FROM_LOCALIZED(WHERE_TO_SAVE_FILE_PROMPT),
                               file->name_length,
                               file->name];
    picker.nameFieldStringValue = fname;
    [fname release];
    int ret = [picker runModal];

    if (ret == NSFileHandlingPanelOKButton) {
        NSURL *destination = picker.URL;
        // FIXME: might be leaking
        postmessage_toxcore(TOX_FILE_ACCEPT, fid, num, strdup(destination.path.UTF8String));
    }
}

void native_autoselect_dir_ft(uint32_t fid, FILE_TRANSFER *file) {
    NSString *downloads =
        [NSSearchPathForDirectoriesInDomains(NSDownloadsDirectory, NSUserDomainMask, YES) firstObject];
    NSString *fname = [[NSString alloc] initWithBytesNoCopy:file->name
                                                     length:file->name_length
                                                   encoding:NSUTF8StringEncoding
                                               freeWhenDone:NO];

    NSString *dest = [downloads stringByAppendingPathComponent:fname];
    [fname release];

    FILE *f = fopen(dest, "wb");
    postmessage_toxcore(TOX_FILE_ACCEPT_AUTO, fid, file->file_number, f);
}

//@"Where do you want to save \"%.*s\"?"
void file_save_inline_image_png(MSG_HEADER *msg) {
    NSSavePanel *picker = [NSSavePanel savePanel];
    NSString *fname =
        [[NSString alloc] initWithBytes:msg->via.ft.name length:msg->via.ft.name_length encoding:NSUTF8StringEncoding];
    picker.message = [NSString
        stringWithFormat:NSSTRING_FROM_LOCALIZED(WHERE_TO_SAVE_FILE_PROMPT), msg->via.ft.name_length, msg->via.ft.name];
    picker.nameFieldStringValue = fname;
    [fname release];
    int ret = [picker runModal];

    if (ret == NSFileHandlingPanelOKButton) {
        NSURL  *destination = picker.URL;
        NSData *d = [NSData dataWithBytesNoCopy:msg->via.ft.data length:msg->via.ft.data_size freeWhenDone:NO];
        [d writeToURL:destination atomically:YES];

        snprintf((char *)msg->via.ft.path, UTOX_FILE_NAME_LENGTH, "inline.png"); // TODO : this seems wrong
        msg->via.ft.inline_png = false;
    }
}
//@"Select one or more files to send."
void openfilesend(void) {
    NSOpenPanel *picker            = [NSOpenPanel openPanel];
    picker.title                   = NSSTRING_FROM_LOCALIZED(SEND_FILE);
    picker.message                 = NSSTRING_FROM_LOCALIZED(SEND_FILE_PROMPT);
    picker.allowsMultipleSelection = YES;
    int ret                        = [picker runModal];

    if (ret == NSFileHandlingPanelOKButton) {
        NSArray *urls = picker.URLs;
        FRIEND *f = flist_get_friend();
        if (!f) {
            LOG_ERR("Cocoa", "Could not get friend.");
            return;
        }

        for (NSURL *url in urls) {
            UTOX_MSG_FT *msg = calloc(1, sizeof(UTOX_MSG_FT));
            if (!msg) {
                LOG_ERR("Cocoa", "Failed to malloc for file sending.");
                return;
            }
            msg->file = fopen(url.path.UTF8String, "r");
            msg->name = (uint8_t*)strdup(url.path.UTF8String);
            postmessage_toxcore(TOX_FILE_SEND_NEW, f->number, 0, msg);
            LOG_INFO("Cocoa", "File %s sent!", url.path.UTF8String);
        }
    }
}

void show_alert_modal(char *title_str, uint16_t title_str_length, char *message, uint16_t message_length) {
    NSString *bytess = [[NSString alloc] initWithBytes:message   length:message_length   encoding:NSUTF8StringEncoding];
    NSString *title  = [[NSString alloc] initWithBytes:title_str length:title_str_length encoding:NSUTF8StringEncoding];

    NSString *emsg = [[NSString alloc] initWithFormat:@"%@%@", title, bytess];
    [title release];
    [bytess release];

    NSAlert *alert    = [[NSAlert alloc] init];
    alert.alertStyle  = NSWarningAlertStyle;
    alert.messageText = emsg;

    [emsg release];
    [alert runModal];
    [alert release];
}

void openfileavatar(void) {
    NSOpenPanel *picker = [NSOpenPanel openPanel];

    picker.title = [[[NSString alloc] initWithBytes:S(SELECT_AVATAR_TITLE)
                                             length:SLEN(SELECT_AVATAR_TITLE)
                                           encoding:NSUTF8StringEncoding] autorelease];
    picker.allowedFileTypes = @[ @"png" ];
    int ret                 = [picker runModal];

    if (ret == NSFileHandlingPanelOKButton) {
        int width, height, bpp, size;
        uint8_t *file_data = stbi_load((char *)picker.URL.path.UTF8String, &width, &height, &bpp, 0);
        uint8_t *img = stbi_write_png_to_mem(file_data, 0, width, height, bpp, &size);
        free(file_data);

        if (!img) {
            show_alert_modal(S(CANT_FIND_FILE_OR_EMPTY), SLEN(CANT_FIND_FILE_OR_EMPTY),
                             (char *)picker.URL.path.UTF8String, sizeof((char *)picker.URL.path.UTF8String));
            return;
        }

        if (size > UTOX_AVATAR_MAX_DATA_LENGTH) {
            free(img);

            char size_str[16];
            int  len = sprint_humanread_bytes(size_str, sizeof(size_str), UTOX_AVATAR_MAX_DATA_LENGTH);

            show_alert_modal(S(AVATAR_TOO_LARGE_MAX_SIZE_IS), SLEN(AVATAR_TOO_LARGE_MAX_SIZE_IS), size_str, len);
        } else {
            postmessage_utox(SELF_AVATAR_SET, size, 0, img);
        }
    }
}
