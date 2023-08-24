#include "CkOwningActor_Fragment_Params.h"

#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include "Net/UnrealNetwork.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_EntityOwningActor_ActorComponent_UE::
    UCk_EntityOwningActor_ActorComponent_UE()
{
    SetIsReplicated(false);
}

// --------------------------------------------------------------------------------------------------------------------

FCk_EntityOwningActor_BasicDetails::
    FCk_EntityOwningActor_BasicDetails(
        AActor* InActor,
        FCk_Handle InHandle)
    : _Actor(InActor)
    , _Handle(MoveTemp(InHandle))
{
}

auto
    FCk_EntityOwningActor_BasicDetails::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_Actor() == InOther.Get_Actor() && Get_Handle() == InOther.Get_Handle();
}

auto
    GetTypeHash(
        const FCk_EntityOwningActor_BasicDetails& InBasicDetails)
    -> uint32
{
    return GetTypeHash(InBasicDetails.Get_Actor()) + GetTypeHash(InBasicDetails.Get_Handle());
}

// --------------------------------------------------------------------------------------------------------------------

