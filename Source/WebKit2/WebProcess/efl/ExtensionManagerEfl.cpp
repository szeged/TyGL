/*
 * Copyright (C) 2014 Ryuan Choi <ryuan.choi@gmail.com>.  All rights reserved.
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

#include "config.h"
#include "ExtensionManagerEfl.h"

#include "Module.h"
#include "WKBundleAPICast.h"
#include "ewk_extension.h"
#include "ewk_extension_private.h"
#include <wtf/NeverDestroyed.h>
#include <wtf/text/CString.h>

namespace WebKit {

ExtensionManagerEfl& ExtensionManagerEfl::singleton()
{
    static NeverDestroyed<ExtensionManagerEfl> extensionManager;
    return extensionManager;
}

ExtensionManagerEfl::ExtensionManagerEfl()
{
}

void ExtensionManagerEfl::initialize(WKBundleRef bundle, WKTypeRef userData)
{
    ASSERT(!m_extension);

    String extensionsDirectory = toImpl(static_cast<WKStringRef>(userData))->string();
    if (extensionsDirectory.isEmpty())
        return;

    m_extension = std::make_unique<EwkExtension>(toImpl(bundle));

    Vector<String> modulePaths = WebCore::listDirectory(extensionsDirectory, ASCIILiteral("*.so"));

    for (size_t i = 0; i < modulePaths.size(); ++i) {
        if (WebCore::fileExists(modulePaths[i])) {
            auto module = std::make_unique<Module>(modulePaths[i]);
            if (!module->load())
                continue;

            Ewk_Extension_Initialize_Function initializeFunction = module->functionPointer<Ewk_Extension_Initialize_Function>("ewk_extension_init");
            if (!initializeFunction)
                return;

            initializeFunction(m_extension.get(), nullptr /* reserved for the future */);
            m_extensionModules.append(module.release());
        }
    }
}

}
