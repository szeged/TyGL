/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef SVGDocument_h
#define SVGDocument_h

#include "Document.h"
#include "FloatPoint.h"

namespace WebCore {

class DOMImplementation;
class SVGElement;
class SVGSVGElement;

class SVGDocument final : public Document {
public:
    static Ref<SVGDocument> create(Frame* frame, const URL& url)
    {
        return adoptRef(*new SVGDocument(frame, url));
    }

    SVGSVGElement* rootElement() const;

    bool zoomAndPanEnabled() const;

    void startPan(const FloatPoint& start);
    void updatePan(const FloatPoint& pos) const;

private:
    SVGDocument(Frame*, const URL&);

    virtual Ref<Document> cloneDocumentWithoutChildren() const override;

    FloatPoint m_translate;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::SVGDocument)
    static bool isType(const WebCore::Document& document) { return document.isSVGDocument(); }
    static bool isType(const WebCore::Node& node) { return is<WebCore::Document>(node) && isType(downcast<WebCore::Document>(node)); }
SPECIALIZE_TYPE_TRAITS_END()

#endif // SVGDocument_h
