#include "CkEntityLifetime_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::algo
{
    auto
        IsDestructionPhase::
        operator()(
            const FCk_Handle& InHandle) const
        -> bool
    {
        return UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InHandle, _DestructionPhase);
    }

    auto
        Is_NOT_DestructionPhase::
        operator()(
            const FCk_Handle& InHandle) const
        -> bool
    {
        return NOT UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InHandle, _DestructionPhase);
    }
}

// --------------------------------------------------------------------------------------------------------------------
