/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2004, 2006, 2007, 2008 Apple Inc. All rights reserved.
    Copyright (C) 2010 Google Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
 */

#ifndef ResourceLoadScheduler_h
#define ResourceLoadScheduler_h

#include "FrameLoaderTypes.h"
#include "ResourceLoaderOptions.h"
#include "ResourceLoadPriority.h"
#include "Timer.h"
#include <wtf/Deque.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class CachedResource;
class Frame;
class URL;
class NetscapePlugInStreamLoader;
class NetscapePlugInStreamLoaderClient;
class ResourceLoader;
class ResourceRequest;
class SubresourceLoader;

class ResourceLoadScheduler {
    WTF_MAKE_NONCOPYABLE(ResourceLoadScheduler); WTF_MAKE_FAST_ALLOCATED;
public:
    friend ResourceLoadScheduler* resourceLoadScheduler();

    WEBCORE_EXPORT virtual PassRefPtr<SubresourceLoader> scheduleSubresourceLoad(Frame*, CachedResource*, const ResourceRequest&, const ResourceLoaderOptions&);
    WEBCORE_EXPORT virtual PassRefPtr<NetscapePlugInStreamLoader> schedulePluginStreamLoad(Frame*, NetscapePlugInStreamLoaderClient*, const ResourceRequest&);
    WEBCORE_EXPORT virtual void remove(ResourceLoader*);
    virtual void setDefersLoading(ResourceLoader*, bool);
    virtual void crossOriginRedirectReceived(ResourceLoader*, const URL& redirectURL);
    
    WEBCORE_EXPORT virtual void servePendingRequests(ResourceLoadPriority minimumPriority = ResourceLoadPriorityVeryLow);
    WEBCORE_EXPORT virtual void suspendPendingRequests();
    WEBCORE_EXPORT virtual void resumePendingRequests();
    
    bool isSerialLoadingEnabled() const { return m_isSerialLoadingEnabled; }
    virtual void setSerialLoadingEnabled(bool b) { m_isSerialLoadingEnabled = b; }

    class Suspender {
    public:
        explicit Suspender(ResourceLoadScheduler& scheduler) : m_scheduler(scheduler) { m_scheduler.suspendPendingRequests(); }
        ~Suspender() { m_scheduler.resumePendingRequests(); }
    private:
        ResourceLoadScheduler& m_scheduler;
    };

protected:
    WEBCORE_EXPORT ResourceLoadScheduler();
    WEBCORE_EXPORT virtual ~ResourceLoadScheduler();

#if USE(QUICK_LOOK)
    WEBCORE_EXPORT bool maybeLoadQuickLookResource(ResourceLoader&);
#endif

private:
    void scheduleLoad(ResourceLoader*);
    void scheduleServePendingRequests();
    void requestTimerFired();

    bool isSuspendingPendingRequests() const { return !!m_suspendPendingRequestsCount; }

    class HostInformation {
        WTF_MAKE_NONCOPYABLE(HostInformation); WTF_MAKE_FAST_ALLOCATED;
    public:
        HostInformation(const String&, unsigned);
        ~HostInformation();
        
        const String& name() const { return m_name; }
        void schedule(ResourceLoader*, ResourceLoadPriority = ResourceLoadPriorityVeryLow);
        void addLoadInProgress(ResourceLoader*);
        void remove(ResourceLoader*);
        bool hasRequests() const;
        bool limitRequests(ResourceLoadPriority) const;

        typedef Deque<RefPtr<ResourceLoader>> RequestQueue;
        RequestQueue& requestsPending(ResourceLoadPriority priority) { return m_requestsPending[priority]; }

    private:                    
        RequestQueue m_requestsPending[ResourceLoadPriorityHighest + 1];
        typedef HashSet<RefPtr<ResourceLoader>> RequestMap;
        RequestMap m_requestsLoading;
        const String m_name;
        const int m_maxRequestsInFlight;
    };

    enum CreateHostPolicy {
        CreateIfNotFound,
        FindOnly
    };
    
    HostInformation* hostForURL(const URL&, CreateHostPolicy = FindOnly);
    WEBCORE_EXPORT void servePendingRequests(HostInformation*, ResourceLoadPriority);

    typedef HashMap<String, HostInformation*, StringHash> HostMap;
    HostMap m_hosts;
    HostInformation* m_nonHTTPProtocolHost;
        
    Timer m_requestTimer;

    unsigned m_suspendPendingRequestsCount;
    bool m_isSerialLoadingEnabled;
};

WEBCORE_EXPORT ResourceLoadScheduler* resourceLoadScheduler();

}

#endif
