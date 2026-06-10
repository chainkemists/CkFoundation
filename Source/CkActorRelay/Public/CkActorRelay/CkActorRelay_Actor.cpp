#include "CkActorRelay_Actor.h"

#include "CkActorRelay_GroupSubsystem.h"

#include "CkActorRelay/CkActorRelay_Log.h"

#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/OwningActor/CkOwningActor_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"
#include "CkEcsExt/EntityScript/CkEntityScript_WithActor.h"
#include "CkEcsExt/EntityScript/CkEntityScript_WithActor_Data.h"

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

    if (HasAuthority())
    {
        auto TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(GetWorld());
        auto SpawnParams = FInstancedStruct::Make<FCk_EntityScript_WithActor_SpawnParams>(this);
        UCk_Utils_EntityScript_UE::Request_SpawnEntity(TransientEntity, UCk_EntityScript_WithActor_UE::StaticClass(), SpawnParams);
    }

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

    DoStartBroadcastWhenReadyPolling();

    return true;
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
