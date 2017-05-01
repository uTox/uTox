#include "main.h"

#include "../commands.h"
#include "../debug.h"
#include "../filesys.h"
#include "../flist.h"
#include "../main.h"
#include "../macros.h"
#include "../settings.h"
#include "../theme.h"
#include "../tox.h"
#include "../ui.h"
#include "../utox.h"

#include "../av/utox_av.h"
#include "../av/video.h"

#include "../native/notify.h"

#include "../ui/dropdown.h"

#include "../layout/settings.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

#include <libgen.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

struct thread_call {
    void *(*func)(void *);
    void *argp;
};

#define DEFAULT_WIDTH (382 * DEFAULT_SCALE)
#define DEFAULT_HEIGHT (320 * DEFAULT_SCALE)

int NATIVE_IMAGE_IS_VALID(NATIVE_IMAGE *img) {
    return img != NULL && img->image != nil;
}

NATIVE_IMAGE *utox_image_to_native(const UTOX_IMAGE data, size_t size, uint16_t *w, uint16_t *h, bool keep_alpha) {
    CFDataRef         idata_copy     = CFDataCreate(kCFAllocatorDefault, data, size);
    CGDataProviderRef src            = CGDataProviderCreateWithCFData(idata_copy);
    CGImageRef        underlying_img = CGImageCreateWithPNGDataProvider(src, NULL, YES, kCGRenderingIntentDefault);
    CGDataProviderRelease(src);
    CFRelease(idata_copy);

    if (underlying_img) {
        *w                = CGImageGetWidth(underlying_img);
        *h                = CGImageGetHeight(underlying_img);
        NATIVE_IMAGE *ret = malloc(sizeof(NATIVE_IMAGE));
        ret->scale        = 1.0;
        ret->image        = underlying_img;
        return ret;
    } else {
        return NULL;
    }
}

void image_set_filter(NATIVE_IMAGE *image, uint8_t filter) {}

void image_set_scale(NATIVE_IMAGE *image, double scale) {
    image->scale = scale;
}

void image_free(NATIVE_IMAGE *img) {
    if (!img) {
        return;
    }
    CGImageRelease(img->image);
    free(img);
}

void *thread_trampoline(void *call) {
    struct thread_call args = *(struct thread_call *)call;
    free(call);

    @autoreleasepool {
        return args.func(args.argp);
    }
}

void thread(void func(void *), void *args) {
    pthread_t      thread_temp;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 1 << 20);

    struct thread_call *call = malloc(sizeof(struct thread_call));
    if (!call) {
        fputs("thread(): no memory so gonna peace", stderr);
        abort();
    }

    call->func = func;
    call->argp = args;

    pthread_create(&thread_temp, &attr, thread_trampoline, call);
    pthread_attr_destroy(&attr);
}

void yieldcpu(uint32_t ms) {
    usleep(1000 * ms);
}

/* *** audio/video *** */
void audio_detect(void) {}

bool audio_init(void *handle) {
    return 0;
}

bool audio_close(void *handle) {
    return 0;
}

bool audio_frame(int16_t *buffer) {
    return 0;
}

/* *** os *** */

#include <mach/clock.h>
#include <mach/mach.h>

uint64_t get_time(void) {
    struct timespec ts;
    clock_serv_t    muhclock;
    mach_timespec_t machtime;
    host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &muhclock);
    clock_get_time(muhclock, &machtime);
    mach_port_deallocate(mach_task_self(), muhclock);
    ts.tv_sec  = machtime.tv_sec;
    ts.tv_nsec = machtime.tv_nsec;

    return ((uint64_t)ts.tv_sec * (1000 * 1000 * 1000)) + (uint64_t)ts.tv_nsec;
}

void config_osdefaults(UTOX_SAVE *r) {
    r->window_x      = 0;
    r->window_y      = 0;
    r->window_width  = DEFAULT_WIDTH;
    r->window_height = DEFAULT_HEIGHT;
}

void ensure_directory_r(char *path, int perm) {
    if ((strcmp(path, "/") == 0) || (strcmp(path, ".") == 0))
        return;

    struct stat finfo;
    if (stat(path, &finfo) != 0) {
        if (errno != ENOENT) {
            printf("stat(%s): %s", path, strerror(errno));
            abort();
        }
    } else {
        return; // already exists
    }

    const char *parent = dirname(path);
    if (!parent)
        abort();

    char *parent_copy = strdup(parent);
    if (!parent_copy)
        abort();

    ensure_directory_r(parent_copy, perm);
    free(parent_copy);

    if (mkdir(path, perm) != 0 && errno != EEXIST) {
        LOG_ERR("Native", "ensure_directory_r(%s): %s", path, strerror(errno));
        abort();
    }
}

bool native_remove_file(const uint8_t *name, size_t length, bool portable_mode) {
    uint8_t path[UTOX_FILE_NAME_LENGTH] = { 0 };

    if (portable_mode) {
        const char *curr = [NSBundle.mainBundle.bundlePath stringByDeletingLastPathComponent].UTF8String;
        snprintf((char *)path, UTOX_FILE_NAME_LENGTH, "%s/tox/", curr);
    } else {
        const char *home = NSHomeDirectory().UTF8String;
        snprintf((char *)path, UTOX_FILE_NAME_LENGTH, "%s/.config/tox/", home);
    }

    if (strlen((const char *)path) + length >= UTOX_FILE_NAME_LENGTH) {
        LOG_TRACE("NATIVE", "File/directory name too long, unable to remove" );
        return 0;
    } else {
        snprintf((char *)path + strlen((const char *)path), UTOX_FILE_NAME_LENGTH - strlen((const char *)path), "%.*s",
                (int)length, (char *)name);
    }

    if (remove((const char *)path)) {
        LOG_ERR("NATIVE", "Unable to delete file!\n\t\t%s" , path);
        return 0;
    } else {
        LOG_INFO("NATIVE", "File deleted!" );
        LOG_TRACE("NATIVE", "\t%s" , path);
    }
    return 1;
}

int ch_mod(uint8_t *file) {
    return chmod((char *)file, S_IRUSR | S_IWUSR);
}

int file_lock(FILE *file, uint64_t start, size_t length) {
    int          result = -1;
    struct flock fl;
    fl.l_type   = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = start;
    fl.l_len    = length;

    result = fcntl(fileno(file), F_SETLK, &fl);
    if (result != -1) {
        return 1;
    } else {
        return 0;
    }
}

int file_unlock(FILE *file, uint64_t start, size_t length) {
    int          result = -1;
    struct flock fl;
    fl.l_type   = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = start;
    fl.l_len    = length;

    result = fcntl(fileno(file), F_SETLK, &fl);
    if (result != -1) {
        return 1;
    } else {
        return 0;
    }
}

void flush_file(FILE *file) {
    fflush(file);
    int fd = fileno(file);
    fsync(fd);
}

int resize_file(FILE *file, uint64_t size) {
    // https://github.com/trbs/fallocate/blob/master/fallocate/_fallocatemodule.c
    int      fd    = fileno(file);
    fstore_t stuff = { F_ALLOCATECONTIG, F_PEOFPOSMODE, 0, size, 0 };
    int      err   = fcntl(fd, F_PREALLOCATE, &stuff);
    if (err == -1) {
        stuff.fst_flags = F_ALLOCATEALL;
        err             = fcntl(fd, F_PREALLOCATE, &stuff);
    }

    if (err != -1) {
        err = ftruncate(fd, size);
    }

    return err;
}

void postmessage_utox(UTOX_MSG msg, uint16_t param1, uint16_t param2, void *data) {
    /* If you notice any data races, or interesting bugs that appear in OSX but not xlib,
     * replace async( with sync( */
    dispatch_async(dispatch_get_main_queue(), ^{
            utox_message_dispatch(msg, param1, param2, data);
            });
}

void init_ptt(void) {
    settings.push_to_talk = 1;
}

static bool is_ctrl_down = 0;
bool        check_ptt_key(void) {
    return settings.push_to_talk ? is_ctrl_down : 1;
}

void exit_ptt(void) {
    settings.push_to_talk = 0;
}

void redraw(void) {
    uToxAppDelegate *ad = (uToxAppDelegate *)[NSApp delegate];
    [ad soilWindowContents];
}

void openurl(char *str) {
    if (try_open_tox_uri(str)) {
        redraw();
        return;
    }

    NSString *urls = [[NSString alloc] initWithCString:(char *)str encoding:NSUTF8StringEncoding];

    NSURL *url = NULL;
    if (!strncasecmp((const char *)str, "http://", 7) || !strncasecmp((const char *)str, "https://", 8)) {
        url = [NSURL URLWithString:urls];
    } else /* it's a path */ {
        url = [NSURL fileURLWithPath:urls];
    }

    [[NSWorkspace sharedWorkspace] openURL:url];
    [urls release];
}

void force_redraw(void *UNUSED(args)) {
    redraw();
}

void launch_at_startup(bool should) {
    LSSharedFileListRef items = LSSharedFileListCreate(kCFAllocatorDefault, kLSSharedFileListSessionLoginItems, NULL);
    if (should) {
        CFRelease(LSSharedFileListInsertItemURL(items, kLSSharedFileListItemLast, NULL, NULL,
                    (__bridge CFURLRef)[NSBundle mainBundle].bundleURL, NULL, NULL));
    } else {
        CFArrayRef current_items = LSSharedFileListCopySnapshot(items, NULL);
        for (int i = 0; i < CFArrayGetCount(current_items); ++i) {
            LSSharedFileListItemRef it = (void *)CFArrayGetValueAtIndex(current_items, i);
            CFURLRef                urlornull;
#if MAC_OS_X_VERSION_MIN_REQUIRED < 101000
            LSSharedFileListItemResolve(it, 0, &urlornull, NULL);
#else
            urlornull = LSSharedFileListItemCopyResolvedURL(it, 0, NULL);
#endif
            if (urlornull) {
                if (CFEqual(urlornull, (__bridge CFURLRef)[NSBundle mainBundle].bundleURL)) {
                    // this is ours, remove it.
                    LSSharedFileListItemRemove(items, it);
                    CFRelease(urlornull);
                    break;
                }
                CFRelease(urlornull);
            }
        }
        CFRelease(current_items);
        // we're not in the login items list.
    }
    CFRelease(items);
}

@implementation uToxAppDelegate {
    id global_event_listener;
    id local_event_listener;
}

- (void)applicationWillFinishLaunching:(NSNotification *)notification {
    [[NSAppleEventManager sharedAppleEventManager] setEventHandler:self
                                                       andSelector:@selector(handleAppleEvent:withReplyEvent:)
                                                     forEventClass:kInternetEventClass
                                                        andEventID:kAEGetURL];
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    setup_cursors();
    NSImageView *dock_icon                                 = [[NSImageView alloc] initWithFrame:CGRectZero];
    dock_icon.image                                        = [NSApplication sharedApplication].applicationIconImage;
    [NSApplication sharedApplication].dockTile.contentView = dock_icon;
    [dock_icon release];

    global_event_listener = [NSEvent addGlobalMonitorForEventsMatchingMask:NSFlagsChangedMask
                                                                   handler:^(NSEvent *e) {
                                                                       is_ctrl_down = e.modifierFlags & NSFunctionKeyMask;
                                                                   }];

    local_event_listener = [NSEvent addLocalMonitorForEventsMatchingMask:NSFlagsChangedMask
                                                                 handler:^NSEvent *(NSEvent *e) {
                                                                     is_ctrl_down = e.modifierFlags & NSFunctionKeyMask;
                                                                     return e;
                                                                 }];

    ironclad = [[NSMutableDictionary alloc] init];

    // hold COMMAND to start utox in portable mode
    // unfortunately, OS X doesn't have the luxury of passing argv in the GUI
    if ([NSEvent modifierFlags] & NSCommandKeyMask) {
        settings.portable_mode = 1;
    }

    /* load save data */
    theme_load(settings.theme);

    char title_name[128];
    snprintf(title_name, 128, "%s %s (version: %s)", TITLE, SUB_TITLE, VERSION);

#define WINDOW_MASK (NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask)
    self.utox_window = [[NSWindow alloc]
        initWithContentRect:(NSRect) { settings.window_x, settings.window_y, settings.window_width, settings.window_height }
                  styleMask:WINDOW_MASK
                    backing:NSBackingStoreBuffered
                      defer:NO
                     screen:[NSScreen mainScreen]];
                            self.utox_window.releasedWhenClosed = NO;
#undef WINDOW_MASK
                            self.utox_window.delegate = self;
                            self.utox_window.title    = @(title_name);

                            settings.window_width  = self.utox_window.frame.size.width;
                            settings.window_height = self.utox_window.frame.size.height;

                            self.utox_window.contentView =
                                [[[uToxView alloc] initWithFrame:(CGRect){ 0, 0, self.utox_window.frame.size }] autorelease];

                            ui_size(settings.window_width, settings.window_height);
                            ui_rescale(ui_scale);

                            /* start the tox thread */
                            thread(toxcore_thread, NULL);

                            self.nameMenuItem   = [[[NSMenuItem alloc] initWithTitle:@"j" action:NULL keyEquivalent:@""] autorelease];
                            self.statusMenuItem = [[[NSMenuItem alloc] initWithTitle:@"j" action:NULL keyEquivalent:@""] autorelease];
                            update_tray();
                            //[self.nameMenuItem release];
                            //[self.statusMenuItem release];

                            max_video_width  = [NSScreen mainScreen].frame.size.width;
                            max_video_height = [NSScreen mainScreen].frame.size.height;

                            [self.utox_window makeFirstResponder:self.utox_window.contentView];
                            [self.utox_window makeKeyAndOrderFront:self];
}

- (NSMenu *)applicationDockMenu:(NSApplication *)sender {
    if (!self.dockMenu) {
        self.dockMenu = [[[NSMenu alloc] init] autorelease];
        [self.dockMenu addItem:self.nameMenuItem];
        [self.dockMenu addItem:self.statusMenuItem];
    }
    return self.dockMenu;
}

- (void)applicationDidBecomeActive:(NSNotification *)notification {
    if ([NSUserNotification class]) {
        // don't clutter up NC with stale messages
        [[NSUserNotificationCenter defaultUserNotificationCenter] removeAllDeliveredNotifications];
    }

    // clear badge
    [NSApplication sharedApplication].dockTile.badgeLabel = nil;
}

- (void)applicationWillTerminate:(NSNotification *)notification {
    postmessage_utoxav(UTOXAV_KILL, 0, 0, NULL);
    postmessage_toxcore(TOX_KILL, 0, 0, NULL);

    UTOX_SAVE d = {
        // from bottom of screen
        // TODO: translate to xy from top
        .window_x      = self.utox_window.frame.origin.x,
        .window_y      = self.utox_window.frame.origin.y,
        .window_width  = self.utox_window.frame.size.width,
        .window_height = self.utox_window.frame.size.height,
    };

    config_save(&d);

    [NSEvent removeMonitor:global_event_listener];
    [NSEvent removeMonitor:local_event_listener];

    /* wait for threads to exit */
    while (tox_thread_init) {
        yieldcpu(1);
    }

}

- (void)soilWindowContents {
    uToxView *cv = self.utox_window.contentView;
    [cv setNeedsDisplay:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    if (settings.close_to_tray) {
        return NO;
    } else {
        return YES;
    }
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)sender hasVisibleWindows:(BOOL)flag {
    [self.utox_window makeKeyAndOrderFront:self];
    return NO;
}

- (void)windowDidChangeScreen:(NSNotification *)notification {
    if (notification.object == self.utox_window) {
        NSScreen *screen = self.utox_window.screen;
        max_video_width  = screen.frame.size.width;
        max_video_height = screen.frame.size.height;
    }
}

- (void)handleAppleEvent:(NSAppleEventDescriptor *)event withReplyEvent:(NSAppleEventDescriptor *)replyEvent {
    NSString *theURL = [event paramDescriptorForKeyword:keyDirectObject].stringValue;
    uint8_t * cs_url = (const uint8_t *)theURL.UTF8String;
    do_tox_url(cs_url, strlen(cs_url));
    [self soilWindowContents];
}

- (uToxView *)mainView {
    return self.utox_window.contentView;
}

- (void)dealloc {
    [ironclad release];
    [devices release];
    [super dealloc];
}

@end

int main(int argc, char const *argv[]) {
    int8_t should_launch_at_startup;
    int8_t set_show_window;
    bool   skip_updater;

    utox_init();

    settings.window_width  = DEFAULT_WIDTH;
    settings.window_height = DEFAULT_HEIGHT;

    parse_args(argc, argv,
               &skip_updater,
               &should_launch_at_startup,
               &set_show_window);

    if (should_launch_at_startup == 1 || should_launch_at_startup == -1) {
        LOG_TRACE("NATIVE", "Start on boot not supported on this OS!" );
    }

    if (set_show_window == 1 || set_show_window == -1) {
        LOG_TRACE("NATIVE", "Showing/hiding windows not supported on this OS!" );
    }

    if (skip_updater == true) {
        LOG_TRACE("NATIVE", "Disabling the updater is not supported on this OS. Updates are managed by the app store." );
    }

    setlocale(LC_ALL, "");

    /* set the width/height of the drawing region */

    ui_size(settings.window_width, settings.window_height);

    /* event loop */
    @autoreleasepool {
        NSApplication *app  = [NSApplication sharedApplication];
        NSArray *maybeMenus = nil;
        BOOL     ok;

        if ([[NSBundle mainBundle] respondsToSelector:@selector(loadNibNamed:owner:topLevelObjects:)]) {
             ok = [[NSBundle mainBundle] loadNibNamed:@"MainMenu" owner:nil topLevelObjects:&maybeMenus];
        } else {
            NSLog(@"warning: loading nib the deprecated way");

            NSMutableArray *tlo  = [[NSMutableArray alloc] init];
            maybeMenus           = tlo;
            NSDictionary *params = @{ NSNibTopLevelObjects : tlo };
            ok = [NSBundle loadNibFile:[[NSBundle mainBundle] pathForResource:@"MainMenu" ofType:@"nib"]
                     externalNameTable:params
                              withZone:nil];
        }

        if (ok) {
            for (id obj in maybeMenus) {
                if ([obj isKindOfClass:[NSMenu class]]) {
                    app.mainMenu = obj;
                    break;
                }
            }
        }

        [maybeMenus release];

        uToxAppDelegate *appdelegate;
        app.delegate = appdelegate = [[uToxAppDelegate alloc] init];
        [app run];
        [appdelegate release]; // never executed
    }

    return 1;
}
