/*
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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
#include "SVGMPathElement.h"

#include "Document.h"
#include "SVGAnimateMotionElement.h"
#include "SVGDocumentExtensions.h"
#include "SVGNames.h"
#include "SVGPathElement.h"
#include "XLinkNames.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_STRING(SVGMPathElement, XLinkNames::hrefAttr, Href, href)
DEFINE_ANIMATED_BOOLEAN(SVGMPathElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGMPathElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(href)
    REGISTER_LOCAL_ANIMATED_PROPERTY(externalResourcesRequired)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGMPathElement::SVGMPathElement(const QualifiedName& tagName, Document& document)
    : SVGElement(tagName, document)
{
    ASSERT(hasTagName(SVGNames::mpathTag));
    registerAnimatedPropertiesForSVGMPathElement();
}

Ref<SVGMPathElement> SVGMPathElement::create(const QualifiedName& tagName, Document& document)
{
    return adoptRef(*new SVGMPathElement(tagName, document));
}

SVGMPathElement::~SVGMPathElement()
{
    clearResourceReferences();
}

void SVGMPathElement::buildPendingResource()
{
    clearResourceReferences();
    if (!inDocument())
        return;

    String id;
    Element* target = SVGURIReference::targetElementFromIRIString(href(), document(), &id);
    if (!target) {
        // Do not register as pending if we are already pending this resource.
        if (document().accessSVGExtensions().isPendingResource(this, id))
            return;

        if (!id.isEmpty()) {
            document().accessSVGExtensions().addPendingResource(id, this);
            ASSERT(hasPendingResources());
        }
    } else if (target->isSVGElement()) {
        // Register us with the target in the dependencies map. Any change of hrefElement
        // that leads to relayout/repainting now informs us, so we can react to it.
        document().accessSVGExtensions().addElementReferencingTarget(this, downcast<SVGElement>(target));
    }

    targetPathChanged();
}

void SVGMPathElement::clearResourceReferences()
{
    document().accessSVGExtensions().removeAllTargetReferencesForElement(this);
}

Node::InsertionNotificationRequest SVGMPathElement::insertedInto(ContainerNode& rootParent)
{
    SVGElement::insertedInto(rootParent);
    if (rootParent.inDocument())
        return InsertionShouldCallDidNotifySubtreeInsertions;
    return InsertionDone;
}

void SVGMPathElement::didNotifySubtreeInsertions(ContainerNode*)
{
    buildPendingResource();
}

void SVGMPathElement::removedFrom(ContainerNode& rootParent)
{
    SVGElement::removedFrom(rootParent);
    notifyParentOfPathChange(&rootParent);
    if (rootParent.inDocument())
        clearResourceReferences();
}

bool SVGMPathElement::isSupportedAttribute(const QualifiedName& attrName)
{
    static NeverDestroyed<HashSet<QualifiedName>> supportedAttributes;
    if (supportedAttributes.get().isEmpty()) {
        SVGURIReference::addSupportedAttributes(supportedAttributes);
        SVGExternalResourcesRequired::addSupportedAttributes(supportedAttributes);
    }
    return supportedAttributes.get().contains<SVGAttributeHashTranslator>(attrName);
}

void SVGMPathElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (!isSupportedAttribute(name)) {
        SVGElement::parseAttribute(name, value);
        return;
    }

    if (SVGURIReference::parseAttribute(name, value))
        return;
    if (SVGExternalResourcesRequired::parseAttribute(name, value))
        return;

    ASSERT_NOT_REACHED();
}

void SVGMPathElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGElement::svgAttributeChanged(attrName);
        return;
    }

    InstanceInvalidationGuard guard(*this);

    if (SVGURIReference::isKnownAttribute(attrName)) {
        buildPendingResource();
        return;
    }

    if (SVGExternalResourcesRequired::isKnownAttribute(attrName))
        return;

    ASSERT_NOT_REACHED();
}

SVGPathElement* SVGMPathElement::pathElement()
{
    Element* target = targetElementFromIRIString(href(), document());
    if (is<SVGPathElement>(target))
        return downcast<SVGPathElement>(target);
    return nullptr;
}

void SVGMPathElement::targetPathChanged()
{
    notifyParentOfPathChange(parentNode());
}

void SVGMPathElement::notifyParentOfPathChange(ContainerNode* parent)
{
    if (is<SVGAnimateMotionElement>(parent))
        downcast<SVGAnimateMotionElement>(*parent).updateAnimationPath();
}

} // namespace WebCore
