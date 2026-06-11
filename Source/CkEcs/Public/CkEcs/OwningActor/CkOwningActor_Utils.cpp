#pragma once

#include "CkOwningActor_Utils.h"

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Fragment.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"

#include "CkEcs/Handle/CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_OwningActor_UE::
    Add(
        FCk_Handle& InHandle,
        AActor* InOwningActor)
    -> void
{
    InHandle.Add<ck::FFragment_OwningActor_Current>(InOwningActor);

    // If this entity is replicated, pair the OwningActor addition with the ReplicationDriver.
    // The driver's Outer is chain-walked to a replicated actor, so we want to ensure the driver
    // — if it was added earlier via the pre-Construct pass in the spawn processor — did NOT get
    // Outer'd to an ancestor's actor before this entity acquired its own. Adding the driver now
    // (with this entity's actor available) gives us the tightest possible Outer resolution.
    if (NOT UCk_Utils_Net_UE::Has(InHandle))
    { return; }

    if (UCk_Utils_Net_UE::Get_Replication(InHandle) != ECk_Replication::Replicates)
    { return; }

    CK_ENSURE_IF_NOT(NOT InHandle.Has<TObjectPtr<UCk_Fragment_EntityReplicationDriver_Rep>>(),
        TEXT("Entity [{}] already has a ReplicationDriver before its OwningActor [{}] was added. "
             "The driver UObject's Outer is resolved at add-time by walking the ownership chain for "
             "a replicated actor; since this entity did not yet own an actor at that moment, the "
             "driver was Outer'd to an ancestor's actor instead of this entity's own. Add the "
             "OwningActor BEFORE any code path that can add a ReplicationDriver to this entity."),
        InHandle, InOwningActor)
    { return; }

    UCk_Utils_EntityReplicationDriver_UE::TryAdd(InHandle);
}

auto
    UCk_Utils_OwningActor_UE::
    SetupActorEntityLink(
        FCk_Handle& InHandle,
        AActor* InActor)
    -> void
{
    auto EntityOwningActorComponent = DoGetOrAdd_EntityOwningActorComponent(InActor);

    if (ck::Is_NOT_Valid(EntityOwningActorComponent))
    { return; }

    EntityOwningActorComponent->_EntityHandle = InHandle;

    // The Actor is now ECS ready — flush any promises queued via Promise_OnActorEcsReady.
    DoFlush_PendingEcsReady(EntityOwningActorComponent, InActor, InHandle);
}

auto
    UCk_Utils_OwningActor_UE::
    Has(
        const FCk_Handle& InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_OwningActor_Current>();
}

auto
    UCk_Utils_OwningActor_UE::
    Ensure(
        const FCk_Handle& InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Entity [{}] does NOT have OwningActor Fragment! The Entity has no associated Actor"), InHandle)
    { return false; }

    return true;
}

auto
    UCk_Utils_OwningActor_UE::
    TryGet_Entity_OwningActor_InOwnershipChain(
        const FCk_Handle& InHandle)
    -> FCk_Handle
{
    auto MaybeActorEntity = UCk_Utils_EntityLifetime_UE::Get_EntityInOwnershipChain_If(InHandle,
    [&](const FCk_Handle& Handle)
    {
        if (Has(Handle))
        { return true; }

        return false;
    });

    return MaybeActorEntity;
}

auto
    UCk_Utils_OwningActor_UE::
    Get_EntityOwningActor(
        const FCk_Handle& InHandle)
    -> AActor*
{
    if (NOT Ensure(InHandle))
    { return {}; }

    return Get_EntityOwningActorBasicDetails(InHandle).Get_Actor().Get();
}

auto
    UCk_Utils_OwningActor_UE::
    TryGet_EntityOwningActor(
        const FCk_Handle& InHandle)
    -> AActor*
{
    if (NOT Has(InHandle))
    { return {}; }

    return Get_EntityOwningActorBasicDetails(InHandle).Get_Actor().Get();
}

auto
    UCk_Utils_OwningActor_UE::
    TryGet_EntityOwningActor_Recursive(
        const FCk_Handle& InHandle)
        -> AActor*
{
    auto MaybeActorEntity = TryGet_Entity_OwningActor_InOwnershipChain(InHandle);

    if (ck::Is_NOT_Valid(MaybeActorEntity))
    { return {}; }

    return Get_EntityOwningActor(MaybeActorEntity);
}

auto
    UCk_Utils_OwningActor_UE::
    Get_EntityOwningActorBasicDetails(
        const FCk_Handle& InHandle)
    -> FCk_EntityOwningActor_BasicDetails
{
    if (NOT Ensure(InHandle))
    { return {}; }

    constexpr auto EvenIfPendingKill = true;
    return FCk_EntityOwningActor_BasicDetails
    {
        InHandle.Get<ck::FFragment_OwningActor_Current, ck::IsValid_Policy_IncludePendingKill>().Get_EntityOwningActor().Get(EvenIfPendingKill), InHandle
    };
}

auto
    UCk_Utils_OwningActor_UE::
    Get_EntityOwningActorBasicDetails_FromActor(
        const AActor* InActor)
    -> FCk_EntityOwningActor_BasicDetails
{
    const auto& ActorEcsHandle = Get_ActorEntityHandle(InActor);

    if (ck::Is_NOT_Valid(ActorEcsHandle))
    { return {}; }

    return Get_EntityOwningActorBasicDetails(ActorEcsHandle);
}

auto
    UCk_Utils_OwningActor_UE::
    Get_ActorEntityHandle(
        const AActor* InActor)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor, ck::IsValid_Policy_IncludePendingKill{}),
        TEXT("Cannot get the ECS Handle of Actor because the Actor is invalid!"))
    { return {}; }

    const auto& EntityOwningActorComp = InActor->FindComponentByClass<UCk_EntityOwningActor_ActorComponent_UE>();

    CK_ENSURE_IF_NOT(ck::IsValid(EntityOwningActorComp),
        TEXT("Actor [{}] does NOT have an Entity Owning Actor Unreal Actor Component! This means it is not ECS ready."),
        InActor)
    { return {}; }

    return EntityOwningActorComp->Get_EntityHandle();
}

auto
    UCk_Utils_OwningActor_UE::
    TryGet_ActorEntityHandle(
        const AActor* InActor)
    -> FCk_Handle
{
    if (ck::Is_NOT_Valid(InActor, ck::IsValid_Policy_IncludePendingKill{}))
    { return {}; }

    const auto& EntityOwningActorComp = InActor->FindComponentByClass<UCk_EntityOwningActor_ActorComponent_UE>();

    if (ck::Is_NOT_Valid(EntityOwningActorComp))
    { return {}; }

    return EntityOwningActorComp->Get_EntityHandle();
}

auto
    UCk_Utils_OwningActor_UE::
    Get_IsActorEcsReady(
        const AActor* InActor)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Cannot check if Actor is ECS ready because it is invalid!"))
    { return {}; }

    const auto EntityOwningActorComp = InActor->GetComponentByClass<UCk_EntityOwningActor_ActorComponent_UE>();

    if (ck::Is_NOT_Valid(EntityOwningActorComp))
    { return false; }

    // The component may exist with an as-yet-unlinked Entity if a promise was queued before the
    // Actor↔Entity link was established. ECS readiness requires the Entity link to be valid.
    return ck::IsValid(EntityOwningActorComp->Get_EntityHandle());
}

auto
    UCk_Utils_OwningActor_UE::
    Promise_OnActorEcsReady(
        AActor* InActor,
        const FCk_Delegate_OwningActor_OnEcsReady& InDelegate,
        ECk_ActorEcsReady_Policy InPolicy)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor),
        TEXT("Promise_OnActorEcsReady called with an invalid Actor"))
    { return; }

    CK_ENSURE_IF_NOT(InDelegate.IsBound(),
        TEXT("Promise_OnActorEcsReady called with an unbound delegate for Actor [{}]"), InActor)
    { return; }

    auto EntityOwningActorComp = DoGetOrAdd_EntityOwningActorComponent(InActor);

    if (ck::Is_NOT_Valid(EntityOwningActorComp))
    { return; }

    if (Get_IsActorEcsReady(InActor))
    {
        const auto Entity = Get_ActorEntityHandle(InActor);

        if (InPolicy == ECk_ActorEcsReady_Policy::LinkEstablished ||
            NOT DoGet_ShouldDeferUntilReplicationComplete(Entity))
        {
            InDelegate.ExecuteIfBound(InActor, Entity);
            return;
        }

        EntityOwningActorComp->_PendingEcsReadyDelegates_ValuesReplicated.Add(InDelegate);
        DoBind_ReplicationCompleteTrampoline(EntityOwningActorComp, Entity);
        return;
    }

    switch (InPolicy)
    {
        case ECk_ActorEcsReady_Policy::LinkEstablished:
            EntityOwningActorComp->_PendingEcsReadyDelegates_LinkEstablished.Add(InDelegate);
            break;
        case ECk_ActorEcsReady_Policy::ValuesReplicated:
            EntityOwningActorComp->_PendingEcsReadyDelegates_ValuesReplicated.Add(InDelegate);
            break;
    }
}

auto
    UCk_Utils_OwningActor_UE::
    Promise_OnActorEcsReady(
        AActor* InActor,
        TFunction<void(AActor*, FCk_Handle)> InCallback,
        ECk_ActorEcsReady_Policy InPolicy)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor),
        TEXT("Promise_OnActorEcsReady called with an invalid Actor"))
    { return; }

    CK_ENSURE_IF_NOT(static_cast<bool>(InCallback),
        TEXT("Promise_OnActorEcsReady called with an empty callback for Actor [{}]"), InActor)
    { return; }

    auto EntityOwningActorComp = DoGetOrAdd_EntityOwningActorComponent(InActor);

    if (ck::Is_NOT_Valid(EntityOwningActorComp))
    { return; }

    if (Get_IsActorEcsReady(InActor))
    {
        const auto Entity = Get_ActorEntityHandle(InActor);

        if (InPolicy == ECk_ActorEcsReady_Policy::LinkEstablished ||
            NOT DoGet_ShouldDeferUntilReplicationComplete(Entity))
        {
            InCallback(InActor, Entity);
            return;
        }

        EntityOwningActorComp->_PendingEcsReadyCallbacks_ValuesReplicated.Add(MoveTemp(InCallback));
        DoBind_ReplicationCompleteTrampoline(EntityOwningActorComp, Entity);
        return;
    }

    switch (InPolicy)
    {
        case ECk_ActorEcsReady_Policy::LinkEstablished:
            EntityOwningActorComp->_PendingEcsReadyCallbacks_LinkEstablished.Add(MoveTemp(InCallback));
            break;
        case ECk_ActorEcsReady_Policy::ValuesReplicated:
            EntityOwningActorComp->_PendingEcsReadyCallbacks_ValuesReplicated.Add(MoveTemp(InCallback));
            break;
    }
}

auto
    UCk_Utils_OwningActor_UE::
    IsEqual(
        const FCk_EntityOwningActor_BasicDetails& InBasicDetailsA,
        const FCk_EntityOwningActor_BasicDetails& InBasicDetailsB)
    -> bool
{
    return InBasicDetailsA == InBasicDetailsB;
}

auto
    UCk_Utils_OwningActor_UE::
    IsNotEqual(
        const FCk_EntityOwningActor_BasicDetails& InBasicDetailsA,
        const FCk_EntityOwningActor_BasicDetails& InBasicDetailsB)
    -> bool
{
    return InBasicDetailsA != InBasicDetailsB;
}

auto
    UCk_Utils_OwningActor_UE::
    Get_ActorEntityHandleFromSelf(
        const AActor* InActor)
    -> FCk_Handle
{
    return Get_ActorEntityHandle(InActor);
}

auto
    UCk_Utils_OwningActor_UE::
    DoGetOrAdd_EntityOwningActorComponent(
        AActor* InActor)
    -> UCk_EntityOwningActor_ActorComponent_UE*
{
    if (auto ExistingComp = InActor->GetComponentByClass<UCk_EntityOwningActor_ActorComponent_UE>();
        ck::IsValid(ExistingComp))
    { return ExistingComp; }

    return UCk_Utils_Actor_UE::Request_AddNewActorComponent<UCk_EntityOwningActor_ActorComponent_UE>
    (
        UCk_Utils_Actor_UE::AddNewActorComponent_Params<UCk_EntityOwningActor_ActorComponent_UE>
        {
            InActor,
        }
    );
}

auto
    UCk_Utils_OwningActor_UE::
    DoFlush_PendingEcsReady(
        UCk_EntityOwningActor_ActorComponent_UE* InComp,
        AActor* InActor,
        const FCk_Handle& InEntity)
    -> void
{
    if (ck::Is_NOT_Valid(InComp))
    { return; }

    DoFlush_PendingEcsReady_LinkEstablished(InComp, InActor, InEntity);

    if (InComp->_PendingEcsReadyDelegates_ValuesReplicated.IsEmpty() &&
        InComp->_PendingEcsReadyCallbacks_ValuesReplicated.IsEmpty())
    { return; }

    if (DoGet_ShouldDeferUntilReplicationComplete(InEntity))
    {
        DoBind_ReplicationCompleteTrampoline(InComp, InEntity);
        return;
    }

    DoFlush_PendingEcsReady_ValuesReplicated(InComp, InActor, InEntity);
}

auto
    UCk_Utils_OwningActor_UE::
    DoFlush_PendingEcsReady_LinkEstablished(
        UCk_EntityOwningActor_ActorComponent_UE* InComp,
        AActor* InActor,
        const FCk_Handle& InEntity)
    -> void
{
    if (ck::Is_NOT_Valid(InComp))
    { return; }

    // Move out before executing so a promise that queues another promise during its own callback
    // does not mutate the container we are iterating (and is itself fired immediately since the
    // Actor is already ECS ready by this point).
    const auto PendingDelegates = MoveTemp(InComp->_PendingEcsReadyDelegates_LinkEstablished);
    const auto PendingCallbacks = MoveTemp(InComp->_PendingEcsReadyCallbacks_LinkEstablished);
    InComp->_PendingEcsReadyDelegates_LinkEstablished.Reset();
    InComp->_PendingEcsReadyCallbacks_LinkEstablished.Reset();

    for (const auto& Delegate : PendingDelegates)
    {
        Delegate.ExecuteIfBound(InActor, InEntity);
    }

    for (const auto& Callback : PendingCallbacks)
    {
        if (Callback)
        { Callback(InActor, InEntity); }
    }
}

auto
    UCk_Utils_OwningActor_UE::
    DoFlush_PendingEcsReady_ValuesReplicated(
        UCk_EntityOwningActor_ActorComponent_UE* InComp,
        AActor* InActor,
        const FCk_Handle& InEntity)
    -> void
{
    if (ck::Is_NOT_Valid(InComp))
    { return; }

    const auto PendingDelegates = MoveTemp(InComp->_PendingEcsReadyDelegates_ValuesReplicated);
    const auto PendingCallbacks = MoveTemp(InComp->_PendingEcsReadyCallbacks_ValuesReplicated);
    InComp->_PendingEcsReadyDelegates_ValuesReplicated.Reset();
    InComp->_PendingEcsReadyCallbacks_ValuesReplicated.Reset();

    for (const auto& Delegate : PendingDelegates)
    {
        Delegate.ExecuteIfBound(InActor, InEntity);
    }

    for (const auto& Callback : PendingCallbacks)
    {
        if (Callback)
        { Callback(InActor, InEntity); }
    }
}

auto
    UCk_Utils_OwningActor_UE::
    DoGet_ShouldDeferUntilReplicationComplete(
        const FCk_Handle& InEntity)
    -> bool
{
    if (UCk_Utils_EntityReplicationDriver_UE::Has(InEntity))
    { return NOT UCk_Utils_EntityReplicationDriver_UE::Get_IsReplicationComplete(InEntity); }

    // The link is established mid-Construct, BEFORE OwningActor::Add adds the ReplicationDriver —
    // fall back to the Entity's replication setting (populated by the spawn processor pre-Construct)
    // to decide whether OnReplicationComplete will eventually fire for this Entity.
    return UCk_Utils_Net_UE::Has(InEntity) &&
        UCk_Utils_Net_UE::Get_Replication(InEntity) == ECk_Replication::Replicates;
}

auto
    UCk_Utils_OwningActor_UE::
    DoBind_ReplicationCompleteTrampoline(
        UCk_EntityOwningActor_ActorComponent_UE* InComp,
        const FCk_Handle& InEntity)
    -> void
{
    if (InComp->_OnReplicationCompleteTrampolineBound)
    { return; }

    auto Delegate = FCk_Delegate_EntityReplicationDriver_OnReplicationComplete{};
    Delegate.BindDynamic(InComp, &UCk_EntityOwningActor_ActorComponent_UE::DoHandle_LinkedEntityReplicationComplete);

    auto Entity = InEntity;
    UCk_Utils_EntityReplicationDriver_UE::Promise_OnReplicationComplete(Entity, Delegate);

    InComp->_OnReplicationCompleteTrampolineBound = true;
}

// --------------------------------------------------------------------------------------------------------------------