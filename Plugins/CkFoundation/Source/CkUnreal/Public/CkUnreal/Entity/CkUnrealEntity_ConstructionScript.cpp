#include "CkUnrealEntity_ConstructionScript.h"

#include "CkUnrealEntity_Fragment_Params.h"

// --------------------------------------------------------------------------------------------------------------------

auto UCk_UnrealEntity_ConstructionScript_PDA::Construct(const FCk_Handle& InHandle) -> void
{
    DoConstruct(InHandle);
}

auto UCKk_Utils_UnrealEntity_ConstructionScript_UE::
BuildEntity(FCk_Handle InHandle, const UCk_UnrealEntity_Base_PDA* InUnrealEntity) -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InUnrealEntity),
        TEXT("InUnrealEntity is INVALID. Cannot build Unreal Entity for Handle [{}]"), InHandle)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Handle is INVALID. Unable to build entity for [{}]"), InUnrealEntity)
    { return {}; }

    return InUnrealEntity->Build(**InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

