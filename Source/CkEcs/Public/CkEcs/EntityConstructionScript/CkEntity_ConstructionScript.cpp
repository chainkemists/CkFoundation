#include "CkEntity_ConstructionScript.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Entity_ConstructionScript_PDA::
    Construct(
        FCk_Handle InHandle)
    -> void
{
    Set_CurrentWorld(UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle));

    UCk_Utils_Handle_UE::Set_DebugName(InHandle, UCk_Utils_Debug_UE::Get_DebugName(this, ECk_DebugName_Verbosity::ShortName));

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
