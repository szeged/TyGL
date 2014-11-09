/*
 * Copyright (C) 2014 Samsung Electronics Ltd.
 * Copyright (C) 2014 Nicolas P. Rougier
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY SAMSUNG ``AS IS'' AND ANY
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

// The font texture atlas code is based on freetype-gl code.
// See details on http://code.google.com/p/freetype-gl/.

#include "config.h"
#include "FontTextureAtlasTyGL.h"

#include "limits.h"

namespace WebCore {
namespace TyGL {

int FontTextureAtlasTyGL::fit(int index, int regionWidth, int regionHeight) const
{
    const IntRect* node = &m_nodes[index];
    int widthLeft = regionWidth;
    const int atlasWidth = width();
    const int atlasHeight = height();

    if (node->x() + regionWidth >= atlasWidth)
        return -1;

    int y = node->y() + node->height();

    while (widthLeft > 0) {
        node = &m_nodes[index++];
        const int nodeBottom = node->y() + node->height();

        if (nodeBottom > y)
            y = nodeBottom;

        if (y + regionHeight >= atlasHeight)
            return -1;

        widthLeft -= node->width();
    }

    return y;
}

void FontTextureAtlasTyGL::mergeNodes()
{
    // FIXME: On the long run this algorithm should be made O(n) instead of O(n*n)
    int count = m_nodes.size() - 1;

    for (int i = 0; i < count; i++) {
        IntRect* node = &m_nodes[i];
        IntRect* next = &m_nodes[i + 1];

        if (node->y() + node->height() == next->y() + next->height()) {
            node->setWidth(node->width() + next->width());
            m_nodes.remove(i + 1);
            count = m_nodes.size() - 1;
            --i;
        }
    }
}

void FontTextureAtlasTyGL::shrinkNodes(int index)
{
    // FIXME: On the long run this algorithm should be made O(n) instead of O(n*n)
    int count = m_nodes.size();

    for (; index < count; ++index) {
        IntRect* node = &m_nodes[index];
        IntRect* prev = &m_nodes[index - 1];

        if (node->x() >= (prev->x() + prev->width()))
            break;

        const int shrinkOffset = prev->x() + prev->width() - node->x();
        node->setX(node->x() + shrinkOffset);
        node->setWidth(node->width() - shrinkOffset);

        if (node->width() > 0)
            break;

        m_nodes.remove(index);
        count = m_nodes.size();
        --index;
    }
}

IntRect FontTextureAtlasTyGL::region(int regionWidth, int regionHeight)
{
    IntRect region = IntRect(0, 0, regionWidth, regionHeight);

    int bestHeight = INT_MAX;
    int bestWidth = INT_MAX;
    int bestIndex = -1;

    int nodesCount = m_nodes.size();

    if (!nodesCount) {
        // Add initial node needed for width based search of free space for nodes.
        m_nodes.append(IntRect(1, 0, width() - 2, 1));
        nodesCount = 1;
    }

    for (int i = 0; i < nodesCount; ++i) {
        // Try to fit the region below each node from the left to the right.
        int y = fit(i, regionWidth, regionHeight);
        if (y >= 0) {
            IntRect* node = &m_nodes[i];
            if (((y + regionHeight) < bestHeight)
                || (((y + regionHeight) == bestHeight) && (node->width() < bestWidth))) {
                bestWidth = node->width();
                bestHeight = y + regionHeight;
                bestIndex = i;
                region.setX(node->x());
                region.setY(y);
            }
        }
    }

    if (bestIndex == -1)
        return IntRect(-1, -1, 0, 0);

    m_nodes.insert(bestIndex, IntRect(region.x(), region.y(), regionWidth, regionHeight));

    // Decrease width of the right nodes since new node was inserted before.
    shrinkNodes(bestIndex + 1);
    // Merge nodes with the same regionHeight on the same level.
    mergeNodes();

    m_usedMemory += regionWidth * regionHeight;
    return region;
}

void FontTextureAtlasTyGL::copyGlyphBitmapToRegion(const IntRect& region, const unsigned char* bitmapBuffer, size_t bitmapStride, size_t bitmapDepth)
{
    const int x = region.x();
    const int y = region.y();
    const int regionWidth = region.width();
    const int regionHeight = region.height();
    const int atlasWidth = width();
    const int atlasHeight = height();

    ASSERT(x > 0 && x < atlasWidth - 1);
    ASSERT(y > 0 && y < atlasHeight - 1);
    ASSERT(x + regionWidth <= atlasWidth - 1);
    ASSERT(y + regionHeight <= atlasHeight - 1);

    switch (bitmapDepth) {
    case 1:
        {
            unsigned char* atlasBuffer = reinterpret_cast<unsigned char*>(m_atlasBackingStore.data()) + (y * atlasWidth + x);
            const unsigned char* bitmapBufferEnd = bitmapBuffer + bitmapStride * regionHeight;

            while (bitmapBuffer < bitmapBufferEnd) {
                const unsigned char* bitmapBufferRowEnd = bitmapBuffer + regionWidth;

                while (bitmapBuffer < bitmapBufferRowEnd)
                    *atlasBuffer++ = *bitmapBuffer++;

                atlasBuffer += atlasWidth - regionWidth;
                bitmapBuffer += bitmapStride - regionWidth;
            }
        }
        break;
    default:
        ASSERT_NOT_REACHED();
        return;
    }

    m_dirty = true;
}

void FontTextureAtlasTyGL::update()
{
    if (!m_dirty)
        return;

    bindTexture();
    // It seems to be the fastest way to update the texture. The data is aligned. With batching it should happen only once per block of text.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width(), height(), 0, GL_ALPHA, GL_UNSIGNED_BYTE, m_atlasBackingStore.data());
    m_dirty = false;
}

} // namespace TyGL
} // namespace WebCore
