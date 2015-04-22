#import "../main.h"

static inline CGRect CGRectCentreInRect(CGRect r1, CGRect r2) {
    return (CGRect){{(r2.size.width - r1.size.width) / 2.0, (r2.size.height - r1.size.height) / 2.0}, r1.size};
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
    NSWindow *ret = [[NSWindow alloc] initWithContentRect:CGRectMake(0, 0, target.frame.size.width, target.frame.size.height) styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:NO screen:target];

    ret.backgroundColor = [NSColor clearColor];
    ret.opaque = NO;
    ret.level = NSStatusWindowLevel;
    return ret;
}

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        returnRect = (CGRect){0};
        CGRect mainFrame = ((uToxAppDelegate *)[NSApp delegate]).utox_window.frame;
        CGRect posRect = CGRectOffset(CGRectCentreInRect((CGRect){0, 0, 480, 40}, mainFrame), mainFrame.origin.x, mainFrame.origin.y);
        NSLog(@"%@", NSStringFromRect(posRect));

        self.instruction = [[[NSTextField alloc] initWithFrame:posRect] autorelease];
        self.instruction.stringValue = @"Drag a box around the area of the screen you want to capture.";
        self.instruction.textColor = [NSColor whiteColor];
        self.instruction.bezeled = NO;
        self.instruction.drawsBackground = NO;
        self.instruction.editable = NO;
        self.instruction.font = [NSFont systemFontOfSize:16.0];
        self.instruction.alignment = NSCenterTextAlignment;
        [self addSubview:self.instruction];
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect {
    //CGRect draw = CGRectStandardize(returnRect);
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
    returnRect = CGRectIntegral(returnRect);
}

- (void)mouseDragged:(NSEvent *)theEvent {
    CGRect oldRect = CGRectStandardize(returnRect);
    returnRect.size = (CGSize){theEvent.locationInWindow.x - returnRect.origin.x, theEvent.locationInWindow.y - returnRect.origin.y};

    [self setNeedsDisplayInRect:oldRect];
    [self setNeedsDisplayInRect:CGRectStandardize(returnRect)];

    if ([self.subviews containsObject:self.instruction])
        [self.instruction removeFromSuperview];
}

- (void)mouseUp:(NSEvent *)theEvent {
    returnRect = CGRectIntegral(CGRectStandardize(returnRect));
    NSLog(@"%@", NSStringFromRect(returnRect));
    utoxshield_display_capping_done(self.isVideo, 0, self.window);
}

- (void)keyDown:(NSEvent *)theEvent {
    switch (theEvent.keyCode) {
        case kVK_Escape:
            utoxshield_display_capping_done(self.isVideo, 1, self.window);
            break;
        default:
            break;
    }
}

- (CGRect)getRect {
    return returnRect;
}

- (NSView *)hitTest:(NSPoint)aPoint {
    return self;
}

@end