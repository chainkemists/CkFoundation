#include "CkActorRelay_Actor.h"

#include "CkActorRelay_GroupSubsystem.h"

#include "CkActorRelay/CkActorRelay_Log.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcs/Snapshot/CkSaveKey_Fragment.h"
#include "CkEcsExt/EntityScript/CkEntityScript_WithActor.h"
#include "CkEcsExt/EntityScript/CkEntityScript_WithActor_Utils.h"

#include <Net/UnrealNetwork.h>
#include <Net/Core/PushModel/PushModel.h>

// --------------------------------------------------------------------------------------------------------------------

ACk_ActorRelay_UE::
    ACk_ActorRelay_UE()
{
    bReplicates = true;
    bAlwaysRelevant = true;
    bReplicateUsingRegisteredSubObjectList = true;
    PrimaryActorTick.bCanEverTick = false;
    PrimaryActorTick.bTickEvenWhenPaused = false;
}

auto
    ACk_ActorRelay_UE::
    BeginPlay()
    -> void
{
    Super::BeginPlay();

    UCk_Utils_EntityScript_WithActor_UE::Request_SpawnEntityScript_OnActor(this, UCk_EntityScript_WithActor_UE::StaticClass());

    OnRep_GroupSubsystemClass();
}

auto
    ACk_ActorRelay_UE::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    constexpr auto Params = FDoRepLifetimeParams{COND_None, REPNOTIFY_Always, true};

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _GroupSubsystemClass, Params);
}

auto
    ACk_ActorRelay_UE::
    InjectGroupSubsystemClass(
        TSubclassOf<UCk_ActorRelay_Group_Subsystem_Base_UE> InGroupSubsystemClass)
    -> void
{
    _GroupSubsystemClass = InGroupSubsystemClass;
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _GroupSubsystemClass, this);
}

auto
    ACk_ActorRelay_UE::
    OnRep_GroupSubsystemClass()
    -> void
{
    if (ck::Is_NOT_Valid(_GroupSubsystemClass))
    { return; }

    if (ck::IsValid(_GroupSubsystem))
    { return; }

    if (DoTryRegisterWithGroupSubsystem())
    { return; }

    ck::actorrelay::Verbose(TEXT("GroupSubsystem not yet resolved for ActorRelay. Will retry every 100ms"));

    auto WeakThis = TWeakObjectPtr(this);
    GetWorld()->GetTimerManager().SetTimer(_RegistrationRetryTimerHandle, [WeakThis]()
    {
        if (ck::Is_NOT_Valid(WeakThis))
        { return; }

        if (WeakThis->DoTryRegisterWithGroupSubsystem())
        {
            ck::actorrelay::Verbose(TEXT("Successfully registered ActorRelay with GroupSubsystem"));
            WeakThis->GetWorld()->GetTimerManager().ClearTimer(WeakThis->_RegistrationRetryTimerHandle);
        }
    }, 0.1f, true);
}

auto
    ACk_ActorRelay_UE::
    DoTryRegisterWithGroupSubsystem()
    -> bool
{
    auto GroupSubsystem = Cast<UCk_ActorRelay_Group_Subsystem_Base_UE>(GetWorld()->GetSubsystemBase(_GroupSubsystemClass));

    if (ck::Is_NOT_Valid(GroupSubsystem))
    {
        ck::actorrelay::Verbose(TEXT("Relay [{}]: group subsystem [{}] not found yet"),
            this, _GroupSubsystemClass);
        return false;
    }

    if (NOT GroupSubsystem->DoRegisterChannelActor(this))
    {
        ck::actorrelay::Verbose(TEXT("Relay [{}]: subsystem [{}] found but registration not possible yet (Owner [{}] not a resolvable PlayerState)"),
            this, GroupSubsystem, GetOwner());
        return false;
    }

    _GroupSubsystem = GroupSubsystem;

    ck::actorrelay::Verbose(TEXT("Relay [{}]: registered with [{}] (Owner [{}])"),
        this, GroupSubsystem, GetOwner());

    DoStamp_SaveKey();
    DoStartBroadcastWhenReadyPolling();

    return true;
}

auto
    ACk_ActorRelay_UE::
    DoStamp_SaveKey()
    -> void
{
    const auto GroupSubsystem = _GroupSubsystem.Get();

    const auto GroupSubsystemIsValid = ck::IsValid(GroupSubsystem);
    CK_ENSURE_IF_NOT(GroupSubsystemIsValid,
        TEXT("Relay [{}]: cannot derive a SaveKey — registration reported success with no resolved group subsystem"), this)
    {}
    if (NOT GroupSubsystemIsValid)
    { return; }

    // Derived from the GROUP TAG alone: a channel has no identity beyond its group, and pool ordering/growth is
    // demand-driven, so there is nothing per-instance that is stable across two runs. A pool of N live channels
    // therefore shares one key and N saved channel rows rendezvous onto a single fresh channel. That consolidation
    // is the intended behaviour — every channel of a group is interchangeable, and the alternative leaves the whole
    // channel-owned subtree with an owner the loader cannot resolve.
    const auto SaveKeyIdentity = GroupSubsystem->Get_GroupTag().GetTagName().ToString();

    UCk_Utils_OwningActor_UE::Promise_OnActorEcsReady(this,
        [SaveKeyIdentity](AActor*, FCk_Handle InChannelEntity) -> void
        {
            ck::save_key::Assign(InChannelEntity, SaveKeyIdentity);
        },
        ECk_ActorEcsReady_Policy::LinkEstablished);
}

auto
    ACk_ActorRelay_UE::
    DoStartBroadcastWhenReadyPolling()
    -> void
{
    auto WeakThis = TWeakObjectPtr(this);
    auto TryBroadcast = [WeakThis]() -> bool
    {
        if (ck::Is_NOT_Valid(WeakThis))
        { return true; }

        auto Subsystem = WeakThis->_GroupSubsystem.Get();
        if (ck::Is_NOT_Valid(Subsystem))
        { return false; }

        if (NOT UCk_Utils_OwningActor_UE::Get_IsActorEcsReady(WeakThis.Get()))
        { return false; }

        Subsystem->DoBroadcastChannelReadyChanged();
        return true;
    };

    if (TryBroadcast())
    { return; }

    GetWorld()->GetTimerManager().SetTimer(_BroadcastReadyTimerHandle, [WeakThis, TryBroadcast]()
    {
        if (ck::Is_NOT_Valid(WeakThis))
        { return; }

        if (TryBroadcast())
        { WeakThis->GetWorld()->GetTimerManager().ClearTimer(WeakThis->_BroadcastReadyTimerHandle); }
    }, 0.1f, true);
}

// --------------------------------------------------------------------------------------------------------------------
