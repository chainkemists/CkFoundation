#include "CkHandle_TypeSafe.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Handle_TypeSafe::
    operator==(
        const FCk_Handle& InOther) const
        -> bool
{
    return InOther.operator==(*this);
}

auto
    FCk_Handle_TypeSafe::
    operator!=(
        const FCk_Handle& InOther) const
        -> bool
{
    return InOther.operator==(*this);
}

auto
    FCk_Handle_TypeSafe::
    operator==(
        const ThisType& InOther) const
        -> bool
{
    return ck::Cast<FCk_Handle>(*this) == ck::Cast<FCk_Handle>(InOther);
}

auto
    FCk_Handle_TypeSafe::operator*()
        -> TOptional<FCk_Registry>
{ return _RawHandle.operator*(); }

auto
    FCk_Handle_TypeSafe::operator*() const
        -> TOptional<FCk_Registry>
{ return _RawHandle.operator*(); }

auto
    FCk_Handle_TypeSafe::operator->()
        -> TOptional<FCk_Registry>
{ return _RawHandle.operator->(); }

auto
    FCk_Handle_TypeSafe::operator->() const
        -> TOptional<FCk_Registry>
{ return _RawHandle.operator->(); }

auto
    FCk_Handle_TypeSafe::IsValid(
        ck::IsValid_Policy_Default) const
        -> bool
{ return _RawHandle.IsValid(ck::IsValid_Policy_Default{}); }

auto
    FCk_Handle_TypeSafe::IsValid(
        ck::IsValid_Policy_IncludePendingKill) const
        -> bool
{ return _RawHandle.IsValid(ck::IsValid_Policy_IncludePendingKill{}); }

auto
    FCk_Handle_TypeSafe::Orphan() const
        -> bool
{ return _RawHandle.Orphan(); }

auto
    FCk_Handle_TypeSafe::Get_ValidHandle(
        FCk_Entity::IdType InEntity) const
        -> FCk_Handle
{ return _RawHandle.Get_ValidHandle(InEntity); }

auto
    FCk_Handle_TypeSafe::Get_Registry()
        -> FCk_Registry&
{ return _RawHandle.Get_Registry(); }

auto
    FCk_Handle_TypeSafe::Get_Registry() const
        -> const FCk_Registry&
{ return _RawHandle.Get_Registry(); }

// --------------------------------------------------------------------------------------------------------------------
