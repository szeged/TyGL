/*
    This file is part of the WebKit open source project.
    This file has been generated by generate-bindings.pl. DO NOT MODIFY!

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

#include "config.h"
#include "JSTestActiveDOMObject.h"

#include "ExceptionCode.h"
#include "JSDOMBinding.h"
#include "JSNode.h"
#include "ScriptExecutionContext.h"
#include "TestActiveDOMObject.h"
#include <runtime/Error.h>
#include <wtf/GetPtr.h>

using namespace JSC;

namespace WebCore {

// Functions

JSC::EncodedJSValue JSC_HOST_CALL jsTestActiveDOMObjectPrototypeFunctionExcitingFunction(JSC::ExecState*);
JSC::EncodedJSValue JSC_HOST_CALL jsTestActiveDOMObjectPrototypeFunctionPostMessage(JSC::ExecState*);

// Attributes

JSC::EncodedJSValue jsTestActiveDOMObjectExcitingAttr(JSC::ExecState*, JSC::JSObject*, JSC::EncodedJSValue, JSC::PropertyName);
JSC::EncodedJSValue jsTestActiveDOMObjectConstructor(JSC::ExecState*, JSC::JSObject*, JSC::EncodedJSValue, JSC::PropertyName);

class JSTestActiveDOMObjectPrototype : public JSC::JSNonFinalObject {
public:
    typedef JSC::JSNonFinalObject Base;
    static JSTestActiveDOMObjectPrototype* create(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::Structure* structure)
    {
        JSTestActiveDOMObjectPrototype* ptr = new (NotNull, JSC::allocateCell<JSTestActiveDOMObjectPrototype>(vm.heap)) JSTestActiveDOMObjectPrototype(vm, globalObject, structure);
        ptr->finishCreation(vm);
        return ptr;
    }

    DECLARE_INFO;
    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }

private:
    JSTestActiveDOMObjectPrototype(JSC::VM& vm, JSC::JSGlobalObject*, JSC::Structure* structure)
        : JSC::JSNonFinalObject(vm, structure)
    {
    }

    void finishCreation(JSC::VM&);
};

class JSTestActiveDOMObjectConstructor : public DOMConstructorObject {
private:
    JSTestActiveDOMObjectConstructor(JSC::Structure*, JSDOMGlobalObject*);
    void finishCreation(JSC::VM&, JSDOMGlobalObject*);

public:
    typedef DOMConstructorObject Base;
    static JSTestActiveDOMObjectConstructor* create(JSC::VM& vm, JSC::Structure* structure, JSDOMGlobalObject* globalObject)
    {
        JSTestActiveDOMObjectConstructor* ptr = new (NotNull, JSC::allocateCell<JSTestActiveDOMObjectConstructor>(vm.heap)) JSTestActiveDOMObjectConstructor(structure, globalObject);
        ptr->finishCreation(vm, globalObject);
        return ptr;
    }

    DECLARE_INFO;
    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }
};

/* Hash table */

static const struct CompactHashIndex JSTestActiveDOMObjectTableIndex[4] = {
    { 1, -1 },
    { 0, -1 },
    { -1, -1 },
    { -1, -1 },
};


static const HashTableValue JSTestActiveDOMObjectTableValues[] =
{
    { "constructor", DontEnum | ReadOnly, NoIntrinsic, (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsTestActiveDOMObjectConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(0) },
    { "excitingAttr", DontDelete | ReadOnly | CustomAccessor, NoIntrinsic, (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsTestActiveDOMObjectExcitingAttr), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(0) },
};

static const HashTable JSTestActiveDOMObjectTable = { 2, 3, true, JSTestActiveDOMObjectTableValues, 0, JSTestActiveDOMObjectTableIndex };
const ClassInfo JSTestActiveDOMObjectConstructor::s_info = { "TestActiveDOMObjectConstructor", &Base::s_info, 0, CREATE_METHOD_TABLE(JSTestActiveDOMObjectConstructor) };

JSTestActiveDOMObjectConstructor::JSTestActiveDOMObjectConstructor(Structure* structure, JSDOMGlobalObject* globalObject)
    : DOMConstructorObject(structure, globalObject)
{
}

void JSTestActiveDOMObjectConstructor::finishCreation(VM& vm, JSDOMGlobalObject* globalObject)
{
    Base::finishCreation(vm);
    ASSERT(inherits(info()));
    putDirect(vm, vm.propertyNames->prototype, JSTestActiveDOMObject::getPrototype(vm, globalObject), DontDelete | ReadOnly);
    putDirect(vm, vm.propertyNames->length, jsNumber(0), ReadOnly | DontDelete | DontEnum);
}

/* Hash table for prototype */

static const HashTableValue JSTestActiveDOMObjectPrototypeTableValues[] =
{
    { "excitingFunction", JSC::Function, NoIntrinsic, (intptr_t)static_cast<NativeFunction>(jsTestActiveDOMObjectPrototypeFunctionExcitingFunction), (intptr_t) (1) },
    { "postMessage", JSC::Function, NoIntrinsic, (intptr_t)static_cast<NativeFunction>(jsTestActiveDOMObjectPrototypeFunctionPostMessage), (intptr_t) (1) },
};

WEBCORE_EXPORT const ClassInfo JSTestActiveDOMObjectPrototype::s_info = { "TestActiveDOMObjectPrototype", &Base::s_info, 0, CREATE_METHOD_TABLE(JSTestActiveDOMObjectPrototype) };

void JSTestActiveDOMObjectPrototype::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    reifyStaticProperties(vm, JSTestActiveDOMObjectPrototypeTableValues, *this);
}

WEBCORE_EXPORT const ClassInfo JSTestActiveDOMObject::s_info = { "TestActiveDOMObject", &Base::s_info, &JSTestActiveDOMObjectTable, CREATE_METHOD_TABLE(JSTestActiveDOMObject) };

JSTestActiveDOMObject::JSTestActiveDOMObject(Structure* structure, JSDOMGlobalObject* globalObject, Ref<TestActiveDOMObject>&& impl)
    : JSDOMWrapper(structure, globalObject)
    , m_impl(&impl.leakRef())
{
}

JSObject* JSTestActiveDOMObject::createPrototype(VM& vm, JSGlobalObject* globalObject)
{
    return JSTestActiveDOMObjectPrototype::create(vm, globalObject, JSTestActiveDOMObjectPrototype::createStructure(vm, globalObject, globalObject->objectPrototype()));
}

JSObject* JSTestActiveDOMObject::getPrototype(VM& vm, JSGlobalObject* globalObject)
{
    return getDOMPrototype<JSTestActiveDOMObject>(vm, globalObject);
}

void JSTestActiveDOMObject::destroy(JSC::JSCell* cell)
{
    JSTestActiveDOMObject* thisObject = static_cast<JSTestActiveDOMObject*>(cell);
    thisObject->JSTestActiveDOMObject::~JSTestActiveDOMObject();
}

JSTestActiveDOMObject::~JSTestActiveDOMObject()
{
    releaseImplIfNotNull();
}

bool JSTestActiveDOMObject::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    JSTestActiveDOMObject* thisObject = jsCast<JSTestActiveDOMObject*>(object);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    return getStaticValueSlot<JSTestActiveDOMObject, Base>(exec, JSTestActiveDOMObjectTable, thisObject, propertyName, slot);
}

EncodedJSValue jsTestActiveDOMObjectExcitingAttr(ExecState* exec, JSObject* slotBase, EncodedJSValue thisValue, PropertyName)
{
    UNUSED_PARAM(exec);
    UNUSED_PARAM(slotBase);
    UNUSED_PARAM(thisValue);
    JSTestActiveDOMObject* castedThis = jsCast<JSTestActiveDOMObject*>(slotBase);
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, castedThis->impl()))
        return JSValue::encode(jsUndefined());
    TestActiveDOMObject& impl = castedThis->impl();
    JSValue result = jsNumber(impl.excitingAttr());
    return JSValue::encode(result);
}


EncodedJSValue jsTestActiveDOMObjectConstructor(ExecState* exec, JSObject*, EncodedJSValue thisValue, PropertyName)
{
    JSTestActiveDOMObject* domObject = jsDynamicCast<JSTestActiveDOMObject*>(JSValue::decode(thisValue));
    if (!domObject)
        return throwVMTypeError(exec);
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, domObject->impl()))
        return JSValue::encode(jsUndefined());
    return JSValue::encode(JSTestActiveDOMObject::getConstructor(exec->vm(), domObject->globalObject()));
}

JSValue JSTestActiveDOMObject::getConstructor(VM& vm, JSGlobalObject* globalObject)
{
    return getDOMConstructor<JSTestActiveDOMObjectConstructor>(vm, jsCast<JSDOMGlobalObject*>(globalObject));
}

EncodedJSValue JSC_HOST_CALL jsTestActiveDOMObjectPrototypeFunctionExcitingFunction(ExecState* exec)
{
    JSValue thisValue = exec->thisValue();
    JSTestActiveDOMObject* castedThis = jsDynamicCast<JSTestActiveDOMObject*>(thisValue);
    if (UNLIKELY(!castedThis))
        return throwThisTypeError(*exec, "TestActiveDOMObject", "excitingFunction");
    ASSERT_GC_OBJECT_INHERITS(castedThis, JSTestActiveDOMObject::info());
    if (!BindingSecurity::shouldAllowAccessToDOMWindow(exec, castedThis->impl()))
        return JSValue::encode(jsUndefined());
    TestActiveDOMObject& impl = castedThis->impl();
    if (UNLIKELY(exec->argumentCount() < 1))
        return throwVMError(exec, createNotEnoughArgumentsError(exec));
    Node* nextChild(JSNode::toWrapped(exec->argument(0)));
    if (UNLIKELY(exec->hadException()))
        return JSValue::encode(jsUndefined());
    impl.excitingFunction(nextChild);
    return JSValue::encode(jsUndefined());
}

EncodedJSValue JSC_HOST_CALL jsTestActiveDOMObjectPrototypeFunctionPostMessage(ExecState* exec)
{
    JSValue thisValue = exec->thisValue();
    JSTestActiveDOMObject* castedThis = jsDynamicCast<JSTestActiveDOMObject*>(thisValue);
    if (UNLIKELY(!castedThis))
        return throwThisTypeError(*exec, "TestActiveDOMObject", "postMessage");
    ASSERT_GC_OBJECT_INHERITS(castedThis, JSTestActiveDOMObject::info());
    TestActiveDOMObject& impl = castedThis->impl();
    if (UNLIKELY(exec->argumentCount() < 1))
        return throwVMError(exec, createNotEnoughArgumentsError(exec));
    const String& message(exec->argument(0).isEmpty() ? String() : exec->argument(0).toString(exec)->value(exec));
    if (UNLIKELY(exec->hadException()))
        return JSValue::encode(jsUndefined());
    impl.postMessage(message);
    return JSValue::encode(jsUndefined());
}

bool JSTestActiveDOMObjectOwner::isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown> handle, void*, SlotVisitor& visitor)
{
    UNUSED_PARAM(handle);
    UNUSED_PARAM(visitor);
    return false;
}

void JSTestActiveDOMObjectOwner::finalize(JSC::Handle<JSC::Unknown> handle, void* context)
{
    JSTestActiveDOMObject* jsTestActiveDOMObject = jsCast<JSTestActiveDOMObject*>(handle.slot()->asCell());
    DOMWrapperWorld& world = *static_cast<DOMWrapperWorld*>(context);
    uncacheWrapper(world, &jsTestActiveDOMObject->impl(), jsTestActiveDOMObject);
    jsTestActiveDOMObject->releaseImpl();
}

#if ENABLE(BINDING_INTEGRITY)
#if PLATFORM(WIN)
#pragma warning(disable: 4483)
extern "C" { extern void (*const __identifier("??_7TestActiveDOMObject@WebCore@@6B@")[])(); }
#else
extern "C" { extern void* _ZTVN7WebCore19TestActiveDOMObjectE[]; }
#endif
#endif
JSC::JSValue toJS(JSC::ExecState*, JSDOMGlobalObject* globalObject, TestActiveDOMObject* impl)
{
    if (!impl)
        return jsNull();
    if (JSValue result = getExistingWrapper<JSTestActiveDOMObject>(globalObject, impl))
        return result;

#if ENABLE(BINDING_INTEGRITY)
    void* actualVTablePointer = *(reinterpret_cast<void**>(impl));
#if PLATFORM(WIN)
    void* expectedVTablePointer = reinterpret_cast<void*>(__identifier("??_7TestActiveDOMObject@WebCore@@6B@"));
#else
    void* expectedVTablePointer = &_ZTVN7WebCore19TestActiveDOMObjectE[2];
#if COMPILER(CLANG)
    // If this fails TestActiveDOMObject does not have a vtable, so you need to add the
    // ImplementationLacksVTable attribute to the interface definition
    COMPILE_ASSERT(__is_polymorphic(TestActiveDOMObject), TestActiveDOMObject_is_not_polymorphic);
#endif
#endif
    // If you hit this assertion you either have a use after free bug, or
    // TestActiveDOMObject has subclasses. If TestActiveDOMObject has subclasses that get passed
    // to toJS() we currently require TestActiveDOMObject you to opt out of binding hardening
    // by adding the SkipVTableValidation attribute to the interface IDL definition
    RELEASE_ASSERT(actualVTablePointer == expectedVTablePointer);
#endif
    return createNewWrapper<JSTestActiveDOMObject>(globalObject, impl);
}

TestActiveDOMObject* JSTestActiveDOMObject::toWrapped(JSC::JSValue value)
{
    if (auto* wrapper = jsDynamicCast<JSTestActiveDOMObject*>(value))
        return &wrapper->impl();
    return nullptr;
}

}
