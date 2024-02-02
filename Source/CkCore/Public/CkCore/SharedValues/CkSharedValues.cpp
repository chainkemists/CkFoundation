#include "CkSharedValues.h"

#include "CkCore/TypeTraits/CkTypeTraits.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_SharedBool::
    FCk_SharedBool()
    : FCk_SharedBool({})
{ }

FCk_SharedBool::
    FCk_SharedBool(
        ValueType InValue)
    : _Ptr(ck::type_traits::MakeNewPtr<PtrType>{}(InValue))
{ }

auto
    FCk_SharedBool::
    operator*() const
    -> ValueType&
{
    return *_Ptr;
}

auto
    FCk_SharedBool::
    operator->() const
    -> ValueType*
{
    return _Ptr.Get();
}

// --------------------------------------------------------------------------------------------------------------------

FCk_SharedInt::
    FCk_SharedInt()
    : FCk_SharedInt({})
{ }

FCk_SharedInt::
    FCk_SharedInt(
        ValueType InValue)
    : _Ptr(ck::type_traits::MakeNewPtr<PtrType>{}(InValue))
{ }

auto
    FCk_SharedInt::
    operator*() const
    -> ValueType&
{
    return *_Ptr;
}

auto
    FCk_SharedInt::
    operator->() const
    -> ValueType*
{
    return _Ptr.Get();
}

// --------------------------------------------------------------------------------------------------------------------

FCk_SharedFloat::
    FCk_SharedFloat()
    : FCk_SharedFloat({})
{ }

FCk_SharedFloat::
    FCk_SharedFloat(
        ValueType InValue)
    : _Ptr(ck::type_traits::MakeNewPtr<PtrType>{}(InValue))
{ }

auto
    FCk_SharedFloat::
    operator*() const
    -> ValueType&
{
    return *_Ptr;
}

auto
    FCk_SharedFloat::
    operator->() const
    -> ValueType*
{
    return _Ptr.Get();
}

// --------------------------------------------------------------------------------------------------------------------

FCk_SharedVec3::
    FCk_SharedVec3()
    : FCk_SharedVec3(ValueType{})
{ }

FCk_SharedVec3::
    FCk_SharedVec3(
        ValueType InValue)
    : _Ptr(ck::type_traits::MakeNewPtr<PtrType>{}(InValue))
{ }

auto
    FCk_SharedVec3::
    operator*() const
    -> ValueType&
{
    return *_Ptr;
}

auto
    FCk_SharedVec3::
    operator->() const
    -> ValueType*
{
    return _Ptr.Get();
}

// --------------------------------------------------------------------------------------------------------------------

FCk_SharedVec2::
    FCk_SharedVec2()
    : FCk_SharedVec2(ValueType{})
{ }

FCk_SharedVec2::
    FCk_SharedVec2(
        ValueType InValue)
    : _Ptr(ck::type_traits::MakeNewPtr<PtrType>{}(InValue))
{ }

auto
    FCk_SharedVec2::
    operator*() const
    -> ValueType&
{
    return *_Ptr;
}

auto
    FCk_SharedVec2::
    operator->() const
    -> ValueType*
{
    return _Ptr.Get();
}

// --------------------------------------------------------------------------------------------------------------------

FCk_SharedString::
    FCk_SharedString()
    : FCk_SharedString(ValueType{})
{ }

FCk_SharedString::
    FCk_SharedString(
        ValueType InValue)
    : _Ptr(ck::type_traits::MakeNewPtr<PtrType>{}(InValue))
{ }

auto
    FCk_SharedString::
    operator*() const
    -> ValueType&
{
    return *_Ptr;
}

auto
    FCk_SharedString::
    operator->() const
    -> ValueType*
{
    return _Ptr.Get();
}

// --------------------------------------------------------------------------------------------------------------------

FCk_SharedName::
    FCk_SharedName()
    : FCk_SharedName(ValueType{})
{ }

FCk_SharedName::
    FCk_SharedName(
        ValueType InValue)
    : _Ptr(ck::type_traits::MakeNewPtr<PtrType>{}(InValue))
{ }

auto
    FCk_SharedName::
    operator*() const
    -> ValueType&
{
    return *_Ptr;
}

auto
    FCk_SharedName::
    operator->() const
    -> ValueType*
{
    return _Ptr.Get();
}

// --------------------------------------------------------------------------------------------------------------------

FCk_SharedText::
    FCk_SharedText()
    : FCk_SharedText(ValueType{})
{ }

FCk_SharedText::
    FCk_SharedText(
        ValueType InValue)
    : _Ptr(ck::type_traits::MakeNewPtr<PtrType>{}(InValue))
{ }

auto
    FCk_SharedText::
    operator*() const
    -> ValueType&
{
    return *_Ptr;
}

auto
    FCk_SharedText::
    operator->() const
    -> ValueType*
{
    return _Ptr.Get();
}

// --------------------------------------------------------------------------------------------------------------------

FCk_SharedRotator::
    FCk_SharedRotator()
    : FCk_SharedRotator(ValueType{})
{ }

FCk_SharedRotator::
    FCk_SharedRotator(
        ValueType InValue)
    : _Ptr(ck::type_traits::MakeNewPtr<PtrType>{}(InValue))
{ }

auto
    FCk_SharedRotator::
    operator*() const
    -> ValueType&
{
    return *_Ptr;
}

auto
    FCk_SharedRotator::
    operator->() const
    -> ValueType*
{
    return _Ptr.Get();
}

// --------------------------------------------------------------------------------------------------------------------

FCk_SharedTransform::
    FCk_SharedTransform()
    : FCk_SharedTransform(ValueType{})
{ }

FCk_SharedTransform::
    FCk_SharedTransform(
        ValueType InValue)
    : _Ptr(ck::type_traits::MakeNewPtr<PtrType>{}(InValue))
{ }

auto
    FCk_SharedTransform::
    operator*() const
    -> ValueType&
{
    return *_Ptr;
}

auto
    FCk_SharedTransform::
    operator->() const
    -> ValueType*
{
    return _Ptr.Get();
}

// --------------------------------------------------------------------------------------------------------------------

FCk_SharedInstancedStruct::
    FCk_SharedInstancedStruct()
    : FCk_SharedInstancedStruct(ValueType{})
{ }

FCk_SharedInstancedStruct::
    FCk_SharedInstancedStruct(
        ValueType InValue)
    : _Ptr(ck::type_traits::MakeNewPtr<PtrType>{}(InValue))
{ }

auto
    FCk_SharedInstancedStruct::
    operator*() const
    -> ValueType&
{
    return *_Ptr;
}

auto
    FCk_SharedInstancedStruct::
    operator->() const
    -> ValueType*
{
    return _Ptr.Get();
}

// --------------------------------------------------------------------------------------------------------------------
