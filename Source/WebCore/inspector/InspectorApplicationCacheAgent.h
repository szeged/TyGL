/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef InspectorApplicationCacheAgent_h
#define InspectorApplicationCacheAgent_h

#include "ApplicationCacheHost.h"
#include "InspectorWebAgentBase.h"
#include <inspector/InspectorBackendDispatchers.h>
#include <inspector/InspectorFrontendDispatchers.h>
#include <wtf/Noncopyable.h>

namespace Inspector {
class InspectorApplicationCacheFrontendDispatcher;
class InspectorObject;
class InspectorValue;
}

namespace WebCore {

class Frame;
class InspectorPageAgent;
class InstrumentingAgents;
class Page;
class ResourceResponse;

typedef String ErrorString;

class InspectorApplicationCacheAgent final : public InspectorAgentBase, public Inspector::InspectorApplicationCacheBackendDispatcherHandler {
    WTF_MAKE_NONCOPYABLE(InspectorApplicationCacheAgent); WTF_MAKE_FAST_ALLOCATED;
public:
    InspectorApplicationCacheAgent(InstrumentingAgents*, InspectorPageAgent*);
    virtual ~InspectorApplicationCacheAgent() { }

    virtual void didCreateFrontendAndBackend(Inspector::InspectorFrontendChannel*, Inspector::InspectorBackendDispatcher*) override;
    virtual void willDestroyFrontendAndBackend(Inspector::InspectorDisconnectReason) override;

    void updateApplicationCacheStatus(Frame*);
    void networkStateChanged();

    virtual void enable(ErrorString&) override;
    virtual void getFramesWithManifests(ErrorString&, RefPtr<Inspector::Protocol::Array<Inspector::Protocol::ApplicationCache::FrameWithManifest>>& result) override;
    virtual void getManifestForFrame(ErrorString&, const String& frameId, String* manifestURL) override;
    virtual void getApplicationCacheForFrame(ErrorString&, const String& frameId, RefPtr<Inspector::Protocol::ApplicationCache::ApplicationCache>&) override;

private:
    Ref<Inspector::Protocol::ApplicationCache::ApplicationCache> buildObjectForApplicationCache(const ApplicationCacheHost::ResourceInfoList&, const ApplicationCacheHost::CacheInfo&);
    Ref<Inspector::Protocol::Array<Inspector::Protocol::ApplicationCache::ApplicationCacheResource>> buildArrayForApplicationCacheResources(const ApplicationCacheHost::ResourceInfoList&);
    Ref<Inspector::Protocol::ApplicationCache::ApplicationCacheResource> buildObjectForApplicationCacheResource(const ApplicationCacheHost::ResourceInfo&);

    DocumentLoader* assertFrameWithDocumentLoader(ErrorString&, String frameId);

    InspectorPageAgent* m_pageAgent;
    std::unique_ptr<Inspector::InspectorApplicationCacheFrontendDispatcher> m_frontendDispatcher;
    RefPtr<Inspector::InspectorApplicationCacheBackendDispatcher> m_backendDispatcher;
};

} // namespace WebCore

#endif // InspectorApplicationCacheAgent_h
