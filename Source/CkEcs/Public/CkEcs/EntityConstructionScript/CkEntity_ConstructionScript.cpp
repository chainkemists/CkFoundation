#include "CkEntity_ConstructionScript.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Entity_ConstructionScript_PDA::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InOptionalParams) const
    -> void
{
    _CurrentWorld = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);

    UCk_Utils_Handle_UE::Set_DebugName(
        InHandle, UCk_Utils_Debug_UE::Get_DebugName(this, ECk_DebugNameVerbosity_Policy::Compact), ECk_Override::DoNotOverride);

    DoConstruct(InHandle, InOptionalParams);
}

auto
    UCk_Entity_ConstructionScript_PDA::
    Request_Construct(
        FCk_Handle& InHandle,
        TSubclassOf<UCk_Entity_ConstructionScript_PDA> InConstructionScript,
        const FInstancedStruct& InOptionalParams)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InConstructionScript),
        TEXT("Unable to proceed with Entity Construction as the Construction Script [{}] is INVALID."), InConstructionScript)
    { return InHandle; }

    UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Entity_ConstructionScript_PDA>(InConstructionScript)->Construct(InHandle, InOptionalParams);
    return InHandle;
}

auto
    UCk_Entity_ConstructionScript_PDA::
    DoConstruct_Implementation(
        FCk_Handle& InHandle,
        const FInstancedStruct& InOptionalParams) const
    -> void
{
}

// --------------------------------------------------------------------------------------------------------------------
