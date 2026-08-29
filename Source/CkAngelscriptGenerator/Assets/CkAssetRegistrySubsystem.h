#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StreamableManager.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Containers/Ticker.h"

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

public:
    // --- Public because the SelfHeal stub synthesizer MUST resolve through the same logic this
    // generator does. None touch instance state. ---

    static auto
    Request_DiscoverAllConfigs() -> TArray<UCkAssetRegistryConfig*>;

    static auto
    Request_DiscoverAssetsInPath(
        const FString& InRootPath) -> TArray<FAssetData>;

    static auto
    Get_NonBlueprintParentClass(
        UClass* InClass) -> UClass*;

    static auto
    Get_CorrectClassNameWithPrefix(
        UClass* InClass) -> FString;

    static auto
    Get_CleanAssetName(
        const FString& InAssetName) -> FString;

    static auto
    Get_OutputDirectoryForRootPath(
        const FString& InRootPath) -> FString;

public:
    /** The id this subsystem registers under with `FCk_AssetReferenceProviderRegistry`, and the word a consumer
     *  prints beside the count. A function rather than a `static const FName`: an `FName` built during static
     *  initialization runs before anything guarantees the name table is up. */
    static auto
    Get_ScriptReferenceProviderId() -> FName;

    /**
     * The `.as` files that reference `InAsset` through its generated accessor, or an empty array when none do.
     *
     * This is the ONLY way to learn that an asset is reachable from AngelScript. The reference is a text call to a
     * generated `assets::` accessor resolved at runtime, so it creates no package dependency edge and
     * `IAssetRegistry::GetReferencers` cannot see it — which is exactly why an auditor asking the graph alone will
     * report a script-critical asset as unreferenced and offer it for deletion.
     *
     * Backed by the same two maps `HandleAssetsPreDelete` warns from, so the dialog and any tool consulting this
     * agree by construction rather than by two copies of one rule. Sorted, because a caller printing the list must
     * read the same on two machines and `FunctionUsageMap`'s storage order is a scan-order detail.
     *
     * Registered with `FCk_AssetReferenceProviderRegistry` under `"AngelScript"` at `Initialize`, which is how
     * consumers reach it without linking this editor-only module.
     */
    auto
    Get_ScriptReferencersOfAsset(
        const FSoftObjectPath& InAsset) const -> TArray<FString>;

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
        const FString& FilePath,
        const TSet<FString>& InGeneratedFunctionNames) const -> TSet<FString>;

    auto
    SeedMapsFromGeneratedFiles() -> void;

    auto
    GenerateAssetRegistryForConfig_Internal(
        UCkAssetRegistryConfig* InConfig) -> void;

    auto
    Get_AssetTypeFromAssetData(
        const FAssetData& InAssetData,
        const FOnAssetTypeResolved& OnResolved) -> void;

    static auto
    Get_AssetTypeFromClass(
        UClass* InAssetClass) -> FString;

    static auto
    Get_CorrectClassNameWithPrefix_String(
        const FString& InClassName,
        bool IsActor) -> FString;

    auto
    Request_ScheduleRegeneration() -> void;

    auto
    ExecuteDelayedRegeneration() -> void;

    auto
    Request_ProcessNextInQueue() -> void;

    static auto
    BuildFileHeader(
        UCkAssetRegistryConfig* InConfig,
        const FString& InRootPath) -> FString;

    // Both Dismiss_* are safe to call when nothing is active.
    auto
    Dismiss_ProgressNotification() -> void;

    auto
    Dismiss_GenerationTicker() -> void;

// --------------------------------------------------------------------------------------------------------------------

private:
    // --- AngelScript asset reference tracking ---

    TMap<FString, FString> AssetPathToFunctionName;

    // Generated function name -> the .as files referencing it.
    TMap<FString, TArray<FString>> FunctionUsageMap;

    FDelegateHandle PreDeleteDelegateHandle;
    FDelegateHandle PostCompileDelegateHandle;

    TSet<FString> ActiveNamespaces;

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
    FProgressNotificationHandle ActiveProgressNotification;
    FTSTicker::FDelegateHandle GenerationTickerHandle;
    bool IsGenerationInProgress = false;

    UPROPERTY(Transient)
    TArray<UCkAssetRegistryConfig*> PendingGenerationQueue;
};

// --------------------------------------------------------------------------------------------------------------------
