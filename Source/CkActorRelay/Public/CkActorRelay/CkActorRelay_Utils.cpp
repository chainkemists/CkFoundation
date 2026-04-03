#include "CkActorRelay_Utils.h"

#include "CkActorRelay_Subsystem.h"

#include "CkActorRelay/CkActorRelay_Log.h"

#include "CkCore/Debug/CkDebug_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ActorRelay_UE::
    Request_AcquireChannel(
        const UObject* InWorldContextObject,
        FGameplayTag InGroupTag)
    -> FCk_ActorRelay_ChannelResult
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWorldContextObject),
        TEXT("WorldContextObject is invalid when acquiring channel"))
    { return {}; }

    auto World = InWorldContextObject->GetWorld();

    CK_ENSURE_IF_NOT(ck::IsValid(World),
        TEXT("World is invalid when acquiring channel"))
    { return {}; }

    auto RelaySubsystem = World->GetSubsystem<UCk_ActorRelay_Subsystem_UE>();

    CK_ENSURE_IF_NOT(ck::IsValid(RelaySubsystem),
        TEXT("ActorRelay subsystem is invalid when acquiring channel"))
    { return {}; }

    return RelaySubsystem->Request_AcquireChannel(InGroupTag);
}

auto
    UCk_Utils_ActorRelay_UE::
    Request_AcquireChannel_ForPlayer(
        const UObject* InWorldContextObject,
        FGameplayTag InGroupTag,
        APlayerState* InPlayerState)
    -> FCk_ActorRelay_ChannelResult
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWorldContextObject),
        TEXT("WorldContextObject is invalid when acquiring channel for player"))
    { return {}; }

    auto World = InWorldContextObject->GetWorld();

    CK_ENSURE_IF_NOT(ck::IsValid(World),
        TEXT("World is invalid when acquiring channel for player"))
    { return {}; }

    auto RelaySubsystem = World->GetSubsystem<UCk_ActorRelay_Subsystem_UE>();

    CK_ENSURE_IF_NOT(ck::IsValid(RelaySubsystem),
        TEXT("ActorRelay subsystem is invalid when acquiring channel for player"))
    { return {}; }

    return RelaySubsystem->Request_AcquireChannel_ForPlayer(InGroupTag, InPlayerState);
}

auto
    UCk_Utils_ActorRelay_UE::
    Get_ChannelEntityCount(
        const FCk_ActorRelay_ChannelResult& InChannelResult)
    -> int32
{
    if (ck::Is_NOT_Valid(InChannelResult))
    { return 0; }

    return UCk_Utils_EntityLifetime_UE::Get_LifetimeDependents(InChannelResult.Get_ChannelEntity()).Num();
}

auto
    UCk_Utils_ActorRelay_UE::
    Get_IsChannelResultValid(
        const FCk_ActorRelay_ChannelResult& InChannelResult)
    -> bool
{
    return ck::IsValid(InChannelResult);
}

// --------------------------------------------------------------------------------------------------------------------
