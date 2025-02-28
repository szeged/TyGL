/*
 * Copyright (C) 2013, 2014 Apple Inc. All rights reserved.
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
#include "FTLJSCall.h"

#if ENABLE(FTL_JIT)

#include "DFGNode.h"
#include "LinkBuffer.h"

namespace JSC { namespace FTL {

JSCall::JSCall()
    : m_stackmapID(UINT_MAX)
    , m_node(nullptr)
    , m_callLinkInfo(nullptr)
    , m_instructionOffset(UINT_MAX)
{
}

JSCall::JSCall(unsigned stackmapID, DFG::Node* node)
    : m_stackmapID(stackmapID)
    , m_node(node)
    , m_callLinkInfo(nullptr)
    , m_instructionOffset(0)
{
}

void JSCall::emit(CCallHelpers& jit)
{
    m_callLinkInfo = jit.codeBlock()->addCallLinkInfo();
    
    CCallHelpers::Jump slowPath = jit.branchPtrWithPatch(
        CCallHelpers::NotEqual, GPRInfo::regT0, m_targetToCheck,
        CCallHelpers::TrustedImmPtr(0));
    
    m_fastCall = jit.nearCall();
    CCallHelpers::Jump done = jit.jump();
    
    slowPath.link(&jit);
    
    jit.move(CCallHelpers::TrustedImmPtr(m_callLinkInfo), GPRInfo::regT2);
    m_slowCall = jit.nearCall();
    
    done.link(&jit);
}

void JSCall::link(VM& vm, LinkBuffer& linkBuffer)
{
    ThunkGenerator generator = linkThunkGeneratorFor(
        m_node->op() == DFG::Construct ? CodeForConstruct : CodeForCall,
        MustPreserveRegisters);
    
    linkBuffer.link(
        m_slowCall, FunctionPtr(vm.getCTIStub(generator).code().executableAddress()));
    
    m_callLinkInfo->isFTL = true;
    m_callLinkInfo->callType = m_node->op() == DFG::Construct ? CallLinkInfo::Construct : CallLinkInfo::Call;
    m_callLinkInfo->codeOrigin = m_node->origin.semantic;
    m_callLinkInfo->callReturnLocation = linkBuffer.locationOfNearCall(m_slowCall);
    m_callLinkInfo->hotPathBegin = linkBuffer.locationOf(m_targetToCheck);
    m_callLinkInfo->hotPathOther = linkBuffer.locationOfNearCall(m_fastCall);
    m_callLinkInfo->calleeGPR = GPRInfo::regT0;
}

} } // namespace JSC::FTL

#endif // ENABLE(FTL_JIT)

