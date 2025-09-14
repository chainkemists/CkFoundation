#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "CkAssetRegistrySubsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCkAssetRegistryConfig;

UCLASS(BlueprintType)
class CKANGELSCRIPTGENERATOR_API UCkAssetRegistrySubsystem : public UEditorSubsystem
{
    GENERATED_BODY()

public:
    virtual auto
    Initialize(
        FSubsystemCollectionBase& Collection) -> void override;

    virtual auto
    Deinitialize() -> void override;

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|AssetRegistry")
    void
    GenerateAllAssetRegistries();

    UFUNCTION(BlueprintCallable, Category = "Ck|AssetRegistry")
    void
    GenerateAssetRegistryForConfig(
        UCkAssetRegistryConfig* InConfig);

private:
    auto
    OnAssetAdded(
        const FAssetData& AssetData) -> void;

    auto
    OnAssetRemoved(
        const FAssetData& AssetData) -> void;

    auto
    OnAssetUpdated(
        const FAssetData& AssetData) -> void;

    auto
    GenerateAssetRegistryForConfig_Internal(
        UCkAssetRegistryConfig* InConfig) -> void;

    static auto
    Request_DiscoverAllConfigs() -> TArray<UCkAssetRegistryConfig*>;

    static auto
    Request_DiscoverAssetsInPath(
        const FString& InRootPath) -> TArray<FAssetData>;

    auto
    Get_GeneratedAssetFunction(
        const FAssetData& InAssetData) -> FString;

    auto
    Get_AssetTypeFromClass(
        UClass* InAssetClass) -> FString;

    static auto
    Get_CleanAssetName(
        const FString& InAssetName) -> FString;

    auto
    Request_ScheduleRegeneration() -> void;

    auto
    ExecuteDelayedRegeneration() -> void;

    static auto
    Get_OutputDirectoryForRootPath(
        const FString& InRootPath) -> FString;

private:
    static constexpr float RegenerationDelay = 1.0f;
    static constexpr int32 AssetProcessingBatchSize = 1000;

    FTimerHandle RegenerationTimerHandle;
    TSet<FString> UsedAssetNames;
    TMap<UClass*, FString> AssetTypeCache;
    TSet<FString> GloballyGeneratedAssets;
};

// --------------------------------------------------------------------------------------------------------------------
