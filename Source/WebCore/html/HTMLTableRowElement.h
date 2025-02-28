/*
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2010, 2013 Apple Inc. All rights reserved.
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

#ifndef HTMLTableRowElement_h
#define HTMLTableRowElement_h

#include "HTMLTablePartElement.h"

namespace WebCore {

class HTMLTableRowElement final : public HTMLTablePartElement {
public:
    static Ref<HTMLTableRowElement> create(Document&);
    static Ref<HTMLTableRowElement> create(const QualifiedName&, Document&);

    int rowIndex() const;
    void setRowIndex(int);

    int sectionRowIndex() const;
    void setSectionRowIndex(int);

    RefPtr<HTMLElement> insertCell(ExceptionCode& ec) { return insertCell(-1, ec); }
    RefPtr<HTMLElement> insertCell(int index, ExceptionCode&);
    void deleteCell(int index, ExceptionCode&);

    RefPtr<HTMLCollection> cells();
    void setCells(HTMLCollection *, ExceptionCode&);

private:
    HTMLTableRowElement(const QualifiedName&, Document&);
};

} // namespace

#endif
