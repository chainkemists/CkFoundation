#include "CkEntityLifetime_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityLifetime_UE::
    Request_DestroyEntity(
        FCk_Handle& InHandle,
        ECk_EntityLifetime_DestructionBehavior InDestructionBehavior)
    -> void
{
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
    Get_TransientEntity(
        const FCk_Handle& InHandle)
    -> FCk_Handle
{
    return Get_TransientEntity(**InHandle);
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
    Request_CreateEntity(
        const FCk_Handle& InHandle,
        PostEntityCreatedFunc InFunc)
    -> HandleType
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Cannot create Entity with Invalid Handle"))
    { return {}; }

    const auto NewEntity = Request_CreateEntity(**InHandle, [&](FCk_Handle InNewEntity)
    {
        InNewEntity.Add<ck::FFragment_LifetimeOwner>(InHandle);

        if (InHandle.Has_Any<ck::FTag_DestroyEntity_Initiate>())
        { InNewEntity.Add<ck::FTag_DestroyEntity_Initiate>(); }

        if (InHandle.Has_Any<ck::FTag_DestroyEntity_Initiate_Confirm>())
        { InNewEntity.Add<ck::FTag_DestroyEntity_Initiate_Confirm>(); }

        // Not doing something like this because it is undefined behavior: *const_cast<FCk_Handle*>(&InHandle)
        auto NonConstHandle = InHandle;

        NonConstHandle.AddOrGet<ck::FFragment_LifetimeDependents>()._Entities.Emplace(InNewEntity);

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

// --------------------------------------------------------------------------------------------------------------------
