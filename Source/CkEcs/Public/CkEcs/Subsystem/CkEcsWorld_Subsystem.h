#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkEcs/World/CkEcsWorld.h"

#include <Subsystems/WorldSubsystem.h>
#include <GameFramework/Info.h>

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

UCLASS(BlueprintType)
class CKECS_API UCk_EcsWorld_Subsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EcsWorld_Subsystem_UE);

public:
    using EcsWorldType = ck::FEcsWorld;

public:
    /** Called when world is ready to start gameplay before the game mode transitions to the correct state and call BeginPlay on all actors */
    auto OnWorldBeginPlay(UWorld& InWorld) -> void override;

private:
    auto DoSpawnWorldActor() -> void;

private:
    UPROPERTY(BlueprintReadOnly, Transient, meta = (AllowPrivateAccess = true))
    FCk_Handle _TransientEntity;

    UPROPERTY(Transient)
    TObjectPtr<ACk_World_Actor_Base_UE> _WorldActor;

public:
    CK_PROPERTY_GET(_TransientEntity);
};

// --------------------------------------------------------------------------------------------------------------------
