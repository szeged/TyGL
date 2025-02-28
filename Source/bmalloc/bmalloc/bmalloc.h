/*
 * Copyright (C) 2014, 2015 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "Cache.h"
#include "Heap.h"
#include "PerProcess.h"
#include "StaticMutex.h"

namespace bmalloc {
namespace api {

inline void* malloc(size_t size)
{
    return Cache::allocate(size);
}

inline void* memalign(size_t alignment, size_t size)
{
    return Cache::allocate(alignment, size);
}

inline void free(void* object)
{
    Cache::deallocate(object);
}

inline void* realloc(void* object, size_t newSize)
{
    return Cache::reallocate(object, newSize);
}

inline void scavengeThisThread()
{
    Cache::scavenge();
}

inline void scavenge()
{
    scavengeThisThread();

    std::unique_lock<StaticMutex> lock(PerProcess<Heap>::mutex());
    PerProcess<Heap>::get()->scavenge(lock, std::chrono::milliseconds(0));
}

} // namespace api
} // namespace bmalloc
