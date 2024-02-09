#include "CkHandle_TypeSafe.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Handle_TypeSafe::
    FCk_Handle_TypeSafe(
        EntityType InEntity,
        const RegistryType& InRegistry)
    : FCk_Handle(InEntity, InRegistry)
{
}

FCk_Handle_TypeSafe::
    FCk_Handle_TypeSafe(
        ThisType&& InOther) noexcept
    : FCk_Handle(MoveTemp(InOther))
{
}

FCk_Handle_TypeSafe::
    FCk_Handle_TypeSafe(
        const FCk_Handle_TypeSafe& InHandle)
    : FCk_Handle(InHandle)
{ }

FCk_Handle_TypeSafe::
    FCk_Handle_TypeSafe(
    const FCk_Handle& InHandle)
    : FCk_Handle(InHandle)
{ }

auto
    FCk_Handle_TypeSafe::
    operator=(
        const ThisType& InOther)
    -> ThisType&
{
    Super::operator=(InOther);
    return *this;
}

auto
    FCk_Handle_TypeSafe::
    operator=(
        ThisType&& InOther) noexcept
        -> ThisType&
{
    _Entity = InOther._Entity;
    _Registry = InOther._Registry;
    _Mapper = InOther._Mapper;
    return *this;
}

auto
    FCk_Handle_TypeSafe::
    operator=(
        const FCk_Handle& InOther)
        -> ThisType&
{
    Super::operator=(InOther);
    return *this;
}

// --------------------------------------------------------------------------------------------------------------------
