#include "main.h"

#include "../friend.h"
#include "../debug.h"
#include "../main.h"
#include "../settings.h"
#include "../tox.h"

#include "../../langs/i18n_decls.h"

#include "../av/utox_av.h"
#include "../av/video.h"

#include "../native/audio.h"
#include "../native/ui.h"
#include "../native/video.h"

#import <OpenGL/gl.h>
#import <OpenGL/glext.h>
#include <pthread.h>

/* MAJOR TODO: S FOR THIS FILE
 * - check clean up and error handling with AVFoundation code.
 */

#define SCREEN_VIDEO_DEVICE_HANDLE ((void *)1)

@implementation uToxAV {
    dispatch_queue_t          _processingQueue;
    AVCaptureSession *        _session;
    AVCaptureVideoDataOutput *_linkerVideo;

    CVImageBufferRef _currentFrame;
    pthread_mutex_t  _frameLock;
    BOOL             _shouldMangleDimensions;
}

- (instancetype)init {
    abort(); // never called, this is here to suppress a warning.
}

- (instancetype)initWithHandle:(void *)video_dev_handle {
    self = [super init];
    if (self) {
        pthread_mutex_init(&_frameLock, NULL);
        _processingQueue = dispatch_queue_create("uToxAV processing queue", DISPATCH_QUEUE_SERIAL);
        _session         = [[AVCaptureSession alloc] init];

        if (video_dev_handle == SCREEN_VIDEO_DEVICE_HANDLE) {
            AVCaptureScreenInput *input = [[AVCaptureScreenInput alloc] initWithDisplayID:desktop_capture_from];
            input.capturesCursor        = YES;
            input.capturesMouseClicks   = YES;
            input.cropRect              = desktop_capture_rect;
            input.scaleFactor           = 1.0 / desktop_capture_scale;

            [_session beginConfiguration];
            [_session addInput:input];
            //_session.sessionPreset = AVCaptureSessionPreset640x480;
            [_session commitConfiguration];

            // CGRect tr = CGRectIntegral(AVMakeRectWithAspectRatioInsideRect(desktop_capture_rect.size, (CGRect){0, 0,
            // 640, 480}));
            video_width             = desktop_capture_rect.size.width;
            video_height            = desktop_capture_rect.size.height;
            _shouldMangleDimensions = NO;

            [input release];
        } else {
            uToxAppDelegate *ad  = (uToxAppDelegate *)[NSApp delegate];
            AVCaptureDevice *dev = [[ad getCaptureDeviceFromHandle:video_dev_handle] retain];

            if (!dev) {
                return nil;
            }

            NSError *       error = NULL;
            AVCaptureInput *input = [[AVCaptureDeviceInput alloc] initWithDevice:dev error:&error];
            [_session beginConfiguration];
            [_session addInput:input];
            _session.sessionPreset = AVCaptureSessionPreset640x480;
            [_session commitConfiguration];

            // pray here
            // we make the assumption that AVFoundation will give us 640x480 video here
            // but if it doesn't we're going to segfault eventually.
            video_width             = 640;
            video_height            = 480;
            _shouldMangleDimensions = YES;

            [input release];
            [dev release];
        }
    }
    return self;
}

- (void)beginCappingFrames {
    _linkerVideo = [[AVCaptureVideoDataOutput alloc] init];
    [_linkerVideo setSampleBufferDelegate:self queue:_processingQueue];
    if (_shouldMangleDimensions) {
        [_linkerVideo setVideoSettings:@{
 (id)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_420YpCbCr8Planar),
           (id)kCVPixelBufferWidthKey : @640,
          (id)kCVPixelBufferHeightKey : @480
        }];
    } else {
        [_linkerVideo setVideoSettings:@{(id)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA) }];
    }
    [_session addOutput:_linkerVideo];
    [_session startRunning];
}

- (void)stopCappingFrames {
    [_session stopRunning];
    [_linkerVideo release];
}

- (void)captureOutput:(AVCaptureOutput *)captureOutput
didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
       fromConnection:(AVCaptureConnection *)connection {
           // NSLog(@"video frame available");

           pthread_mutex_lock(&_frameLock);
           CVImageBufferRef img = CMSampleBufferGetImageBuffer(sampleBuffer);
           if (!img) {
               NSLog(@"uToxAV WARNING: got bad sampleBuffer from avfoundation!!");
           } else {
               CVPixelBufferUnlockBaseAddress(_currentFrame, kCVPixelBufferLock_ReadOnly);
               RELEASE_CHK(CFRelease, _currentFrame);

               _currentFrame = (CVImageBufferRef)CFRetain(img);
               // we're not going to do anything to it, so it's safe to lock it always
               CVPixelBufferLockBaseAddress(_currentFrame, kCVPixelBufferLock_ReadOnly);
           }
           pthread_mutex_unlock(&_frameLock);
       }

- (BOOL)getCurrentFrameIntoChannelsY:(uint8_t *)y U:(uint8_t *)u V:(uint8_t *)v:(uint16_t)w:(uint16_t)h {
    if (!_currentFrame) {
        return NO;
    }

    pthread_mutex_lock(&_frameLock);
    CFRetain(_currentFrame);

    CFTypeID imageType = CFGetTypeID(_currentFrame);
    if (imageType == CVPixelBufferGetTypeID()) {
        // TODO maybe handle other formats
        if (CVPixelBufferGetPixelFormatType(_currentFrame) == kCVPixelFormatType_420YpCbCr8Planar) {
            uint8_t *yPlane = CVPixelBufferGetBaseAddressOfPlane(_currentFrame, 0);
            uint8_t *uPlane = CVPixelBufferGetBaseAddressOfPlane(_currentFrame, 1);
            uint8_t *vPlane = CVPixelBufferGetBaseAddressOfPlane(_currentFrame, 2);

            memcpy(y, yPlane, h * w);
            memcpy(u, uPlane, h * w / 4);
            memcpy(v, vPlane, h * w / 4);
        } else {
            bgrxtoyuv420(y, u, v, CVPixelBufferGetBaseAddress(_currentFrame), w, h);
        }
    } else if (imageType == CVOpenGLBufferGetTypeID()) {
        // OpenGL pbuffer
    } else if (imageType == CVOpenGLTextureGetTypeID()) {
        // OpenGL Texture (Do we need to handle these?)
    }

    CVPixelBufferRelease(_currentFrame);
    pthread_mutex_unlock(&_frameLock);
    return YES;
}

- (void)dealloc {
    [_session release];
    dispatch_release(_processingQueue);
    [super dealloc];
}

@end

@implementation uToxAppDelegate (VideoDevices)

    - (uint16_t)storeVideoDevicesList {
        if (devices) {
            [devices release];
        }
        device_count = 1; /* 1 for desktop */
        devices      = [[NSMutableDictionary alloc] init];

        utox_video_append_device(SCREEN_VIDEO_DEVICE_HANDLE, 1, STR_VIDEO_IN_DESKTOP, 0);

        NSArray *vdevIDs = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
        for (int i = 0; i < vdevIDs.count; i++) {
            AVCaptureDevice *dev  = vdevIDs[i];
            unsigned long    len  = strlen(dev.localizedName.UTF8String);
            char *           data = malloc(sizeof(void *) + len + 1);
            void *           ptr  = (void *)((uint64_t)i + 2);
            memcpy(data, &ptr, sizeof(void *));
            memcpy(data + sizeof(void *), dev.localizedName.UTF8String, len);
            data[sizeof(void *) + len] = 0;

            devices[@(i + 2)] = dev.uniqueID;

            utox_video_append_device(data, 0, data + sizeof(void *), 1);
            device_count++;
        }
        return (uint16_t)device_count;
    }

- (AVCaptureDevice *)getCaptureDeviceFromHandle:(void *)handle {
    NSString *s = devices[@((uintptr_t)handle)];
    if (!s) {
        return nil;
    }
    return [AVCaptureDevice deviceWithUniqueID:s];
}

@end

static uToxAV *   active_video_session  = NULL;
CGDirectDisplayID desktop_capture_from  = 0;
CGRect            desktop_capture_rect  = { 0 };
CGFloat           desktop_capture_scale = 1.0;

#ifdef UTOX_COCOA_BRAVE
#define AV_SESSION_CHK()
#else
#define AV_SESSION_CHK()                  \
    if (!active_video_session) {          \
        LOG_WARN("uToxAV", "no active video session"); \
        abort();                          \
    }
#endif

bool native_video_init(void *handle) {
    NSLog(@"using video: %p", handle);

    if (active_video_session) {
        LOG_ERR("Video", "overlapping video session!");
        abort();
    }

    active_video_session = [[uToxAV alloc] initWithHandle:handle];

    if (!active_video_session) {
        NSLog(@"Video initialization FAILED with handle %p. ", handle);
    }
    return active_video_session ? 1 : 0;
}

void native_video_close(void *handle) {
    [active_video_session release];

    active_video_session = nil;
}

bool native_video_startread(void) {
    AV_SESSION_CHK()

        [active_video_session beginCappingFrames];

    return 1;
}

bool native_video_endread(void) {
    AV_SESSION_CHK()

        [active_video_session stopCappingFrames];

    return 1;
}

int native_video_getframe(uint8_t *y, uint8_t *u, uint8_t *v, uint16_t width, uint16_t height) {
    AV_SESSION_CHK()

        return [active_video_session getCurrentFrameIntoChannelsY:y U:u V:v:width:height];
}

uint16_t native_video_detect(void) {
    uToxAppDelegate *ad = (uToxAppDelegate *)[NSApp delegate];
    return [ad storeVideoDevicesList];
}

// display video

@interface uToxIroncladVideoLayer : CAOpenGLLayer
@property uint8_t *temporaryLoadTexture;
@property int      temporaryWidth;
@property int      temporaryHeight;

@property GLuint texture;
@end

@implementation uToxIroncladVideoLayer

- (void)drawInCGLContext:(CGLContextObj)ctx
             pixelFormat:(CGLPixelFormatObj)pf
            forLayerTime:(CFTimeInterval)t
             displayTime:(const CVTimeStamp *)ts {

                 if (!self.texture) {
                     glEnable(GL_TEXTURE_2D);
                     glGenTextures(1, &_texture);
                     glBindTexture(GL_TEXTURE_2D, self.texture);

                     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
                     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
                     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                 }

                 if (self.temporaryLoadTexture) {
                     glBindTexture(GL_TEXTURE_2D, self.texture);
                     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, self.temporaryWidth, self.temporaryHeight, 0, GL_BGRA, GL_UNSIGNED_BYTE,
                             self.temporaryLoadTexture);
                     self.temporaryLoadTexture = NULL;
                 }

                 glMatrixMode(GL_PROJECTION);
                 glLoadIdentity();
                 glOrtho(0.0, 1.0, 1.0, 0.0, 0.0, 1.0);
                 glViewport(0, 0, self.bounds.size.width, self.bounds.size.height);
                 glMatrixMode(GL_MODELVIEW);

                 glBindTexture(GL_TEXTURE_2D, self.texture);
                 glClearColor(0.0, 0.0, 0.0, 1.0);
                 glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                 glColor4f(1.0, 1.0, 1.0, 1.0);

                 glEnableClientState(GL_VERTEX_ARRAY);
                 glEnableClientState(GL_TEXTURE_COORD_ARRAY);

                 static GLfloat payload[] = {
                     0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 1.0, 1.0, 0.0,
                 };

                 glVertexPointer(2, GL_FLOAT, 0, payload);
                 glTexCoordPointer(2, GL_FLOAT, 0, payload);
                 glDrawArrays(GL_TRIANGLES, 0, 6);
             }

@end

@implementation uToxIroncladVideoContent

- (instancetype)initWithFrame:(NSRect)frameRect {
  self = [super initWithFrame:frameRect];
  if (self) {
      self.layer       = [uToxIroncladVideoLayer layer];
      self.layer.frame = self.bounds;
      [(CAOpenGLLayer *)self.layer setAsynchronous:NO];
      self.wantsLayer = YES;
  }
  return self;
}

- (void)displayImage:(uint8_t *)rgba w:(uint16_t)width h:(uint16_t)height {
    // LOG_TRACE("Video", "wants image of %hu %hu", width, height);
    ((uToxIroncladVideoLayer *)self.layer).temporaryLoadTexture = rgba;
    ((uToxIroncladVideoLayer *)self.layer).temporaryWidth       = width;
    ((uToxIroncladVideoLayer *)self.layer).temporaryHeight      = height;

    [self.layer setNeedsDisplay];
    [self.layer displayIfNeeded];
}

- (void)checkSize {
    self.layer.bounds = self.frame;
}

- (BOOL)isOpaque {
    return YES;
}

@end

@interface uToxIroncladWindow : NSPanel
@property NSUInteger video_id;
// useless subclass, why isn't canBecomeKeyWindow assignable??
- (BOOL)canBecomeKeyWindow;
- (BOOL)canBecomeMainWindow;
@end

@implementation uToxIroncladWindow
- (BOOL)canBecomeKeyWindow {
    return NO;
}

- (BOOL)canBecomeMainWindow {
    return NO;
}
@end

@implementation uToxIroncladView {
    uToxIroncladVideoContent *__strong _videoContent;
}

+ (NSWindow *)createWindow {
#define START_RECT \
    (CGRect) { 0, 0, 100, 100 }
    NSWindow *ret = [[uToxIroncladWindow alloc] initWithContentRect:START_RECT
                                                          styleMask:NSHUDWindowMask |
                                                                    NSUtilityWindowMask |
                                                                    NSClosableWindowMask |
                                                                    NSTitledWindowMask |
                                                                    NSResizableWindowMask
                                                            backing:NSBackingStoreBuffered
                                                              defer:YES];
    ret.hidesOnDeactivate = NO;
    uToxIroncladView *iv  = [[self alloc] initWithFrame:ret.frame];
    ret.contentView       = iv;
    [iv release];
    return ret;
#undef START_RECT
}

- (instancetype)initWithFrame:(NSRect)frameRect {
  self = [super initWithFrame:frameRect];
  if (self) {
      _videoContent = [[uToxIroncladVideoContent alloc]
          initWithFrame:(CGRect){ 0, 0, frameRect.size.width, frameRect.size.height }];
      _videoContent.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
      [self addSubview:_videoContent];
  }
  return self;
}

- (void)resizeSubviewsWithOldSize:(NSSize)oldSize {
    [super resizeSubviewsWithOldSize:oldSize];
    [_videoContent checkSize];
}

- (void)displayImage:(uint8_t *)rgba w:(uint16_t)width h:(uint16_t)height {
    [_videoContent displayImage:rgba w:width h:height];
}

- (void)dealloc {
    [_videoContent release];
    [super dealloc];
}

@end

@implementation uToxAppDelegate (IroncladManager)

    - (void)setIroncladWindow:(NSWindow *)w forID:(uint32_t)id {
        ironclad[@(id)] = w;
        w.delegate      = self;
    }

- (void)releaseIroncladWindowForID:(uint32_t)id {
      [ironclad removeObjectForKey:@(id)];
}

- (NSWindow *)ironcladWindowForID:(uint32_t)id {
    return ironclad[@(id)];
}

- (void)windowWillClose:(NSNotification *)notification {
    if ([notification.object isKindOfClass:uToxIroncladWindow.class]) {
        switch (((uToxIroncladWindow *)notification.object).video_id) {
            case 0: {
                settings.video_preview = 0;
                video_end(0);
                postmessage_utoxav(UTOXAV_STOP_VIDEO, 1, 0, NULL);
                break;
            }

            default: {
                FRIEND *f = get_friend(((uToxIroncladWindow *)notification.object).video_id - 1);
                if (!f) {
                    LOG_ERR("Cocoa", "Could not get friend with number: %u",
                            ((uToxIroncladWindow *)notification.object).video_id - 1);
                    return;
                }

                postmessage_toxcore(TOX_CALL_DISCONNECT, f->number, 0, NULL);
                break;
            }
        }
        redraw();
    }
}

@end

void video_frame(uint32_t id, uint8_t *img_data, uint16_t width, uint16_t height, bool resize) {
    uToxAppDelegate *utoxapp = (uToxAppDelegate *)[NSApp delegate];
    NSWindow *        win    = [utoxapp ironcladWindowForID:id];
    uToxIroncladView *view   = win.contentView;

    if (!win) {
        LOG_WARN("Video", "BUG: video_frame called for bogus Ironclad id %lu", id);
    }

    CGSize s = view.videoSize;
    if (resize || s.width != width || s.height != height) {
        LOG_WARN("Video", "frame size changed, if this happens too often file a bug");

        CGFloat chrome_metric_w = win.frame.size.width - [win.contentView frame].size.width;
        CGFloat chrome_metric_h = win.frame.size.height - [win.contentView frame].size.height;
        int rswidth             = width + chrome_metric_w;
        int rsheight            = height + chrome_metric_h;
        [win setFrame:(CGRect) { win.frame.origin.x, CGRectGetMaxY(win.frame) - rsheight, rswidth, rsheight }
              display:YES
              animate:NO];
                      win.contentAspectRatio = (CGSize){ width, height };
                      view.videoSize         = (CGSize){ width, height };
    }

    [view displayImage:img_data w:width h:height];
}

void video_begin(uint32_t _id, char *name, uint16_t name_length, uint16_t width, uint16_t height) {
    if ([(uToxAppDelegate *)[NSApp delegate] ironcladWindowForID:_id])
        return;

    uToxIroncladWindow *video_win = (uToxIroncladWindow *)[uToxIroncladView createWindow];
    video_win.title = [[[NSString alloc] initWithBytes:name
                                                length:name_length
                                              encoding:NSUTF8StringEncoding]
                                           autorelease];
    video_win.video_id = _id;

    uToxAppDelegate *utoxapp = (uToxAppDelegate *)[NSApp delegate];
    NSWindow *utoxwin        = utoxapp.utox_window;

    CGFloat chrome_metric_w = video_win.frame.size.width - [video_win.contentView frame].size.width;
    CGFloat chrome_metric_h = video_win.frame.size.height - [video_win.contentView frame].size.height;
    int rswidth             = width + chrome_metric_w;
    int rsheight            = height + chrome_metric_h;

    [video_win setFrame:(CGRect) { CGRectGetMaxX(utoxwin.frame),
        CGRectGetMaxY(utoxwin.frame) - rsheight, rswidth, rsheight }
        display:YES];

    ((uToxIroncladView *)video_win.contentView).videoSize = (CGSize){ width, height };
    video_win.contentAspectRatio                          = (CGSize){ width, height };
    [utoxapp setIroncladWindow:video_win forID:_id];

    [video_win makeKeyAndOrderFront:utoxapp];
    [video_win release];
}

void video_end(uint32_t id) {
    uToxAppDelegate *utoxapp = (uToxAppDelegate *)[NSApp delegate];
    [utoxapp releaseIroncladWindowForID:id];
}
