#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StreamableManager.h"
#include "CkAssetRegistrySubsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCkAssetRegistryConfig;

DECLARE_DELEGATE_OneParam(FOnAssetTypeResolved, const FString& /*AssetType*/);

USTRUCT()
struct FPendingAssetInfo
{
    GENERATED_BODY()

    FAssetData AssetData;
    FOnAssetTypeResolved OnResolvedDelegate;

    FPendingAssetInfo() = default;

    FPendingAssetInfo(const FAssetData& InAssetData, const FOnAssetTypeResolved& InDelegate)
        : AssetData(InAssetData), OnResolvedDelegate(InDelegate)
    {}
};

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
    // Asset registry callback handlers
    auto
    OnAssetAdded(
        const FAssetData& AssetData) -> void;

    auto
    OnAssetRemoved(
        const FAssetData& AssetData) -> void;

    auto
    OnAssetUpdated(
        const FAssetData& AssetData) -> void;

    // Internal generation without global reset
    auto
    GenerateAssetRegistryForConfig_Internal(
        UCkAssetRegistryConfig* InConfig) -> void;

    // Discovery and processing
    auto
    Request_DiscoverAllConfigs() -> TArray<UCkAssetRegistryConfig*>;

    auto
    Request_DiscoverAssetsInPath(
        const FString& InRootPath) -> TArray<FAssetData>;

    auto
    Get_GeneratedAssetFunction(
        const FAssetData& InAssetData) -> FString;

    // Async asset type resolution
    auto
    Get_AssetTypeFromAssetData_Async(
        const FAssetData& InAssetData,
        const FOnAssetTypeResolved& OnResolved) -> void;

    auto
    Get_AssetTypeFromAssetData_Immediate(
        const FAssetData& InAssetData) -> FString;

    auto
    OnAssetLoaded(
        const FAssetData& OriginalAssetData,
        const FOnAssetTypeResolved& OnResolved) -> void;

    auto
    Get_AssetTypeFromLoadedBlueprint(
        UBlueprint* LoadedBlueprint) -> FString;

    auto
    Get_NativeParentClass(
        UClass* InClass) -> UClass*;

    auto
    Get_AssetTypeFromClass(
        UClass* InAssetClass) -> FString;

    auto
    Get_CorrectClassNameWithPrefix(
        UClass* InClass) -> FString;

    auto
    Get_CorrectClassNameWithPrefix_String(
        const FString& InClassName,
        bool bIsActor) -> FString;

    auto
    Get_CleanAssetName(
        const FString& InAssetName) -> FString;

    // Delayed regeneration system
    auto
    Request_ScheduleRegeneration() -> void;

    auto
    ExecuteDelayedRegeneration() -> void;

    // Output directory resolution
    auto
    Get_OutputDirectoryForRootPath(
        const FString& InRootPath) -> FString;

    // Batch processing for async operations
    auto
    ProcessAssetBatch_Async(
        const TArray<FAssetData>& Assets,
        int32 BatchIndex,
        UCkAssetRegistryConfig* Config) -> void;

    auto
    OnAssetBatchProcessed(
        const TArray<FString>& AssetFunctions,
        int32 BatchIndex,
        int32 TotalBatches,
        UCkAssetRegistryConfig* Config,
        TSharedPtr<FString> AccumulatedContent) -> void;

    auto
    FinalizeAssetRegistryGeneration(
        const FString& Content,
        UCkAssetRegistryConfig* Config) -> void;

private:
    FTimerHandle RegenerationTimerHandle;

    static constexpr float RegenerationDelay = 1.0f;

    TSet<FString> UsedAssetNames;

    TMap<UClass*, FString> AssetTypeCache;

    static constexpr int32 AssetProcessingBatchSize = 100; // Smaller batches for async processing

    TSet<FString> GloballyGeneratedAssets;

    // Async loading support
    FStreamableManager StreamableManager;
    TArray<FPendingAssetInfo> PendingAssetResolutions;
    int32 PendingAssetCount = 0;
};

// --------------------------------------------------------------------------------------------------------------------
