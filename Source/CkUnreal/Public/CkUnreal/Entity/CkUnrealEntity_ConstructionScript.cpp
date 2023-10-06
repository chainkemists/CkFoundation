#include "CkUnrealEntity_ConstructionScript.h"

#include "CkUnrealEntity_Fragment_Params.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto UCKk_Utils_UnrealEntity_ConstructionScript_UE::
BuildEntity(
    FCk_Handle InHandle,
    const UCk_UnrealEntity_Base_PDA* InUnrealEntity) -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InUnrealEntity),
        TEXT("InUnrealEntity is INVALID. Cannot build Unreal Entity for Handle [{}]"), InHandle)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Handle is INVALID. Unable to build entity for [{}]"), InUnrealEntity)
    { return {}; }

    const auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);
    InUnrealEntity->Build(NewEntity);
    return NewEntity;
}

// --------------------------------------------------------------------------------------------------------------------

