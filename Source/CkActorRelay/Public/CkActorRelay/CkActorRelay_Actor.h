#pragma once

#include "CkActorRelay_Fragment_Data.h"

#include "CkActorRelay_Actor.generated.h"

class UCk_ActorRelay_Group_Subsystem_Base_UE;

/*-----------------------------------------------------------------------------
                          ACTOR RELAY BASE
------------------------------------------------------------------------------*/

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

private:
    UPROPERTY(ReplicatedUsing = OnRep_GroupSubsystemClass)
    TSubclassOf<UCk_ActorRelay_Group_Subsystem_Base_UE> _GroupSubsystemClass;

    UPROPERTY(Transient)
    TWeakObjectPtr<UCk_ActorRelay_Group_Subsystem_Base_UE> _GroupSubsystem;

    FTimerHandle _RegistrationRetryTimerHandle;

public:
    CK_PROPERTY_GET(_GroupSubsystem);
};

// --------------------------------------------------------------------------------------------------------------------
