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

#ifndef RenderThemeTyGL_h
#define RenderThemeTyGL_h

#include "RenderTheme.h"

namespace WebCore {

class RenderThemeTyGL : public RenderTheme {
public:
    static PassRefPtr<RenderTheme> create();

    virtual ~RenderThemeTyGL();

    virtual String extraDefaultStyleSheet() override;
    virtual String extraQuirksStyleSheet() override;
    virtual String extraPlugInsStyleSheet() override;

    virtual void systemFont(WebCore::CSSValueID, FontDescription&) const; // FIXME: override.

    // Returns the repeat interval of the animation for the progress bar.
    virtual double animationRepeatIntervalForProgressBar(RenderProgress&) const override;
    // Returns the duration of the animation for the progress bar.
    virtual double animationDurationForProgressBar(RenderProgress&) const override;

#if ENABLE(METER_ELEMENT)
    virtual IntSize meterSizeForBounds(const RenderMeter&, const IntRect&) const override;
    virtual bool supportsMeter(ControlPart) const override;
#endif

protected:
    virtual void updateCachedSystemFontDescription(CSSValueID systemFontID, FontDescription&) const override;

    // The platform selection color.
    virtual Color platformActiveSelectionBackgroundColor() const override;
    virtual Color platformInactiveSelectionBackgroundColor() const override;
    virtual Color platformActiveSelectionForegroundColor() const override;
    virtual Color platformInactiveSelectionForegroundColor() const override;

    virtual Color platformActiveListBoxSelectionBackgroundColor() const override;
    virtual Color platformInactiveListBoxSelectionBackgroundColor() const override;
    virtual Color platformActiveListBoxSelectionForegroundColor() const override;
    virtual Color platformInactiveListBoxSelectionForegroundColor() const override;

    // Highlighting colors for TextMatches.
    virtual Color platformActiveTextSearchHighlightColor() const override;
    virtual Color platformInactiveTextSearchHighlightColor() const override;

    virtual Color platformFocusRingColor() const override;

#if ENABLE(TOUCH_EVENTS)
    virtual Color platformTapHighlightColor() const override;
#endif

    virtual bool paintButton(const RenderObject&, const PaintInfo&, const IntRect&) override;
    virtual bool paintTextField(const RenderObject&, const PaintInfo&, const FloatRect&) override;
    virtual bool paintTextArea(const RenderObject&, const PaintInfo&, const FloatRect&) override;

    virtual bool paintCheckbox(const RenderObject&, const PaintInfo&, const IntRect&) override;
    virtual void setCheckboxSize(RenderStyle&) const override;

    virtual bool paintRadio(const RenderObject&, const PaintInfo&, const IntRect&) override;
    virtual void setRadioSize(RenderStyle&) const override;

    virtual bool paintMenuList(const RenderObject&, const PaintInfo&, const FloatRect&) override;
    virtual void adjustMenuListStyle(StyleResolver&, RenderStyle&, Element*) const override;

    virtual void adjustInnerSpinButtonStyle(StyleResolver&, RenderStyle&, Element*) const override;
    virtual bool paintInnerSpinButton(const RenderObject&, const PaintInfo&, const IntRect&) override;

    virtual void adjustProgressBarStyle(StyleResolver&, RenderStyle&, Element*) const override;
    virtual bool paintProgressBar(const RenderObject&, const PaintInfo&, const IntRect&) override;

#if ENABLE(METER_ELEMENT)
    virtual void adjustMeterStyle(StyleResolver&, RenderStyle&, Element*) const override;
    virtual bool paintMeter(const RenderObject&, const PaintInfo&, const IntRect&) override;
#endif

#if ENABLE(VIDEO)
    virtual String extraMediaControlsStyleSheet() override;
    virtual bool usesVerticalVolumeSlider() const { return false; }
    virtual bool usesMediaControlStatusDisplay() { return true; }
    virtual bool usesMediaControlVolumeSlider() const { return false; }

    virtual bool paintMediaPlayButton(const RenderObject&, const PaintInfo&, const IntRect&) override;
    virtual bool paintMediaMuteButton(const RenderObject&, const PaintInfo&, const IntRect&) override;
    virtual bool paintMediaSeekBackButton(const RenderObject&, const PaintInfo&, const IntRect&) override;
    virtual bool paintMediaSeekForwardButton(const RenderObject&, const PaintInfo&, const IntRect&) override;
    virtual bool paintMediaSliderTrack(const RenderObject&, const PaintInfo&, const IntRect&) override;
    virtual bool paintMediaVolumeSliderContainer(const RenderObject&, const PaintInfo&, const IntRect&) override;
    virtual bool paintMediaVolumeSliderTrack(const RenderObject&, const PaintInfo&, const IntRect&) override;
    virtual bool paintMediaRewindButton(const RenderObject&, const PaintInfo&, const IntRect&) override;
#endif

    virtual bool paintSliderTrack(const RenderObject&, const PaintInfo&, const IntRect&) override;
    virtual void adjustSliderTrackStyle(StyleResolver&, RenderStyle&, Element*) const override;

    virtual bool paintSliderThumb(const RenderObject&, const PaintInfo&, const IntRect&) override;
    virtual void adjustSliderThumbStyle(StyleResolver&, RenderStyle&, Element*) const override;

    virtual void adjustSliderThumbSize(RenderStyle&, Element*) const override;

    void setThemePath(const String& newThemePath);


#if ENABLE(DATALIST_ELEMENT)
    virtual IntSize sliderTickSize() const override;
    virtual int sliderTickOffsetFromTrackCenter() const override;
    virtual LayoutUnit sliderTickSnappingThreshold() const override;

    virtual bool supportsDataListUI(const AtomicString&) const override;
#endif

private:
    RenderThemeTyGL();

    static const int s_menuListBorder = 5;
    static const int s_menuListArrowSize = 6;

    static const int s_innerSpinButtonBorder = 3;
    static const int s_innerSpinButtonArrowSize = 2;

    static const int s_progressAnimationFrames = 10;
    static constexpr double s_progressAnimationInterval = 0.125;

    static const int s_sliderThumbWidth = 10;
    static const int s_sliderThumbHeight = 20;

    static const int s_sliderTrackHeight = 6;
};

}

#endif // RenderThemeTyGL_h
