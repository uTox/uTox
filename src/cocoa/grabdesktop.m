#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>

#include "main.h"

#include "../flist.h"
#include "../friend.h"
#include "../ui.h"

#include "../av/utox_av.h"

#include "../native/keyboard.h"

static void stardust_display_capping_done(bool video, uint64_t ret, NSWindow *window);
static inline CGRect CGRectCentreInRect(CGRect r1, CGRect r2) {
    return (CGRect){ { (r2.size.width - r1.size.width) / 2.0, (r2.size.height - r1.size.height) / 2.0 }, r1.size };
}

@interface uToxStardustWindow : NSWindow
@end

@implementation uToxStardustWindow

- (NSRect)constrainFrameRect:(NSRect)frameRect toScreen:(NSScreen *)screen {
    return frameRect;
}

@end

@implementation uToxStardustView {
    CGRect returnRect;
}

+ (NSWindow *)createWindowOnScreen:(NSScreen *)target {
    NSWindow *ret =
        [[NSWindow alloc] initWithContentRect:CGRectMake(0, 0, target.frame.size.width, target.frame.size.height)
                                    styleMask:NSBorderlessWindowMask
                                      backing:NSBackingStoreBuffered
                                        defer:NO
                                       screen:target];

    ret.backgroundColor = [NSColor clearColor];
    ret.opaque          = NO;
    ret.level           = NSStatusWindowLevel;
    return ret;
}

// Hacky patch I stole from https://developer.apple.com/reference/uikit/nstextalignment/nstextalignmentcenter
#define NSTextAlignmentCenter 2


- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        returnRect       = (CGRect) { 0 };
        CGRect mainFrame = ((uToxAppDelegate *)[NSApp delegate]).utox_window.frame;
        CGRect posRect   = CGRectOffset(CGRectCentreInRect((CGRect){ 0, 0, 480, 40 }, mainFrame), mainFrame.origin.x,
                                      mainFrame.origin.y);
        NSLog(@"%@", NSStringFromRect(posRect));

        self.instruction                 = [[[NSTextField alloc] initWithFrame:posRect] autorelease];
        self.instruction.stringValue     = NSSTRING_FROM_LOCALIZED(SCREEN_CAPTURE_PROMPT);
        self.instruction.textColor       = [NSColor whiteColor];
        self.instruction.bezeled         = NO;
        self.instruction.drawsBackground = NO;
        self.instruction.editable        = NO;
        self.instruction.font            = [NSFont systemFontOfSize:16.0];
        self.instruction.alignment       = NSTextAlignmentCenter;
        [self addSubview:self.instruction];
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect {
    // CGRect draw = CGRectStandardize(returnRect);
    [[NSColor colorWithCalibratedRed:0 green:0 blue:0 alpha:0.6] set];
    NSRectFill(dirtyRect);

    if (returnRect.size.height != 0 || returnRect.size.width != 0) {
        CGContextRef c = [NSGraphicsContext currentContext].graphicsPort;
        CGContextSetBlendMode(c, kCGBlendModeClear);
        CGContextClearRect(c, returnRect);
    }
}

- (void)mouseDown:(NSEvent *)theEvent {
    returnRect.origin = theEvent.locationInWindow;
    returnRect        = CGRectIntegral(returnRect);
}

- (void)mouseDragged:(NSEvent *)theEvent {
    CGRect oldRect  = CGRectStandardize(returnRect);
    returnRect.size = (CGSize){ theEvent.locationInWindow.x - returnRect.origin.x,
                                theEvent.locationInWindow.y - returnRect.origin.y };

    [self setNeedsDisplayInRect:oldRect];
    [self setNeedsDisplayInRect:CGRectStandardize(returnRect)];

    if ([self.subviews containsObject:self.instruction]) {
        [self.instruction removeFromSuperview];
    }
}

- (void)mouseUp:(NSEvent *)theEvent {
    returnRect = CGRectIntegral(CGRectStandardize(returnRect));
    NSLog(@"%@", NSStringFromRect(returnRect));

    CGDirectDisplayID dispid = [self.window.screen.deviceDescription[@"NSScreenNumber"] unsignedIntegerValue];
    stardust_display_capping_done(self.isVideo, (uint64_t)dispid << 8, self.window);
}

- (void)keyDown:(NSEvent *)theEvent {
    // NSLog(@"%@", theEvent);
    switch (theEvent.keyCode) {
        case kVK_Escape: stardust_display_capping_done(self.isVideo, 1, self.window); break;
        default: break;
    }
}

- (CGRect)getRect {
    return returnRect;
}

- (NSView *)hitTest:(NSPoint)aPoint {
    return self;
}

@end

stardust_context_t stardust_context = { 0 };

static void stardust_display_capping_done(bool video, uint64_t ret, NSWindow *window) {
    uToxStardustView *v      = window.contentView;
    NSScreen *        target = window.screen;

    if ((ret & 0xff) == 0) {
        CGDirectDisplayID screen_id = (ret >> 8) & 0xffffffffffffffUL;

        if (!video) {
            CGRect rect         = [v getRect];
            rect.origin.y       = target.frame.size.height - rect.origin.y - rect.size.height;
            CGImageRef inliness = CGWindowListCreateImage(rect, kCGWindowListOptionOnScreenBelowWindow,
                                                          window.windowNumber, kCGWindowImageDefault);
            NATIVE_IMAGE *img = malloc(sizeof(NATIVE_IMAGE));
            img->scale        = 1.0;
            img->image        = inliness;

            CFMutableDataRef      dat  = CFDataCreateMutable(kCFAllocatorDefault, 0);
            CGImageDestinationRef dest = CGImageDestinationCreateWithData(dat, kUTTypePNG, 1, NULL);
            CGImageDestinationAddImage(dest, inliness, NULL);
            CGImageDestinationFinalize(dest);
            CFRelease(dest);

            size_t   size      = CFDataGetLength(dat);
            uint8_t *owned_ptr = malloc(size);
            memcpy(owned_ptr, CFDataGetBytePtr(dat), size);
            CFRelease(dat);

            friend_sendimage(flist_get_sel_friend(), img, CGImageGetWidth(inliness), CGImageGetHeight(inliness),
                             (UTOX_IMAGE)owned_ptr, size);
        } else {
            desktop_capture_from  = screen_id;
            CGRect rect           = [v getRect];
            desktop_capture_scale = [window backingScaleFactor];

            // for video, it must be divisible by 8 or we get distortion
            desktop_capture_rect = rect;
            desktop_capture_rect.size.width -= (uint32_t)desktop_capture_rect.size.width % 8;
            desktop_capture_rect.size.height -= (uint32_t)desktop_capture_rect.size.height % 8;
            postmessage_utoxav(UTOXAV_SET_VIDEO_IN, 0, 0, (void *)1);
        }
    }

    // CSA false positive: this has a +1 refcount from desktopgrab()
    [stardust_context.window release];

    stardust_context.window            = nil;
    stardust_context.view              = nil;
    stardust_context.finished_callback = NULL;
}

// do not breakpoint in this function or you're gonna have a fun time
void desktopgrab(bool video) {
    uToxAppDelegate *ad = (uToxAppDelegate *)[NSApp delegate];
    NSScreen *target    = [ad.utox_window screen];

    NSWindow *        window = [uToxStardustView createWindowOnScreen:target];
    uToxStardustView *v =
        [[uToxStardustView alloc] initWithFrame:(CGRect){ 0, 0, window.frame.size.width, window.frame.size.height }];
    v.video            = video;
    window.contentView = v;
    [window makeKeyAndOrderFront:ad];

    // dirty hack to get around a first responder issue
    stardust_context.window            = window;
    stardust_context.view              = v;
    stardust_context.finished_callback = &stardust_display_capping_done;

    [v release];
    postmessage_utoxav(UTOXAV_SET_VIDEO_IN, 1, 0, NULL);
}
