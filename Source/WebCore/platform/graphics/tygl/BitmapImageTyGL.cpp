/*
 * Copyright (C) 2013, 2014 University of Szeged. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY UNIVERSITY OF SZEGED ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL UNIVERSITY OF SZEGED OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "BitmapImage.h"

#include "GraphicsContext.h"
#include "NativeImageTyGL.h"
#include "PlatformContextTyGL.h"
#include "Timer.h"

namespace WebCore {

BitmapImage::BitmapImage(PassNativeImagePtr nativeImage, ImageObserver* observer)
    : Image(observer)
    , m_currentFrame(0)
    , m_frames(0)
    , m_frameTimer(nullptr)
    , m_repetitionCount(cAnimationNone)
    , m_repetitionCountStatus(Unknown)
    , m_repetitionsComplete(0)
    , m_decodedSize(0)
    , m_frameCount(1)
    , m_isSolidColor(false)
    , m_checkedForSolidColor(false)
    , m_animationFinished(true)
    , m_allDataReceived(true)
    , m_haveSize(true)
    , m_sizeAvailable(true)
    , m_haveFrameCount(true)
{
    NativeImagePtr source = nativeImage;
    int width = source->width();
    int height = source->height();
    m_decodedSize = width * height * 4;
    m_size = IntSize(width, height);

    m_frames.grow(1);
    m_frames[0].m_frame = source;
    m_frames[0].m_hasAlpha = source->hasAlphaChannel();
    m_frames[0].m_haveMetadata = true;
    checkForSolidColor();
}

void BitmapImage::determineMinimumSubsamplingLevel() const
{
    m_minimumSubsamplingLevel = 0;
}

void BitmapImage::checkForSolidColor()
{
    m_isSolidColor = false;
    m_checkedForSolidColor = true;

    if (frameCount() != 1)
        return;

    NativeImagePtr nativeImage = frameAtIndex(m_currentFrame);
    if (!nativeImage) // If it's too early we won't have an image yet.
        return;

    if (nativeImage->width() != 1 || nativeImage->height() != 1 || !nativeImage->texture())
        return;

    m_solidColor = nativeImage->pixelColor();

    m_isSolidColor = true;
}

void BitmapImage::draw(GraphicsContext* context, const FloatRect& destRect, const FloatRect& srcRect, ColorSpace styleColorSpace, CompositeOperator op, BlendMode blendMode, ImageOrientationDescription description)
{
    startAnimation();

    NativeImagePtr image = nativeImageForCurrentFrame();
    if (!image)
        return;

    if (mayFillWithSolidColor()) {
        fillWithSolidColor(context, destRect, solidColor(), styleColorSpace, op);
        return;
    }

    context->platformContext()->copyImage(destRect, image, srcRect, op, blendMode);
}

} //namespace WebCore
