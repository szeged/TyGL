/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "InspectorDOMStorageAgent.h"

#include "DOMWindow.h"
#include "Database.h"
#include "Document.h"
#include "ExceptionCode.h"
#include "ExceptionCodeDescription.h"
#include "Frame.h"
#include "InspectorPageAgent.h"
#include "InstrumentingAgents.h"
#include "Page.h"
#include "PageGroup.h"
#include "SecurityOrigin.h"
#include "Storage.h"
#include "StorageNamespaceProvider.h"
#include "VoidCallback.h"
#include <inspector/InspectorFrontendDispatchers.h>
#include <inspector/InspectorValues.h>
#include <wtf/Vector.h>

using namespace Inspector;

namespace WebCore {

InspectorDOMStorageAgent::InspectorDOMStorageAgent(InstrumentingAgents* instrumentingAgents, InspectorPageAgent* pageAgent)
    : InspectorAgentBase(ASCIILiteral("DOMStorage"), instrumentingAgents)
    , m_pageAgent(pageAgent)
    , m_enabled(false)
{
    m_instrumentingAgents->setInspectorDOMStorageAgent(this);
}

InspectorDOMStorageAgent::~InspectorDOMStorageAgent()
{
    m_instrumentingAgents->setInspectorDOMStorageAgent(nullptr);
    m_instrumentingAgents = nullptr;
}

void InspectorDOMStorageAgent::didCreateFrontendAndBackend(Inspector::InspectorFrontendChannel* frontendChannel, InspectorBackendDispatcher* backendDispatcher)
{
    m_frontendDispatcher = std::make_unique<InspectorDOMStorageFrontendDispatcher>(frontendChannel);
    m_backendDispatcher = InspectorDOMStorageBackendDispatcher::create(backendDispatcher, this);
}

void InspectorDOMStorageAgent::willDestroyFrontendAndBackend(InspectorDisconnectReason)
{
    m_frontendDispatcher = nullptr;
    m_backendDispatcher.clear();

    ErrorString unused;
    disable(unused);
}

void InspectorDOMStorageAgent::enable(ErrorString&)
{
    m_enabled = true;
}

void InspectorDOMStorageAgent::disable(ErrorString&)
{
    m_enabled = false;
}

void InspectorDOMStorageAgent::getDOMStorageItems(ErrorString& errorString, const RefPtr<InspectorObject>&& storageId, RefPtr<Inspector::Protocol::Array<Inspector::Protocol::Array<String>>>& items)
{
    Frame* frame;
    RefPtr<StorageArea> storageArea = findStorageArea(errorString, storageId.copyRef(), frame);
    if (!storageArea) {
        errorString = ASCIILiteral("No StorageArea for given storageId");
        return;
    }

    auto storageItems = Inspector::Protocol::Array<Inspector::Protocol::Array<String>>::create();

    for (unsigned i = 0; i < storageArea->length(); ++i) {
        String key = storageArea->key(i);
        String value = storageArea->item(key);

        auto entry = Inspector::Protocol::Array<String>::create();
        entry->addItem(key);
        entry->addItem(value);
        storageItems->addItem(WTF::move(entry));
    }

    items = WTF::move(storageItems);
}

void InspectorDOMStorageAgent::setDOMStorageItem(ErrorString& errorString, const RefPtr<InspectorObject>&& storageId, const String& key, const String& value)
{
    Frame* frame;
    RefPtr<StorageArea> storageArea = findStorageArea(errorString, storageId.copyRef(), frame);
    if (!storageArea) {
        errorString = ASCIILiteral("Storage not found");
        return;
    }

    bool quotaException = false;
    storageArea->setItem(frame, key, value, quotaException);
    if (quotaException)
        errorString = ExceptionCodeDescription(QUOTA_EXCEEDED_ERR).name;
}

void InspectorDOMStorageAgent::removeDOMStorageItem(ErrorString& errorString, const RefPtr<InspectorObject>&& storageId, const String& key)
{
    Frame* frame;
    RefPtr<StorageArea> storageArea = findStorageArea(errorString, storageId.copyRef(), frame);
    if (!storageArea) {
        errorString = ASCIILiteral("Storage not found");
        return;
    }

    storageArea->removeItem(frame, key);
}

String InspectorDOMStorageAgent::storageId(Storage* storage)
{
    ASSERT(storage);
    Document* document = storage->frame()->document();
    ASSERT(document);
    DOMWindow* window = document->domWindow();
    ASSERT(window);
    RefPtr<SecurityOrigin> securityOrigin = document->securityOrigin();
    bool isLocalStorage = window->optionalLocalStorage() == storage;
    return storageId(securityOrigin.get(), isLocalStorage)->toJSONString();
}

RefPtr<Inspector::Protocol::DOMStorage::StorageId> InspectorDOMStorageAgent::storageId(SecurityOrigin* securityOrigin, bool isLocalStorage)
{
    return Inspector::Protocol::DOMStorage::StorageId::create()
        .setSecurityOrigin(securityOrigin->toRawString())
        .setIsLocalStorage(isLocalStorage)
        .release();
}

void InspectorDOMStorageAgent::didDispatchDOMStorageEvent(const String& key, const String& oldValue, const String& newValue, StorageType storageType, SecurityOrigin* securityOrigin, Page*)
{
    if (!m_frontendDispatcher || !m_enabled)
        return;

    RefPtr<Inspector::Protocol::DOMStorage::StorageId> id = storageId(securityOrigin, storageType == LocalStorage);

    if (key.isNull())
        m_frontendDispatcher->domStorageItemsCleared(id);
    else if (newValue.isNull())
        m_frontendDispatcher->domStorageItemRemoved(id, key);
    else if (oldValue.isNull())
        m_frontendDispatcher->domStorageItemAdded(id, key, newValue);
    else
        m_frontendDispatcher->domStorageItemUpdated(id, key, oldValue, newValue);
}

RefPtr<StorageArea> InspectorDOMStorageAgent::findStorageArea(ErrorString& errorString, const RefPtr<InspectorObject>&& storageId, Frame*& targetFrame)
{
    String securityOrigin;
    bool isLocalStorage = false;
    bool success = storageId->getString(ASCIILiteral("securityOrigin"), securityOrigin);
    if (success)
        success = storageId->getBoolean(ASCIILiteral("isLocalStorage"), isLocalStorage);
    if (!success) {
        errorString = ASCIILiteral("Invalid storageId format");
        targetFrame = nullptr;
        return nullptr;
    }

    targetFrame = m_pageAgent->findFrameWithSecurityOrigin(securityOrigin);
    if (!targetFrame) {
        errorString = ASCIILiteral("Frame not found for the given security origin");
        return nullptr;
    }

    return m_pageAgent->page()->storageNamespaceProvider().localStorageArea(*targetFrame->document());
}

} // namespace WebCore
