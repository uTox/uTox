#ifndef SCA_CURSOR_H
#define SCA_CURSOR_H

#import <AppKit/AppKit.h>

static inline CGImageRef create_zoom_cursor(CGFloat scale, BOOL plus) {
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGColorRef black = CGColorCreateGenericRGB(0, 0, 0, 1.0);
    CGColorRef white = CGColorCreateGenericRGB(1, 1, 1, 1.0);

    CGContextRef draw = CGBitmapContextCreate(NULL, 16 * scale, 16 * scale, 8, 0, cs, (CGBitmapInfo)kCGImageAlphaPremultipliedLast);
    CGColorSpaceRelease(cs);

    CGContextBeginPath(draw);
    CGContextMoveToPoint(draw, 2., 14.);
    CGContextAddLineToPoint(draw, 15., 1.);
    //CGContextCloseSubpath(draw);

    CGContextSetStrokeColorWithColor(draw, black);
    CGContextSetLineWidth(draw, 3.0);
    CGContextDrawPath(draw, kCGPathStroke);

    CGContextBeginPath(draw);
    CGContextAddEllipseInRect(draw, CGRectInset((CGRect){0, 4, 12, 12}, 0.5, 0.5));

    CGContextSetLineWidth(draw, 1.0);
    CGContextSetFillColorWithColor(draw, white);
    CGContextDrawPath(draw, kCGPathFillStroke);

    CGContextBeginPath(draw);
    CGContextAddRect(draw, (CGRect){3, 9, 6, 2});

    if (plus)
        CGContextAddRect(draw, (CGRect){5, 7, 2, 6});

    CGContextSetFillColorWithColor(draw, black);
    CGContextDrawPath(draw, kCGPathFill);

    CGImageRef glass = CGBitmapContextCreateImage(draw);
    CGContextRelease(draw);
    CGColorRelease(white);
    CGColorRelease(black);
    return glass;
}

static inline NSCursor *create_zoom_in_cursor(void) {
    CGImageRef base = create_zoom_cursor(1.0, YES);
    CGImageRef base2x = create_zoom_cursor(2.0, YES);

    NSBitmapImageRep *b = [[NSBitmapImageRep alloc] initWithCGImage:base];
    NSBitmapImageRep *b2x = [[NSBitmapImageRep alloc] initWithCGImage:base2x];
    NSImage *cur = [[NSImage alloc] initWithSize:(CGSize){16, 16}];
    [cur addRepresentation:b];
    [cur addRepresentation:b2x];

    NSCursor *ret = [[NSCursor alloc] initWithImage:cur hotSpot:(CGPoint){6, 6}];

    [b release];
    [b2x release];
    [cur release];
    CGImageRelease(base);
    CGImageRelease(base2x);

    return ret;
}

static inline NSCursor *create_zoom_out_cursor(void) {
    CGImageRef base = create_zoom_cursor(1.0, NO);
    CGImageRef base2x = create_zoom_cursor(2.0, NO);

    NSBitmapImageRep *b = [[NSBitmapImageRep alloc] initWithCGImage:base];
    NSBitmapImageRep *b2x = [[NSBitmapImageRep alloc] initWithCGImage:base2x];
    NSImage *cur = [[NSImage alloc] initWithSize:(CGSize){16, 16}];
    [cur addRepresentation:b];
    [cur addRepresentation:b2x];

    NSCursor *ret = [[NSCursor alloc] initWithImage:cur hotSpot:(CGPoint){6, 6}];

    [b release];
    [b2x release];
    [cur release];
    CGImageRelease(base);
    CGImageRelease(base2x);

    return ret;
}

#endif
