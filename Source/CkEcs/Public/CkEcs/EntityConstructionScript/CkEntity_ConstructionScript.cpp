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
        UObject* InOptionalObjectConstructionScript) const
    -> void
{
    _CurrentWorld = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);

    UCk_Utils_Handle_UE::Set_DebugName(
        InHandle, UCk_Utils_Debug_UE::Get_DebugName(this, ECk_DebugNameVerbosity_Policy::Compact), ECk_Override::DoNotOverride);

    DoConstruct(InHandle);

    if (ck::IsValid(InOptionalObjectConstructionScript) && InOptionalObjectConstructionScript->Implements<UCk_Entity_ConstructionScript_Interface>())
    {
        ICk_Entity_ConstructionScript_Interface::Execute_DoConstruct(InOptionalObjectConstructionScript, InHandle);
    }
}

auto
    UCk_Entity_ConstructionScript_PDA::
    Request_Construct(
        FCk_Handle& InHandle,
        TSubclassOf<UCk_Entity_ConstructionScript_PDA> InConstructionScript,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle
{
    const auto& ConstructionScriptCDO = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Entity_ConstructionScript_PDA>(InConstructionScript);
    return Request_Construct_Instanced(InHandle, ConstructionScriptCDO, InDelegate);
}

auto
    UCk_Entity_ConstructionScript_PDA::
    Request_Construct_Instanced(
        FCk_Handle& InHandle,
        const UCk_Entity_ConstructionScript_PDA* InConstructionScript,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle
{
    const auto ConstructionScriptIsValid = ck::IsValid(InConstructionScript);
    CK_ENSURE_IF_NOT(ConstructionScriptIsValid,
        TEXT("Unable to proceed with Entity Construction as the Construction Script [{}] is INVALID."), InConstructionScript)
    {}
    if (NOT ConstructionScriptIsValid)
    {
        InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Failed_NotEnqueued);
        return InHandle;
    }

    InConstructionScript->Construct(InHandle, nullptr);

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack.
    InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Succeeded);

    return InHandle;
}

auto
    UCk_Entity_ConstructionScript_PDA::
    Request_Construct_Multiple(
        FCk_Handle& InHandle,
        TArray<TSubclassOf<UCk_Entity_ConstructionScript_PDA>> InConstructionScript,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle
{
    for (const auto& ConstructionScript : InConstructionScript)
    {
        Request_Construct(InHandle, ConstructionScript, {});
    }

    // Immediate mutation — nothing is enqueued, so completion is synchronous on this stack. Per-script
    // rejections are reported by the individual ensures; this reports only that the batch was walked.
    InDelegate.ExecuteIfBound(InHandle, ECk_Request_OperationResult::Succeeded);

    return InHandle;
}

auto
    UCk_Entity_ConstructionScript_PDA::
    ShowReplicationInEditor() const
    -> bool
{
    return true;
}

auto
    UCk_Entity_ConstructionScript_PDA::
    DoConstruct_Implementation(
        FCk_Handle& InHandle) const
    -> void
{
}

// --------------------------------------------------------------------------------------------------------------------
