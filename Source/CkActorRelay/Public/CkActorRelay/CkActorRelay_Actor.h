#pragma once

#include "CkActorRelay_Fragment_Data.h"

#include <Containers/Ticker.h>

#include "CkActorRelay_Actor.generated.h"

class UCk_ActorRelay_Group_Subsystem_Base_UE;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract)
class CKACTORRELAY_API ACk_ActorRelay_UE : public AActor
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_ActorRelay_UE);

    friend class UCk_ActorRelay_Group_Subsystem_Base_UE;

public:
    ACk_ActorRelay_UE();

protected:
    auto
    BeginPlay() -> void override;

    // Both polls below outlive a single frame, so a relay that dies mid-poll must take its tickers with it — an
    // FTSTicker delegate is not owned by the world and would otherwise keep firing against a dead actor.
    auto
    EndPlay(
        const EEndPlayReason::Type InEndPlayReason) -> void override;

    auto
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const -> void override;

private:
    auto
    InjectGroupSubsystemClass(
        TSubclassOf<UCk_ActorRelay_Group_Subsystem_Base_UE> InGroupSubsystemClass) -> void;

private:
    UFUNCTION()
    void
    OnRep_GroupSubsystemClass();

private:
    auto
    DoTryRegisterWithGroupSubsystem() -> bool;

    auto
    DoStartBroadcastWhenReadyPolling() -> void;

    auto
    DoStamp_SaveKey() -> void;

    auto
    DoStop_PollTickers() -> void;

private:
    UPROPERTY(ReplicatedUsing = OnRep_GroupSubsystemClass)
    TSubclassOf<UCk_ActorRelay_Group_Subsystem_Base_UE> _GroupSubsystemClass;

    UPROPERTY(Transient)
    TWeakObjectPtr<UCk_ActorRelay_Group_Subsystem_Base_UE> _GroupSubsystem;

    // WALL clock, not FTimerManager. A snapshot load freezes game time (M-C) and FTimerManager ticks the DILATED
    // delta, so a channel whose readiness only arrives on a game-time poll can never become ready DURING a load —
    // which is exactly when the loader is waiting on it.
    FTSTicker::FDelegateHandle _RegistrationRetryTickerHandle;
    FTSTicker::FDelegateHandle _BroadcastReadyTickerHandle;

public:
    CK_PROPERTY_GET(_GroupSubsystem);
};

// --------------------------------------------------------------------------------------------------------------------
