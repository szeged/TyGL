/*
 * Copyright (C) 2015 Canon Inc.
 * Copyright (C) 2015 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted, provided that the following conditions
 * are required to be met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Canon Inc. nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CANON INC. AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CANON INC. AND ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ReadableStream_h
#define ReadableStream_h

#if ENABLE(STREAMS_API)

#include "ActiveDOMObject.h"
#include "ReadableStreamSource.h"
#include "ScriptWrappable.h"
#include <functional>
#include <wtf/Ref.h>

namespace WebCore {

class ScriptExecutionContext;

// ReadableStream implements the core of the streams API ReadableStream functionality.
// It handles in particular the backpressure according the queue size.
// ReadableStream is using a ReadableStreamSource to get data in its queue.
// See https://streams.spec.whatwg.org/#rs
class ReadableStream : public ActiveDOMObject, public ScriptWrappable, public RefCounted<ReadableStream> {
public:
    enum class State {
        Waiting,
        Closed,
        Readable,
        Errored
    };

    static Ref<ReadableStream> create(ScriptExecutionContext&, Ref<ReadableStreamSource>&&);
    virtual ~ReadableStream();

    virtual const char* activeDOMObjectName() const override { return "ReadableStream"; }

    // JS API implementation.
    String state() const;

    typedef std::function<void(RefPtr<ReadableStream>)> SuccessCallback;
    void closed(SuccessCallback);
    void ready(SuccessCallback);

private:
    ReadableStream(ScriptExecutionContext&, Ref<ReadableStreamSource>&&);

    State m_state;
    Ref<ReadableStreamSource> m_source;
};

}

#endif

#endif // ReadableStream_h
