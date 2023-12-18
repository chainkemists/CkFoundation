#pragma once

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkAbilityCue_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKABILITY_API UCk_AbilityCue_Subsystem_UE : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_AbilityCue_Subsystem_UE);

public:
    auto Initialize(FSubsystemCollectionBase& Collection) -> void override;
    auto Deinitialize() -> void override;

private:
    auto DoOnEngineInitComplete() -> void;
	void DoHandleAssetAddedDeleted(const FAssetData&);
	void DoHandleRenamed(const FAssetData&, const FString&);

    void DoPopulateAllAggregators();
    void DoAddAggregator(const FAssetData& InAggregatorData);

private:
    UPROPERTY(Transient)
    TMap<FName, TSoftObjectPtr<class UCk_AbilityCue_Aggregator_DA>> _Aggregators;
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
