#include "CkActorRelay_Utils.h"

#include "CkActorRelay_GroupSubsystem.h"
#include "CkActorRelay_Subsystem.h"

#include "CkActorRelay/CkActorRelay_Log.h"

#include "CkCore/Debug/CkDebug_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PendingActorRelay_UE::
    Promise_OnAcquired(
        FCk_Handle_PendingActorRelay& InPending,
        const FCk_Delegate_ActorRelay_Acquired& InDelegate)
    -> void
{
    CK_ENSURE_IF_NOT(InDelegate.IsBound(),
        TEXT("Promise_OnAcquired called with an unbound dynamic delegate"))
    { return; }

    auto GroupSubsystem = InPending._GroupSubsystem.Get();
    CK_ENSURE_IF_NOT(ck::IsValid(GroupSubsystem),
        TEXT("Promise_OnAcquired called on an invalid pending handle (no group subsystem)"))
    { return; }

    if (InPending._Resolved)
    {
        ck::actorrelay::Warning(TEXT("Promise_OnAcquired called on an already-resolved pending handle for group [{}]"),
            GroupSubsystem->Get_GroupTag());
        return;
    }

    if (auto Result = GroupSubsystem->DoTryResolve(InPending);
        ck::IsValid(Result))
    {
        InPending._Resolved = true;
        InDelegate.ExecuteIfBound(Result);
        return;
    }

    const auto Kind   = InPending._Kind;
    auto WeakSubsystem = TWeakObjectPtr(GroupSubsystem);
    auto WeakPlayer    = InPending._PlayerState;
    auto SubHandlePtr  = MakeShared<FDelegateHandle>();

    *SubHandlePtr = GroupSubsystem->_OnChannelReadyChanged.AddLambda(
        [InDelegate, WeakSubsystem, WeakPlayer, Kind, SubHandlePtr]()
        {
            auto Subsystem = WeakSubsystem.Get();
            if (ck::Is_NOT_Valid(Subsystem))
            { return; }

            if (NOT InDelegate.IsBound())
            {
                Subsystem->_OnChannelReadyChanged.Remove(*SubHandlePtr);
                return;
            }

            auto Snapshot = FCk_Handle_PendingActorRelay{};
            Snapshot._GroupSubsystem = WeakSubsystem;
            Snapshot._Kind = Kind;
            Snapshot._PlayerState = WeakPlayer;

            if (auto Result = Subsystem->DoTryResolve(Snapshot);
                ck::IsValid(Result))
            {
                InDelegate.ExecuteIfBound(Result);
                Subsystem->_OnChannelReadyChanged.Remove(*SubHandlePtr);
            }
        });

    InPending._SubscriptionHandle = *SubHandlePtr;
}

auto
    UCk_Utils_PendingActorRelay_UE::
    Promise_OnAcquired(
        FCk_Handle_PendingActorRelay& InPending,
        TFunction<void(FCk_ActorRelay_ChannelResult)> InCallback)
    -> void
{
    CK_ENSURE_IF_NOT(static_cast<bool>(InCallback),
        TEXT("Promise_OnAcquired called with an empty callback"))
    { return; }

    auto GroupSubsystem = InPending._GroupSubsystem.Get();
    CK_ENSURE_IF_NOT(ck::IsValid(GroupSubsystem),
        TEXT("Promise_OnAcquired called on an invalid pending handle (no group subsystem)"))
    { return; }

    if (InPending._Resolved)
    {
        ck::actorrelay::Warning(TEXT("Promise_OnAcquired called on an already-resolved pending handle for group [{}]"),
            GroupSubsystem->Get_GroupTag());
        return;
    }

    if (auto Result = GroupSubsystem->DoTryResolve(InPending);
        ck::IsValid(Result))
    {
        InPending._Resolved = true;
        InCallback(Result);
        return;
    }

    const auto Kind   = InPending._Kind;
    auto WeakSubsystem = TWeakObjectPtr(GroupSubsystem);
    auto WeakPlayer    = InPending._PlayerState;
    auto SubHandlePtr  = MakeShared<FDelegateHandle>();

    *SubHandlePtr = GroupSubsystem->_OnChannelReadyChanged.AddLambda(
        [Callback = MoveTemp(InCallback), WeakSubsystem, WeakPlayer, Kind, SubHandlePtr]()
        {
            auto Subsystem = WeakSubsystem.Get();
            if (ck::Is_NOT_Valid(Subsystem))
            { return; }

            auto Snapshot = FCk_Handle_PendingActorRelay{};
            Snapshot._GroupSubsystem = WeakSubsystem;
            Snapshot._Kind = Kind;
            Snapshot._PlayerState = WeakPlayer;

            if (auto Result = Subsystem->DoTryResolve(Snapshot);
                ck::IsValid(Result))
            {
                Callback(Result);
                Subsystem->_OnChannelReadyChanged.Remove(*SubHandlePtr);
            }
        });

    InPending._SubscriptionHandle = *SubHandlePtr;
}

auto
    UCk_Utils_PendingActorRelay_UE::
    Get_IsValid(
        const FCk_Handle_PendingActorRelay& InHandle)
    -> bool
{
    return ck::IsValid(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ActorRelay_UE::
    Request_AcquireChannel(
        const UObject* InWorldContextObject,
        FGameplayTag InGroupTag)
    -> FCk_Handle_PendingActorRelay
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
    -> FCk_Handle_PendingActorRelay
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
