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

// Note: all the "#if 0" part originated from the nix port, we keep them now because it
// could help the implementation.

#include "config.h"
#include "RenderThemeTyGL.h"

#include "IntRect.h"
#include "TyGLDefs.h"
#include "HTMLMediaElement.h"
#include "InputTypeNames.h"
#include "PaintInfo.h"
#include "PlatformContextTyGL.h"
#include "UserAgentStyleSheets.h"
#include "RenderProgress.h"
#if ENABLE(METER_ELEMENT)
#include "HTMLMeterElement.h"
#include "RenderMeter.h"
#endif

namespace WebCore {

// Initialize default font size.
float RenderThemeTyGL::defaultFontSize = 16.0f;

static void setSizeIfAuto(RenderStyle& style, const IntSize& size)
{
    if (style.width().isIntrinsicOrAuto())
        style.setWidth(Length(size.width(), Fixed));
    if (style.height().isAuto())
        style.setHeight(Length(size.height(), Fixed));
}

PassRefPtr<RenderTheme> RenderTheme::themeForPage(Page*)
{
    return RenderThemeTyGL::create();
}

PassRefPtr<RenderTheme> RenderThemeTyGL::create()
{
    return adoptRef(new RenderThemeTyGL);
}

RenderThemeTyGL::RenderThemeTyGL()
    : RenderTheme()
{
}

RenderThemeTyGL::~RenderThemeTyGL()
{
}

String RenderThemeTyGL::extraDefaultStyleSheet()
{
    return "";
}

String RenderThemeTyGL::extraQuirksStyleSheet()
{
    return "";
}

String RenderThemeTyGL::extraPlugInsStyleSheet()
{
    return "";
}

Color RenderThemeTyGL::platformActiveSelectionBackgroundColor() const
{
    return Color(TyGL::blue);
}

Color RenderThemeTyGL::platformInactiveSelectionBackgroundColor() const
{
    return Color(Color::gray);
}

Color RenderThemeTyGL::platformActiveSelectionForegroundColor() const
{
    return Color(Color::white);
}

Color RenderThemeTyGL::platformInactiveSelectionForegroundColor() const
{
    return Color(Color::white);
}

Color RenderThemeTyGL::platformActiveListBoxSelectionBackgroundColor() const
{
    return platformActiveSelectionBackgroundColor();
}

Color RenderThemeTyGL::platformInactiveListBoxSelectionBackgroundColor() const
{
    return platformInactiveSelectionBackgroundColor();
}

Color RenderThemeTyGL::platformActiveListBoxSelectionForegroundColor() const
{
    return platformActiveSelectionForegroundColor();
}

Color RenderThemeTyGL::platformInactiveListBoxSelectionForegroundColor() const
{
    return platformActiveSelectionForegroundColor();
}

Color RenderThemeTyGL::platformActiveTextSearchHighlightColor() const
{
    return Color(TyGL::orange);
}

Color RenderThemeTyGL::platformInactiveTextSearchHighlightColor() const
{
    return Color(TyGL::yellow);
}

Color RenderThemeTyGL::platformFocusRingColor() const
{
    return  Color(Color::black);
}

#if ENABLE(TOUCH_EVENTS)
Color RenderThemeTyGL::platformTapHighlightColor() const
{
    return Color();
}
#endif

void RenderThemeTyGL::systemFont(WebCore::CSSValueID, FontDescription&) const
{
}

void RenderThemeTyGL::updateCachedSystemFontDescription(CSSValueID systemFontID, FontDescription& fontDescription) const
{
    fontDescription.setOneFamily("Sans");
    fontDescription.setSpecifiedSize(defaultFontSize);
    fontDescription.setIsAbsoluteSize(true);
    fontDescription.setWeight(FontWeightNormal);
    fontDescription.setItalic(FontItalicOff);
}

bool RenderThemeTyGL::paintButton(const RenderObject& object, const PaintInfo& info, const IntRect& rect)
{
    info.context->platformContext()->drawUIElement(PlatformGraphicsContext::Button, rect, this, &object);
    return false;
}

bool RenderThemeTyGL::paintTextField(const RenderObject& object, const PaintInfo& info, const FloatRect& rect)
{
    // WebThemeEngine does not handle border rounded corner and background image
    // so return true to draw CSS border and background.
    if (object.style().hasBorderRadius() || object.style().hasBackgroundImage())
        return true;

    const IntRect intRect(rect);

    info.context->platformContext()->drawUIElement(PlatformGraphicsContext::EditBox, intRect, this, &object);
    return false;
}

bool RenderThemeTyGL::paintTextArea(const RenderObject& object, const PaintInfo& info, const FloatRect& rect)
{
    return paintTextField(object, info, rect);
}

bool RenderThemeTyGL::paintCheckbox(const RenderObject& object, const PaintInfo& info, const IntRect& rect)
{
    info.context->platformContext()->drawUIElement(PlatformGraphicsContext::Checkbox, rect, this, &object);
    return false;
}

void RenderThemeTyGL::setCheckboxSize(RenderStyle& style) const
{
    // If the width and height are both specified, then we have nothing to do.
    if (!style.width().isIntrinsicOrAuto() && !style.height().isAuto())
        return;

    IntSize size = IntSize(13, 13);
    setSizeIfAuto(style, size);
}

bool RenderThemeTyGL::paintRadio(const RenderObject& object, const PaintInfo& info, const IntRect& rect)
{
    info.context->platformContext()->drawUIElement(PlatformGraphicsContext::RadioButton, rect, this, &object);
    return false;
}

void RenderThemeTyGL::setRadioSize(RenderStyle& style) const
{
    // If the width and height are both specified, then we have nothing to do.
    if (!style.width().isIntrinsicOrAuto() && !style.height().isAuto())
        return;

    IntSize size = IntSize(13, 13);
    setSizeIfAuto(style, size);
}

bool RenderThemeTyGL::paintMenuList(const RenderObject& o, const PaintInfo& i, const FloatRect& rect)
{
#if 0
    themeEngine()->paintMenuList(webCanvas(i), getWebThemeState(this, o), toNixRect(rect));
#endif

    return false;
}

void RenderThemeTyGL::adjustMenuListStyle(StyleResolver&, RenderStyle& style, Element*) const
{
    style.resetBorder();
    style.setWhiteSpace(PRE);

    int paddingTop = s_menuListBorder;
    int paddingLeft = s_menuListBorder;
    int paddingBottom = s_menuListBorder;
    int paddingRight = 2 * s_menuListBorder + s_menuListArrowSize;

    style.setPaddingTop(Length(paddingTop, Fixed));
    style.setPaddingRight(Length(paddingRight, Fixed));
    style.setPaddingBottom(Length(paddingBottom, Fixed));
    style.setPaddingLeft(Length(paddingLeft, Fixed));
}

void RenderThemeTyGL::adjustProgressBarStyle(StyleResolver&, RenderStyle& style, Element*) const
{
    style.setBoxShadow(nullptr);
}

bool RenderThemeTyGL::paintProgressBar(const RenderObject& object, const PaintInfo& info, const IntRect& rect)
{
    const RenderProgress* renderProgress = &(downcast<RenderProgress>(object));

    info.context->platformContext()->fillRect(rect, Color(TyGL::gray225));

    IntRect progress;
    if (renderProgress->isDeterminate()) {
        progress.setLocation(rect.location());
        progress.setWidth(rect.width() * renderProgress->position());
        progress.setHeight(rect.height());
    } else {
        int width = rect.width() / 5.0;
        float x = rect.x() + fabs(renderProgress->animationProgress() - 0.5) * 2 * (rect.width() - width);
        progress.setX(x);
        progress.setY(rect.y());
        progress.setWidth(width);
        progress.setHeight(rect.height());
    }
    info.context->platformContext()->fillRect(progress, Color(TyGL::green));

    return false;
}

double RenderThemeTyGL::animationRepeatIntervalForProgressBar(RenderProgress&) const
{
    return s_progressAnimationInterval;
}

double RenderThemeTyGL::animationDurationForProgressBar(RenderProgress&) const
{
    return s_progressAnimationInterval * s_progressAnimationFrames * 2; // "2" for back and forth.
}

bool RenderThemeTyGL::paintSliderTrack(const RenderObject& object, const PaintInfo& info, const IntRect& rect)
{
    IntRect track(rect);
    track.setY(rect.y() + (rect.height() - s_sliderTrackHeight) / 2.0);
    track.setHeight(s_sliderTrackHeight);

    info.context->platformContext()->fillRect(track, Color(TyGL::gray225));

#if ENABLE(DATALIST_ELEMENT)
    paintSliderTicks(object, info, rect);
#endif

    return false;
}

void RenderThemeTyGL::adjustSliderTrackStyle(StyleResolver&, RenderStyle& style, Element*) const
{
    style.setBoxShadow(nullptr);
}

bool RenderThemeTyGL::paintSliderThumb(const RenderObject& object, const PaintInfo& info, const IntRect& rect)
{
    Color color;

    if (isPressed(object) || isHovered(object))
        color = Color(TyGL::gray240);
    else
        color = Color(TyGL::gray225);

    info.context->platformContext()->fillRect(rect, color);

    Color gray(Color::gray);

    IntRect left(rect.x(), rect.y(), 1, rect.height());
    IntRect right(rect.maxX() - 1, rect.y(), 1, rect.height());
    IntRect bottom(rect.x(), rect.y(), rect.width(), 1);
    IntRect top(rect.x(), rect.maxY() - 1, rect.width(), 1);

    info.context->platformContext()->fillRect(left, gray);
    info.context->platformContext()->fillRect(right, gray);
    info.context->platformContext()->fillRect(bottom, gray);
    info.context->platformContext()->fillRect(top, gray);

    return false;
}

void RenderThemeTyGL::adjustSliderThumbStyle(StyleResolver& styleResolver, RenderStyle& style, Element* element) const
{
    RenderTheme::adjustSliderThumbStyle(styleResolver, style, element);
    style.setBoxShadow(nullptr);
}

void RenderThemeTyGL::adjustSliderThumbSize(RenderStyle& style, Element*) const
{
    ControlPart part = style.appearance();
    if (part == SliderThumbVerticalPart) {
        style.setWidth(Length(s_sliderThumbWidth, Fixed));
        style.setHeight(Length(s_sliderThumbHeight, Fixed));
    } else if (part == SliderThumbHorizontalPart) {
        style.setWidth(Length(s_sliderThumbWidth, Fixed));
        style.setHeight(Length(s_sliderThumbHeight, Fixed));
    }
}

#if ENABLE(DATALIST_ELEMENT)
IntSize RenderThemeTyGL::sliderTickSize() const
{
    return IntSize(1, 6);
}

int RenderThemeTyGL::sliderTickOffsetFromTrackCenter() const
{
    return -12;
}

LayoutUnit RenderThemeTyGL::sliderTickSnappingThreshold() const
{
    return 5;
}

bool RenderThemeTyGL::supportsDataListUI(const AtomicString& type) const
{
    return type == InputTypeNames::range();
}
#endif

void RenderThemeTyGL::adjustInnerSpinButtonStyle(StyleResolver&, RenderStyle& style, Element*) const
{
    style.resetBorder();
    style.setWhiteSpace(PRE);

    int paddingTop = s_innerSpinButtonBorder;
    int paddingLeft = s_innerSpinButtonBorder;
    int paddingBottom = s_innerSpinButtonBorder;
    int paddingRight = 2 * s_innerSpinButtonBorder + s_innerSpinButtonArrowSize;

    style.setPaddingTop(Length(paddingTop, Fixed));
    style.setPaddingRight(Length(paddingRight, Fixed));
    style.setPaddingBottom(Length(paddingBottom, Fixed));
    style.setPaddingLeft(Length(paddingLeft, Fixed));
}

bool RenderThemeTyGL::paintInnerSpinButton(const RenderObject& object, const PaintInfo& info, const IntRect& rect)
{
    IntRect up(rect);
    up.setHeight(rect.height() / 2.0);

    IntRect down(up);
    down.setY(up.maxY());

    Color gray(TyGL::gray225);

    if (!isPressed(object))
        info.context->platformContext()->fillRect(rect, gray);
    else {
        if (isSpinUpButtonPartPressed(object)) {
            info.context->platformContext()->fillRect(up, Color(Color::gray));
            info.context->platformContext()->fillRect(down, gray);
        } else {
            info.context->platformContext()->fillRect(up, gray);
            info.context->platformContext()->fillRect(down, Color(Color::gray));
        }
    }

    info.context->platformContext()->drawUIArrow(up, true);
    info.context->platformContext()->drawUIArrow(down, false);

    return false;
}

#if ENABLE(METER_ELEMENT)
void RenderThemeTyGL::adjustMeterStyle(StyleResolver&, RenderStyle& style, Element*) const
{
    style.setBoxShadow(nullptr);
}

IntSize RenderThemeTyGL::meterSizeForBounds(const RenderMeter&, const IntRect& bounds) const
{
    return bounds.size();
}

bool RenderThemeTyGL::supportsMeter(ControlPart part) const
{
    switch (part) {
    case RelevancyLevelIndicatorPart:
    case DiscreteCapacityLevelIndicatorPart:
    case RatingLevelIndicatorPart:
    case MeterPart:
    case ContinuousCapacityLevelIndicatorPart:
        return true;
    default:
        return false;
    }
}

bool RenderThemeTyGL::paintMeter(const RenderObject& object, const PaintInfo& info, const IntRect& rect)
{
    if (!object.isMeter())
        return true;

    HTMLMeterElement* meterElement = downcast<RenderMeter>(object).meterElement();

    info.context->platformContext()->fillRect(rect, Color(TyGL::gray225));

    double min = meterElement->min();
    double max = meterElement->max();
    double clampedValue = (meterElement->value() > meterElement->max()) ? meterElement->max()
        : ((meterElement->value() < meterElement->min()) ? meterElement->min() : meterElement->value());
    double normalizedValue = (clampedValue - min) / (max - min);

    double low = meterElement->low();
    double high = meterElement->high();
    double optimum = meterElement->optimum();

    double optimumHigh = (optimum > high) ? max : ((optimum < low) ? low : high);
    double optimumLow = (optimum > high) ? high : ((optimum < low) ? min : low);

    int red = (clampedValue < optimumHigh && clampedValue > optimumLow) ? 0 : 255;
    int green = (optimum > high && clampedValue < low || optimum < low && clampedValue > high) ? 0 : 255;

    Color color = Color(red, green, 0);
    info.context->platformContext()->fillRect(IntRect(rect.location(), IntSize(rect.width() * normalizedValue, rect.height())), color);

    return false;
}
#endif

#if ENABLE(VIDEO)

String RenderThemeTyGL::extraMediaControlsStyleSheet()
{
    return String(mediaControlsUserAgentStyleSheet, sizeof(mediaControlsUserAgentStyleSheet));
}

bool RenderThemeTyGL::paintMediaPlayButton(const RenderObject& o, const PaintInfo& i, const IntRect& rect)
{
#if 0
    auto state = toHTMLMediaElement(o->node()->shadowHost())->canPlay() ? Nix::ThemeEngine::StatePaused : Nix::ThemeEngine::StatePlaying;
    themeEngine()->paintMediaPlayButton(webCanvas(i), state, toNixRect(rect));
#endif

    return false;
}

bool RenderThemeTyGL::paintMediaMuteButton(const RenderObject& o, const PaintInfo& i, const IntRect& rect)
{
#if 0
    auto state = toHTMLMediaElement(o->node()->shadowHost())->muted() ? Nix::ThemeEngine::StateMuted : Nix::ThemeEngine::StateNotMuted;
    themeEngine()->paintMediaMuteButton(webCanvas(i), state, toNixRect(rect));
#endif
    return false;
}

bool RenderThemeTyGL::paintMediaSeekBackButton(const RenderObject&, const PaintInfo& i, const IntRect& rect)
{
#if 0
    themeEngine()->paintMediaSeekBackButton(webCanvas(i), toNixRect(rect));
#endif

    return false;
}

bool RenderThemeTyGL::paintMediaSeekForwardButton(const RenderObject&, const PaintInfo& i, const IntRect& rect)
{
#if 0
    themeEngine()->paintMediaSeekForwardButton(webCanvas(i), toNixRect(rect));
#endif

    return false;
}

bool RenderThemeTyGL::paintMediaSliderTrack(const RenderObject&, const PaintInfo&, const IntRect&)
{
    return false;
}

bool RenderThemeTyGL::paintMediaVolumeSliderContainer(const RenderObject&, const PaintInfo&, const IntRect&)
{
    return false;
}

bool RenderThemeTyGL::paintMediaVolumeSliderTrack(const RenderObject&, const PaintInfo&, const IntRect&)
{
    return false;
}

bool RenderThemeTyGL::paintMediaRewindButton(const RenderObject&, const PaintInfo& i, const IntRect& rect)
{
#if 0
    themeEngine()->paintMediaRewindButton(webCanvas(i), toNixRect(rect));
#endif

    return false;
}

void RenderThemeTyGL::setThemePath(const String&)
{
}

#endif

}
