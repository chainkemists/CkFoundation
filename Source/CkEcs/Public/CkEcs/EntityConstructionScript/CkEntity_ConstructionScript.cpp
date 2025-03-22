#include "CkEntity_ConstructionScript.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    ICk_Entity_ContextInjector_Interface::
    InjectContext(
        FCk_Handle& InContextEntity)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InContextEntity),
        TEXT("Trying to Inject an INVALID Context Entity"))
    { return; }

    _Context = InContextEntity;
}

auto
    ICk_Entity_ContextInjector_Interface::
    Get_InjectedContext() const
    -> FCk_Handle
{
    return _Context.Get(FCk_Handle{});
}

auto
    ICk_Entity_ContextInjector_Interface::
    ClearInjectedContext()
    -> void
{
    _Context.Reset();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Entity_ConstructionScript_PDA::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InOptionalParams,
        const UObject* InOptionalObjectConstructionScript) const
    -> void
{
    _CurrentWorld = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);

    UCk_Utils_Handle_UE::Set_DebugName(
        InHandle, UCk_Utils_Debug_UE::Get_DebugName(this, ECk_DebugNameVerbosity_Policy::Compact), ECk_Override::DoNotOverride);

    if (ck::IsValid(InOptionalObjectConstructionScript) && InOptionalObjectConstructionScript->Implements<UCk_Entity_ConstructionScript_Interface>())
    {
        ICk_Entity_ConstructionScript_Interface::Execute_DoConstruct(InOptionalObjectConstructionScript, InHandle);
    }

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
    const auto& ConstructionScriptCDO = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Entity_ConstructionScript_PDA>(InConstructionScript);
    return Request_Construct_Instanced(InHandle, ConstructionScriptCDO, InOptionalParams);
}

auto
    UCk_Entity_ConstructionScript_PDA::
    Request_Construct_Instanced(
        FCk_Handle& InHandle,
        UCk_Entity_ConstructionScript_PDA* InConstructionScript,
        FInstancedStruct InOptionalParams)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InConstructionScript),
        TEXT("Unable to proceed with Entity Construction as the Construction Script [{}] is INVALID."), InConstructionScript)
    { return InHandle; }

    InConstructionScript->Construct(InHandle, InOptionalParams, nullptr);
    return InHandle;
}

auto
    UCk_Entity_ConstructionScript_PDA::
    Request_Construct_Multiple(
        FCk_Handle& InHandle,
        TArray<TSubclassOf<UCk_Entity_ConstructionScript_PDA>> InConstructionScript,
        const FInstancedStruct& InOptionalParams)
    -> FCk_Handle
{
    for (const auto& ConstructionScript : InConstructionScript)
    {
        Request_Construct(InHandle, ConstructionScript, InOptionalParams);
    }

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
