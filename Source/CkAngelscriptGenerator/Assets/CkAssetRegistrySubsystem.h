#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StreamableManager.h"
#include "Misc/ScopedSlowTask.h"

#include "CkAssetRegistrySubsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCkAssetRegistryConfig;

DECLARE_DELEGATE_ThreeParams(FOnAssetTypeResolved, FString /*AssetType*/, bool /*IsBlueprint*/, bool /*IsEditorOnly*/);

DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnAssetRegistryProgressDelegate, int32, ProcessedAssets, int32, TotalAssets);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAssetRegistryProgress, int32, ProcessedAssets, int32, TotalAssets);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(FOnAssetRegistryCompleteDelegate, int32, GeneratedFunctions, int32, SkippedAssets, int32, TotalAssets);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAssetRegistryComplete, int32, GeneratedFunctions, int32, SkippedAssets, int32, TotalAssets);

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

public:
    UPROPERTY(BlueprintAssignable, Category = "Ck|AssetRegistry")
    FOnAssetRegistryProgress OnAssetRegistryProgress;

    UPROPERTY(BlueprintAssignable, Category = "Ck|AssetRegistry")
    FOnAssetRegistryComplete OnAssetRegistryComplete;

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

    // --- AngelScript asset reference tracking ---

    auto
    HandleAssetsPreDelete(
        const TArray<UObject*>& AssetsToDelete) -> void;

    auto
    HandleAngelscriptPostCompile() -> void;

    auto
    ScanScriptFilesForUsage() -> void;

    auto
    ScanSingleScriptFile(
        const FString& FilePath) const -> TSet<FString>;

    static auto
    Get_ScriptDirectory() -> FString;

    auto
    SeedMapsFromGeneratedFiles() -> void;

    auto
    GenerateAssetRegistryForConfig_Internal(
        UCkAssetRegistryConfig* InConfig) -> void;

    static auto
    Request_DiscoverAllConfigs() -> TArray<UCkAssetRegistryConfig*>;

    static auto
    Request_DiscoverAssetsInPath(
        const FString& InRootPath) -> TArray<FAssetData>;

    auto
    Get_AssetTypeFromAssetData(
        const FAssetData& InAssetData,
        const FOnAssetTypeResolved& OnResolved) -> void;

    static auto
    IsEditorOnlyClass(
        const UClass* InClass) -> bool;

    static auto
    Get_NonBlueprintParentClass(
        UClass* InClass) -> UClass*;

    static auto
    Get_AssetTypeFromClass(
        UClass* InAssetClass) -> FString;

    static auto
    Get_CorrectClassNameWithPrefix(
        UClass* InClass) -> FString;

    static auto
    Get_CorrectClassNameWithPrefix_String(
        const FString& InClassName,
        bool IsActor) -> FString;

    static auto
    Get_CleanAssetName(
        const FString& InAssetName) -> FString;

    auto
    Request_ScheduleRegeneration() -> void;

    auto
    ExecuteDelayedRegeneration() -> void;

    auto
    Request_ProcessNextInQueue() -> void;

    static auto
    Get_OutputDirectoryForRootPath(
        const FString& InRootPath) -> FString;

    static auto
    BuildFileHeader(
        UCkAssetRegistryConfig* InConfig,
        const FString& InRootPath) -> FString;

// --------------------------------------------------------------------------------------------------------------------

private:
    // --- AngelScript asset reference tracking ---

    // Map 1: Asset path → generated function name (populated during registry generation)
    TMap<FString, FString> AssetPathToFunctionName;

    // Map 2: Function name → script files that reference it (populated by scanning .as files)
    TMap<FString, TArray<FString>> FunctionUsageMap;

    // Delegate handles for cleanup
    FDelegateHandle PreDeleteDelegateHandle;
    FDelegateHandle PostCompileDelegateHandle;

    // Cached namespaces from all active configs
    TSet<FString> ActiveNamespaces;

    // --- End AngelScript asset reference tracking ---

    static constexpr float REGENERATION_DELAY_SECONDS = 1.0f;
    static constexpr float ASSET_LOAD_TIMEOUT_SECONDS = 30.0f;
    static constexpr TCHAR BLUEPRINT_CLASS_NAME[] = TEXT("Blueprint");
    static constexpr TCHAR PARENT_CLASS_TAG[] = TEXT("ParentClass");
    static constexpr TCHAR OBJECT_REDIRECTOR_CLASS[] = TEXT("ObjectRedirector");
    static constexpr TCHAR USER_DEFINED_STRUCT_CLASS[] = TEXT("UserDefinedStruct");

    FTimerHandle RegenerationTimerHandle;
    TSet<FString> UsedAssetNames;
    TMap<UClass*, FString> AssetTypeCache;
    TSet<FString> GloballyGeneratedAssets;
    FStreamableManager StreamableManager;
    TSharedPtr<FScopedSlowTask> ActiveSlowTask;
    bool IsGenerationInProgress = false;

    UPROPERTY(Transient)
    TArray<UCkAssetRegistryConfig*> PendingGenerationQueue;
};

// --------------------------------------------------------------------------------------------------------------------
