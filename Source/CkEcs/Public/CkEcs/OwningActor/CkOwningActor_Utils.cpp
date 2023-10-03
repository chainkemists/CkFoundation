#pragma once

#include "CkOwningActor_Utils.h"

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

    return FCk_EntityOwningActor_BasicDetails{ InHandle.Get<ck::FFragment_OwningActor_Current>().Get_EntityOwningActor().Get(), InHandle };
}

auto
    UCk_Utils_OwningActor_UE::
    Get_EntityOwningActorBasicDetails_FromActor(
        const AActor* InActor)
    -> FCk_EntityOwningActor_BasicDetails
{
    const auto& actorEcsHandle = Get_ActorEcsHandle(InActor);

    if (ck::Is_NOT_Valid(actorEcsHandle))
    { return {}; }

    return Get_EntityOwningActorBasicDetails(actorEcsHandle);
}

auto
    UCk_Utils_OwningActor_UE::
    Get_ActorEcsHandle(
        const AActor* InActor)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Cannot get the ECS Handle of Actor because it is invalid!"))
    { return {}; }

    const auto& entityOwningActorComp = InActor->FindComponentByClass<UCk_EntityOwningActor_ActorComponent_UE>();

    CK_ENSURE_IF_NOT(ck::IsValid(entityOwningActorComp),
        TEXT("Actor [{}] does NOT have an Entity Owning Actor Unreal Actor Component! This means it is not ECS ready."),
        InActor)
    { return {}; }

    return entityOwningActorComp->Get_EntityHandle();
}

auto
    UCk_Utils_OwningActor_UE::
    Get_IsActorEcsReady(
        AActor* InActor)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Cannot check if Actor is ECS ready because it is invalid!"))
    { return {}; }

    const auto& entityOwningActorComp = InActor->GetComponentByClass<UCk_EntityOwningActor_ActorComponent_UE>();

    return ck::IsValid(entityOwningActorComp);
}

// --------------------------------------------------------------------------------------------------------------------
