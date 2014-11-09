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
#include "AffineTransform.h"
#include "BitmapImage.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "Image.h"
#include "ImageObserver.h"
#include "PlatformContextTyGL.h"

namespace WebCore {

void Image::drawPattern(GraphicsContext* context, const FloatRect& tileRect, const AffineTransform& patternTransform,
    const FloatPoint& phase, ColorSpace, CompositeOperator op, const FloatRect& destRect, BlendMode)
{
   startAnimation();
   RefPtr<NativeImageTyGL> nativeImageToTile = nativeImageForCurrentFrame();
   if (!nativeImageToTile)
       return;

   // In determining whether to repeat or not, we'll need to "scroll" the source tile so that it is offset correctly: "phase" contains the information for this,
   // though it also includes destRect.x/y, which we remove.
   const FloatPoint tileOffset = FloatPoint(phase.x() - destRect.x(), phase.y() - destRect.y());

   // WebKit does not directly tell us whether to repeat horizontally or vertically, but if repeating in a direction is not required,
   // WebKit will *trim* destRect accordingly. We can use this fact to infer whether to repeat-x/y: if we are being asked to draw out-of-bounds
   // of the source tile, we must repeat.
   const bool repeatX = (tileRect.x() - tileOffset.x() + destRect.width() >= tileRect.width());
   const bool repeatY = (tileRect.y() - tileOffset.y() + destRect.height() >= tileRect.height());
   RefPtr<BitmapImage> webkitImageToTile(BitmapImage::create(nativeImageToTile));
   // NOTE: I've never seen patternTransform be anything other than the identity, even when we are scaling and rotating. Instead, the request to scale/ rotate
   // the context is made by Webkit before calling Image::drawPattern and presumably undone afterwards. We thus commandeer Pattern's "pattern space transform"
   // in order to transmit the offset information.
   RefPtr<WebCore::Pattern> fillPattern(WebCore::Pattern::create(webkitImageToTile, repeatX, repeatY));
   WebCore::AffineTransform patternOffset;
   patternOffset.translate(phase.x(), phase.y());
   fillPattern->setPatternSpaceTransform(patternOffset);

   context->platformContext()->fillRect(destRect, PlatformContextTyGL::Coloring(fillPattern));

   if (imageObserver())
       imageObserver()->didDraw(this);
}

bool FrameData::clear(bool clearMetadata)
{
    if (clearMetadata)
        m_haveMetadata = false;

    if (m_frame) {
        m_frame.clear();
        return true;
    }
    return false;
}

} // namespace WebCore
