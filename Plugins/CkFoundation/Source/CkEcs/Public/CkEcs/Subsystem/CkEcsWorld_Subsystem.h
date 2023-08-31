#pragma once

#include "CkMacros/CkMacros.h"

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
    using FEcsWorldType = ck::FEcsWorld;

public:
    ACk_World_Actor_Base_UE();

protected:
    virtual auto Tick(float DeltaSeconds) -> void override;

public:
    virtual auto Initialize(ETickingGroup InTickingGroup) -> void;

protected:
    TOptional<FEcsWorldType> _EcsWorld;
};


// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKECS_API UCk_EcsWorld_Subsystem_UE : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EcsWorld_Subsystem_UE);

public:
    using FEcsWorldType = ck::FEcsWorld;

public:
    virtual auto Deinitialize() -> void override;
    virtual auto Initialize(FSubsystemCollectionBase& Collection) -> void override;

public:
    UFUNCTION(BlueprintImplementableEvent)
    void OnInitialize();

    UFUNCTION(BlueprintImplementableEvent)
    void OnDeInitialize();

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
    TObjectPtr<ACk_World_Actor_Base_UE> _WorldActor = nullptr;

public:
    CK_PROPERTY_GET(_TransientEntity);
};