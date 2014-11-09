/*
 * Copyright (C) 2014 University of Szeged
 * Copyright (C) 2014 Samsung Electronics Ltd.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PlatformContextTyGL.h"

#include "RenderThemeTyGL.h"
#include "ShaderCommonTyGL.h"
#include "UiAtlasTyGL.h"

namespace WebCore {

RefPtr<Image> PlatformContextTyGL::s_uiElements = 0;

void PlatformContextTyGL::loadUITexture()
{
    s_uiElements = Image::loadPlatformResource("uiElements");
}

void PlatformContextTyGL::drawUIElement(UIElement element, const IntRect& rect, RenderTheme* theme, const RenderObject* object)
{
    static int initialized = 0;
    if (!initialized) {
        loadUITexture();
        initialized = 1;
    }

    ASSERT(s_uiElements);

    if(!s_uiElements)
        return;

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    switch(element) {
    case Button:
        paintButton(rect, theme->isPressed(*object));
        break;
    case Checkbox:
        paintCheckbox(rect, theme->isChecked(*object));
        break;
    case RadioButton:
        paintRadioButton(rect, theme->isChecked(*object));
        break;
    case ScrollbarThumb:
        paintScrollbarThumb(rect);
        break;
    case EditBox:
        paintEditBox(rect, theme->isFocused(*object));
        break;
    }
}

void PlatformContextTyGL::paintButton(const IntRect& rect, bool pressed)
{
    float x1 = rect.x() + transform().transform().m_matrix[4];;
    float x2 = x1 + rect.width();
    float y1 = rect.y() + transform().transform().m_matrix[5];;
    float y2 = y1 + rect.height();

    float texStartPointX = (pressed)? TyGL::buttonTexXPressed : TyGL::buttonTexX;
    float texStartPointY = (pressed)? TyGL::buttonTexYPressed : TyGL::buttonTexY;

    float xCoords[4] = { x1, x1 + TyGL::buttonPadding, x2 - TyGL::buttonPadding, x2 };
    float yCoords[4] = { y1, y1 + TyGL::buttonPadding, y2 - TyGL::buttonPadding, y2 };

    float xTexCoords[4] = { texStartPointX, texStartPointX + TyGL::buttonTexPadding,
        texStartPointX + TyGL::buttonTexWidth - TyGL::buttonTexPadding, texStartPointX + TyGL::buttonTexWidth };
    float yTexCoords[4] = { texStartPointY, texStartPointY + TyGL::buttonTexPadding,
        texStartPointY + TyGL::buttonTexWidth - TyGL::buttonTexPadding, texStartPointY + TyGL::buttonTexWidth };

    float positions[9 * 4];
    float texCoords[9 * 4];

    int index;

    for(int i = 0; i < 9; ++i) {
        index = i * 4;

        positions[index] = xCoords[i % 3];
        positions[index + 1] = yCoords[i / 3];
        positions[index + 2] = xCoords[i % 3 + 1];
        positions[index + 3] = yCoords[i / 3 + 1];

        texCoords[index] = xTexCoords[i % 3];
        texCoords[index + 1] = yTexCoords[i / 3];
        texCoords[index + 2] = xTexCoords[i % 3 + 1];
        texCoords[index + 3] = yTexCoords[i / 3 + 1];
    }

    paintTexturedQuads(9, s_uiElements->nativeImageForCurrentFrame(), positions, texCoords, rect);
}

void PlatformContextTyGL::paintRadioButton(const IntRect& rect, bool checked)
{
    float x1 = rect.x() + transform().transform().m_matrix[4];;
    float x2 = x1 + rect.width();
    float y1 = rect.y() + transform().transform().m_matrix[5];;
    float y2 = y1 + rect.height();

    float texStartPointX = (checked)? TyGL::radioTexXChecked : TyGL::radioTexX;
    float texStartPointY = (checked)? TyGL::radioTexYChecked : TyGL::radioTexY;

    float positions[4] = { x1, y1, x2, y2 };
    float texCoords[4] = { texStartPointX, texStartPointY, texStartPointX + TyGL::radioTexWidth, texStartPointY + TyGL::radioTexWidth };

    paintTexturedQuads(1, s_uiElements->nativeImageForCurrentFrame(), positions, texCoords, rect);
}

void PlatformContextTyGL::paintCheckbox(const IntRect& rect, bool checked)
{
    float x1 = rect.x() + transform().transform().m_matrix[4];;
    float x2 = x1 + rect.width();
    float y1 = rect.y() + transform().transform().m_matrix[5];;
    float y2 = y1 + rect.height();

    float texStartPointX = (checked)? TyGL::checkboxTexXChecked : TyGL::checkboxTexX;
    float texStartPointY = (checked)? TyGL::checkboxTexYChecked : TyGL::checkboxTexY;

    float positions[4] = { x1, y1, x2, y2 };
    float texCoords[4] = { texStartPointX, texStartPointY, texStartPointX + TyGL::checkboxTexWidth, texStartPointY + TyGL::checkboxTexWidth };

    paintTexturedQuads(1, s_uiElements->nativeImageForCurrentFrame(), positions, texCoords, rect);
}

void PlatformContextTyGL::paintScrollbarThumb(const IntRect& rect)
{
    float x1 = rect.x() + transform().transform().m_matrix[4];;
    float x2 = x1 + rect.width();
    float y1 = rect.y() + transform().transform().m_matrix[5];;
    float y2 = y1 + rect.height();

    float texPadding = TyGL::scrollbarThumbTexPadding;
    float texWidth = TyGL::scrollbarThumbTexWidth;
    float texHeight = TyGL::scrollbarThumbTexHeight;
    float texStartPointX = TyGL::scrollbarThumbTexX;
    float texStartPointY = TyGL::scrollbarThumbTexY;

    float xCoords[4] = { x1, x1 + TyGL::scrollbarThumbPadding, x2 - TyGL::scrollbarThumbPadding, x2 };
    float yCoords[4] = { y1, y1 + TyGL::scrollbarThumbPadding, y2 - TyGL::scrollbarThumbPadding, y2 };

    float xTexCoords[4] = { texStartPointX, texStartPointX + texPadding, texStartPointX + texWidth - texPadding, texStartPointX + texWidth };
    float yTexCoords[4] = { texStartPointY, texStartPointY + texPadding, texStartPointY + texHeight - texPadding, texStartPointY + texHeight };

    float positions[9 * 4];
    float texCoords[9 * 4];

    int index;

    for(int i = 0; i < 9; ++i) {
        index = i * 4;

        positions[index] = xCoords[i % 3];
        positions[index + 1] = yCoords[i / 3];
        positions[index + 2] = xCoords[i % 3 + 1];
        positions[index + 3] = yCoords[i / 3 + 1];

        texCoords[index] = xTexCoords[i % 3];
        texCoords[index + 1] = yTexCoords[i / 3];
        texCoords[index + 2] = xTexCoords[i % 3 + 1];
        texCoords[index + 3] = yTexCoords[i / 3 + 1];
    }

    paintTexturedQuads(9, s_uiElements->nativeImageForCurrentFrame(), positions, texCoords, rect);
}

void PlatformContextTyGL::paintEditBox(const IntRect& rect, bool focused)
{
    Color color;
    if (focused)
        color = Color(TyGL::blue);
    else
        color = Color(Color::gray);

    FloatRect left(rect.x(), rect.y(), 1, rect.height());
    FloatRect right(rect.maxX() - 1, rect.y(), 1, rect.height());
    FloatRect bottom(rect.x(), rect.y(), rect.width(), 1);
    FloatRect top(rect.x(), rect.maxY() - 1, rect.width(), 1);

    fillRect(left, color);
    fillRect(right, color);
    fillRect(bottom, color);
    fillRect(top, color);
}

void PlatformContextTyGL::drawUIArrow(const IntRect& rect, bool up)
{
    float x1 = rect.x() + transform().transform().m_matrix[4];;
    float x2 = x1 + rect.width();
    float y1 = rect.y() + transform().transform().m_matrix[5];;
    float y2 = y1 + rect.height();

    float texCoords[4] = { TyGL::arrowTexX, TyGL::arrowTexY, TyGL::arrowTexX + TyGL::arrowTexWidth, TyGL::arrowTexY + TyGL::arrowTexWidth };
    float positions[4];

    if (up) {
        positions[0] = x1;
        positions[1] = y1;
        positions[2] = x2;
        positions[3] = y2;
    } else {
        positions[0] = x1;
        positions[1] = y2;
        positions[2] = x2;
        positions[3] = y1;
    }

    paintTexturedQuads(1, s_uiElements->nativeImageForCurrentFrame(), positions, texCoords, rect);
}

} // namespace WebCore
