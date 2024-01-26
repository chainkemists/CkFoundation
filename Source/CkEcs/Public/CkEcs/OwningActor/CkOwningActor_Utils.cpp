#pragma once

#include "CkOwningActor_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/OwningActor/CkOwningActor_Fragment.h"

#include "CkEcs/Handle/CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_OwningActor_UE::
    Add(
        FCk_Handle InHandle,
        AActor* InOwningActor)
    -> void
{
    InHandle.Add<ck::FFragment_OwningActor_Current>(InOwningActor);
}

auto
    UCk_Utils_OwningActor_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has<ck::FFragment_OwningActor_Current>();
}

auto
    UCk_Utils_OwningActor_UE::
    Ensure(
        FCk_Handle InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Entity [{}] does NOT have OwningActor Fragment! The Entity has no associated Actor"), InHandle)
    { return false; }

    return true;
}

auto
    UCk_Utils_OwningActor_UE::
    Get_EntityOwningActor(
        FCk_Handle InHandle)
    -> AActor*
{
    if (NOT Ensure(InHandle))
    { return {}; }

    return Get_EntityOwningActorBasicDetails(InHandle).Get_Actor().Get();
}

auto
    UCk_Utils_OwningActor_UE::
    Get_EntityOwningActorBasicDetails(
        FCk_Handle InHandle)
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
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Cannot get the ECS Handle of Actor because the Actor is invalid!"))
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
    Get_IsActorEcsReady(
        AActor* InActor)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Cannot check if Actor is ECS ready because it is invalid!"))
    { return {}; }

    const auto& EntityOwningActorComp = InActor->GetComponentByClass<UCk_EntityOwningActor_ActorComponent_UE>();

    return ck::IsValid(EntityOwningActorComp);
}

auto
    UCk_Utils_OwningActor_UE::
    Get_ActorEntityHandleFromSelf(
        const AActor* InActor)
    -> FCk_Handle
{
    return Get_ActorEntityHandle(InActor);
}

// --------------------------------------------------------------------------------------------------------------------
