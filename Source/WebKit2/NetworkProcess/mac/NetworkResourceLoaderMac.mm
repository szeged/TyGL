/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#import "config.h"
#import "NetworkResourceLoader.h"

#if ENABLE(NETWORK_PROCESS)

#import "NetworkDiskCacheMonitor.h"
#import "ShareableResource.h"
#import <WebCore/CFNetworkSPI.h>
#import <WebCore/ResourceHandle.h>
#import <WebCore/SharedBuffer.h>

using namespace WebCore;

namespace WebKit {

static void tryGetShareableHandleFromCFData(ShareableResource::Handle& handle, CFDataRef data)
{
    RefPtr<SharedMemory> sharedMemory = SharedMemory::createFromVMBuffer((void*)CFDataGetBytePtr(data), CFDataGetLength(data));
    if (!sharedMemory) {
        LOG_ERROR("Failed to create VM shared memory for cached resource.");
        return;
    }

    size_t size = sharedMemory->size();
    RefPtr<ShareableResource> resource = ShareableResource::create(sharedMemory.release(), 0, size);
    resource->createHandle(handle);
}

void NetworkResourceLoader::tryGetShareableHandleFromCFURLCachedResponse(ShareableResource::Handle& handle, CFCachedURLResponseRef cachedResponse)
{
    CFDataRef data = _CFCachedURLResponseGetMemMappedData(cachedResponse);

    tryGetShareableHandleFromCFData(handle, data);
}

void NetworkResourceLoader::tryGetShareableHandleFromSharedBuffer(ShareableResource::Handle& handle, SharedBuffer& buffer)
{
    static CFURLCacheRef cache = CFURLCacheCopySharedURLCache();
    ASSERT(isMainThread());
    ASSERT(cache == adoptCF(CFURLCacheCopySharedURLCache()));

    if (!cache)
        return;

    CFDataRef data = buffer.existingCFData();
    if (!data)
        return;

    if (_CFURLCacheIsResponseDataMemMapped(cache, data) == kCFBooleanFalse)
        return;

    tryGetShareableHandleFromCFData(handle, data);
}

size_t NetworkResourceLoader::fileBackedResourceMinimumSize()
{
    return SharedMemory::systemPageSize();
}

#if USE(CFNETWORK)

void NetworkResourceLoader::willCacheResponseAsync(ResourceHandle* handle, CFCachedURLResponseRef cfResponse)
{
    ASSERT_UNUSED(handle, handle == m_handle);

    if (m_bytesReceived >= fileBackedResourceMinimumSize())
        NetworkDiskCacheMonitor::monitorFileBackingStoreCreation(cfResponse, this);

    m_handle->continueWillCacheResponse(cfResponse);
}

#else

void NetworkResourceLoader::willCacheResponseAsync(ResourceHandle* handle, NSCachedURLResponse *nsResponse)
{
    ASSERT_UNUSED(handle, handle == m_handle);

    if (m_bytesReceived >= fileBackedResourceMinimumSize())
        NetworkDiskCacheMonitor::monitorFileBackingStoreCreation([nsResponse _CFCachedURLResponse], this);

    m_handle->continueWillCacheResponse(nsResponse);
}

#endif // !USE(CFNETWORK)

} // namespace WebKit

#endif // ENABLE(NETWORK_PROCESS)

