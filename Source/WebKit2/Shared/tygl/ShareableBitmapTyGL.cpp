/*
 * Copyright (C) 2013, 2014 University of Szeged. All Rights Reserved.
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
#include "ShareableBitmap.h"

#include <WebCore/BitmapImage.h>
#include <WebCore/GLDefs.h>
#include <WebCore/GraphicsContext.h>
#include <WebCore/NativeImageTyGL.h>
#include <WebCore/PlatformContextTyGL.h>
#include <wtf/RefPtr.h>

using namespace WebCore;

namespace WebKit {

std::unique_ptr<GraphicsContext> ShareableBitmap::createGraphicsContext()
{
    RefPtr<PlatformGraphicsContext> context = PlatformGraphicsContext::create(m_sharedImage);
    return std::make_unique<GraphicsContext>(PlatformContextTyGL::release(context));
}

void ShareableBitmap::paint(GraphicsContext& context, const IntPoint& dstPoint, const IntRect& srcRect)
{
}

void ShareableBitmap::paint(GraphicsContext& context, float scaleFactor, const IntPoint& dstPoint, const IntRect& srcRect)
{
}

PassRefPtr<Image> ShareableBitmap::createImage()
{
    RefPtr<Image> image = BitmapImage::create(m_sharedImage);
    return image.release();
}

} //namespace WebKit
