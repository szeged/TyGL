/*
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
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
 *
 */

#ifndef HTMLTableSectionElement_h
#define HTMLTableSectionElement_h

#include "HTMLNames.h"
#include "HTMLTablePartElement.h"

namespace WebCore {

class HTMLTableSectionElement final : public HTMLTablePartElement {
public:
    static Ref<HTMLTableSectionElement> create(const QualifiedName&, Document&);

    RefPtr<HTMLElement> insertRow(ExceptionCode& ec) { return insertRow(-1, ec); }
    RefPtr<HTMLElement> insertRow(int index, ExceptionCode&);
    void deleteRow(int index, ExceptionCode&);

    int numRows() const;

    const AtomicString& align() const;
    void setAlign(const AtomicString&);

    const AtomicString& ch() const;
    void setCh(const AtomicString&);

    const AtomicString& chOff() const;
    void setChOff(const AtomicString&);

    const AtomicString& vAlign() const;
    void setVAlign(const AtomicString&);

    RefPtr<HTMLCollection> rows();

private:
    HTMLTableSectionElement(const QualifiedName& tagName, Document&);

    virtual const StyleProperties* additionalPresentationAttributeStyle() override;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_BEGIN(WebCore::HTMLTableSectionElement)
    static bool isType(const WebCore::HTMLElement& element) { return element.hasTagName(WebCore::HTMLNames::theadTag) || element.hasTagName(WebCore::HTMLNames::tfootTag) || element.hasTagName(WebCore::HTMLNames::tbodyTag); }
    static bool isType(const WebCore::Node& node) { return is<WebCore::HTMLElement>(node) && isType(downcast<WebCore::HTMLElement>(node)); }
SPECIALIZE_TYPE_TRAITS_END()

#endif
