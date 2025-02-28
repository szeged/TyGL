/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WorkQueue_h
#define WorkQueue_h

#if OS(DARWIN)
#include <dispatch/dispatch.h>
#endif

#include <chrono>
#include <functional>
#include <wtf/Forward.h>
#include <wtf/FunctionDispatcher.h>
#include <wtf/Functional.h>
#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/Threading.h>
#include <wtf/Vector.h>

#if PLATFORM(GTK) || PLATFORM(EFL)
#include "PlatformProcessIdentifier.h"
#endif

#if PLATFORM(GTK)
#include <wtf/gobject/GMainLoopSource.h>
#include <wtf/gobject/GRefPtr.h>
#elif PLATFORM(EFL)
#include <DispatchQueueEfl.h>
#endif

class WorkQueue final : public FunctionDispatcher {
public:
    enum class QOS {
        UserInteractive,
        UserInitiated,
        Default,
        Utility,
        Background
    };
    
    static PassRefPtr<WorkQueue> create(const char* name, QOS = QOS::Default);
    virtual ~WorkQueue();

    virtual void dispatch(std::function<void ()>) override;
    void dispatchAfter(std::chrono::nanoseconds, std::function<void ()>);

#if OS(DARWIN)
    dispatch_queue_t dispatchQueue() const { return m_dispatchQueue; }
#elif PLATFORM(GTK)
    void registerSocketEventHandler(int, std::function<void ()>, std::function<void ()>);
    void unregisterSocketEventHandler(int);
#elif PLATFORM(EFL)
    void registerSocketEventHandler(int, std::function<void ()>);
    void unregisterSocketEventHandler(int);
#endif

private:
    explicit WorkQueue(const char* name, QOS);

    void platformInitialize(const char* name, QOS);
    void platformInvalidate();

#if OS(DARWIN)
    static void executeFunction(void*);
    dispatch_queue_t m_dispatchQueue;
#elif PLATFORM(GTK)
    ThreadIdentifier m_workQueueThread;
    GRefPtr<GMainContext> m_eventContext;
    Mutex m_eventLoopLock;
    GRefPtr<GMainLoop> m_eventLoop;
    GMainLoopSource m_socketEventSource;
#elif PLATFORM(EFL)
    RefPtr<DispatchQueue> m_dispatchQueue;
#endif
};

#endif // WorkQueue_h
