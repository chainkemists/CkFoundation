#pragma once

#include "CkActorInfo_Utils.h"

#include "CkActor/ActorInfo/CkActorInfo_Fragment.h"
#include "CkActor/ActorModifier/CkActorModifier_Utils.h"

#include "CkCore/Actor/CkActor_Utils.h"
#include "CkCore/ObjectReplication/CkObjectReplicatorComponent.h"

#include "CkEcs/Handle/CkHandle.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ActorInfo_UE::
    Add(
        FCk_Handle                               InHandle,
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
                true
            },
            [&](UCk_ActorInfo_ActorComponent_UE* InComp)
            {
                InComp->_EntityHandle = InHandle;
            }
        );

        InHandle.Get<ck::FCk_Fragment_OwningActor>().Get_Bootstrapper()->_AssociatedActor = InActor;

        UCk_Utils_Actor_UE::Request_AddNewActorComponent<UCk_ObjectReplicator_Component>
        (
            UCk_Utils_Actor_UE::AddNewActorComponent_Params<UCk_ObjectReplicator_Component>
            {
                InActor,
                true
            },
            [&](UCk_ObjectReplicator_Component* InComp)
            {
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
    Link(AActor* InActor, FCk_Handle InHandle) -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActor), TEXT("Unable to link Actor [{}] to Entity [{}]. Actor is INVALID."), InActor, InHandle)
    { return; }

    CK_ENSURE_IF_NOT(NOT Has(InHandle), TEXT("Unable to link Actor [{}] to Entity [{}]. Entity is already linked."), InActor, InHandle)
    { return; }

    InHandle.Add<ck::FCk_Fragment_ActorInfo_Params>(FCk_Fragment_ActorInfo_ParamsData
    {
        InActor->GetClass(),
        InActor->GetActorTransform(),
        InActor->GetOwner(),
        InActor->GetIsReplicated() ? ECk_Actor_NetworkingType::Replicated : ECk_Actor_NetworkingType::Local
    });

    InHandle.Add<ck::FCk_Fragment_ActorInfo_Current>(InActor);
    InHandle.Add<ck::FTag_ActorInfo_LinkUp>();

    if (InActor->GetComponentByClass<UCk_ActorInfo_ActorComponent_UE>())
    { return; }

    UCk_Utils_Actor_UE::Request_AddNewActorComponent<UCk_ActorInfo_ActorComponent_UE>
    (
        UCk_Utils_Actor_UE::AddNewActorComponent_Params<UCk_ActorInfo_ActorComponent_UE>
        {
            InActor,
            true
        },
        [&](UCk_ActorInfo_ActorComponent_UE* InComp)
        {
            InComp->_EntityHandle = InHandle;
        }
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

auto
    UCk_Utils_OwningActor_UE::
    Add(FCk_Handle InHandle, AActor* InOwningActor, UCk_EcsBootstrapper_Base_UE* InBootstrapper)
    -> void
{
    InHandle.Add<ck::FCk_Fragment_OwningActor>(InOwningActor, InBootstrapper);
}

auto
    UCk_Utils_OwningActor_UE::
    Has(FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has<ck::FCk_Fragment_OwningActor>();
}

auto
    UCk_Utils_OwningActor_UE::
    Ensure(FCk_Handle InHandle)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Entity [{}] does NOT have OwningActor!"), InHandle)
    { return false; }

    return true;
}

auto
    UCk_Utils_OwningActor_UE::
    Get_OwningActor(FCk_Handle InHandle) -> AActor*
{
    if (NOT Ensure(InHandle))
    { return {}; }

    return InHandle.Get<ck::FCk_Fragment_OwningActor>().Get_OwningActor().Get();
}

// --------------------------------------------------------------------------------------------------------------------
