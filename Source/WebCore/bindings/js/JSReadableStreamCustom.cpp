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

#include "config.h"

#if ENABLE(STREAMS_API)
#include "JSReadableStream.h"

#include "ExceptionCode.h"
#include "JSDOMPromise.h"
#include "ReadableStream.h"
#include "ReadableStreamJSSource.h"

using namespace JSC;

namespace WebCore {

JSValue JSReadableStream::read(ExecState* exec)
{
    JSValue error = createError(exec, ASCIILiteral("read is not implemented"));
    return exec->vm().throwException(exec, error);
}

JSValue JSReadableStream::ready(ExecState* exec) const
{
    DeferredWrapper wrapper(exec, globalObject());
    auto successCallback = [wrapper](RefPtr<ReadableStream> stream) mutable {
        wrapper.resolve(stream.get());
    };
    impl().ready(WTF::move(successCallback));

    return wrapper.promise();
}

JSValue JSReadableStream::closed(ExecState* exec) const
{
    DeferredWrapper wrapper(exec, globalObject());
    auto successCallback = [wrapper](RefPtr<ReadableStream> stream) mutable {
        wrapper.resolve(stream.get());
    };
    impl().closed(WTF::move(successCallback));

    return wrapper.promise();
}

JSValue JSReadableStream::cancel(ExecState* exec)
{
    JSValue error = createError(exec, ASCIILiteral("cancel is not implemented"));
    return exec->vm().throwException(exec, error);
}

JSValue JSReadableStream::pipeTo(ExecState* exec)
{
    JSValue error = createError(exec, ASCIILiteral("pipeTo is not implemented"));
    return exec->vm().throwException(exec, error);
}

JSValue JSReadableStream::pipeThrough(ExecState* exec)
{
    JSValue error = createError(exec, ASCIILiteral("pipeThrough is not implemented"));
    return exec->vm().throwException(exec, error);
}

EncodedJSValue JSC_HOST_CALL constructJSReadableStream(ExecState* exec)
{
    DOMConstructorObject* jsConstructor = jsCast<DOMConstructorObject*>(exec->callee());
    ASSERT(jsConstructor);
    ScriptExecutionContext* scriptExecutionContext = jsConstructor->scriptExecutionContext();

    Ref<ReadableStreamJSSource> source = ReadableStreamJSSource::create(exec);
    if (source->isErrored())
        return throwVMError(exec, source->error());

    RefPtr<ReadableStream> readableStream = ReadableStream::create(*scriptExecutionContext, Ref<ReadableStreamSource>(source.get()));

    VM& vm = exec->vm();
    JSGlobalObject* globalObject = exec->callee()->globalObject();
    JSReadableStream* jsReadableStream = JSReadableStream::create(JSReadableStream::createStructure(vm, globalObject, JSReadableStream::createPrototype(vm, globalObject)), jsCast<JSDOMGlobalObject*>(globalObject), readableStream.releaseNonNull());

    if (!source->start())
        return throwVMError(exec, source->error());

    return JSValue::encode(jsReadableStream);
}

} // namespace WebCore

#endif
