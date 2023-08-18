#include "CkActorInfo_Fragment_Params.h"

#include "Net/UnrealNetwork.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Fragment_ActorInfo_ParamsData::
    FCk_Fragment_ActorInfo_ParamsData(
        TSubclassOf<AActor> InActorClass,
        FTransform InActorSpawnTransform,
        TObjectPtr<AActor> InOwner,
        ECk_Actor_NetworkingType InNetworkingType)
    : _ActorClass(InActorClass)
    , _ActorSpawnTransform(MoveTemp(InActorSpawnTransform))
    , _Owner(InOwner)
    , _NetworkingType(InNetworkingType)
{
}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Ecs_ReplicatedObject::
    OnRep_AssociatedActor(AActor* InActor)
    -> void
{
}

auto UCk_Ecs_ReplicatedObject::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(ThisType, _AssociatedActor, COND_None, REPNOTIFY_Always);
}

// --------------------------------------------------------------------------------------------------------------------

FCk_ActorInfo_BasicDetails::
    FCk_ActorInfo_BasicDetails(
        AActor* InActor,
        FCk_Handle InHandle)
    : _Actor(InActor)
    , _Handle(MoveTemp(InHandle))
{
}

auto
    FCk_ActorInfo_BasicDetails::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_Actor() == InOther.Get_Actor(); // TODO: Compare handle as well => && Get_Handle() == InOther.Get_Handle();
}

auto
    GetTypeHash(
        const FCk_ActorInfo_BasicDetails& InBasicDetails)
    -> uint32
{
    return GetTypeHash(InBasicDetails.Get_Actor()); // TODO: overload GetTypeHash(): + GetTypeHash(InBasicDetails.Get_Handle());
}

// --------------------------------------------------------------------------------------------------------------------

