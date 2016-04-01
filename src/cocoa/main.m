#include "../main.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <libgen.h>
#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>

#ifdef MAKEFILE
#include "interaction.m"
#include "drawing.m"
#include "video.m"
#include "grabdesktop.m"
#endif

#define DEFAULT_WIDTH (382 * DEFAULT_SCALE)
#define DEFAULT_HEIGHT (320 * DEFAULT_SCALE)

void debug(const char *fmt, ...) {
    va_list l;
    va_start(l, fmt);
    NSLogv(@(fmt), l);
    va_end(l);
}

int UTOX_NATIVE_IMAGE_IS_VALID(UTOX_NATIVE_IMAGE *img) {
    return img->image != nil;
}

UTOX_NATIVE_IMAGE *decode_image(const UTOX_IMAGE data, size_t size, uint16_t *w, uint16_t *h, _Bool keep_alpha) {
    CFDataRef idata_copy = CFDataCreate(kCFAllocatorDefault, data, size);
    CGDataProviderRef src = CGDataProviderCreateWithCFData(idata_copy);
    CGImageRef underlying_img = CGImageCreateWithPNGDataProvider(src, NULL, YES, kCGRenderingIntentDefault);
    CGDataProviderRelease(src);
    CFRelease(idata_copy);

    if (underlying_img) {
        *w = CGImageGetWidth(underlying_img);
        *h = CGImageGetHeight(underlying_img);
        UTOX_NATIVE_IMAGE *ret = malloc(sizeof(UTOX_NATIVE_IMAGE));
        ret->scale = 1.0;
        ret->image = underlying_img;
        return ret;
    } else {
        return NULL;
    }
}

void image_set_filter(UTOX_NATIVE_IMAGE *image, uint8_t filter) {

}

void image_set_scale(UTOX_NATIVE_IMAGE *image, double scale) {
    image->scale = scale;
}

void image_free(UTOX_NATIVE_IMAGE *img) {
    CGImageRelease(img->image);
    free(img);
}

static BOOL theme_set_on_argv = NO;

void thread(void func(void*), void *args) {
    pthread_t thread_temp;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 1 << 20);
    pthread_create(&thread_temp, &attr, (void*(*)(void*))func, args);
    pthread_attr_destroy(&attr);
}

void yieldcpu(uint32_t ms) {
    usleep(1000 * ms);
}

/* *** audio/video *** */
void audio_detect(void) {}

_Bool audio_init(void *handle) {
    return 0;
}

_Bool audio_close(void *handle) {
    return 0;
}

_Bool audio_frame(int16_t *buffer) {
    return 0;
}

/* *** os *** */

int systemlang(void) {
    // FIXME maybe replace with NSLocale?
    char *str = getenv("LC_MESSAGES");
    if(!str) {
        str = getenv("LANG");
    }
    if(!str) {
        return DEFAULT_LANG;
    }
    return ui_guess_lang_by_posix_locale(str, DEFAULT_LANG);
}

uint64_t get_time(void) {
    struct timespec ts;
    clock_serv_t muhclock;
    mach_timespec_t machtime;
    host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &muhclock);
    clock_get_time(muhclock, &machtime);
    mach_port_deallocate(mach_task_self(), muhclock);
    ts.tv_sec = machtime.tv_sec;
    ts.tv_nsec = machtime.tv_nsec;

    return ((uint64_t)ts.tv_sec * (1000 * 1000 * 1000)) + (uint64_t)ts.tv_nsec;
}

void openurl(char_t *str) {
    NSString *urls = [[NSString alloc] initWithCString:(char *)str encoding:NSUTF8StringEncoding];

    NSURL *url = NULL;
    if (!strncasecmp((const char *)str, "http://", 7) ||
        !strncasecmp((const char *)str, "https://", 8)) {
        url = [NSURL URLWithString:urls];
    } else /* it's a path */ {
        url = [NSURL fileURLWithPath:urls];
    }

    [[NSWorkspace sharedWorkspace] openURL:url];
    [urls release];
}

void config_osdefaults(UTOX_SAVE *r) {
    r->window_x = 0;
    r->window_y = 0;
    r->window_width = DEFAULT_WIDTH;
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
        debug("ensure_directory_r(%s): %s", path, strerror(errno));
        abort();
    }
}


/** Takes data from µTox and saves it, just how the OS likes it saved!
 *
 * Returns 1 on failure. Used to set save_needed in tox thread */
static _Bool native_save_data(const uint8_t *name, size_t name_length, const uint8_t *data, size_t length){
    uint8_t path[UTOX_FILE_NAME_LENGTH];
    uint8_t atomic_path[UTOX_FILE_NAME_LENGTH];

    if (utox_portable) {
        const char *curr = [NSBundle.mainBundle.bundlePath stringByDeletingLastPathComponent].UTF8String;
        snprintf((char *)path, UTOX_FILE_NAME_LENGTH, "%s/tox/", curr);
    } else {
        const char *home = NSHomeDirectory().UTF8String;
        snprintf((char*)path, UTOX_FILE_NAME_LENGTH, "%s/.config/tox/", home);
    }

    mkdir((char*)path, 0700);

    if (strlen((const char*)path) + name_length >= UTOX_FILE_NAME_LENGTH - strlen(".atomic")){
        debug("NATIVE:\tSave directory name too long\n");
        return 0;
    } else {
        snprintf((char*)path + strlen((const char*)path), UTOX_FILE_NAME_LENGTH - strlen((const char*)path), "%s", name);
        snprintf((char*)atomic_path, UTOX_FILE_NAME_LENGTH, "%s.atomic", path);
    }

    FILE *file = fopen((const char*)atomic_path, "wb");
    if (file) {
        fwrite(data, length, 1, file);
        fclose(file);

        if (rename((const char*)atomic_path, (const char*)path)) {
            /* Consider backing up this file instead of overwriting it. */
            debug("NATIVE:\t%sUnable to move file!\n", atomic_path);
            return 1;
        }
        return 0;
    } else {
        debug("NATIVE:\tUnable to open %s to write save\n", path);
        return 1;
    }

    return 1;
}

_Bool native_save_data_tox(uint8_t *data, size_t length){
    uint8_t name[] = "tox_save.tox";
    return native_save_data(name, strlen((const char*)name), data, length);
}

_Bool native_save_data_utox(uint8_t *data, size_t length){
    uint8_t name[] = "utox_save";
    return native_save_data(name, strlen((const char*)name), data, length);
}

_Bool native_save_data_log(void){
    return 0;

}

/** Takes data from µTox and loads it up! */
static uint8_t *native_load_data(const uint8_t *name, size_t name_length, size_t *out_size){
    uint8_t path[UTOX_FILE_NAME_LENGTH];
    uint8_t *data;

    if (utox_portable) {
        const char *curr = [NSBundle.mainBundle.bundlePath stringByDeletingLastPathComponent].UTF8String;
        snprintf((char *)path, UTOX_FILE_NAME_LENGTH, "%s/tox/", curr);
    } else {
        const char *home = NSHomeDirectory().UTF8String;
        snprintf((char*)path, UTOX_FILE_NAME_LENGTH, "%s/.config/tox/", home);
    }

    if (strlen((const char*)path) + name_length >= UTOX_FILE_NAME_LENGTH){
        debug("NATIVE:\tLoad directory name too long\n");
        return 0;
    } else {
        snprintf((char*)path + strlen((const char*)path), UTOX_FILE_NAME_LENGTH - strlen((const char*)path), "%s", name);
    }


    FILE *file = fopen((const char*)path, "rb");
    if (!file) {
        //debug("NATIVE:\tUnable to open/read %s\n", path);
        if (out_size) {*out_size = 0;}
        return NULL;
    }


    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    data = malloc(size);
    if (!data) {
        fclose(file);
        if (out_size) {*out_size = 0;}
        return NULL;
    } else {
        fseek(file, 0, SEEK_SET);

        if(fread(data, size, 1, file) != 1) {
            debug("NATIVE:\tRead error on %s\n", path);
            fclose(file);
            free(data);
            if (out_size) {*out_size = 0;}
            return NULL;
        }

    fclose(file);
    }

    if (out_size) {*out_size = size;}
    return data;
}

uint8_t *native_load_data_tox(size_t *size){
    uint8_t name[][20] = { "tox_save.tox",
                           "tox_save.tox.atomic",
                           "tox_save.tmp",
                           "tox_save"
    };

    uint8_t *data;

    for (int i = 0; i < 4; i++) {
        data = native_load_data(name[i], strlen((const char*)name[i]), size);
        if (data) {
            return data;
        } else {
            debug("NATIVE:\tUnable to load %s\n", name[i]);
        }
    }
    return NULL;
}

UTOX_SAVE *native_load_data_utox(void){
    uint8_t name[] = "utox_save";
    return (UTOX_SAVE*)native_load_data(name, strlen((const char*)name), NULL);
}

uint8_t *native_load_data_log(size_t *size){
    return 0;
}



/* it occured to me that we should probably make datapath allocate memory for its caller */
int datapath(uint8_t *dest) {
    if (utox_portable) {
        const char *home = [NSBundle.mainBundle.bundlePath stringByDeletingLastPathComponent].UTF8String;
        int l = sprintf((char*)dest, "%.238s/tox", home);
        ensure_directory_r((char*)dest, 0700);
        dest[l++] = '/';

        return l;
    } else {
        const char *home = NSHomeDirectory().UTF8String;
        int l = sprintf((char*)dest, "%.230s/.config/tox", home);
        ensure_directory_r((char*)dest, 0700);
        dest[l++] = '/';

        return l;
    }
}

int datapath_subdir(uint8_t *dest, const char *subdir) {
    int l = datapath(dest);
    l += sprintf((char*)(dest+l), "%s", subdir);
    mkdir((char*)dest, 0700);
    dest[l++] = '/';

    return l;
}

int ch_mod(uint8_t *file){
    return chmod((char*)file, S_IRUSR | S_IWUSR);
}

int file_lock(FILE *file, uint64_t start, size_t length){
    int result = -1;
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = start;
    fl.l_len = length;

    result = fcntl(fileno(file), F_SETLK, &fl);
    if(result != -1){
        return 1;
    } else {
        return 0;
    }
}

int file_unlock(FILE *file, uint64_t start, size_t length) {
    int result = -1;
    struct flock fl;
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = start;
    fl.l_len = length;

    result = fcntl(fileno(file), F_SETLK, &fl);
    if(result != -1){
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
    int fd = fileno(file);
    fstore_t stuff = {F_ALLOCATECONTIG, F_PEOFPOSMODE, 0, size, 0};
    int err = fcntl(fd, F_PREALLOCATE, &stuff);
    if (err == -1) {
        stuff.fst_flags = F_ALLOCATEALL;
        err = fcntl(fd, F_PREALLOCATE, &stuff);
    }

    if (err != -1) {
        err = ftruncate(fd, size);
    }

    return err;
}

void postmessage(uint32_t msg, uint16_t param1, uint16_t param2, void *data) {
    /* If you notice any data races, or interesting bugs that appear in OSX but not xlib,
     * replace async( with sync( */
    dispatch_async(dispatch_get_main_queue(), ^{
        tox_message(msg, param1, param2, data);
    });
}

void init_ptt(void) {
    push_to_talk = 1;
}

static _Bool is_ctrl_down = 0;
_Bool check_ptt_key(void){
    return push_to_talk? is_ctrl_down : 1;
}

void exit_ptt(void) {
    push_to_talk = 0;
}

void redraw(void) {
    uToxAppDelegate *ad = (uToxAppDelegate *)[NSApp delegate];
    [ad soilWindowContents];
}

void launch_at_startup(int should) {
    LSSharedFileListRef items = LSSharedFileListCreate(kCFAllocatorDefault, kLSSharedFileListSessionLoginItems, NULL);
    if (should) {
        CFRelease(LSSharedFileListInsertItemURL(items, kLSSharedFileListItemLast, NULL, NULL, (__bridge CFURLRef)[NSBundle mainBundle].bundleURL, NULL, NULL));
    } else {
        CFArrayRef current_items = LSSharedFileListCopySnapshot(items, NULL);
        for (int i = 0; i < CFArrayGetCount(current_items); ++i) {
            LSSharedFileListItemRef it = (void *)CFArrayGetValueAtIndex(current_items, i);
            CFURLRef urlornull;
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
    id  local_event_listener;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    setup_cursors();
    NSImageView *dock_icon = [[NSImageView alloc] initWithFrame:CGRectZero];
    dock_icon.image = [NSApplication sharedApplication].applicationIconImage;
    [NSApplication sharedApplication].dockTile.contentView = dock_icon;
    [dock_icon release];

    global_event_listener = [NSEvent addGlobalMonitorForEventsMatchingMask:NSFlagsChangedMask handler:^(NSEvent *e) {
        is_ctrl_down = e.modifierFlags & NSFunctionKeyMask;
    }];

    local_event_listener = [NSEvent addLocalMonitorForEventsMatchingMask:NSFlagsChangedMask handler:^NSEvent *(NSEvent *e) {
        is_ctrl_down = e.modifierFlags & NSFunctionKeyMask;
        return e;
    }];

    ironclad = [[NSMutableDictionary alloc] init];

    // hold COMMAND to start utox in portable mode
    // unfortunately, OS X doesn't have the luxury of passing argv in the GUI
    if ([NSEvent modifierFlags] & NSCommandKeyMask) {
        utox_portable = 1;
    }

    /* load save data */
    UTOX_SAVE *save = config_load();
    if (!theme_set_on_argv) {
        theme = save->theme;
    }
    theme_load(theme);

    char title_name[128];
    snprintf(title_name, 128, "%s %s (version: %s)", TITLE, SUB_TITLE, VERSION);

#define WINDOW_MASK (NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask | NSResizableWindowMask)
    self.utox_window = [[NSWindow alloc] initWithContentRect:(NSRect){save->window_x, save->window_y, save->window_width, save->window_height} styleMask:WINDOW_MASK backing:NSBackingStoreBuffered defer:NO screen:[NSScreen mainScreen]];
    self.utox_window.releasedWhenClosed = NO;
#undef WINDOW_MASK
    self.utox_window.delegate = self;
    self.utox_window.title = @(title_name);

    utox_window_width = self.utox_window.frame.size.width;
    utox_window_height = self.utox_window.frame.size.height;

    self.utox_window.contentView = [[[uToxView alloc] initWithFrame:(CGRect){0, 0, self.utox_window.frame.size}] autorelease];
    ui_set_scale((save->scale + 1) ?: 2);
    ui_size(utox_window_width, utox_window_height);

    /* start the tox thread */
    thread(toxcore_thread, NULL);

    self.nameMenuItem = [[[NSMenuItem alloc] initWithTitle:@"j" action:NULL keyEquivalent:@""] autorelease];
    self.statusMenuItem = [[[NSMenuItem alloc] initWithTitle:@"j" action:NULL keyEquivalent:@""] autorelease];
    update_tray();
    //[self.nameMenuItem release];
    //[self.statusMenuItem release];

    max_video_width = [NSScreen mainScreen].frame.size.width;
    max_video_height = [NSScreen mainScreen].frame.size.height;

    [self.utox_window makeFirstResponder:self.utox_window.contentView];
    [self.utox_window makeKeyAndOrderFront:self];

    /* done with save */
    free(save);
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
        .window_x = self.utox_window.frame.origin.x,
        .window_y = self.utox_window.frame.origin.y,
        .window_width = self.utox_window.frame.size.width,
        .window_height = self.utox_window.frame.size.height,
    };

    config_save(&d);

    [NSEvent removeMonitor:global_event_listener];
    [NSEvent removeMonitor:local_event_listener];

    /* wait for threads to exit */
    while(tox_thread_init) {
        yieldcpu(1);
    }

    debug("clean exit\n");
}

- (void)soilWindowContents {
    uToxView *cv = self.utox_window.contentView;
    [cv setNeedsDisplay:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    if (close_to_tray) {
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
        max_video_width = screen.frame.size.width;
        max_video_height = screen.frame.size.height;
    }
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
    bool theme_was_set_on_argv;
    int8_t should_launch_at_startup;
    int8_t set_show_window;
    bool no_updater;

    parse_args(argc, argv, &theme_was_set_on_argv, &should_launch_at_startup, &set_show_window, &no_updater);

    if (should_launch_at_startup == 1 || should_launch_at_startup == -1) {
        debug("Start on boot not supported on this OS!\n");
    }

    if (set_show_window == 1 || set_show_window == -1) {
        debug("Showing/hiding windows not supported on this OS!\n");
    }

    if (no_updater == true) {
        debug("Disabling the updater is not supported on this OS. Updates are managed by the app store.\n");
    }

    setlocale(LC_ALL, "");

    LANG = systemlang();
    dropdown_language.selected = dropdown_language.over = LANG;

    /* set the width/height of the drawing region */
    utox_window_width = DEFAULT_WIDTH;
    utox_window_height = DEFAULT_HEIGHT;
    ui_size(utox_window_width, utox_window_height);

    /* event loop */
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];
        NSArray *maybeMenus = nil;
        BOOL ok;

        if ([[NSBundle mainBundle] respondsToSelector:@selector(loadNibNamed:owner:topLevelObjects:)]) {
            ok = [[NSBundle mainBundle] loadNibNamed:@"MainMenu" owner:nil topLevelObjects:&maybeMenus];
        } else {
            NSLog(@"warning: loading nib the deprecated way");

            NSMutableArray *tlo = [[NSMutableArray alloc] init];
            maybeMenus = tlo;
            NSDictionary *params = @{NSNibTopLevelObjects: tlo};
            ok = [NSBundle loadNibFile:[[NSBundle mainBundle] pathForResource:@"MainMenu" ofType:@"nib"] externalNameTable:params withZone:nil];
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
