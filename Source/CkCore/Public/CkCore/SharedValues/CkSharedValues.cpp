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
    : _Ptr(ck::type_traits::MakeNewPtr<PtrType>{}((InValue)))
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
    : _Ptr(ck::type_traits::MakeNewPtr<PtrType>{}((InValue)))
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
    : _Ptr(ck::type_traits::MakeNewPtr<PtrType>{}((InValue)))
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

FCk_SharedVector::
    FCk_SharedVector()
    : FCk_SharedVector(ValueType{})
{ }

FCk_SharedVector::
    FCk_SharedVector(
        ValueType InValue)
    : _Ptr(ck::type_traits::MakeNewPtr<PtrType>{}((InValue)))
{ }

auto
    FCk_SharedVector::
    operator*() const
    -> ValueType&
{
    return *_Ptr;
}

auto
    FCk_SharedVector::
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
    : _Ptr(ck::type_traits::MakeNewPtr<PtrType>{}((InValue)))
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

FCk_SharedRotator::
    FCk_SharedRotator()
    : FCk_SharedRotator(ValueType{})
{ }

FCk_SharedRotator::
    FCk_SharedRotator(
        ValueType InValue)
    : _Ptr(ck::type_traits::MakeNewPtr<PtrType>{}((InValue)))
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
