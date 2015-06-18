#ifndef uTox_objc_main_h
#define uTox_objc_main_h

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

@class uToxStardustView;
extern CGDirectDisplayID desktop_capture_from;
extern CGRect desktop_capture_rect;
extern CGFloat desktop_capture_scale;
typedef struct {
    NSWindow *window;
    uToxStardustView *view;
    void (*finished_callback)(_Bool, uint64_t, NSWindow *);
} stardust_context_t;
extern stardust_context_t stardust_context;

void setup_cursors(void);

#define RELEASE_CHK(func, obj) if ((obj)) \
    func((obj));

//#define HAS_CUSTOM_EDIT_DRAW_IMPLEMENTATION

#define MAC_OS_AT_LEAST_DO(a, b, c) \
    if ([[NSProcessInfo processInfo] respondsToSelector:@selector(isOperatingSystemAtLeastVersion:)] && \
        [[NSProcessInfo processInfo] isOperatingSystemAtLeastVersion:(NSOperatingSystemVersion){a, b, c}])
#define AT_LEAST_YOSEMITE_DO      MAC_OS_AT_LEAST_DO(10, 10, 0)

// gotta use the old version checker here
#define AT_LEAST_MAVERICKS_DO     if (NSFoundationVersionNumber >= NSFoundationVersionNumber10_9)
#define AT_LEAST_MOUNTAIN_LION_DO if (NSFoundationVersionNumber >= NSFoundationVersionNumber10_8)

#define NSSTRING_FROM_LOCALIZED(msgid) [[[NSString alloc] initWithBytes:S(msgid) length:SLEN(msgid) encoding:NSUTF8StringEncoding] autorelease]


struct utox_native_image {
    CGImageRef image;
    double scale;
};

@class uToxView;
@interface uToxAppDelegate : NSResponder <NSApplicationDelegate, NSWindowDelegate> {
    NSMutableDictionary *devices;
    NSMutableDictionary *ironclad;
}
@property (retain) NSWindow *utox_window;
@property (retain) NSMenuItem *nameMenuItem;
@property (retain) NSMenuItem *statusMenuItem;
@property (retain) NSMenu *dockMenu;

- (uToxView *)mainView;
- (void)soilWindowContents;
@end

/* Webcam management */

@interface uToxAppDelegate (VideoDevices)
- (void *)storeVideoDevicesList;
- (AVCaptureDevice *)getCaptureDeviceFromHandle:(void *)handle;
@end

/* Video call satellite windows */

@interface uToxAppDelegate (IroncladManager)
- (void)setIroncladWindow:(NSWindow *)w forID:(uint32_t)id;
- (void)releaseIroncladWindowForID:(uint32_t)id;
- (NSWindow *)ironcladWindowForID:(uint32_t)id;
@end

/* Main UI */

@interface uToxView : NSView
@end

@interface uToxView (UserInteraction) <NSTextInputClient>
@end

/* Desktop rectangle selector */

@interface uToxStardustView : NSView
+ (NSWindow *)createWindowOnScreen:(NSScreen *)target;

@property (strong) NSTextField *instruction;
@property (getter=isVideo) BOOL video;
- (CGRect)getRect;
@end

/* Video capture */

@interface uToxAV : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
- (instancetype)initWithHandle:(void *)video_dev_handle NS_DESIGNATED_INITIALIZER;
@end

@interface uToxIroncladView : NSView
+ (NSWindow *)createWindow;

- (void)displayImage:(uint8_t *)rgba w:(uint16_t)width h:(uint16_t)height;
@end

#endif
