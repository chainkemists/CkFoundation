#include "CkTransientEntity_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_TransientEntity_UE::
    Get_World(
        const FCk_Handle& InHandle)
    -> UWorld*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Unable to Get_World from TransientEntity with INVALID Handle [{}]"), InHandle)
    { return {}; }

    const auto& TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InHandle.Get_Registry());

    CK_ENSURE_IF_NOT(ck::IsValid(TransientEntity),
        TEXT("Could NOT get the TransientEntity from Handle [{}]"), InHandle)
    { return {}; }

    const auto& WorldPtr = TransientEntity.Get<TWeakObjectPtr<UWorld>>();

    CK_ENSURE_IF_NOT(ck::IsValid(WorldPtr),
        TEXT("Could NOT get a valid World from TransientEntity [{}]"), TransientEntity)
    { return {}; }

    return WorldPtr.Get();

}

// --------------------------------------------------------------------------------------------------------------------
