/*
 * Copyright (C) 2005, 2006, 2007, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WKGraphics_h
#define WKGraphics_h

#if TARGET_OS_IPHONE

#import <CoreGraphics/CoreGraphics.h>

typedef int WKCompositeOperation;
typedef uint32_t CGFontAntialiasingStyle;

#ifdef __cplusplus
extern "C" {
#endif

CGContextRef WKGetCurrentGraphicsContext(void);
void WKSetCurrentGraphicsContext(CGContextRef context);

void WKRectFill(CGContextRef context, CGRect aRect);
void WKRectFillUsingOperation(CGContextRef, CGRect, WKCompositeOperation);

CGImageRef WKGraphicsCreateImageFromBundleWithName(const char *image_file);
CGPatternRef WKCreatePatternFromCGImage(CGImageRef imageRef);
void WKSetPattern(CGContextRef context, CGPatternRef pattern, bool fill, bool stroke);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
class WKFontAntialiasingStateSaver
{
public:

    WKFontAntialiasingStateSaver(CGContextRef context, bool useOrientationDependentFontAntialiasing)
        : m_context(context)
        , m_useOrientationDependentFontAntialiasing(useOrientationDependentFontAntialiasing)
    {
    }

    void setup(bool isLandscapeOrientation);
    void restore();

private:

#if TARGET_IPHONE_SIMULATOR
#pragma clang diagnostic push
#if defined(__has_warning) && __has_warning("-Wunused-private-field")
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif
#endif
    CGContextRef m_context;
    bool m_useOrientationDependentFontAntialiasing;
    CGFontAntialiasingStyle m_oldAntialiasingStyle;
#if TARGET_IPHONE_SIMULATOR
#pragma clang diagnostic pop
#endif
};
#endif

#endif // TARGET_OS_IPHONE

#endif // WKGraphics_h
