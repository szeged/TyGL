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
#include "ImageBuffer.h"

#include "BitmapImage.h"
#include "GraphicsContext.h"
#include "JPEGImageEncoder.h"
#include "MIMETypeRegistry.h"
#include "PNGImageEncoder.h"
#include "PlatformContextTyGL.h"
#include <wtf/text/Base64.h>

namespace WebCore {

ImageBufferData::ImageBufferData()
{
    m_nativeImage = NativeImageTyGL::create(0, 0);
}

ImageBufferData::ImageBufferData(const FloatSize& size)
{
    m_nativeImage = NativeImageTyGL::create(size.width(), size.height());
    m_nativeImage->bindFbo();
    glClear(GL_COLOR_BUFFER_BIT);
}

ImageBuffer::ImageBuffer(const FloatSize& size, float /* resolutionScale */, ColorSpace, RenderingMode renderingMode, bool& success)
    : m_data(size)
    , m_size(size)
    , m_logicalSize(size)
{
    success = false;
    RefPtr<PlatformContextTyGL> context(PlatformContextTyGL::create(m_data.nativeImage()));
    m_context = adoptPtr(new GraphicsContext(PlatformContextTyGL::release(context)));
    success = true;
}

ImageBuffer::~ImageBuffer()
{
}

GraphicsContext* ImageBuffer::context() const
{
    return m_context.get();
}

PassRefPtr<Image> ImageBuffer::copyImage(BackingStoreCopy copyBehavior, ScaleBehavior) const
{
    RefPtr<NativeImageTyGL> nativeImage = m_data.nativeImage();
    if (copyBehavior == CopyBackingStore)
        return BitmapImage::create(nativeImage->clone());

    // BitmapImage will release the passed in texture on destruction.
    return BitmapImage::create(nativeImage.release());
}

BackingStoreCopy ImageBuffer::fastCopyImageMode()
{
    return CopyBackingStore;
}

void ImageBuffer::clip(GraphicsContext* context, const FloatRect& maskRect) const
{
    context->platformContext()->clipToImageBuffer(m_data.nativeImage(), maskRect);
}

void ImageBuffer::draw(GraphicsContext* destinationContext, ColorSpace styleColorSpace, const FloatRect& destRect, const FloatRect& srcRect,
    CompositeOperator op, BlendMode blendMode, bool useLowQualityScale)
{
    UNUSED_PARAM(useLowQualityScale);
    destinationContext->platformContext()->copyImage(destRect, m_context->platformContext()->targetTexture(), srcRect, op, blendMode);
}

void ImageBuffer::drawPattern(GraphicsContext* context, const FloatRect& srcRect, const AffineTransform& patternTransform,
                              const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op, const FloatRect& destRect, BlendMode blendMode)
{
}

void ImageBuffer::platformTransformColorSpace(const Vector<int>& lookUpTable)
{
}

String ImageBuffer::toDataURL(const String& mimeType, const double* quality, CoordinateSystem) const
{
    ASSERT(MIMETypeRegistry::isSupportedImageMIMETypeForEncoding(mimeType));

    const String nullDataURL = "data:,";

    if (m_size.isEmpty())
        return nullDataURL;

    enum {
        EncodeJPEG,
        EncodePNG,
    } encodeType = mimeType.lower() == "image/png" ? EncodePNG : EncodeJPEG;

    RefPtr<Uint8ClampedArray> imageData = encodeType == EncodePNG
        ? getUnmultipliedImageData(IntRect(IntPoint(0, 0), m_size))
        : getPremultipliedImageData(IntRect(IntPoint(0, 0), m_size));

    Vector<char> output;
    if (encodeType == EncodePNG) {
        if (!compressRGBABigEndianToPNG(imageData->data(), m_size, output))
            return nullDataURL;
    } else {
        if (!compressRGBABigEndianToJPEG(imageData->data(), m_size, output, quality))
            return nullDataURL;
    }

    Vector<char> base64;
    base64Encode(output, base64);
    output.clear();

    return String::format("data:%s;base64,%s", mimeType.utf8().data(), base64.data());
}

PassRefPtr<Uint8ClampedArray> ImageBuffer::getUnmultipliedImageData(const IntRect& rect, CoordinateSystem) const
{
    int width = rect.width();
    int height = rect.height();
    unsigned long bufferSize = width * height * 4;

    RefPtr<Uint8ClampedArray> buffer = Uint8ClampedArray::createUninitialized(bufferSize);
    const_cast<ImageBufferData*>(&m_data)->nativeImage()->bindFbo();
    context()->platformContext()->flushPendingDraws();
    glReadPixels(rect.x(), rect.y(), width, height, GL_RGBA, GL_UNSIGNED_BYTE, buffer->data());

    unsigned char* pixel = buffer->data();
    unsigned char* end = pixel + bufferSize;

    while (pixel < end) {
        if (pixel[3] && pixel[3] != 255) {
            float alphaFactor = 255 / pixel[3];
            pixel[0] *= alphaFactor;
            pixel[1] *= alphaFactor;
            pixel[2] *= alphaFactor;
        }
        pixel += 4;
    }

    return buffer.release();
}

PassRefPtr<Uint8ClampedArray> ImageBuffer::getPremultipliedImageData(const IntRect& rect, CoordinateSystem) const
{
    int width = rect.width();
    int height = rect.height();

    RefPtr<Uint8ClampedArray> buffer = Uint8ClampedArray::createUninitialized(width * height * 4);
    const_cast<ImageBufferData*>(&m_data)->nativeImage()->bindFbo();
    context()->platformContext()->flushPendingDraws();
    glReadPixels(rect.x(), rect.y(), width, height, GL_RGBA, GL_UNSIGNED_BYTE, &buffer);

    return buffer.release();
}

void ImageBuffer::putByteArray(Multiply multiplied, Uint8ClampedArray* source, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint, CoordinateSystem)
{
    m_data.nativeImage()->bindTexture();
    glTexSubImage2D(GL_TEXTURE_2D, 0, destPoint.x(), destPoint.y(), sourceSize.width(), sourceSize.height(), GL_RGBA, GL_UNSIGNED_BYTE, source->data());
}

}// namespace WebCore
