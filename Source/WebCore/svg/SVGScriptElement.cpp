/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2007 Rob Buis <buis@kde.org>
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

#include "config.h"
#include "SVGScriptElement.h"

#include "Attribute.h"
#include "Document.h"
#include "Event.h"
#include "EventNames.h"
#include "HTMLNames.h"
#include "SVGAnimatedStaticPropertyTearOff.h"
#include "SVGElementInstance.h"
#include "XLinkNames.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_STRING(SVGScriptElement, XLinkNames::hrefAttr, Href, href)
DEFINE_ANIMATED_BOOLEAN(SVGScriptElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGScriptElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(href)
    REGISTER_LOCAL_ANIMATED_PROPERTY(externalResourcesRequired)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGScriptElement::SVGScriptElement(const QualifiedName& tagName, Document& document, bool wasInsertedByParser, bool alreadyStarted)
    : SVGElement(tagName, document)
    , ScriptElement(*this, wasInsertedByParser, alreadyStarted)
    , m_svgLoadEventTimer(*this, &SVGElement::svgLoadEventTimerFired)
{
    ASSERT(hasTagName(SVGNames::scriptTag));
    registerAnimatedPropertiesForSVGScriptElement();
}

Ref<SVGScriptElement> SVGScriptElement::create(const QualifiedName& tagName, Document& document, bool insertedByParser)
{
    return adoptRef(*new SVGScriptElement(tagName, document, insertedByParser, false));
}

bool SVGScriptElement::isSupportedAttribute(const QualifiedName& attrName)
{
    static NeverDestroyed<HashSet<QualifiedName>> supportedAttributes;
    if (supportedAttributes.get().isEmpty()) {
        SVGURIReference::addSupportedAttributes(supportedAttributes);
        SVGExternalResourcesRequired::addSupportedAttributes(supportedAttributes);
        supportedAttributes.get().add(SVGNames::typeAttr);
        supportedAttributes.get().add(HTMLNames::onerrorAttr);
    }
    return supportedAttributes.get().contains<SVGAttributeHashTranslator>(attrName);
}

void SVGScriptElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (!isSupportedAttribute(name)) {
        SVGElement::parseAttribute(name, value);
        return;
    }

    if (name == SVGNames::typeAttr)
        return;

    if (name == HTMLNames::onerrorAttr) {
        setAttributeEventListener(eventNames().errorEvent, name, value);
        return;
    }

    if (SVGURIReference::parseAttribute(name, value))
        return;
    if (SVGExternalResourcesRequired::parseAttribute(name, value))
        return;

    ASSERT_NOT_REACHED();
}

void SVGScriptElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGElement::svgAttributeChanged(attrName);
        return;
    }

    InstanceInvalidationGuard guard(*this);

    if (attrName == SVGNames::typeAttr || attrName == HTMLNames::onerrorAttr)
        return;

    if (SVGURIReference::isKnownAttribute(attrName)) {
        handleSourceAttribute(href());
        return;
    }

    if (SVGExternalResourcesRequired::handleAttributeChange(this, attrName))
        return;

    ASSERT_NOT_REACHED();
}

Node::InsertionNotificationRequest SVGScriptElement::insertedInto(ContainerNode& rootParent)
{
    SVGElement::insertedInto(rootParent);
    ScriptElement::insertedInto(rootParent);
    if (rootParent.inDocument())
        SVGExternalResourcesRequired::insertedIntoDocument(this);
    return InsertionDone;
}

void SVGScriptElement::childrenChanged(const ChildChange& change)
{
    SVGElement::childrenChanged(change);
    ScriptElement::childrenChanged();
}

bool SVGScriptElement::isURLAttribute(const Attribute& attribute) const
{
    return attribute.name() == sourceAttributeValue();
}

void SVGScriptElement::finishParsingChildren()
{
    SVGElement::finishParsingChildren();
    SVGExternalResourcesRequired::finishParsingChildren();
}

void SVGScriptElement::addSubresourceAttributeURLs(ListHashSet<URL>& urls) const
{
    SVGElement::addSubresourceAttributeURLs(urls);

    addSubresourceURL(urls, document().completeURL(href()));
}

String SVGScriptElement::sourceAttributeValue() const
{
    return href();
}

String SVGScriptElement::charsetAttributeValue() const
{
    return String();
}

String SVGScriptElement::typeAttributeValue() const
{
    return getAttribute(SVGNames::typeAttr).string();
}

String SVGScriptElement::languageAttributeValue() const
{
    return String();
}

String SVGScriptElement::forAttributeValue() const
{
    return String();
}

String SVGScriptElement::eventAttributeValue() const
{
    return String();
}

bool SVGScriptElement::asyncAttributeValue() const
{
    return false;
}

bool SVGScriptElement::deferAttributeValue() const
{
    return false;
}

bool SVGScriptElement::hasSourceAttribute() const
{
    return hasAttribute(XLinkNames::hrefAttr);
}

RefPtr<Element> SVGScriptElement::cloneElementWithoutAttributesAndChildren(Document& targetDocument)
{
    return adoptRef(new SVGScriptElement(tagQName(), targetDocument, false, alreadyStarted()));
}

#ifndef NDEBUG
bool SVGScriptElement::filterOutAnimatableAttribute(const QualifiedName& name) const
{
    return name == SVGNames::typeAttr;
}
#endif

}
