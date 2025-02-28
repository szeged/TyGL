/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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

#ifndef SVGCursorElement_h
#define SVGCursorElement_h

#include "SVGAnimatedBoolean.h"
#include "SVGAnimatedLength.h"
#include "SVGAnimatedString.h"
#include "SVGElement.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGTests.h"
#include "SVGURIReference.h"

namespace WebCore {

class SVGCursorElement final : public SVGElement,
                               public SVGTests,
                               public SVGExternalResourcesRequired,
                               public SVGURIReference {
public:
    static Ref<SVGCursorElement> create(const QualifiedName&, Document&);

    virtual ~SVGCursorElement();

    void addClient(SVGElement*);
    void removeClient(SVGElement*);
    void removeReferencedElement(SVGElement*);

private:
    SVGCursorElement(const QualifiedName&, Document&);

    virtual bool isValid() const override { return SVGTests::isValid(); }

    bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) override;
    virtual void svgAttributeChanged(const QualifiedName&) override;

    virtual bool rendererIsNeeded(const RenderStyle&) override { return false; }

    virtual void addSubresourceAttributeURLs(ListHashSet<URL>&) const override;

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGCursorElement)
        DECLARE_ANIMATED_LENGTH(X, x)
        DECLARE_ANIMATED_LENGTH(Y, y)
        DECLARE_ANIMATED_STRING(Href, href)
        DECLARE_ANIMATED_BOOLEAN(ExternalResourcesRequired, externalResourcesRequired)
    END_DECLARE_ANIMATED_PROPERTIES

    // SVGTests
    virtual void synchronizeRequiredFeatures() override { SVGTests::synchronizeRequiredFeatures(this); }
    virtual void synchronizeRequiredExtensions() override { SVGTests::synchronizeRequiredExtensions(this); }
    virtual void synchronizeSystemLanguage() override { SVGTests::synchronizeSystemLanguage(this); }

    HashSet<SVGElement*> m_clients;
};

} // namespace WebCore

#endif
