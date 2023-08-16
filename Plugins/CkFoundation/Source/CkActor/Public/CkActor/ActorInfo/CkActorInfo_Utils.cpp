#pragma once

#include "CkActorInfo_Utils.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkActorInfo_Fragment.h"

#include "Actor/CkActor_Utils.h"

#include "CkActor/ActorModifier/CkActorModifier_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ActorInfo_UE::
    Add(
        FCk_Handle                                InHandle,
        const FCk_Fragment_ActorInfo_ParamsData& InParams)
    -> void
{
    InHandle.Add<ck::FCk_Fragment_ActorInfo_Params>(InParams);
    InHandle.Add<ck::FCk_Fragment_ActorInfo_Current>();

    const auto& actorToSpawn        = InParams.Get_ActorClass();
    const auto& actorSpawnTransform = InParams.Get_ActorSpawnTransform();

    CK_ENSURE_IF_NOT(ck::IsValid(actorToSpawn), TEXT("Cannot spawn Entity Actor for [{}] because it is INVALID"), InHandle)
    { return; }

    auto AddUnrealActorInfoCompFunc = [InHandle](AActor* InActor) mutable -> void
    {
        InHandle.Get<ck::FCk_Fragment_ActorInfo_Current>()._EntityActor = InActor;

        UCk_Utils_Actor_UE::Request_AddNewActorComponent<UCk_ActorInfo_ActorComponent_UE>
        (
            UCk_Utils_Actor_UE::AddNewActorComponent_Params<UCk_ActorInfo_ActorComponent_UE>
            {
                InActor,
                UCk_ActorInfo_ActorComponent_UE::StaticClass(),
                true
            },
            [&](UCk_ActorInfo_ActorComponent_UE* InComp)
            {
                InComp->_EntityHandle = InHandle;
            }
        );
    };

    const auto& spawnActorParams = FCk_Utils_Actor_SpawnActor_Params{InParams.Get_Owner(), InParams.Get_ActorClass()}
        .Set_SpawnTransform(actorSpawnTransform)
        .Set_CollisionHandlingOverride(ESpawnActorCollisionHandlingMethod::AlwaysSpawn)
        .Set_NetworkingType(InParams.Get_NetworkingType())
        .Set_SpawnPolicy(ECk_Utils_Actor_SpawnActorPolicy::Default);

    UCk_Utils_ActorModifier_UE::Request_SpawnActor
    (
        InHandle,
        FCk_Request_ActorModifier_SpawnActor{spawnActorParams, FCk_SpawnActor_PostSpawn_Params{}, AddUnrealActorInfoCompFunc},
        {}
    );
}

auto
    UCk_Utils_ActorInfo_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has_All<ck::FCk_Fragment_ActorInfo_Params, ck::FCk_Fragment_ActorInfo_Current>();
}

auto
    UCk_Utils_ActorInfo_UE::
    Ensure(
        FCk_Handle InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Entity [{}] does NOT have ActorInfo! The Entity has no associated Actor"), InHandle)
    { return false; }

    return true;
}

auto
    UCk_Utils_ActorInfo_UE::
    Get_ActorInfoBasicDetails(
        FCk_Handle InHandle)
    -> FCk_ActorInfo_BasicDetails
{
    if (NOT Ensure(InHandle))
    { return {}; }

    return FCk_ActorInfo_BasicDetails{InHandle.Get<ck::FCk_Fragment_ActorInfo_Current>().Get_EntityActor().Get(), InHandle};
}

auto
    UCk_Utils_ActorInfo_UE::
    Get_ActorInfoBasicDetails_FromActor(
        const AActor* InActor)
    -> FCk_ActorInfo_BasicDetails
{
    const auto& actorEcsHandle = Get_ActorEcsHandle(InActor);

    if (ck::Is_NOT_Valid(actorEcsHandle))
    { return {}; }

    return Get_ActorInfoBasicDetails(actorEcsHandle);
}

auto
    UCk_Utils_ActorInfo_UE::
    Get_ActorEcsHandle(
        const AActor* InActor)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Cannot get the ECS Handle of Actor because it is invalid!"))
    { return {}; }

    const auto& actorInfoComp = InActor->FindComponentByClass<UCk_ActorInfo_ActorComponent_UE>();

    CK_ENSURE_IF_NOT(ck::IsValid(actorInfoComp),
        TEXT("Actor [{}] does NOT have an Actor Info Unreal Actor Component! This means it is not ECS ready."),
        InActor)
    { return {}; }

    return actorInfoComp->Get_EntityHandle();
}

auto
    UCk_Utils_ActorInfo_UE::
    Get_IsActorEcsReady(
        AActor* InActor)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Cannot check if Actor is ECS ready because it is invalid!"))
    { return {}; }

    const auto& unrealEntityConstructionScriptComp = InActor->FindComponentByClass<UCk_ActorInfo_ActorComponent_UE>();

    return ck::IsValid(unrealEntityConstructionScriptComp);
}

// --------------------------------------------------------------------------------------------------------------------
