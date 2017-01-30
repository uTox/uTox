#import "main.h"

#import "../debug.h"
#import "../main.h"
#import "../settings.h"
#import "../ui/draw.h"
#import "../ui/svg.h"


#ifdef UTOX_COCOA_BRAVE
#define DRAW_TARGET_CHK()
#else
#define DRAW_TARGET_CHK()                                                          \
    if (!currently_drawing_into_view) {                                            \
        LOG_ERR("OSX", "bug: currently_drawing_into_view is nil in %s", __func__); \
        abort();                                                                   \
    }
#endif

CGImageRef       bitmaps[BM_ENDMARKER + 1] = { NULL };
CTFontRef        fonts[FONT_MISC + 1]      = { NULL };
static uToxView *__unsafe_unretained currently_drawing_into_view;
static struct __global_d_state {
    CTFontRef  _use_font;
    uint32_t   _use_font_color;
    CGColorRef _use_font_color_ref;
} global_text_state = { 0 };

#ifdef UTOX_COCOA_USE_CALIBRATED
// broken, but you have choices later
#define CGColorSpace_CREATE CGColorSpaceCreateCalibratedRGB
#define COLOR_WITH_RED colorWithCalibratedRed
#else
#define CGColorSpace_CREATE CGColorSpaceCreateDeviceRGB
#define COLOR_WITH_RED colorWithDeviceRed
#endif

@implementation uToxView {
    NSMutableDictionary *_colorCache;
}

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        _colorCache              = [[NSMutableDictionary alloc] init];
        self.autoresizesSubviews = YES;
        self.autoresizingMask    = NSViewHeightSizable | NSViewWidthSizable;
        [self addSubview:[[[NSView alloc] initWithFrame:CGRectZero] autorelease]];
    }
    return self;
}

- (void)becomeDrawTarget {
    currently_drawing_into_view      = self;
    self.didDrawInlineVideoThisFrame = NO;
    CGContextSetTextMatrix([[NSGraphicsContext currentContext] graphicsPort], CGAffineTransformIdentity);
}

- (void)resignAsDrawTarget {
    if (!self.didDrawInlineVideoThisFrame) {
        [self.inlineVideo removeFromSuperview];
    }
    currently_drawing_into_view = nil;
}

- (void)drawRect:(NSRect)dirtyRect {
    [self becomeDrawTarget];

    panel_draw(&panel_root, 0, 0, settings.window_width, settings.window_height);

    [self resignAsDrawTarget];
}

- (void)resizeSubviewsWithOldSize:(NSSize)oldSize {
    settings.window_width  = self.frame.size.width;
    settings.window_height = self.frame.size.height;
    ui_size(settings.window_width, settings.window_height);

    [self.inputContext invalidateCharacterCoordinates];
}

- (NSColor *)color:(uint32_t)packed {
    NSColor *ret = [_colorCache objectForKey:@(packed)];
    if (!ret) {
        float r = ((packed >> 16) & 0xFF) / 255.0, g = ((packed >> 8) & 0xFF) / 255.0, b = ((packed)&0xFF) / 255.0;
        // calibrated colours don't match those in X11...
        ret = [NSColor COLOR_WITH_RED:r green:g blue:b alpha:1.0];
        [_colorCache setObject:ret forKey:@(packed)];
    }
    return ret;
}

- (void)dealloc {
    [_inlineVideo release];
    _inlineVideo = nil;

    [super dealloc];
}

@end

/* convert invalid utf8 to valid garbage */
CFStringRef try_to_interpret_string(char *str, uint16_t length) {
    CFStringEncoding  try_list[] = { kCFStringEncodingUTF8, kCFStringEncodingISOLatin1, 0 };
    CFStringEncoding *c          = try_list;
    CFStringRef       string     = NULL;

    while (*c) {
        string = CFStringCreateWithBytes(kCFAllocatorDefault, str, length, *c, NO);
        if (string) {
            break;
        }
        c++;
    }

    return string;
}

int drawtext_want_width(int x, int y, char *str, uint16_t length, BOOL wants_width) {
    DRAW_TARGET_CHK()

    CGContextRef context = [NSGraphicsContext currentContext].graphicsPort;
    CFStringRef string   = try_to_interpret_string(str, length);
    if (!string) {
        return 0;
    }

    CTFontRef font = global_text_state._use_font;

    CGFloat sz = currently_drawing_into_view.frame.size.height;
    y          = sz - y - CTFontGetSize(font);

    CFStringRef keys[]   = { kCTFontAttributeName, kCTForegroundColorAttributeName };
    CFTypeRef   values[] = { font, global_text_state._use_font_color_ref };

    CFDictionaryRef attributes = CFDictionaryCreate(kCFAllocatorDefault, (const void **)&keys, (const void **)&values,
                                                    2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    CFAttributedStringRef attrString = CFAttributedStringCreate(kCFAllocatorDefault, string, attributes);
    CFRelease(string);
    CFRelease(attributes);

    CTLineRef line = CTLineCreateWithAttributedString(attrString);

    // Set text position and draw the line into the graphics context
    CGContextSetTextPosition(context, x, y);
    CTLineDraw(line, context);
    CFRelease(line);
    CFRelease(attrString);
    return wants_width ?
               round(CTLineGetTypographicBounds(line, NULL, NULL, NULL) + CTLineGetTrailingWhitespaceWidth(line)) :
               0;
}

void drawtext(int x, int y, const char *str, uint16_t length) {
    drawtext_want_width(x, y, str, length, NO);
}

int drawtext_getwidth(int x, int y, const char *str, uint16_t length) {
    return drawtext_want_width(x, y, str, length, YES);
}

void drawtextwidth(int x, int width, int y, const char *str, uint16_t length) {
    DRAW_TARGET_CHK()

    CGContextRef context = [NSGraphicsContext currentContext].graphicsPort;
    CFStringRef string   = try_to_interpret_string(str, length);
    if (!string) {
        return;
    }

    CTFontRef font = global_text_state._use_font;

    CGFloat sz = currently_drawing_into_view.frame.size.height;
    y          = sz - y - CTFontGetSize(font);

    CFStringRef keys[]   = { kCTFontAttributeName, kCTForegroundColorAttributeName };
    CFTypeRef   values[] = { font, global_text_state._use_font_color_ref };

    CFDictionaryRef attributes = CFDictionaryCreate(kCFAllocatorDefault, (const void **)&keys, (const void **)&values,
                                                    2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    CFAttributedStringRef attrString = CFAttributedStringCreate(kCFAllocatorDefault, string, attributes);
    CFAttributedStringRef ellipse    = CFAttributedStringCreate(kCFAllocatorDefault, CFSTR("\u2026"), attributes);
    CFRelease(string);
    CFRelease(attributes);

    CTLineRef ellipse_line = CTLineCreateWithAttributedString(ellipse);
    CTLineRef line         = CTLineCreateWithAttributedString(attrString);
    CTLineRef cut_line     = CTLineCreateTruncatedLine(line, width, kCTLineTruncationEnd, ellipse_line);
    if (!cut_line) {
        LOG_WARN("OSX", "warning: space given not enough for drawtextwidth, bailing");
        goto free_everything;
    }

    // Set text position and draw the line into the graphics context
    CGContextSetTextPosition(context, x, y);
    CTLineDraw(cut_line, context);
    CFRelease(cut_line);

free_everything:
    CFRelease(line);
    CFRelease(ellipse_line);
    CFRelease(attrString);
    CFRelease(ellipse);
}

void drawtextwidth_right(int x, int width, int y, const char *str, uint16_t length) {
    DRAW_TARGET_CHK()

    CGContextRef context = [NSGraphicsContext currentContext].graphicsPort;
    CFStringRef string   = try_to_interpret_string(str, length);
    if (!string) {
        return;
    }

    CTFontRef font = global_text_state._use_font;

    CGFloat sz = currently_drawing_into_view.frame.size.height;
    y          = sz - y - CTFontGetSize(font);

    CFStringRef keys[]   = { kCTFontAttributeName, kCTForegroundColorAttributeName };
    CFTypeRef   values[] = { font, global_text_state._use_font_color_ref };

    CFDictionaryRef attributes = CFDictionaryCreate(kCFAllocatorDefault, (const void **)&keys, (const void **)&values,
                                                    2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    CFAttributedStringRef attrString = CFAttributedStringCreate(kCFAllocatorDefault, string, attributes);
    CFAttributedStringRef ellipse    = CFAttributedStringCreate(kCFAllocatorDefault, CFSTR("\u2026"), attributes);
    CFRelease(string);
    CFRelease(attributes);

    CTLineRef ellipse_line = CTLineCreateWithAttributedString(ellipse);
    CTLineRef line         = CTLineCreateWithAttributedString(attrString);
    CTLineRef cut_line     = CTLineCreateTruncatedLine(line, width, kCTLineTruncationEnd, ellipse_line);
    if (!cut_line) {
        LOG_WARN("OSX", "warning: space given not enough for drawtextwidth, bailing");
        goto free_everything;
    }

    CGFloat checkleading = width - CTLineGetTypographicBounds(cut_line, NULL, NULL, NULL);

    // Set text position and draw the line into the graphics context
    CGContextSetTextPosition(context, x + checkleading, y);
    CTLineDraw(cut_line, context);
    CFRelease(cut_line);

free_everything:
    CFRelease(line);
    CFRelease(ellipse_line);
    CFRelease(attrString);
    CFRelease(ellipse);
}

void drawtextrange(int x, int x2, int y, const char *str, uint16_t length) {
    drawtextwidth(x, x2 - x, y, str, length);
}

void drawtextrangecut(int x, int x2, int y, const char *str, uint16_t length) {
    drawtextwidth(x, x2 - x, y, str, length);
}

int textwidth(const char *str, uint16_t length) {
    CFStringRef string = try_to_interpret_string(str, length);
    if (!string) {
        return 0;
    }

    CTFontRef font = global_text_state._use_font;

    CFStringRef keys[]   = { kCTFontAttributeName };
    CFTypeRef   values[] = { font };

    CFDictionaryRef attributes = CFDictionaryCreate(kCFAllocatorDefault, (const void **)&keys, (const void **)&values,
                                                    1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    CFAttributedStringRef attrString = CFAttributedStringCreate(kCFAllocatorDefault, string, attributes);
    CFRelease(string);
    CFRelease(attributes);

    CTLineRef line = CTLineCreateWithAttributedString(attrString);

    int ret = round(CTLineGetTypographicBounds(line, NULL, NULL, NULL) + CTLineGetTrailingWhitespaceWidth(line));
    CFRelease(line);
    CFRelease(attrString);
    return ret;
}

int textfit(const char *str, uint16_t length, int width) {
    CFStringRef string = try_to_interpret_string(str, length);
    if (!string) {
        return 0;
    }

    CTFontRef font = global_text_state._use_font;

    CFStringRef keys[]   = { kCTFontAttributeName };
    CFTypeRef   values[] = { font };

    CFDictionaryRef attributes = CFDictionaryCreate(kCFAllocatorDefault, (const void **)&keys, (const void **)&values,
                                                    1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    CFAttributedStringRef attrString = CFAttributedStringCreate(kCFAllocatorDefault, string, attributes);
    CFRelease(attributes);

    CTLineRef line = CTLineCreateWithAttributedString(attrString);
    int       ind  = CTLineGetStringIndexForPosition(line, (CGPoint){ width, 0 });

    CFRelease(line);
    CFRelease(attrString);

    if (ind == -1) {
        ++ind;
    }

    int ret = [[(NSString *)string substringToIndex:ind] lengthOfBytesUsingEncoding:NSUTF8StringEncoding];

    CFRelease(string);
    return ret;
}

int textfit_near(const char *str, uint16_t length, int width) {
    return textfit(str, length, width);
}

void setfont(int id) {
    global_text_state._use_font = fonts[id];
}

uint32_t setcolor(uint32_t color) {
    uint32_t ret = global_text_state._use_font_color;

    if (global_text_state._use_font_color_ref) {
        CGColorRelease(global_text_state._use_font_color_ref);
    }

    CGColorSpaceRef cs = CGColorSpace_CREATE();
    float           r = ((color >> 16) & 0xFF) / 255.0, g = ((color >> 8) & 0xFF) / 255.0, b = ((color)&0xFF) / 255.0;
    CGFloat         comp[4]               = { r, g, b, 1.0 };
    global_text_state._use_font_color_ref = CGColorCreate(cs, comp);
    CGColorSpaceRelease(cs);

    global_text_state._use_font_color = color;
    return ret;
}

void setscale_fonts(void) {
    for (int i = 0; i < sizeof(fonts) / sizeof(CTFontRef); ++i) {
        RELEASE_CHK(CFRelease, fonts[i]);
    }

#define COCOA_BASE_FONT_OLD "LucidaGrande"
#define COCOA_BASE_FONT_YOSEMITE "HelveticaNeue"
// San Francisco
#define COCOA_BASE_FONT_ELCAPITAN ".SFNSText"

    const char *fontname;
    AT_LEAST_ELCAPITAN_DO {
        fontname = COCOA_BASE_FONT_ELCAPITAN;
    }
    else AT_LEAST_YOSEMITE_DO {
        fontname = COCOA_BASE_FONT_YOSEMITE;
    }
    else {
        fontname = COCOA_BASE_FONT_OLD;
    }
    CFStringRef reg  = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%s-Regular"), fontname);
    CFStringRef bold = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%s-Bold"), fontname);

    fonts[FONT_TEXT]      = CTFontCreateWithNameAndOptions(reg, SCALE(12.0), NULL, kCTFontOptionsDefault);
    fonts[FONT_STATUS]    = CTFontCreateWithNameAndOptions(reg, SCALE(11.0), NULL, kCTFontOptionsDefault);
    fonts[FONT_LIST_NAME] = CTFontCreateWithNameAndOptions(reg, SCALE(12.0), NULL, kCTFontOptionsDefault);
    fonts[FONT_TITLE]     = CTFontCreateWithNameAndOptions(bold, SCALE(12.0), NULL, kCTFontOptionsDefault);
    fonts[FONT_SELF_NAME] = CTFontCreateWithNameAndOptions(bold, SCALE(14.0), NULL, kCTFontOptionsDefault);
    fonts[FONT_MISC]      = CTFontCreateWithNameAndOptions(bold, SCALE(10.0), NULL, kCTFontOptionsDefault);
#undef COCOA_BASE_FONT_NEW
#undef COCOA_BASE_FONT_OLD

    AT_LEAST_ELCAPITAN_DO {
        font_small_lineheight = CTFontGetBoundingBox(fonts[FONT_TEXT]).size.height;
    }
    else {
        font_small_lineheight = (CTFontGetBoundingBox(fonts[FONT_TEXT]).size.height - CTFontGetDescent(fonts[FONT_TEXT]));
    }
    font_small_lineheight += CTFontGetLeading(fonts[FONT_TEXT]);

    CFRelease(reg);
    CFRelease(bold);
}

void setscale(void) {
    LOG_WARN("OSX", "%d", ui_scale);
    uToxAppDelegate *ad = (uToxAppDelegate *)[NSApplication sharedApplication].delegate;
    float old_scale     = ui_scale;
    // handle OS X retina capability gracefully
    ui_scale *= ad.utox_window.backingScaleFactor;

    for (int i = 0; i < (sizeof(bitmaps) / sizeof(CGImageRef)); ++i) {
        RELEASE_CHK(CGImageRelease, bitmaps[i]);
    }

    svg_draw(1);
    // now we have 2x images, if applicable
    ui_scale = old_scale;

    ad.utox_window.minSize = (CGSize){ SCALE(MAIN_WIDTH), SCALE(MAIN_HEIGHT) };
}

void cgdataprovider_is_finished(void *info, const void *data, size_t size) {
    free((void *)data);
}

void drawalpha(int bm, int x, int y, int width, int height, uint32_t color) {
    DRAW_TARGET_CHK()

    [NSGraphicsContext saveGraphicsState];

    CGFloat sz   = currently_drawing_into_view.frame.size.height;
    CGRect  rect = {.origin =
                       {
                           .x = x, .y = sz - y - height,
                       },
                   .size =
                       {
                           .width = width, .height = height,
                       } };

    float         r = ((color >> 16) & 0xFF) / 255.0, g = ((color >> 8) & 0xFF) / 255.0, b = ((color)&0xFF) / 255.0;
    const CGFloat colour_parts[] = { r, g, b, 1.0 };

    CGContextRef this = [[NSGraphicsContext currentContext] graphicsPort];
    CGContextClipToMask(this, rect, bitmaps[bm]);
    CGContextSetFillColor(this, colour_parts);
    CGContextFillRect(this, rect);

    [NSGraphicsContext restoreGraphicsState];
}

void loadalpha(int bm, void *data, int width, int height) {
    // TODO: inline assembly
    size_t         bs  = width * height * 4;
    unsigned char *buf = calloc(bs, 1);
    for (int i = 3; i < bs; i += 4) {
        buf[i] = *(unsigned char *)(data++);
    }

    CGColorSpaceRef   cs  = CGColorSpace_CREATE();
    CGDataProviderRef dat = CGDataProviderCreateWithData(NULL, buf, bs, &cgdataprovider_is_finished);
    bitmaps[bm] =
        CGImageCreate(width, height, 8, 32, width * 4, cs, kCGBitmapByteOrderDefault | kCGImageAlphaPremultipliedLast,
                      dat, NULL, NO, kCGRenderingIntentDefault);
    CGDataProviderRelease(dat);
    CGColorSpaceRelease(cs);
}

void draw_rect_frame(int x, int y, int width, int height, uint32_t color) {
    DRAW_TARGET_CHK()

    CGFloat sz   = currently_drawing_into_view.frame.size.height;
    CGRect  rect = CGRectInset((CGRect){.origin =
                                           {
                                               .x = x, .y = sz - y - height,
                                           },
                                       .size =
                                           {
                                               .width = width, .height = height,
                                           } },
                              0.5, 0.5);

    [[currently_drawing_into_view color:color] set];
    [[NSBezierPath bezierPathWithRect:rect] stroke];
}

void draw_rect_fill(int x, int y, int width, int height, uint32_t color) {
    DRAW_TARGET_CHK()

    CGFloat sz   = currently_drawing_into_view.frame.size.height;
    CGRect  rect = {.origin =
                       {
                           .x = x, .y = sz - y - height,
                       },
                   .size =
                       {
                           .width = width, .height = height,
                       } };

    [[currently_drawing_into_view color:color] set];
    NSRectFill(rect);
}

void drawrect(int x, int y, int right, int bottom, uint32_t color) {
    draw_rect_fill(x, y, right - x, bottom - y, color);
}

void drawhline(int x, int y, int x2, uint32_t color) {
    DRAW_TARGET_CHK()

    CGFloat sz   = currently_drawing_into_view.frame.size.height;
    CGRect  rect = {.origin =
                       {
                           .x = x, .y = sz - y - 1,
                       },
                   .size =
                       {
                           .width = x2 - x, .height = 1,
                       } };

    [[currently_drawing_into_view color:color] set];
    NSRectFill(rect);
}

void drawvline(int x, int y, int y2, uint32_t color) {
    DRAW_TARGET_CHK()

    CGFloat sz   = currently_drawing_into_view.frame.size.height;
    CGRect  rect = {.origin =
                       {
                           .x = x, .y = sz - y2,
                       },
                   .size =
                       {
                           .width = 1, .height = y2 - y,
                       } };

    [[currently_drawing_into_view color:color] set];
    NSRectFill(rect);
}

void pushclip(int x, int y, int width, int height) {
    DRAW_TARGET_CHK()

    [NSGraphicsContext saveGraphicsState];

    CGFloat sz = currently_drawing_into_view.frame.size.height;
    NSRectClip((CGRect){ x, sz - y - height, width, height });
}

void popclip(void) {
    DRAW_TARGET_CHK()

    // will work fine as long as nobody does any other weirdness with gstate
    [NSGraphicsContext restoreGraphicsState];
}

void enddraw(int x, int y, int width, int height) {}

void draw_image(const NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height, uint32_t imgx, uint32_t imgy) {
    DRAW_TARGET_CHK()

    // LOG_WARN("OSX", "%lu %lu %lf", imgx, imgy, image->scale);

    CGFloat sz   = currently_drawing_into_view.frame.size.height;
    CGRect  rect = {.origin =
                       {
                           .x = x, .y = sz - y - height,
                       },
                   .size =
                       {
                           .width = width, .height = height,
                       } };

    CGContextRef this = [NSGraphicsContext currentContext].graphicsPort;
    CGImageRef di =
        CGImageCreateWithImageInRect(image->image, (CGRect){ imgx, imgy, width / image->scale, height / image->scale });
    CGContextDrawImage(this, rect, di);
    CFRelease(di);
}

void draw_inline_image(uint8_t *img_data, size_t size, uint16_t w, uint16_t h, int x, int y) {
    DRAW_TARGET_CHK()

    uToxIroncladVideoContent *inlineVideo = currently_drawing_into_view.inlineVideo;
    if (!inlineVideo) {
        inlineVideo = [[uToxIroncladVideoContent alloc] initWithFrame:(CGRect){ { 0, 0 }, { 16, 16 } }];
        currently_drawing_into_view.inlineVideo = inlineVideo;
    }

    CGFloat sz = currently_drawing_into_view.frame.size.height;

    CGRect rect = {.origin =
                       {
                           .x = x, .y = sz - y - h,
                       },
                   .size =
                       {
                           .width = w, .height = h,
                       } };

    if (!CGRectEqualToRect(rect, inlineVideo.frame)) {
        inlineVideo.frame = rect;
        [inlineVideo checkSize];
    }

    if (inlineVideo.superview != currently_drawing_into_view) {
        [currently_drawing_into_view addSubview:inlineVideo];
    }

    [inlineVideo displayImage:img_data w:w h:h];
    currently_drawing_into_view.didDrawInlineVideoThisFrame = YES;
}
