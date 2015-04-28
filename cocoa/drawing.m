#import "../main.h"

#ifdef UTOX_COCOA_BRAVE
#define DRAW_TARGET_CHK()
#else
#define DRAW_TARGET_CHK() if (!currently_drawing_into_view) { \
    debug("bug: currently_drawing_into_view is nil in %s", __func__); \
    abort(); \
}
#endif

CGImageRef bitmaps[BM_CI1 + 1]  = { NULL };
CTFontRef  fonts[FONT_MISC + 1] = { NULL };
static uToxView *__unsafe_unretained currently_drawing_into_view;
static struct __global_d_state {
    CTFontRef  _use_font;
    uint32_t   _use_font_color;
    CGColorRef _use_font_color_ref;
} global_text_state = { 0 };

#ifdef UTOX_COCOA_USE_CALIBRATED
// broken, but you have choices later
#define CGColorSpace_CREATE CGColorSpaceCreateCalibratedRGB
#define COLOR_WITH_RED      colorWithCalibratedRed
#else
#define CGColorSpace_CREATE CGColorSpaceCreateDeviceRGB
#define COLOR_WITH_RED      colorWithDeviceRed
#endif

@implementation uToxView {
    NSMutableDictionary *_colorCache;
}

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super initWithFrame:frameRect];
    if (self) {
        _colorCache = [[NSMutableDictionary alloc] init];
        self.autoresizesSubviews = YES;
        self.autoresizingMask = NSViewHeightSizable | NSViewWidthSizable;
        [self addSubview:[[[NSView alloc] initWithFrame:CGRectZero] autorelease]];
    }
    return self;
}

- (void)becomeDrawTarget {
    currently_drawing_into_view = self;
    CGContextSetTextMatrix([[NSGraphicsContext currentContext] graphicsPort], CGAffineTransformIdentity);
}

- (void)resignAsDrawTarget {
    currently_drawing_into_view = nil;
}

- (void)drawRect:(NSRect)dirtyRect {
    [self becomeDrawTarget];

    panel_draw(&panel_main, 0, 0, utox_window_width, utox_window_height);

    [self resignAsDrawTarget];
}

- (void)resizeSubviewsWithOldSize:(NSSize)oldSize {
    utox_window_width = self.frame.size.width;
    utox_window_height = self.frame.size.height;
    ui_size(utox_window_width, utox_window_height);
}

- (NSColor *)color:(uint32_t)packed {
    NSColor *ret = [_colorCache objectForKey:@(packed)];
    if (!ret) {
        float r = ((packed >> 16) & 0xFF) / 255.0,
              g = ((packed >> 8) & 0xFF) / 255.0,
              b = ((packed) & 0xFF) / 255.0;
        // calibrated colours don't match those in X11...
        ret = [NSColor COLOR_WITH_RED:r green:g blue:b alpha:1.0];
        [_colorCache setObject:ret forKey:@(packed)];
    }
    return ret;
}

@end

int drawtext_want_width(int x, int y, char_t *str, STRING_IDX length, BOOL wants_width) {
    DRAW_TARGET_CHK()

    CGContextRef context = [NSGraphicsContext currentContext].graphicsPort;
    CFStringRef string = CFStringCreateWithBytes(kCFAllocatorDefault, str, length, kCFStringEncodingUTF8, NO);
    CTFontRef font = global_text_state._use_font;

    CGFloat sz = currently_drawing_into_view.frame.size.height;
    y = sz - y - CTFontGetSize(font);

    CFStringRef keys[] = { kCTFontAttributeName, kCTForegroundColorAttributeName };
    CFTypeRef values[] = { font, global_text_state._use_font_color_ref };

    CFDictionaryRef attributes = CFDictionaryCreate(kCFAllocatorDefault, (const void**)&keys,
                                                    (const void**)&values, 2,
                                                    &kCFTypeDictionaryKeyCallBacks,
                                                    &kCFTypeDictionaryValueCallBacks);

    CFAttributedStringRef attrString = CFAttributedStringCreate(kCFAllocatorDefault, string, attributes);
    CFRelease(string);
    CFRelease(attributes);

    CTLineRef line = CTLineCreateWithAttributedString(attrString);

    // Set text position and draw the line into the graphics context
    CGContextSetTextPosition(context, x, y);
    CTLineDraw(line, context);
    CFRelease(line);
    CFRelease(attrString);
    return wants_width? round(CTLineGetTypographicBounds(line, NULL, NULL, NULL) + CTLineGetTrailingWhitespaceWidth(line)) : 0;
}

void drawtext(int x, int y, char_t *str, STRING_IDX length) {
    drawtext_want_width(x, y, str, length, NO);
}

int drawtext_getwidth(int x, int y, char_t *str, STRING_IDX length) {
    return drawtext_want_width(x, y, str, length, YES);
}

void drawtextwidth(int x, int width, int y, char_t *str, STRING_IDX length) {
    DRAW_TARGET_CHK()

    CGContextRef context = [NSGraphicsContext currentContext].graphicsPort;
    CFStringRef string = CFStringCreateWithBytes(kCFAllocatorDefault, str, length, kCFStringEncodingUTF8, NO);
    CTFontRef font = global_text_state._use_font;

    CGFloat sz = currently_drawing_into_view.frame.size.height;
    y = sz - y - CTFontGetSize(font);

    CFStringRef keys[] = { kCTFontAttributeName, kCTForegroundColorAttributeName };
    CFTypeRef values[] = { font, global_text_state._use_font_color_ref };

    CFDictionaryRef attributes = CFDictionaryCreate(kCFAllocatorDefault, (const void**)&keys,
                                                    (const void**)&values, 2,
                                                    &kCFTypeDictionaryKeyCallBacks,
                                                    &kCFTypeDictionaryValueCallBacks);

    CFAttributedStringRef attrString = CFAttributedStringCreate(kCFAllocatorDefault, string, attributes);
    CFAttributedStringRef ellipse = CFAttributedStringCreate(kCFAllocatorDefault, CFSTR("\u2026"), attributes);
    CFRelease(string);
    CFRelease(attributes);

    CTLineRef ellipse_line = CTLineCreateWithAttributedString(ellipse);
    CTLineRef line = CTLineCreateWithAttributedString(attrString);
    CTLineRef cut_line = CTLineCreateTruncatedLine(line, width, kCTLineTruncationEnd, ellipse_line);
    if (!cut_line) {
        debug("warning: space given not enough for drawtextwidth, bailing");
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

void drawtextwidth_right(int x, int width, int y, char_t *str, STRING_IDX length) {
    DRAW_TARGET_CHK()

    CGContextRef context = [NSGraphicsContext currentContext].graphicsPort;
    CFStringRef string = CFStringCreateWithBytes(kCFAllocatorDefault, str, length, kCFStringEncodingUTF8, NO);
    CTFontRef font = global_text_state._use_font;

    CGFloat sz = currently_drawing_into_view.frame.size.height;
    y = sz - y - CTFontGetSize(font);

    CFStringRef keys[] = { kCTFontAttributeName, kCTForegroundColorAttributeName };
    CFTypeRef values[] = { font, global_text_state._use_font_color_ref };

    CFDictionaryRef attributes = CFDictionaryCreate(kCFAllocatorDefault, (const void**)&keys,
                                                    (const void**)&values, 2,
                                                    &kCFTypeDictionaryKeyCallBacks,
                                                    &kCFTypeDictionaryValueCallBacks);

    CFAttributedStringRef attrString = CFAttributedStringCreate(kCFAllocatorDefault, string, attributes);
    CFAttributedStringRef ellipse = CFAttributedStringCreate(kCFAllocatorDefault, CFSTR("\u2026"), attributes);
    CFRelease(string);
    CFRelease(attributes);

    CTLineRef ellipse_line = CTLineCreateWithAttributedString(ellipse);
    CTLineRef line = CTLineCreateWithAttributedString(attrString);
    CTLineRef cut_line = CTLineCreateTruncatedLine(line, width, kCTLineTruncationEnd, ellipse_line);
    if (!cut_line) {
        debug("warning: space given not enough for drawtextwidth, bailing");
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

void drawtextrange(int x, int x2, int y, char_t *str, STRING_IDX length) {
    drawtextwidth(x, x2 - x, y, str, length);
}

void drawtextrangecut(int x, int x2, int y, char_t *str, STRING_IDX length) {
    drawtextwidth(x, x2 - x, y, str, length);
}

int textwidth(char_t *str, STRING_IDX length) {
    CFStringRef string = CFStringCreateWithBytes(kCFAllocatorDefault, str, length, kCFStringEncodingUTF8, NO);
    CTFontRef font = global_text_state._use_font;

    CFStringRef keys[] = { kCTFontAttributeName, kCTForegroundColorAttributeName };
    CFTypeRef values[] = { font, global_text_state._use_font_color_ref };

    CFDictionaryRef attributes = CFDictionaryCreate(kCFAllocatorDefault, (const void**)&keys,
                                                    (const void**)&values, 2,
                                                    &kCFTypeDictionaryKeyCallBacks,
                                                    &kCFTypeDictionaryValueCallBacks);

    CFAttributedStringRef attrString = CFAttributedStringCreate(kCFAllocatorDefault, string, attributes);
    CFRelease(string);
    CFRelease(attributes);

    CTLineRef line = CTLineCreateWithAttributedString(attrString);

    int ret = round(CTLineGetTypographicBounds(line, NULL, NULL, NULL) + CTLineGetTrailingWhitespaceWidth(line));
    CFRelease(line);
    CFRelease(attrString);
    return ret;
}

int textfit(char_t *str, STRING_IDX length, int width) {
    CFStringRef string = CFStringCreateWithBytes(kCFAllocatorDefault, str, length, kCFStringEncodingUTF8, NO);
    CTFontRef font = global_text_state._use_font;

    CFStringRef keys[] = { kCTFontAttributeName };
    CFTypeRef values[] = { font };

    CFDictionaryRef attributes = CFDictionaryCreate(kCFAllocatorDefault, (const void**)&keys,
                                                    (const void**)&values, 1,
                                                    &kCFTypeDictionaryKeyCallBacks,
                                                    &kCFTypeDictionaryValueCallBacks);

    CFAttributedStringRef attrString = CFAttributedStringCreate(kCFAllocatorDefault, string, attributes);
    CFRelease(attributes);

    CTLineRef line = CTLineCreateWithAttributedString(attrString);
    int ind = CTLineGetStringIndexForPosition(line, (CGPoint){width, 0});

    CFRelease(line);
    CFRelease(attrString);

    if (ind == -1)
        ++ind;

    int ret = [[(NSString *)string substringToIndex:ind] lengthOfBytesUsingEncoding:NSUTF8StringEncoding];

    CFRelease(string);
    return ret;
}

int textfit_near(char_t *str, STRING_IDX length, int width) {
    return textfit(str, length, width);
}

void setfont(int id) {
    global_text_state._use_font = fonts[id];
}

uint32_t setcolor(uint32_t color) {
    uint32_t ret = global_text_state._use_font_color;

    if (global_text_state._use_font_color_ref)
        CGColorRelease(global_text_state._use_font_color_ref);

    CGColorSpaceRef cs = CGColorSpace_CREATE();
    float r = ((color >> 16) & 0xFF) / 255.0,
    g = ((color >> 8) & 0xFF) / 255.0,
    b = ((color) & 0xFF) / 255.0;
    CGFloat comp[4] = {r, g, b, 1.0};
    global_text_state._use_font_color_ref = CGColorCreate(cs, comp);
    CGColorSpaceRelease(cs);

    global_text_state._use_font_color = color;
    return ret;
}

void reload_fonts(void) {
    for (int i = 0; i < sizeof(fonts) / sizeof(CTFontRef); ++i)
        RELEASE_CHK(CFRelease, fonts[i]);

#define COCOA_BASE_FONT_OLD "LucidaGrande"
#define COCOA_BASE_FONT_NEW "HelveticaNeue"

    const char *fontname;
    AT_LEAST_YOSEMITE_DO {
        // yosemite, use HelveticaNeue
        fontname = COCOA_BASE_FONT_NEW;
    } else {
        fontname = COCOA_BASE_FONT_OLD;
    }
    CFStringRef reg = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%s-Regular"), fontname);
    CFStringRef bold = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%s-Bold"), fontname);

#define SCALE (SCALE / 2.0)
    fonts[FONT_TEXT] = CTFontCreateWithNameAndOptions(reg, 12.0 * SCALE, NULL, kCTFontOptionsDefault);
    fonts[FONT_STATUS] = CTFontCreateWithNameAndOptions(reg, 11.0 * SCALE, NULL, kCTFontOptionsDefault);
    fonts[FONT_LIST_NAME] = CTFontCreateWithNameAndOptions(reg, 12.0 * SCALE, NULL, kCTFontOptionsDefault);
    fonts[FONT_TITLE] = CTFontCreateWithNameAndOptions(bold, 12.0 * SCALE, NULL, kCTFontOptionsDefault);
    fonts[FONT_SELF_NAME] = CTFontCreateWithNameAndOptions(bold, 14.0 * SCALE, NULL, kCTFontOptionsDefault);
    fonts[FONT_MISC] = CTFontCreateWithNameAndOptions(bold, 10.0 * SCALE, NULL, kCTFontOptionsDefault);
#undef SCALE
#undef COCOA_BASE_FONT_NEW
#undef COCOA_BASE_FONT_OLD 

    font_small_lineheight = (CTFontGetBoundingBox(fonts[FONT_TEXT]).size.height - CTFontGetDescent(fonts[FONT_TEXT]));
    CFRelease(reg);
    CFRelease(bold);
}

void setscale(void) {
    debug("%d", SCALE);
    uToxAppDelegate *ad = (uToxAppDelegate *)[NSApplication sharedApplication].delegate;
    uint8_t old_scale = SCALE;
    // handle OS X retina capability gracefully
    SCALE *= ad.utox_window.backingScaleFactor;

    for (int i = 0; i < (sizeof(bitmaps) / sizeof(CGImageRef)); ++i)
        RELEASE_CHK(CGImageRelease, bitmaps[i]);

    svg_draw(1);
    // now we have 2x images, if applicable
    SCALE = old_scale;

    // CT fonts automatically obey scale
    reload_fonts();
}

void cgdataprovider_is_finished(void *info, const void *data, size_t size) {
    free((void *)data);
}

void drawalpha(int bm, int x, int y, int width, int height, uint32_t color) {
    DRAW_TARGET_CHK()

    [NSGraphicsContext saveGraphicsState];

    CGFloat sz = currently_drawing_into_view.frame.size.height;
    CGRect rect = {
        .origin = {
            .x = x,
            .y = sz - y - height,
        },
        .size = {
            .width = width,
            .height = height,
        }
    };

    float r = ((color >> 16) & 0xFF) / 255.0,
          g = ((color >> 8) & 0xFF) / 255.0,
          b = ((color) & 0xFF) / 255.0;
    const CGFloat colour_parts[] = {r, g, b, 1.0};

    CGContextRef this = [[NSGraphicsContext currentContext] graphicsPort];
    CGContextClipToMask(this, rect, bitmaps[bm]);
    CGContextSetFillColor(this, colour_parts);
    CGContextFillRect(this, rect);

    [NSGraphicsContext restoreGraphicsState];
}

void loadalpha(int bm, void *data, int width, int height) {
    // TODO: inline assembly
    size_t bs = width * height * 4;
    unsigned char *buf = calloc(bs, 1);
    for (int i = 3; i < bs; i += 4) {
        buf[i] = *(unsigned char *)(data++);
    }

    CGColorSpaceRef cs = CGColorSpace_CREATE();
    CGDataProviderRef dat = CGDataProviderCreateWithData(NULL, buf, bs, &cgdataprovider_is_finished);
    bitmaps[bm] = CGImageCreate(width, height, 8, 32, width * 4, cs, kCGBitmapByteOrderDefault | kCGImageAlphaPremultipliedLast, dat, NULL, NO, kCGRenderingIntentDefault);
    CGDataProviderRelease(dat);
    CGColorSpaceRelease(cs);
}

void framerectw(int x, int y, int width, int height, uint32_t color) {
    DRAW_TARGET_CHK()

    CGFloat sz = currently_drawing_into_view.frame.size.height;
    CGRect rect = CGRectInset((CGRect){
        .origin = {
            .x = x,
            .y = sz - y - height,
        },
        .size = {
            .width = width,
            .height = height,
        }
    }, 0.5, 0.5);

    [[currently_drawing_into_view color:color] set];
    [[NSBezierPath bezierPathWithRect:rect] stroke];
}

void drawrectw(int x, int y, int width, int height, uint32_t color) {
    DRAW_TARGET_CHK()

    CGFloat sz = currently_drawing_into_view.frame.size.height;
    CGRect rect = {
        .origin = {
            .x = x,
            .y = sz - y - height,
        },
        .size = {
            .width = width,
            .height = height,
        }
    };

    [[currently_drawing_into_view color:color] set];
    NSRectFill(rect);
}

void framerect(int x, int y, int right, int bottom, uint32_t color) {
    framerectw(x, y, right - x, bottom - y, color);
}

void drawrect(int x, int y, int right, int bottom, uint32_t color) {
    drawrectw(x, y, right - x, bottom - y, color);
}

void drawhline(int x, int y, int x2, uint32_t color) {
    DRAW_TARGET_CHK()

    CGFloat sz = currently_drawing_into_view.frame.size.height;
    CGRect rect = {
        .origin = {
            .x = x,
            .y = sz - y - 1,
        },
        .size = {
            .width = x2 - x,
            .height = 1,
        }
    };

    [[currently_drawing_into_view color:color] set];
    NSRectFill(rect);
}

void drawvline(int x, int y, int y2, uint32_t color) {
    DRAW_TARGET_CHK()

    CGFloat sz = currently_drawing_into_view.frame.size.height;
    CGRect rect = {
        .origin = {
            .x = x,
            .y = sz - y2,
        },
        .size = {
            .width = 1,
            .height = y2 - y,
        }
    };

    [[currently_drawing_into_view color:color] set];
    NSRectFill(rect);
}

void pushclip(int x, int y, int width, int height) {
    DRAW_TARGET_CHK()

    [NSGraphicsContext saveGraphicsState];

    CGFloat sz = currently_drawing_into_view.frame.size.height;
    NSRectClip((CGRect){x, sz - y - height, width, height});
}

void popclip(void) {
    DRAW_TARGET_CHK()
    
    // will work fine as long as nobody does any other weirdness with gstate
    [NSGraphicsContext restoreGraphicsState];
}

void enddraw(int x, int y, int width, int height) {

}

void draw_image(const UTOX_NATIVE_IMAGE *image, int x, int y, uint32_t width, uint32_t height, uint32_t imgx, uint32_t imgy) {
    DRAW_TARGET_CHK()

    CGFloat sz = currently_drawing_into_view.frame.size.height;
    CGRect rect = {
        .origin = {
            .x = x,
            .y = sz - y - height,
        },
        .size = {
            .width = width,
            .height = height,
        }
    };

    CGContextRef this = [NSGraphicsContext currentContext].graphicsPort;
    CGContextDrawImage(this, rect, image->image);
}
