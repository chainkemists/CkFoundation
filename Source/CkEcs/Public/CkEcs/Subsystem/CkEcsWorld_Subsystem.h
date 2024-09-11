#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkEcs/World/CkEcsWorld.h"
#include "CkEcs/Processor/CkProcessorScript.h"
#include "CkEcs/ProcessorInjector/CkEcsMetaProcessorInjector.h"

#include <Subsystems/WorldSubsystem.h>
#include <GameFramework/Info.h>

#include "CkEcsWorld_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, NotBlueprintable, NotBlueprintType, EditInlineNew)
class CKECS_API UCk_EcsWorld_ProcessorInjector_Base_UE : public UObject
{
    GENERATED_BODY()

    friend class ACk_EcsWorld_Actor_UE;
    friend class ACk_NewEcsWorld_Actor_UE;

public:
    using EcsWorldType = ck::FEcsWorld;

public:
    CK_GENERATED_BODY(UCk_EcsWorld_ProcessorInjector_Base_UE);

protected:
    virtual auto
    DoInjectProcessors(
        EcsWorldType& InWorld) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, NotBlueprintable, NotBlueprintType)
class CKECS_API UCk_EcsWorld_ProcessorScriptInjector_UE : public UCk_EcsWorld_ProcessorInjector_Base_UE
{
    GENERATED_BODY()

private:
    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<UCk_Ecs_ProcessorScript_Base_UE> _Processor;

private:
    auto
    DoInjectProcessors(
        EcsWorldType& InWorld) -> void override;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, NotBlueprintType)
class CKECS_API ACk_EcsWorld_Actor_UE final : public AInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_EcsWorld_Actor_UE);

public:
    friend class UCk_EcsWorld_Subsystem_UE;

public:
    using EcsWorldType = ck::FEcsWorld;

public:
    ACk_EcsWorld_Actor_UE();

protected:
    auto
    Tick(
        float DeltaSeconds) -> void override;

public:
    auto
    Initialize(
        const FCk_Registry& InRegistry,
        const FCk_Ecs_MetaProcessorInjectors_Info& InMetaInjectorInfo) -> void;

protected:
    struct FWorldInfo
    {
        int32 _MaxNumberOfPumps = 1;
        TOptional<EcsWorldType> _EcsWorld;
    };

    FWorldInfo _WorldToTick;
    TStatId _StatId_Tick;
    TStatId _StatId_Pump;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable)
class CKECS_API ACk_NewEcsWorld_Actor_UE final : public AInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_NewEcsWorld_Actor_UE);

public:
    using EcsWorldType = ck::FEcsWorld;

public:
    ACk_NewEcsWorld_Actor_UE();

protected:
    auto
    BeginPlay() -> void override;

    auto
    Tick(
        float DeltaSeconds) -> void override;

    UPROPERTY(EditDefaultsOnly, meta=(AllowPrivateAccess))
    TArray<FCk_Ecs_MetaProcessorInjectors_Info> _MetaProcessorInjectors;

protected:
    EcsWorldType _EcsWorld;
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
    auto
    Initialize(
        FSubsystemCollectionBase& Collection) -> void override;

    /** Called when world is ready to start gameplay before the game mode transitions to the correct state and call BeginPlay on all actors */
    auto OnWorldBeginPlay(UWorld& InWorld) -> void override;

private:
    auto DoSpawnWorldActors(UWorld& InWorld) -> void;

private:
    UPROPERTY(BlueprintReadOnly, Transient, meta = (AllowPrivateAccess = true))
    FCk_Handle _TransientEntity;

private:
    TMultiMap<ETickingGroup, TArray<TStrongObjectPtr<ACk_EcsWorld_Actor_UE>>> _WorldActors;

private:
    FCk_Registry _Registry;

public:
    CK_PROPERTY_GET(_TransientEntity);
    CK_PROPERTY_GET(_Registry);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKECS_API UCk_Utils_EcsWorld_Subsystem_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    using SubsystemType = UCk_EcsWorld_Subsystem_UE;

public:
    static auto
    Get_TransientEntity(
        const UWorld* InWorld) -> FCk_Handle;
};

// --------------------------------------------------------------------------------------------------------------------
