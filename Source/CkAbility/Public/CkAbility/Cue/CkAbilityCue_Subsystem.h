#pragma once

#include "CkAbility/Cue/CkAbilityCue_Fragment_Data.h"

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

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
        FGameplayCueParameters InParams);

    UFUNCTION(NetMulticast, Reliable)
    void
    Request_ExecuteAbilityCue(
        FGameplayCueParameters InParams);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKABILITY_API UCk_AbilityCueReplicator_Subsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_AbilityCueReplicator_Subsystem_UE);

public:
    auto
    Initialize(
        FSubsystemCollectionBase& Collection) -> void override;

    UFUNCTION(BlueprintCallable)
    void
    Request_ExecuteAbilityCue(
        const FGameplayCueParameters& InRequest);

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<ACk_AbilityCueReplicator_UE>> _AbilityCueReplicators;
    int32 _NextAvailableReplicator = 0;

    // TODO: drive this through a tuner
    static constexpr int32 NumberOfReplicators = 5;
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
    DoHandleAssetAddedDeletedOrRenamed(
        const FAssetData&) -> void;

    auto
    DoPopulateAllAggregators() -> void;

    auto
    DoAddAggregator(
        const FAssetData& InAggregatorData) -> void;

public:
    auto
    Get_AbilityCue(
        const FGameplayCueParameters& InParams) -> UCk_AbilityCue_Config_PDA*;

private:
    UPROPERTY(Transient)
    TMap<FGameplayTag, FSoftObjectPath> _AbilityCues;

    UPROPERTY(Transient)
    TMap<FName, TSoftObjectPtr<class UCk_AbilityCue_Aggregator_PDA>> _Aggregators;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKABILITY_API UCk_Utils_AbilityCue_Subsystem_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    using SubsystemType = UCk_Utils_AbilityCue_Subsystem_UE;

public:
    static auto
    Get_Subsystem(const UWorld* InWorld) -> SubsystemType*;
};

// --------------------------------------------------------------------------------------------------------------------
