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
    friend class UCk_EcsWorld_Stats_Subsystem_UE;

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

private:
    struct FWorldInfo
    {
        int32 _MaxNumberOfPumps = 1;
        TOptional<EcsWorldType> _EcsWorld;
    };

    FWorldInfo _WorldToTick;
    TStatId _TickStatId;
    FString _TickStatName;
    ETickingGroup _UnrealTickingGroup;
    FGameplayTag _EcsWorldTickingGroup;
    ECk_Ecs_WorldStatCollection_Policy _StatCollectionPolicy = ECk_Ecs_WorldStatCollection_Policy::DoNotCollect;
    FName _EcsWorldDisplayName;

public:
    CK_PROPERTY_GET(_UnrealTickingGroup);
    CK_PROPERTY_GET(_TickStatName);
    CK_PROPERTY_GET(_EcsWorldTickingGroup);
    CK_PROPERTY_GET(_StatCollectionPolicy);
    CK_PROPERTY_GET(_EcsWorldDisplayName);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKECS_API UCk_EcsWorld_Subsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EcsWorld_Subsystem_UE);

public:
    friend class UCk_EcsWorld_Stats_Subsystem_UE;

public:
    using EcsWorldType = ck::FEcsWorld;

public:
    auto
    Initialize(
        FSubsystemCollectionBase& Collection) -> void override;

    /** Called when world is ready to start gameplay before the game mode transitions to the correct state and call BeginPlay on all actors */
    auto
    OnWorldBeginPlay(
        UWorld& InWorld) -> void override;

private:
    auto DoSpawnWorldActors(
        UWorld& InWorld) -> void;

private:
    UPROPERTY(BlueprintReadOnly, Transient, meta = (AllowPrivateAccess = true))
    FCk_Handle _TransientEntity;

private:
    TMap<TEnumAsByte<ETickingGroup>, TArray<TStrongObjectPtr<ACk_EcsWorld_Actor_UE>>> _WorldActors_ByUnrealTickingGroup;
    TMap<FGameplayTag, TStrongObjectPtr<ACk_EcsWorld_Actor_UE>> _WorldActors_ByEcsWorldTickingGroup;

private:
    FCk_Registry _Registry;

public:
    // Should only be used by internal functions that truly require the Registry directly (i.e. usage should be RARE)
    template <typename T_Func>
    auto
    Request_PerformOperationOnInternalRegistry(
        T_Func InFunc);

public:
    CK_PROPERTY_GET(_TransientEntity);
    CK_PROPERTY_GET(_Registry);
};

// --------------------------------------------------------------------------------------------------------------------

template <typename T_Func>
auto
    UCk_EcsWorld_Subsystem_UE::
    Request_PerformOperationOnInternalRegistry(
        T_Func InFunc)
{
   _Registry.Request_PerformOperationOnInternalRegistry(InFunc);
}

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
