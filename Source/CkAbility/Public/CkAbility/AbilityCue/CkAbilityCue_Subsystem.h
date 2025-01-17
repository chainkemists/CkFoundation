#pragma once

#include "CkAbility/AbilityCue/CkAbilityCue_Fragment_Data.h"

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

// needed for non-unity builds
#include <GameFramework/GameModeBase.h>
#include <Subsystems/EngineSubsystem.h>

#include "CkAbilityCue_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_AbilityCue_Config_PDA;

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKABILITY_API ACk_AbilityCueReplicator_UE : public AActor
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_AbilityCueReplicator_UE);

public:
    ACk_AbilityCueReplicator_UE();

public:
    UFUNCTION(Server, Unreliable)
    void
    Server_RequestExecuteAbilityCue(
        FGameplayTag InCueName,
        FCk_AbilityCue_Params InParams);

    UFUNCTION(NetMulticast, Unreliable)
    void
    Request_ExecuteAbilityCue(
        FGameplayTag InCueName,
        FCk_AbilityCue_Params InParams);

protected:
    virtual auto
    BeginPlay() -> void override;

private:
    UPROPERTY(Transient)
    TWeakObjectPtr<class UCk_AbilityCue_Subsystem_UE> _Subsystem_AbilityCue;

    UPROPERTY(Transient)
    TWeakObjectPtr<class UCk_EcsWorld_Subsystem_UE> _Subsystem_EcsWorldSubsystem;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_AbilityCueReplicator")
class CKABILITY_API UCk_AbilityCueReplicator_Subsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

    friend class ACk_AbilityCueReplicator_UE;

public:
    CK_GENERATED_BODY(UCk_AbilityCueReplicator_Subsystem_UE);

public:
    auto
    Initialize(
        FSubsystemCollectionBase& InCollection) -> void override;

    auto
    Deinitialize() -> void override;

    UFUNCTION(BlueprintCallable)
    void
    Request_ExecuteAbilityCue(
        FGameplayTag InCueName,
        const FCk_AbilityCue_Params& InRequest);

    UFUNCTION(BlueprintCallable)
    void
    Request_ExecuteAbilityCue_Local(
        FGameplayTag InCueName,
        const FCk_AbilityCue_Params& InRequest);

private:
    auto
    DoSpawnCueReplicatorActorsForPlayerController(
        APlayerController* InPlayerController) -> void;

private:
    auto
    OnLoginEvent(
        AGameModeBase* InGameModeBase,
        APlayerController* InPlayerController) -> void;

    auto
    OnPostLoadMapWithWorld(
        UWorld* InWorld) -> void;

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<ACk_AbilityCueReplicator_UE>> _AbilityCueReplicators;
    int32                                           _NextAvailableReplicator = 0;

    UPROPERTY(Transient)
    TSet<TWeakObjectPtr<APlayerController>> _ValidPlayerControllers;

private:
    FDelegateHandle _PostLoginDelegateHandle;
    FDelegateHandle _PostLoadMapWithWorldDelegateHandle;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKABILITY_API UCk_AbilityCue_Subsystem_UE : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_AbilityCue_Subsystem_UE);

public:
    using ConfigType = UCk_AbilityCue_Aggregator_PDA::ConfigType;

public:
    auto
    Initialize(
        FSubsystemCollectionBase& Collection) -> void override;

    auto
    Deinitialize() -> void override;

public:
    auto
    Request_PopulateAllAggregators() -> void;

private:
    auto
    DoOnEngineInitComplete() -> void;

    auto
    DoHandleAssetAddedDeleted(
        const FAssetData&) -> void;

    auto
    DoHandleRenamed(
        const FAssetData&,
        const FString&) -> void;

    auto
    DoAssetUpdated(
        const FAssetData&) -> void;

    auto
    DoHandleAssetAddedDeletedOrRenamed(
        const FAssetData&) -> void;

    auto
    DoAddAggregator(
        const FAssetData& InAggregatorData) -> void;

public:
    auto
    Get_AbilityCue_ConstructionScript(
        const FGameplayTag& InCueName) -> class UCk_Entity_ConstructionScript_PDA*;

private:
    UPROPERTY(Transient)
    TMap<FGameplayTag, TObjectPtr<UCk_AbilityCue_Config_PDA>> _EntityConfigs;

    UPROPERTY(Transient)
    TMap<FName, TSoftObjectPtr<class UCk_AbilityCue_Aggregator_PDA>> _Aggregators;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKABILITY_API UCk_Utils_AbilityCue_Subsystem_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    using SubsystemType = UCk_AbilityCue_Subsystem_UE;

public:
    static auto
    Request_PopulateAggregators() -> void;
};

// --------------------------------------------------------------------------------------------------------------------
