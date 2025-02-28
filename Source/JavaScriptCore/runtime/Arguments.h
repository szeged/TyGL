/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2006, 2007, 2008, 2009, 2014 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Cameron Zwarich (cwzwarich@uwaterloo.ca)
 *  Copyright (C) 2007 Maks Orlovich
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef Arguments_h
#define Arguments_h

#include "CodeOrigin.h"
#include "JSFunction.h"
#include "JSGlobalObject.h"
#include "JSLexicalEnvironment.h"
#include "Interpreter.h"
#include "ObjectConstructor.h"
#include "WriteBarrierInlines.h"
#include <wtf/StdLibExtras.h>

namespace JSC {

enum ArgumentsMode {
    NormalArgumentsCreationMode,
    ClonedArgumentsCreationMode,
    FakeArgumentValuesCreationMode
};

class Arguments : public JSNonFinalObject {
    friend class JIT;
    friend class JSArgumentsIterator;
public:
    typedef JSNonFinalObject Base;

    static Arguments* create(VM& vm, CallFrame* callFrame, JSLexicalEnvironment* lexicalEnvironment, ArgumentsMode mode = NormalArgumentsCreationMode)
    {
        Arguments* arguments = new (NotNull, allocateCell<Arguments>(vm.heap, offsetOfInlineRegisterArray() + registerArraySizeInBytes(callFrame))) Arguments(callFrame);
        arguments->finishCreation(callFrame, lexicalEnvironment, mode);
        return arguments;
    }
        
    static Arguments* create(VM& vm, CallFrame* callFrame, InlineCallFrame* inlineCallFrame, ArgumentsMode mode = NormalArgumentsCreationMode)
    {
        Arguments* arguments = new (NotNull, allocateCell<Arguments>(vm.heap, offsetOfInlineRegisterArray() + registerArraySizeInBytes(inlineCallFrame))) Arguments(callFrame);
        arguments->finishCreation(callFrame, inlineCallFrame, mode);
        return arguments;
    }

    enum { MaxArguments = 0x10000 };

private:
    enum NoParametersType { NoParameters };
        
    Arguments(CallFrame*);
    Arguments(CallFrame*, NoParametersType);
        
public:
    DECLARE_INFO;

    static void visitChildren(JSCell*, SlotVisitor&);
    static void copyBackingStore(JSCell*, CopyVisitor&, CopyToken);

    void fillArgList(ExecState*, MarkedArgumentBuffer&);

    uint32_t length(ExecState* exec) const 
    {
        if (UNLIKELY(m_overrodeLength))
            return get(exec, exec->propertyNames().length).toUInt32(exec);
        return m_numArguments; 
    }
        
    void copyToArguments(ExecState*, CallFrame*, uint32_t copyLength, int32_t firstArgumentOffset);
    void tearOff(CallFrame*);
    void tearOff(CallFrame*, InlineCallFrame*);
    void tearOffForCloning(CallFrame*);
    void tearOffForCloning(CallFrame*, InlineCallFrame*);
    bool isTornOff() const { return m_registers == (&registerArray() - CallFrame::offsetFor(1) - 1); }

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype) 
    { 
        return Structure::create(vm, globalObject, prototype, TypeInfo(ArgumentsType, StructureFlags), info()); 
    }
    
    static ptrdiff_t offsetOfActivation() { return OBJECT_OFFSETOF(Arguments, m_lexicalEnvironment); }
    static ptrdiff_t offsetOfNumArguments() { return OBJECT_OFFSETOF(Arguments, m_numArguments); }
    static ptrdiff_t offsetOfOverrodeLength() { return OBJECT_OFFSETOF(Arguments, m_overrodeLength); }
    static ptrdiff_t offsetOfIsStrictMode() { return OBJECT_OFFSETOF(Arguments, m_isStrictMode); }
    static ptrdiff_t offsetOfRegisters() { return OBJECT_OFFSETOF(Arguments, m_registers); }
    static ptrdiff_t offsetOfInlineRegisterArray() { return WTF::roundUpToMultipleOf<8>(sizeof(Arguments)); }
    static ptrdiff_t offsetOfSlowArgumentData() { return OBJECT_OFFSETOF(Arguments, m_slowArgumentData); }
    static ptrdiff_t offsetOfCallee() { return OBJECT_OFFSETOF(Arguments, m_callee); }
    
protected:
    static const unsigned StructureFlags = OverridesGetOwnPropertySlot | InterceptsGetOwnPropertySlotByIndexEvenWhenLengthIsNotZero | OverridesGetPropertyNames | JSObject::StructureFlags;

    void finishCreation(CallFrame*, JSLexicalEnvironment*, ArgumentsMode);
    void finishCreation(CallFrame*, InlineCallFrame*, ArgumentsMode);

private:
    static bool getOwnPropertySlot(JSObject*, ExecState*, PropertyName, PropertySlot&);
    static bool getOwnPropertySlotByIndex(JSObject*, ExecState*, unsigned propertyName, PropertySlot&);
    static void getOwnPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);
    static void put(JSCell*, ExecState*, PropertyName, JSValue, PutPropertySlot&);
    static void putByIndex(JSCell*, ExecState*, unsigned propertyName, JSValue, bool shouldThrow);
    static bool deleteProperty(JSCell*, ExecState*, PropertyName);
    static bool deletePropertyByIndex(JSCell*, ExecState*, unsigned propertyName);
    static bool defineOwnProperty(JSObject*, ExecState*, PropertyName, const PropertyDescriptor&, bool shouldThrow);
    void createStrictModeCallerIfNecessary(ExecState*);
    void createStrictModeCalleeIfNecessary(ExecState*);

    static size_t registerArraySizeInBytes(CallFrame* callFrame) { return sizeof(WriteBarrier<Unknown>) * callFrame->argumentCount(); }
    static size_t registerArraySizeInBytes(InlineCallFrame* inlineCallFrame) { return sizeof(WriteBarrier<Unknown>) * (inlineCallFrame->arguments.size() - 1); }
    bool isArgument(size_t);
    bool trySetArgument(VM&, size_t argument, JSValue);
    JSValue tryGetArgument(size_t argument);
    bool isDeletedArgument(size_t);
    bool tryDeleteArgument(VM&, size_t);
    WriteBarrierBase<Unknown>& argument(size_t);
    void allocateSlowArguments(VM&);

    void init(CallFrame*);

    WriteBarrier<JSLexicalEnvironment> m_lexicalEnvironment;

    unsigned m_numArguments;

    // We make these full byte booleans to make them easy to test from the JIT,
    // and because even if they were single-bit booleans we still wouldn't save
    // any space.
    bool m_overrodeLength; 
    bool m_overrodeCallee;
    bool m_overrodeCaller;
    bool m_isStrictMode;

    WriteBarrierBase<Unknown>* m_registers;
    WriteBarrier<Unknown>& registerArray() { return *reinterpret_cast<WriteBarrier<Unknown>*>(reinterpret_cast<char*>(this) + offsetOfInlineRegisterArray()); }
    const WriteBarrier<Unknown>& registerArray() const { return *reinterpret_cast<const WriteBarrier<Unknown>*>(reinterpret_cast<const char*>(this) + offsetOfInlineRegisterArray()); }

public:
    struct SlowArgumentData {
    public:
        SlowArgumentData()
            : m_bytecodeToMachineCaptureOffset(0)
        {
        }

        SlowArgument* slowArguments()
        {
            return reinterpret_cast<SlowArgument*>(WTF::roundUpToMultipleOf<8>(reinterpret_cast<size_t>(this + 1)));
        }

        int bytecodeToMachineCaptureOffset() const { return m_bytecodeToMachineCaptureOffset; }
        void setBytecodeToMachineCaptureOffset(int newOffset) { m_bytecodeToMachineCaptureOffset = newOffset; }

        static size_t sizeForNumArguments(unsigned numArguments)
        {
            return WTF::roundUpToMultipleOf<8>(sizeof(SlowArgumentData)) + sizeof(SlowArgument) * numArguments;
        }

    private:
        int m_bytecodeToMachineCaptureOffset; // Add this if you have a bytecode offset into captured registers and you want the machine offset instead. Subtract if you want to do the opposite. 
    };
    
private:
    CopyWriteBarrier<SlowArgumentData> m_slowArgumentData;

    WriteBarrier<JSFunction> m_callee;
};

Arguments* asArguments(JSValue);

inline Arguments* asArguments(JSValue value)
{
    ASSERT(asObject(value)->inherits(Arguments::info()));
    return static_cast<Arguments*>(asObject(value));
}

inline Arguments::Arguments(CallFrame* callFrame)
    : Base(callFrame->vm(), callFrame->lexicalGlobalObject()->argumentsStructure())
{
}

inline Arguments::Arguments(CallFrame* callFrame, NoParametersType)
    : Base(callFrame->vm(), callFrame->lexicalGlobalObject()->argumentsStructure())
{
}

inline void Arguments::allocateSlowArguments(VM& vm)
{
    if (!!m_slowArgumentData)
        return;

    void* backingStore;
    if (!vm.heap.tryAllocateStorage(this, SlowArgumentData::sizeForNumArguments(m_numArguments), &backingStore))
        RELEASE_ASSERT_NOT_REACHED();
    m_slowArgumentData.set(vm, this, static_cast<SlowArgumentData*>(backingStore));

    for (size_t i = 0; i < m_numArguments; ++i) {
        ASSERT(m_slowArgumentData->slowArguments()[i].status == SlowArgument::Normal);
        m_slowArgumentData->slowArguments()[i].index = CallFrame::argumentOffset(i);
    }
}

inline bool Arguments::tryDeleteArgument(VM& vm, size_t argument)
{
    if (!isArgument(argument))
        return false;
    allocateSlowArguments(vm);
    m_slowArgumentData->slowArguments()[argument].status = SlowArgument::Deleted;
    return true;
}

inline bool Arguments::trySetArgument(VM& vm, size_t argument, JSValue value)
{
    if (!isArgument(argument))
        return false;
    this->argument(argument).set(vm, this, value);
    return true;
}

inline JSValue Arguments::tryGetArgument(size_t argument)
{
    if (!isArgument(argument))
        return JSValue();
    return this->argument(argument).get();
}

inline bool Arguments::isDeletedArgument(size_t argument)
{
    if (argument >= m_numArguments)
        return false;
    if (!m_slowArgumentData)
        return false;
    if (m_slowArgumentData->slowArguments()[argument].status != SlowArgument::Deleted)
        return false;
    return true;
}

inline bool Arguments::isArgument(size_t argument)
{
    if (argument >= m_numArguments)
        return false;
    if (m_slowArgumentData && m_slowArgumentData->slowArguments()[argument].status == SlowArgument::Deleted)
        return false;
    return true;
}

inline WriteBarrierBase<Unknown>& Arguments::argument(size_t argument)
{
    ASSERT(isArgument(argument));
    if (!m_slowArgumentData)
        return m_registers[CallFrame::argumentOffset(argument)];

    int index = m_slowArgumentData->slowArguments()[argument].index;
    if (m_slowArgumentData->slowArguments()[argument].status != SlowArgument::Captured)
        return m_registers[index];

    RELEASE_ASSERT(m_lexicalEnvironment);
    return m_lexicalEnvironment->registerAt(index - m_slowArgumentData->bytecodeToMachineCaptureOffset());
}

inline void Arguments::finishCreation(CallFrame* callFrame, JSLexicalEnvironment* lexicalEnvironment, ArgumentsMode mode)
{
    Base::finishCreation(callFrame->vm());
    ASSERT(inherits(info()));

    JSFunction* callee = jsCast<JSFunction*>(callFrame->callee());
    m_callee.set(callFrame->vm(), this, callee);
    m_overrodeLength = false;
    m_overrodeCallee = false;
    m_overrodeCaller = false;
    m_isStrictMode = callFrame->codeBlock()->isStrictMode();

    switch (mode) {
    case NormalArgumentsCreationMode: {
        m_numArguments = callFrame->argumentCount();
        m_registers = reinterpret_cast<WriteBarrierBase<Unknown>*>(callFrame->registers());

        CodeBlock* codeBlock = callFrame->codeBlock();
        if (codeBlock->hasSlowArguments()) {
            SymbolTable* symbolTable = codeBlock->symbolTable();
            const SlowArgument* slowArguments = codeBlock->machineSlowArguments();
            allocateSlowArguments(callFrame->vm());
            size_t count = std::min<unsigned>(m_numArguments, symbolTable->parameterCount());
            for (size_t i = 0; i < count; ++i)
                m_slowArgumentData->slowArguments()[i] = slowArguments[i];
            m_slowArgumentData->setBytecodeToMachineCaptureOffset(
                codeBlock->framePointerOffsetToGetActivationRegisters());
        }
        if (codeBlock->needsActivation()) {
            RELEASE_ASSERT(lexicalEnvironment && lexicalEnvironment == callFrame->lexicalEnvironment());
            m_lexicalEnvironment.set(callFrame->vm(), this, lexicalEnvironment);
        }
        // The bytecode generator omits op_tear_off_lexical_environment in cases of no
        // declared parameters, so we need to tear off immediately.
        if (m_isStrictMode || !callee->jsExecutable()->parameterCount())
            tearOff(callFrame);
        break;
    }

    case ClonedArgumentsCreationMode: {
        m_numArguments = callFrame->argumentCount();
        m_registers = reinterpret_cast<WriteBarrierBase<Unknown>*>(callFrame->registers());
        tearOffForCloning(callFrame);
        break;
    }

    case FakeArgumentValuesCreationMode: {
        m_numArguments = 0;
        m_registers = nullptr;
        tearOff(callFrame);
        break;
    }
    }
}

inline void Arguments::finishCreation(CallFrame* callFrame, InlineCallFrame* inlineCallFrame, ArgumentsMode mode)
{
    Base::finishCreation(callFrame->vm());
    ASSERT(inherits(info()));

    JSFunction* callee = inlineCallFrame->calleeForCallFrame(callFrame);
    m_callee.set(callFrame->vm(), this, callee);
    m_overrodeLength = false;
    m_overrodeCallee = false;
    m_overrodeCaller = false;
    m_isStrictMode = jsCast<FunctionExecutable*>(inlineCallFrame->executable.get())->isStrictMode();

    switch (mode) {
    case NormalArgumentsCreationMode: {
        m_numArguments = inlineCallFrame->arguments.size() - 1;
        
        if (m_numArguments) {
            int offsetForArgumentOne = inlineCallFrame->arguments[1].virtualRegister().offset();
            m_registers = reinterpret_cast<WriteBarrierBase<Unknown>*>(callFrame->registers()) + offsetForArgumentOne - virtualRegisterForArgument(1).offset();
        } else
            m_registers = 0;
        
        ASSERT(!jsCast<FunctionExecutable*>(inlineCallFrame->executable.get())->symbolTable(inlineCallFrame->specializationKind())->slowArguments());
        
        // The bytecode generator omits op_tear_off_lexical_environment in cases of no
        // declared parameters, so we need to tear off immediately.
        if (m_isStrictMode || !callee->jsExecutable()->parameterCount())
            tearOff(callFrame, inlineCallFrame);
        break;
    }
        
    case ClonedArgumentsCreationMode: {
        m_numArguments = inlineCallFrame->arguments.size() - 1;
        if (m_numArguments) {
            int offsetForArgumentOne = inlineCallFrame->arguments[1].virtualRegister().offset();
            m_registers = reinterpret_cast<WriteBarrierBase<Unknown>*>(callFrame->registers()) + offsetForArgumentOne - virtualRegisterForArgument(1).offset();
        } else
            m_registers = 0;
        
        ASSERT(!jsCast<FunctionExecutable*>(inlineCallFrame->executable.get())->symbolTable(inlineCallFrame->specializationKind())->slowArguments());
        
        tearOffForCloning(callFrame, inlineCallFrame);
        break;
    }
        
    case FakeArgumentValuesCreationMode: {
        m_numArguments = 0;
        m_registers = nullptr;
        tearOff(callFrame);
        break;
    } }
}

} // namespace JSC

#endif // Arguments_h
