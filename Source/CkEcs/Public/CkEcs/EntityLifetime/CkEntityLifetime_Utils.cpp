#include "CkEntityLifetime_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/SharedValues/CkSharedValues.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/Delegates/CkDelegates.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Utils.h"
#include "CkEcs/Net/CkNet_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityLifetime_UE::
    Request_DestroyEntity(
        FCk_Handle& InHandle,
        ECk_EntityLifetime_DestructionBehavior InDestructionBehavior)
    -> void
{
    QUICK_SCOPE_CYCLE_COUNTER(Request_Destroy_Entity)

    if (ck::Is_NOT_Valid(InHandle))
    { return; }

    if (InHandle.Has_Any<ck::FTag_DestroyEntity_Initiate, ck::FTag_DestroyEntity_Await, ck::FTag_DestroyEntity_Finalize>())
    { return; }

    switch(InDestructionBehavior)
    {
        case ECk_EntityLifetime_DestructionBehavior::ForceDestroy:
        {
            break;
        }
        case ECk_EntityLifetime_DestructionBehavior::DestroyOnlyIfOrphan:
        {
            if (NOT InHandle.Orphan())
            { return; }

            break;
        }
        default:
        {
            CK_INVALID_ENUM(InDestructionBehavior);
            return;
        }
    }

    ck::ecs::VeryVerbose(TEXT("Entity [{}] set to 'Initiate Destruction'"), InHandle);
    InHandle.AddOrGet<ck::FTag_DestroyEntity_Initiate>();

    for (auto& LifeTimeDependents : Get_LifetimeDependents(InHandle))
    {
        Request_DestroyEntity(LifeTimeDependents, InDestructionBehavior);
    }
}

auto
    UCk_Utils_EntityLifetime_UE::
    Request_CreateEntity(
        const FCk_Handle& InHandle)
    -> FCk_Handle
{
    return Request_CreateEntity(InHandle, PostEntityCreatedFunc{});
}

auto
    UCk_Utils_EntityLifetime_UE::
    Get_LifetimeOwner(
        const FCk_Handle& InHandle,
        ECk_PendingKill_Policy InPendingKillPolicy)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(InHandle.Has<ck::FFragment_LifetimeOwner>(),
        TEXT("The Entity [{}] does NOT have a LifetimeOwner. Was this Entity created by Request_CreateEntity(RegistryType)?"),
        InHandle)
    { return {}; }

    switch(InPendingKillPolicy)
    {
        case ECk_PendingKill_Policy::ExcludePendingKill:
        {
            return InHandle.Get<ck::FFragment_LifetimeOwner>().Get_Entity();
        }
        case ECk_PendingKill_Policy::IncludePendingKill:
        {
            return InHandle.Get<ck::FFragment_LifetimeOwner, ck::IsValid_Policy_IncludePendingKill>().Get_Entity();
        }
    }

    return {};
}

auto
    UCk_Utils_EntityLifetime_UE::
    Get_LifetimeDependents(
        const FCk_Handle& InHandle)
    -> TArray<FCk_Handle>
{
    if (NOT InHandle.Has<ck::FFragment_LifetimeDependents>())
    { return {}; }

    const auto& Dependents = InHandle.Get<ck::FFragment_LifetimeDependents>();

    auto Ret = TArray<FCk_Handle>{};

    for (const auto Dependent : Dependents.Get_Entities())
    {
        // we do NOT clean up the Lifetime Owner's dependents Array mainly for perf reasons
        // if this becomes a hot-spot, we may opt to clean up the Array on Entity destruction
        // in the FProcessor_EntityLifetime_TriggerDestroyEntity Processor
        if (NOT ck::IsValid(Dependent, InHandle))
        { continue; }

        Ret.Emplace(Dependent);
    }

    return Ret;
}

auto
    UCk_Utils_EntityLifetime_UE::
    Get_IsPendingDestroy(
        const FCk_Handle& InHandle,
        ECk_EntityLifetime_DestructionPhase InDestructionPhase)
    -> bool
{
    switch(InDestructionPhase)
    {
        case ECk_EntityLifetime_DestructionPhase::Initiated:
        {
            return InHandle.Has_Any<ck::FTag_DestroyEntity_Initiate>();
        }
        case ECk_EntityLifetime_DestructionPhase::Confirmed:
        {
            return InHandle.Has_Any<ck::FTag_DestroyEntity_Await, ck::FTag_DestroyEntity_Finalize>();
        }
        case ECk_EntityLifetime_DestructionPhase::InitiatedOrConfirmed:
        {
            return InHandle.Has_Any<ck::FTag_DestroyEntity_Initiate, ck::FTag_DestroyEntity_Await, ck::FTag_DestroyEntity_Finalize>();
        }
    }

    return false;
}

auto
    UCk_Utils_EntityLifetime_UE::
    Get_IsTransientEntity(
        const FCk_Handle& InHandle)
    -> bool
{
    if (ck::Is_NOT_Valid(InHandle))
    { return false; }

    return Get_TransientEntity(InHandle.Get_Registry()) == InHandle;
}

auto
    UCk_Utils_EntityLifetime_UE::
    Get_WorldForEntity(
        const FCk_Handle& InHandle)
    -> UWorld*
{
    if (InHandle.Has<TWeakObjectPtr<UWorld>>())
    {
        const auto NetMode = InHandle.Get<TWeakObjectPtr<UWorld>>().Get()->GetNetMode();
        return InHandle.Get<TWeakObjectPtr<UWorld>>().Get();
    }

    const auto& LifeTimeOwner = Get_LifetimeOwner(InHandle);

    if (ck::Is_NOT_Valid(LifeTimeOwner))
    { return {}; }

    return Get_WorldForEntity(LifeTimeOwner);
}

auto
    UCk_Utils_EntityLifetime_UE::
    Get_EntityInOwnershipChain_If(
        FCk_Handle& InHandle,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Predicate_InHandle_OutResult& InPredicate)
    -> FCk_Handle
{
    return Get_EntityInOwnershipChain_If(InHandle,
    [&](const FCk_Handle& InAttribute)  -> bool
    {
        const FCk_SharedBool PredicateResult;

        if (InPredicate.IsBound())
        {
            InPredicate.Execute(InAttribute, PredicateResult, InOptionalPayload);
        }

        return *PredicateResult;
    });
}

auto
    UCk_Utils_EntityLifetime_UE::
    BindTo_OnUpdate(
        FCk_Handle& InHandle,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_Lifetime_OnDestroy& InDelegate)
    -> void
{
    ck::UUtils_Signal_EntityDestroyed::Bind(InHandle, InDelegate, InBehavior);
}

auto
    UCk_Utils_EntityLifetime_UE::
    UnbindFrom_OnUpdate(
        FCk_Handle& InHandle,
        const FCk_Delegate_Lifetime_OnDestroy& InDelegate)
    -> void
{
    ck::UUtils_Signal_EntityDestroyed::Unbind(InHandle, InDelegate);
}

auto
    UCk_Utils_EntityLifetime_UE::
    Get_EntityNetRole(
        const FCk_Handle& InEntity)
    -> ECk_Net_EntityNetRole
{
    if (ck::Is_NOT_Valid(InEntity))
    { return ECk_Net_EntityNetRole::None; }

    if (InEntity.Has<ck::FFragment_Net_Params>())
    { return InEntity.Get<ck::FFragment_Net_Params>().Get_ConnectionSettings().Get_NetRole(); }

    return Get_EntityNetRole(Get_LifetimeOwner(InEntity));
}

auto
    UCk_Utils_EntityLifetime_UE::
    Get_EntityNetMode(
        const FCk_Handle& InEntity)
    -> ECk_Net_NetModeType
{
    if (ck::Is_NOT_Valid(InEntity))
    { return ECk_Net_NetModeType::Unknown; }

    if (InEntity.Has<ck::FFragment_Net_Params>())
    { return InEntity.Get<ck::FFragment_Net_Params>().Get_ConnectionSettings().Get_NetMode(); }

    return Get_EntityNetMode(Get_LifetimeOwner(InEntity));
}

auto
    UCk_Utils_EntityLifetime_UE::
    Request_CreateEntity(
        const FCk_Handle& InHandle,
        PostEntityCreatedFunc InFunc)
    -> HandleType
{
    QUICK_SCOPE_CYCLE_COUNTER(Request_Create_Entity)

    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Cannot create Entity with Invalid Handle"))
    { return {}; }

    const auto NewEntity = Request_CreateEntity(**InHandle, [&](FCk_Handle InNewEntity)
    {
        Request_SetupEntityWithLifetimeOwner(InNewEntity, InHandle);

        if (InFunc)
        {
            InFunc(InNewEntity);
        }
    });

    return NewEntity;
}

auto
    UCk_Utils_EntityLifetime_UE::
    Request_CreateEntity(
        RegistryType& InRegistry,
        PostEntityCreatedFunc InFunc)
    -> HandleType
{
    QUICK_SCOPE_CYCLE_COUNTER(Request_Create_Entity)

    const auto& NewEntity = InRegistry.CreateEntity();
    InRegistry.Add<ck::FTag_EntityJustCreated>(NewEntity);

    auto NewEntityHandle = HandleType{ NewEntity, InRegistry };

    if (InFunc)
    {
        InFunc(NewEntityHandle);
    }

    return NewEntityHandle;
}

auto
    UCk_Utils_EntityLifetime_UE::
    Request_CreateEntity(
        RegistryType& InRegistry,
        const EntityIdHint& InEntityHint,
        PostEntityCreatedFunc InFunc)
    -> HandleType
{
    QUICK_SCOPE_CYCLE_COUNTER(Request_Create_Entity)

    const auto& NewEntity = InRegistry.CreateEntity(InEntityHint.Get_Entity());
    InRegistry.Add<ck::FTag_EntityJustCreated>(NewEntity);

    auto NewEntityHandle = HandleType{ NewEntity, InRegistry };

    if (InFunc)
    {
        InFunc(NewEntityHandle);
    }

    return NewEntityHandle;
}

auto
    UCk_Utils_EntityLifetime_UE::
    Get_TransientEntity(
        const RegistryType& InRegistry)
    -> HandleType
{
    return HandleType{InRegistry.Get_TransientEntity(), InRegistry};
}

auto
    UCk_Utils_EntityLifetime_UE::
    Request_SetupEntityWithLifetimeOwner(
        FCk_Handle& InNewEntity,
        const FCk_Handle& InLifetimeOwner)
    -> void
{
    InNewEntity.Add<ck::FFragment_LifetimeOwner>(InLifetimeOwner);

#ifdef CK_COPY_NET_PARAMS_ON_EVERY_ENTITY
    if (NOT Get_IsTransientEntity(InLifetimeOwner) && InLifetimeOwner.Has<ck::FFragment_Net_Params>())
    {
        const auto& ConnectionSettings = InLifetimeOwner.Get<ck::FFragment_Net_Params>().Get_ConnectionSettings();
        if (ConnectionSettings.Get_NetRole() == ECk_Net_EntityNetRole::Authority)
        {
            InNewEntity.Add<ck::FTag_HasAuthority>();
        }

        if (ConnectionSettings.Get_NetMode() == ECk_Net_NetModeType::Host)
        {
            InNewEntity.Add<ck::FTag_NetMode_IsHost>();
        }

        InNewEntity.Add<ck::FFragment_Net_Params>(InLifetimeOwner.Get<ck::FFragment_Net_Params>());
    }
#endif

    if (InLifetimeOwner.Has_Any<ck::FTag_DestroyEntity_Initiate>())
    { InNewEntity.Add<ck::FTag_DestroyEntity_Initiate>(); }

    if (InLifetimeOwner.Has_Any<ck::FTag_DestroyEntity_Initiate_Confirm>())
    { InNewEntity.Add<ck::FTag_DestroyEntity_Initiate_Confirm>(); }

    // Not doing something like this because it is undefined behavior: *const_cast<FCk_Handle*>(&InHandle)
    auto NonConstLifetimeOwnerHandle = InLifetimeOwner;

    NonConstLifetimeOwnerHandle.AddOrGet<ck::FFragment_LifetimeDependents>()._Entities.Emplace(InNewEntity);
}

auto
    UCk_Utils_EntityLifetime_UE::
    Request_TransferLifetimeOwner(
        FCk_Handle& InEntity,
        const FCk_Handle& InNewLifetimeOwner)
    -> void
{
    const auto& CurrentLifetimeOwner = InEntity.Get<ck::FFragment_LifetimeOwner>();
    auto CurrentLifetimeOwnerEntity = CurrentLifetimeOwner.Get_Entity();

    if (InNewLifetimeOwner == CurrentLifetimeOwnerEntity)
    { return; }

    CurrentLifetimeOwnerEntity.Get<ck::FFragment_LifetimeDependents>()._Entities.RemoveSingle(InEntity);

    InEntity.Replace<ck::FFragment_LifetimeOwner>(InNewLifetimeOwner);

    if (InNewLifetimeOwner.Has_Any<ck::FTag_DestroyEntity_Initiate>())
    { InEntity.AddOrGet<ck::FTag_DestroyEntity_Initiate>(); }

    if (InNewLifetimeOwner.Has_Any<ck::FTag_DestroyEntity_Initiate_Confirm>())
    { InEntity.AddOrGet<ck::FTag_DestroyEntity_Initiate_Confirm>(); }

    // Not doing something like this because it is undefined behavior: *const_cast<FCk_Handle*>(&InHandle)
    auto NonConstNewLifetimeOwnerHandle = InNewLifetimeOwner;

    NonConstNewLifetimeOwnerHandle.AddOrGet<ck::FFragment_LifetimeDependents>()._Entities.Emplace(InEntity);
}

// --------------------------------------------------------------------------------------------------------------------
