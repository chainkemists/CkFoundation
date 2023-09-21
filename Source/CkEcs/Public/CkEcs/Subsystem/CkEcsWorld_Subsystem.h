#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/World/CkEcsWorld.h"

#include <Subsystems/WorldSubsystem.h>

#include "CkEcsWorld_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, NotBlueprintable, NotBlueprintType)
class CKECS_API ACk_World_Actor_Base_UE : public AInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_World_Actor_Base_UE);

public:
    friend class UCk_EcsWorld_Subsystem_UE;

public:
    using EcsWorldType = ck::FEcsWorld;

public:
    ACk_World_Actor_Base_UE();

protected:
    virtual auto Tick(float DeltaSeconds) -> void override;

public:
    virtual auto Initialize(ETickingGroup InTickingGroup) -> void;

protected:
    TOptional<EcsWorldType> _EcsWorld;
};


// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKECS_API UCk_EcsWorld_Subsystem_UE : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EcsWorld_Subsystem_UE);

public:
    using EcsWorldType = ck::FEcsWorld;

public:
    virtual auto Deinitialize() -> void override;
    virtual auto Initialize(FSubsystemCollectionBase& Collection) -> void override;

    /** Called once all UWorldSubsystems have been initialized */
    virtual auto PostInitialize() -> void override;

    /** Called when world is ready to start gameplay before the game mode transitions to the correct state and call BeginPlay on all actors */
    virtual auto OnWorldBeginPlay(UWorld& InWorld) -> void override;

    virtual auto ShouldCreateSubsystem(UObject* Outer) const -> bool override;

protected:
    // Called when determining whether to create this Subsystem
    virtual auto DoesSupportWorldType(const EWorldType::Type WorldType) const -> bool override;

public:
    UFUNCTION(BlueprintImplementableEvent)
    void OnInitialize();

    UFUNCTION(BlueprintImplementableEvent)
    void OnAllWorldSubsystemsInitialized();

    UFUNCTION(BlueprintImplementableEvent)
    void OnDeinitialize();

private:
    auto DoSpawnWorldActor() -> void;

private:
    UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = true))
    TEnumAsByte<ETickingGroup> _TickingGroup = TG_PrePhysics;

    UPROPERTY(BlueprintReadOnly, Transient, meta = (AllowPrivateAccess = true))
    FCk_Handle _TransientEntity;

    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<ACk_World_Actor_Base_UE> _WorldActorClass;

    UPROPERTY(Transient)
    TObjectPtr<ACk_World_Actor_Base_UE> _WorldActor;

public:
    CK_PROPERTY_GET(_TransientEntity);
};

// --------------------------------------------------------------------------------------------------------------------
