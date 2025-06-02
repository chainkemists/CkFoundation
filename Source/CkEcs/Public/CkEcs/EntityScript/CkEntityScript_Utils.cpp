#include "CkEntityScript_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment_Data.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_EntityScript_UE, FCk_Handle_EntityScript, ck::FFragment_EntityScript_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityScript_UE::
    Get_ScriptClass(
        const FCk_Handle_EntityScript& InHandle)
    -> TSubclassOf<UCk_EntityScript_UE>
{
    const auto& Current = InHandle.Get<ck::FFragment_EntityScript_Current>();

    CK_ENSURE_IF_NOT(ck::IsValid(Current.Get_Script()), TEXT("The EntityScript [{}] for Handle [{}] is NOT valid"),
        Current.Get_Script(), InHandle)
    { return {}; }

    return InHandle.Get<ck::FFragment_EntityScript_Current>().Get_Script()->GetClass();
}

auto
    UCk_Utils_EntityScript_UE::
    Request_SpawnEntity(
        FCk_Handle& InLifetimeOwner,
        TSubclassOf<UCk_EntityScript_UE> InEntityScriptClass,
        FInstancedStruct InSpawnParams)
    -> FCk_Handle_PendingEntityScript
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InLifetimeOwner);

    // Request_CreateEntity does NOT copy NetParams if the Lifetime Owner is a transient Entity
    // (which should probably be revisited). For now, we manually copy the NetParams
    if (UCk_Utils_EntityLifetime_UE::Get_IsTransientEntity(InLifetimeOwner))
    { UCk_Utils_Net_UE::Copy(InLifetimeOwner, NewEntity); }

    return Add(NewEntity, InEntityScriptClass, InSpawnParams);
}

auto
    UCk_Utils_EntityScript_UE::
    TryGet_Entity_EntityScript_InOwnershipChain(
        const FCk_Handle& InHandle)
    -> FCk_Handle
{
    return UCk_Utils_EntityLifetime_UE::Get_EntityInOwnershipChain_If(InHandle,
    [&](const FCk_Handle& Handle)
    {
        return Handle.Has<ck::FFragment_EntityScript_Current>();
    });
}

auto
    UCk_Utils_EntityScript_UE::
    Add(
        FCk_Handle& InScriptEntity,
        const TSubclassOf<UCk_EntityScript_UE>& InEntityScriptClass,
        const FInstancedStruct& InSpawnParams,
        FCk_EntityScript_PostConstruction_Func InOptionalFunc)
    -> FCk_Handle_PendingEntityScript
{
    CK_ENSURE_IF_NOT(ck::IsValid(InEntityScriptClass), TEXT("Invalid EntityScript supplied, cannot request to Spawn Entity"))
    { return {}; }

    UCk_Utils_Handle_UE::Set_DebugName(InScriptEntity, *ck::Format_UE(TEXT("{}"), InEntityScriptClass));

    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InScriptEntity);

    const auto Request = FCk_Request_EntityScript_SpawnEntity{
                             InScriptEntity,
                             UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InScriptEntity),
                             InEntityScriptClass}
                        .Set_SpawnParams(InSpawnParams)
                        .Set_PostConstruction_Func(InOptionalFunc);

    RequestEntity.Add<ck::FFragment_EntityScript_RequestSpawnEntity>(Request);

    return FCk_Handle_PendingEntityScript{InScriptEntity};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PendingEntityScript_UE::
    Promise_OnConstructed(
        FCk_Handle_PendingEntityScript& InPendingEntityScript,
        const FCk_Delegate_EntityScript_Constructed& InDelegate)
    -> void
{
    ck::UUtils_Signal_OnConstructed_PostFireUnbind::Bind(
        InPendingEntityScript.Get_EntityUnderConstruction(), InDelegate, ECk_Signal_BindingPolicy::FireIfPayloadInFlight);
}

// --------------------------------------------------------------------------------------------------------------------
