#include "CkEntity_ConstructionScript.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkEcs/Handle/CkHandle.h"

// this is for debugging only
// TODO: wrap in pre-processor macro to compile this out
struct NAME_ConstructionScript
{
    FText _Name;
};

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Entity_ConstructionScript_PDA::
    Construct(
        FCk_Handle InHandle)
    -> void
{
    Set_CurrentWorld(UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle));

    const auto& DebugName = UCk_Utils_Debug_UE::Get_DebugName_AsText(this, ECk_DebugName_Verbosity::ShortName);
    InHandle.Add<NAME_ConstructionScript>(UCk_Utils_Debug_UE::Get_DebugName_AsText(this, ECk_DebugName_Verbosity::ShortName));
    ck::ecs::Log(TEXT("[EntityMap] [{}] -> [{}]"), InHandle, DebugName);
    DoConstruct(InHandle);
}

auto
    UCk_Entity_ConstructionScript_PDA::
    DoConstruct_Implementation(
        const FCk_Handle& InHandle)
    -> void
{
}

// --------------------------------------------------------------------------------------------------------------------
