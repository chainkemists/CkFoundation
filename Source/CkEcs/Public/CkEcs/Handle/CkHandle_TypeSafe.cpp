#include "CkHandle_TypeSafe.h"

// --------------------------------------------------------------------------------------------------------------------

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

auto
    FCk_Handle_TypeSafe::
    operator=(
        ThisType InOther)
    -> ThisType&
{
    Swap(InOther);
    return *this;
}

// --------------------------------------------------------------------------------------------------------------------
