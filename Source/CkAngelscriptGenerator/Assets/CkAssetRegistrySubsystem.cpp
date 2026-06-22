#include "CkAssetRegistrySubsystem.h"

#include "CkAssetRegistryConfig.h"
#include "CkAngelscriptGenerator/Assets/CkAssetRegistry_ClassResolver.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_RegenOwnership.h"

#include "CkCore/IO/CkIO_Utils.h"
#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Reflection/CkReflection_Utils.h"

#include <AssetRegistry/AssetRegistryModule.h>
#include <Containers/Ticker.h>
#include <Editor.h>
#include <Engine/Blueprint.h>
#include <Engine/Engine.h>
#include <GameFramework/Actor.h>
#include <HAL/FileManager.h>
#include <Interfaces/IPluginManager.h>
#include <Kismet2/BlueprintEditorUtils.h>
#include <Misc/FileHelper.h>
#include <Misc/MessageDialog.h>
#include <Misc/Paths.h>
#include <TimerManager.h>

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptCodeModule.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Get_AssetTypeFromAssetData(
        const FAssetData& InAssetData,
        const FOnAssetTypeResolved& OnResolved)
    -> void
{
    ck::angelscriptgenerator::Log(TEXT("Get_AssetTypeFromAssetData called for: {}"), InAssetData.AssetName);

    auto AssetPath = InAssetData.GetSoftObjectPath();
    auto AssetName = InAssetData.AssetName.ToString();

    // Sync resolve — runs BEFORE the async-load so the result lands before the
    // regen-completion event fires. Without this, slow async callbacks return
    // AFTER the batch is declared complete, dropping the accessor from the
    // output and triggering the self-heal synth/cleanup loop.
    //
    // BP-gating is load-bearing: an AnimMontage containing embedded
    // AnimNotifyState `_C` exports would fool the linker walk into returning
    // the embedded BP's class instead of the asset's. AssetData::GetClass() is
    // authoritative for non-BPs and doesn't need the walk.
    if (const auto AssetClass = InAssetData.GetClass(); ck::IsValid(AssetClass))
    {
        const auto IsBlueprintAsset = AssetClass->IsChildOf<UBlueprint>();

        if (IsBlueprintAsset)
        {
            const auto SyncResolved = ck::angelscriptgenerator::FCkAssetRegistry_ClassResolver
                ::Resolve_ViaPackageReader(AssetPath.ToString());

            if (NOT SyncResolved.ClassName.IsEmpty())
            {
                // ResolvedClass is null when the name was derived without a
                // live UClass (AS reload window), but those classes are
                // runtime by definition (UCk_* AS classes aren't editor-only),
                // so the false-default is the correct answer in that failure
                // mode. Native editor-utility parents (UEditorUtilityWidget,
                // etc.) always carry a live UClass since they're loaded at
                // engine startup.
                const auto IsEditorOnly = ck::IsValid(SyncResolved.ResolvedClass)
                    && UCk_Utils_Reflection_UE::Is_EditorOnlyClass(SyncResolved.ResolvedClass);

                ck::angelscriptgenerator::Log(TEXT("Sync-resolved BP class via FPackageReader linker: {} for {} (IsEditorOnly: {})"),
                    SyncResolved.ClassName, AssetName, IsEditorOnly);

                constexpr auto IsBlueprintLike = true;
                OnResolved.ExecuteIfBound(SyncResolved.ClassName, IsBlueprintLike, IsEditorOnly);
                return;
            }

            // ViaPackageReader couldn't resolve the parent. A Blueprint whose parent
            // class was deleted/renamed must NOT fall through to the async load below:
            // regenerating its BPGC requires the parent class, so the package load is
            // cancelled and its streamable completion delegate never fires — *PendingAssets
            // never drains, WriteCanonicalAndAdvance never runs, and the whole batch stalls
            // (IsGenerationInProgress stuck true, completion delegate never broadcast, and
            // the progress notification hangs sub-100% forever). Resolve the parent straight
            // from the asset-registry tags (no asset load) and SKIP the BP if it has none —
            // either way the BP branch always fires the callback and returns.
            auto BlueprintParentClass = UBlueprint::GetBlueprintParentClassFromAssetTags(InAssetData);

            if (ck::Is_NOT_Valid(BlueprintParentClass))
            {
                ck::angelscriptgenerator::Warning(
                    TEXT("Skipping Blueprint with no resolvable parent class (deleted/renamed parent?): {}"), AssetName);
                OnResolved.ExecuteIfBound(FString{}, false, false);
                return;
            }

            const auto NativeParentClass = Get_NonBlueprintParentClass(BlueprintParentClass);

            if (ck::Is_NOT_Valid(NativeParentClass))
            {
                ck::angelscriptgenerator::Warning(
                    TEXT("Skipping Blueprint whose parent chain has no native/script ancestor: {}"), AssetName);
                OnResolved.ExecuteIfBound(FString{}, false, false);
                return;
            }

            const auto ParentIsEditorOnly = UCk_Utils_Reflection_UE::Is_EditorOnlyClass(NativeParentClass);
            const auto ParentClassName = Get_CorrectClassNameWithPrefix(NativeParentClass);

            ck::angelscriptgenerator::Log(TEXT("Sync-resolved BP parent via AssetData tags: {} for {} (IsEditorOnly: {})"),
                ParentClassName, AssetName, ParentIsEditorOnly);

            constexpr auto IsBlueprintLike = true;
            OnResolved.ExecuteIfBound(ParentClassName, IsBlueprintLike, ParentIsEditorOnly);
            return;
        }
        else
        {
            // Non-BP path. Loses LoadedAsset->IsEditorOnly() (instance-level
            // UObject flag) vs the async path; preserves the class-level
            // Is_EditorOnlyClass check, which is the load-bearing branch on
            // game assets.
            if (const auto NativeParentClass = Get_NonBlueprintParentClass(AssetClass);
                ck::IsValid(NativeParentClass))
            {
                const auto IsEditorOnly = UCk_Utils_Reflection_UE::Is_EditorOnlyClass(NativeParentClass);
                const auto Result = Get_CorrectClassNameWithPrefix(NativeParentClass);

                ck::angelscriptgenerator::Log(TEXT("Sync-resolved non-BP class via AssetData: {} for {} (IsEditorOnly: {})"),
                    Result, AssetName, IsEditorOnly);

                constexpr auto IsBlueprintLike = false;
                OnResolved.ExecuteIfBound(Result, IsBlueprintLike, IsEditorOnly);
                return;
            }
        }
    }

    ck::angelscriptgenerator::Log(TEXT("Loading asset asynchronously: {}"), AssetName);

    const auto LoadHandle = StreamableManager.RequestAsyncLoad(
        AssetPath,
        FStreamableDelegate::CreateLambda([this, AssetName, OnResolved, AssetPath]()
        {
            auto LoadedAsset = AssetPath.ResolveObject();

            if (ck::Is_NOT_Valid(LoadedAsset))
            {
                ck::angelscriptgenerator::Warning(TEXT("Failed to load asset: {}"), AssetName);

                auto MessageSegments = FCk_MessageSegments{
                    {FCk_TokenizedMessage{ck::Format_UE(TEXT("Failed to load asset: {}"), AssetName)}}
                };
                auto Params = FCk_Utils_EditorOnly_PushNewEditorMessage_Params{
                    TEXT("AssetRegistry"), MessageSegments
                };
                Params.Set_MessageSeverity(ECk_EditorMessage_Severity::Warning);
                UCk_Utils_EditorOnly_UE::Request_PushNewEditorMessage(Params);

                OnResolved.ExecuteIfBound(FString{}, false, false);
                return;
            }

            auto IsEditorOnly = LoadedAsset->IsEditorOnly();

            if (auto LoadedBlueprint = Cast<UBlueprint>(LoadedAsset))
            {
                auto ParentClass = LoadedBlueprint->ParentClass;
                if (ck::Is_NOT_Valid(ParentClass))
                {
                    ck::angelscriptgenerator::Warning(TEXT("Blueprint has no parent class: {}"), AssetName);
                    OnResolved.ExecuteIfBound(FString{}, false, IsEditorOnly);
                    return;
                }

                if (auto NativeParentClass = Get_NonBlueprintParentClass(ParentClass);
                    ck::IsValid(NativeParentClass))
                {
                    // Check if the class is from an editor-only module
                    if (UCk_Utils_Reflection_UE::Is_EditorOnlyClass(NativeParentClass))
                    { IsEditorOnly = true; }

                    auto Result = Get_CorrectClassNameWithPrefix(NativeParentClass);
                    ck::angelscriptgenerator::Log(TEXT("Resolved Blueprint parent class: {} for {}"), Result, AssetName);
                    OnResolved.ExecuteIfBound(Result, true, IsEditorOnly); // true = is Blueprint
                }
                else
                {
                    ck::angelscriptgenerator::Warning(TEXT("Could not find native parent class for: {}"), AssetName);
                    OnResolved.ExecuteIfBound(FString{}, false, IsEditorOnly);
                }
            }
            else
            {
                // For non-Blueprint assets (DataAssets, curves, etc.), use the loaded object's class
                if (const auto AssetClass = LoadedAsset->GetClass();
                    ck::IsValid(AssetClass))
                {
                    // Also traverse inheritance for non-Blueprint assets in case they inherit from Blueprint classes
                    if (const auto NativeParentClass = Get_NonBlueprintParentClass(AssetClass);
                        ck::IsValid(NativeParentClass))
                    {
                        // Check if the class is from an editor-only module
                        if (UCk_Utils_Reflection_UE::Is_EditorOnlyClass(NativeParentClass))
                        { IsEditorOnly = true; }

                        auto Result = Get_CorrectClassNameWithPrefix(NativeParentClass);
                        ck::angelscriptgenerator::Log(TEXT("Resolved asset native parent class: {} for {}"), Result, AssetName);
                        OnResolved.ExecuteIfBound(Result, false, IsEditorOnly); // false = not Blueprint
                    }
                    else
                    {
                        ck::angelscriptgenerator::Warning(TEXT("Could not find native parent class for asset: {}"), AssetName);
                        OnResolved.ExecuteIfBound(FString{}, false, IsEditorOnly);
                    }
                }
                else
                {
                    ck::angelscriptgenerator::Warning(TEXT("Could not get class for loaded asset: {}"), AssetName);
                    OnResolved.ExecuteIfBound(FString{}, false, IsEditorOnly);
                }
            }
        })
    );

    if (NOT LoadHandle.IsValid())
    {
        ck::angelscriptgenerator::Warning(TEXT("Failed to start async load for asset: {}"), AssetName);
        OnResolved.ExecuteIfBound(FString{}, false, false);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Get_NonBlueprintParentClass(
        UClass* InClass)
    -> UClass*
{
    if (ck::Is_NOT_Valid(InClass))
    { return nullptr; }

    auto CurrentClass = InClass;

    while (ck::IsValid(CurrentClass))
    {
#if WITH_ANGELSCRIPT_CK
        const auto IsBlueprint = CurrentClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint) && NOT CurrentClass->bIsScriptClass;
#else
        const auto IsBlueprint = CurrentClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint);
#endif

        ck::angelscriptgenerator::Log(TEXT("Checking class: {} (HasAnyClassFlags(CLASS_CompiledFromBlueprint): {})"),
            CurrentClass->GetName(), CurrentClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint));

        // If this class is not compiled from Blueprint, it's native
        if (NOT IsBlueprint)
        {
            ck::angelscriptgenerator::Log(TEXT("Found non blueprint class: {}"), CurrentClass->GetName());
            return CurrentClass;
        }

        // Move up to the parent class - this will traverse Blueprint inheritance chains
        CurrentClass = CurrentClass->GetSuperClass();
    }

    ck::angelscriptgenerator::Warning(TEXT("Could not find non blueprint class - reached top of hierarchy"));
    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);

    ck::angelscriptgenerator::Log(TEXT("Initialized Asset Registry Subsystem"));

    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    auto& AssetRegistry = AssetRegistryModule.Get();

    AssetRegistry.OnAssetAdded().AddUObject(this, &UCkAssetRegistrySubsystem::OnAssetAdded);
    AssetRegistry.OnAssetRemoved().AddUObject(this, &UCkAssetRegistrySubsystem::OnAssetRemoved);
    AssetRegistry.OnAssetUpdated().AddUObject(this, &UCkAssetRegistrySubsystem::OnAssetUpdated);

    // Pre-delete warning for assets referenced in AngelScript
    PreDeleteDelegateHandle = FEditorDelegates::OnAssetsPreDelete.AddUObject(
        this, &UCkAssetRegistrySubsystem::HandleAssetsPreDelete);

    // Rebuild usage map after every AS compilation (initial + hot-reload)
#if WITH_ANGELSCRIPT_CK
    PostCompileDelegateHandle = FAngelscriptCodeModule::GetPostCompile().AddUObject(
        this, &UCkAssetRegistrySubsystem::HandleAngelscriptPostCompile);
#endif

    // Seed Map 1 from existing generated files and run initial usage scan
    SeedMapsFromGeneratedFiles();
    ScanScriptFilesForUsage();

    ck::angelscriptgenerator::Log(TEXT("Asset registry callbacks registered for real-time config discovery"));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Deinitialize()
    -> void
{
    Dismiss_GenerationTicker();
    Dismiss_ProgressNotification();
    IsGenerationInProgress = false;
    PendingGenerationQueue.Empty();

    FEditorDelegates::OnAssetsPreDelete.Remove(PreDeleteDelegateHandle);

#if WITH_ANGELSCRIPT_CK
    if (FModuleManager::Get().IsModuleLoaded("AngelscriptCode"))
    {
        FAngelscriptCodeModule::GetPostCompile().Remove(PostCompileDelegateHandle);
    }
#endif

    if (FModuleManager::Get().IsModuleLoaded("AssetRegistry"))
    {
        const auto& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
        auto& AssetRegistry = AssetRegistryModule.Get();

        AssetRegistry.OnAssetAdded().RemoveAll(this);
        AssetRegistry.OnAssetRemoved().RemoveAll(this);
        AssetRegistry.OnAssetUpdated().RemoveAll(this);
    }

    if (ck::IsValid(GEditor))
    { GEditor->GetTimerManager()->ClearTimer(RegenerationTimerHandle); }

    Super::Deinitialize();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Dismiss_ProgressNotification()
    -> void
{
    if (NOT ActiveProgressNotification.IsValid())
    { return; }

    FSlateNotificationManager::Get().CancelProgressNotification(ActiveProgressNotification);
    ActiveProgressNotification.Reset();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Dismiss_GenerationTicker()
    -> void
{
    if (NOT GenerationTickerHandle.IsValid())
    { return; }

    FTSTicker::GetCoreTicker().RemoveTicker(GenerationTickerHandle);
    GenerationTickerHandle.Reset();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    GenerateAllAssetRegistries()
    -> void
{
    // In-flight guard: generation now spans multiple frames (per-frame ticker), so a
    // re-trigger landing mid-run (config-asset change → 1s timer → ExecuteDelayedRegeneration,
    // or a PostCompile/PostInit ticker) must NOT reset the shared dedup/name state under the
    // active batch — that would corrupt its output and can kick a self-heal regen cycle.
    // Reschedule instead; the timer retries once the current run drains.
    if (IsGenerationInProgress)
    {
        ck::angelscriptgenerator::Warning(
            TEXT("GenerateAllAssetRegistries requested while a generation is in progress — rescheduling."));
        Request_ScheduleRegeneration();
        return;
    }

    GloballyGeneratedAssets.Reset();

    ck::angelscriptgenerator::Log(TEXT("=== Generating All Asset Registries ==="));

    auto AllConfigs = Request_DiscoverAllConfigs();

    if (AllConfigs.IsEmpty())
    {
        ck::angelscriptgenerator::Warning(TEXT("No Asset Registry config assets found"));
        return;
    }

    ck::angelscriptgenerator::Log(TEXT("Found {} Asset Registry config assets"), AllConfigs.Num());

    auto DispatchedCount = int32{0};
    auto InvalidConfigCount = int32{0};

    for (auto Config : AllConfigs)
    {
        CK_ENSURE_IF_NOT(ck::IsValid(Config),
            TEXT("Invalid Asset Registry config found"))
        {
            InvalidConfigCount++;
            continue;
        }

        ck::angelscriptgenerator::Log(TEXT("Generating for config: {}"), Config->GetDisplayName());

        GenerateAssetRegistryForConfig_Internal(Config);
        DispatchedCount++;
    }

    // _Internal generates synchronously OR queues when a generation is already
    // in flight — per-config success is reported by OnAssetRegistryComplete,
    // so only dispatch counts are knowable here.
    ck::angelscriptgenerator::Log(TEXT("Asset Registry generation dispatched: {} configs ({} invalid skipped)"),
                                 DispatchedCount, InvalidConfigCount);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    GenerateAssetRegistryForConfig(
        UCkAssetRegistryConfig* InConfig)
    -> void
{
    // In-flight guard (see GenerateAllAssetRegistries): never reset shared dedup/name state
    // under an active multi-frame run. Reschedule and let the timer retry once it drains.
    if (IsGenerationInProgress)
    {
        ck::angelscriptgenerator::Warning(
            TEXT("GenerateAssetRegistryForConfig requested while a generation is in progress — rescheduling."));
        Request_ScheduleRegeneration();
        return;
    }

    GloballyGeneratedAssets.Reset();
    GenerateAssetRegistryForConfig_Internal(InConfig);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    GenerateAssetRegistryForConfig_Internal(
        UCkAssetRegistryConfig* InConfig)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InConfig),
        TEXT("Cannot generate asset registry for invalid config"))
    { return; }

    // Single-writer gate (G8): the single choke point for every *Assets.as canonical write
    // (editor buttons, PostCompile/PostInit AR tickers, asset-change regen queue). A secondary
    // must write nothing — clear the queue and broadcast a "did nothing" completion so listeners
    // aren't left hanging, and never flip IsGenerationInProgress (which a deferred ticker reads).
    if (NOT FCkAngelscriptGenerator_RegenOwnership::Try_AcquireOrGet_IsOwner(
            TEXT("AssetRegistrySubsystem.GenerateAssetRegistryForConfig")))
    {
        ck::angelscriptgenerator::Warning(
            TEXT("[AssetRegistry] Skipped generation for [{}] — this editor instance is a SECONDARY ")
            TEXT("(another instance owns Script/Generated regen)."), InConfig->GetDisplayName());
        PendingGenerationQueue.Empty();
        OnAssetRegistryComplete.Broadcast(0, 0, 0);
        return;
    }

    // If generation is in progress, queue this request
    if (IsGenerationInProgress)
    {
        // Check if this config is already in the queue to avoid duplicates
        if (NOT PendingGenerationQueue.Contains(InConfig))
        {
            PendingGenerationQueue.Add(InConfig);
            ck::angelscriptgenerator::Log(TEXT("Asset registry generation in progress, queued: {} (Queue size: {})"),
                                         InConfig->GetDisplayName(), PendingGenerationQueue.Num());
        }
        else
        {
            ck::angelscriptgenerator::Log(TEXT("Config already in queue, skipping: {}"), InConfig->GetDisplayName());
        }
        return;
    }

    IsGenerationInProgress = true;

    auto RootPath = InConfig->AssetDiscoveryRoot;
    auto OutputFileName = InConfig->OutputFileName;

    ck::angelscriptgenerator::Log(TEXT("=== Generating Asset Registry for Config: {} ==="), InConfig->GetDisplayName());

    CK_ENSURE_IF_NOT(NOT RootPath.IsEmpty(),
        TEXT("Asset discovery root path is empty for config [{}]"), InConfig->GetDisplayName())
    {
        IsGenerationInProgress = false;
        Request_ProcessNextInQueue();
        return;
    }

    auto DiscoveredAssets = Request_DiscoverAssetsInPath(RootPath);

    if (DiscoveredAssets.IsEmpty())
    {
        ck::angelscriptgenerator::Warning(TEXT("No assets found under path: {}"), RootPath);
        IsGenerationInProgress = false;
        OnAssetRegistryComplete.Broadcast(0, 0, 0);
        Request_ProcessNextInQueue();
        return;
    }

    UsedAssetNames.Reset();
    AssetPathToFunctionName.Reset();
    ActiveNamespaces.Add(InConfig->Namespace);

    DiscoveredAssets.Sort([](const FAssetData &A, const FAssetData &B) {
        return A.GetSoftObjectPath().ToString() < B.GetSoftObjectPath().ToString();
    });

    auto TotalAssets = DiscoveredAssets.Num();
    ck::angelscriptgenerator::Log(TEXT("Processing {} assets with async loading"), TotalAssets);

    // Non-blocking status-bar progress notification (bottom-right), the same surface
    // the engine uses for shader/static-mesh compilation. Unlike FScopedSlowTask::MakeDialog
    // this does NOT block editor interaction. Headless/commandlet boots have no status-bar
    // handler registered, so these calls are silent no-ops there. Auto-dismisses once
    // work-done reaches total (the last async callback drives it there); the completion
    // sites then Cancel to remove the entry from the status bar's tracking array.
    ActiveProgressNotification = FSlateNotificationManager::Get().StartProgressNotification(
        FText::FromString(ck::Format_UE(TEXT("Generating Asset Registry: {}"), InConfig->OutputFileName)),
        TotalAssets);

    auto Content = BuildFileHeader(InConfig, RootPath);
    auto GeneratedFunctionCount = MakeShared<int32>(0);
    auto SkippedAssetCount = MakeShared<int32>(0);
    auto ProcessedAssetCount = MakeShared<int32>(0);
    auto PendingAssets = MakeShared<int32>(0);
    auto CollectedFunctions = MakeShared<TArray<FString>>();
    auto CollectedLoadFunctions = MakeShared<TArray<FString>>();
    auto DispatchComplete = MakeShared<bool>(false);

    CollectedFunctions->Reserve(TotalAssets);
    CollectedLoadFunctions->Reserve(TotalAssets);

    // Single write per config. Fires from the last async callback to drain, OR
    // directly post-loop when every asset resolved sync (Race A path).
    auto WriteCanonicalAndAdvance = [this, Content, CollectedFunctions, CollectedLoadFunctions,
                                     GeneratedFunctionCount, SkippedAssetCount, InConfig, TotalAssets]()
    {
        auto FinalContent = Content;

        CollectedFunctions->Sort();
        for (const auto& Function : *CollectedFunctions)
        {
            if (NOT Function.IsEmpty())
            {
                FinalContent += Function;
            }
        }

        FinalContent += TEXT("}\n\n");

        FinalContent += TEXT("// Blocking loads - loads asset immediately\n");
        FinalContent += ck::Format_UE(TEXT("namespace {}::load\n{{\n"), InConfig->Namespace);

        CollectedLoadFunctions->Sort();
        for (const auto& LoadFunction : *CollectedLoadFunctions)
        {
            if (NOT LoadFunction.IsEmpty())
            {
                FinalContent += LoadFunction;
            }
        }

        FinalContent += TEXT("}\n");

        auto OutputDir = Get_OutputDirectoryForRootPath(InConfig->AssetDiscoveryRoot);
        auto OutputPath = OutputDir / InConfig->OutputFileName;

        // Refuse to clobber a populated canonical with a 0-accessor regen --
        // AR hasn't finished cataloging this mount yet; next pass will retry.
        if (*GeneratedFunctionCount == 0 && IFileManager::Get().FileExists(*OutputPath))
        {
            auto ExistingContent = FString{};
            if (FFileHelper::LoadFileToString(ExistingContent, *OutputPath))
            {
                const auto AccessorToken = FString{TEXT("TSoftObjectPtr<")};
                auto PriorAccessorCount = int32{0};
                auto SearchStart = int32{0};
                while (true)
                {
                    const auto Found = ExistingContent.Find(AccessorToken, ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchStart);
                    if (Found == INDEX_NONE)
                    { break; }
                    PriorAccessorCount++;
                    SearchStart = Found + AccessorToken.Len();
                }

                if (PriorAccessorCount > 0)
                {
                    ck::angelscriptgenerator::Warning(
                        TEXT("Refusing to overwrite [{}] - regen produced 0 accessors but existing file has {} accessor(s). ")
                        TEXT("Likely cause: AssetRegistry has not finished cataloging this mount yet. ")
                        TEXT("Skipping write; next regen pass will retry once catalog is ready."),
                        InConfig->OutputFileName, PriorAccessorCount);

                    Dismiss_ProgressNotification();
                    IsGenerationInProgress = false;
                    ScanScriptFilesForUsage();
                    OnAssetRegistryComplete.Broadcast(*GeneratedFunctionCount, *SkippedAssetCount, TotalAssets);
                    Request_ProcessNextInQueue();
                    return;
                }
            }
        }

        IFileManager::Get().MakeDirectory(*OutputDir, true);

        if (FFileHelper::SaveStringToFile(FinalContent, *OutputPath))
        {
            ck::angelscriptgenerator::Log(TEXT("Generated: {} with {} functions ({} assets skipped)"),
                                         InConfig->OutputFileName, *GeneratedFunctionCount, *SkippedAssetCount);
        }
        else
        {
            ck::angelscriptgenerator::Warning(TEXT("Failed to write file: {}"), OutputPath);
        }

        Dismiss_ProgressNotification();
        IsGenerationInProgress = false;
        ScanScriptFilesForUsage();
        OnAssetRegistryComplete.Broadcast(*GeneratedFunctionCount, *SkippedAssetCount, TotalAssets);
        Request_ProcessNextInQueue();
    };

    // Per-asset dispatch (the body is unchanged — hoisted into a reusable lambda so the
    // per-frame ticker below can call it on a slice of assets at a time).
    auto DispatchOneAsset = [this, PendingAssets, CollectedFunctions, CollectedLoadFunctions,
                             GeneratedFunctionCount, SkippedAssetCount, ProcessedAssetCount,
                             InConfig, TotalAssets, DispatchComplete, WriteCanonicalAndAdvance]
        (const FAssetData& AssetData) -> void
    {
        (*PendingAssets)++;

        Get_AssetTypeFromAssetData(AssetData, FOnAssetTypeResolved::CreateLambda(
            [this, AssetData, PendingAssets, CollectedFunctions, CollectedLoadFunctions, GeneratedFunctionCount,
             SkippedAssetCount, ProcessedAssetCount, InConfig, TotalAssets, DispatchComplete, WriteCanonicalAndAdvance]
            (const FString& AssetType, bool IsBlueprint, bool IsEditorOnly)
            {
                auto AssetFunction = FString{};

                if (AssetData.AssetClassPath.GetAssetName() != OBJECT_REDIRECTOR_CLASS &&
                    NOT UCk_Utils_IO_UE::Get_IsTemporaryAsset(AssetData.AssetName.ToString()))
                {
                    auto BaseAssetName = Get_CleanAssetName(AssetData.AssetName.ToString());

                    if (auto AssetPath = AssetData.GetSoftObjectPath().ToString();
                        NOT GloballyGeneratedAssets.Contains(AssetPath) && NOT AssetType.IsEmpty())
                    {
                        auto FinalAssetName = BaseAssetName;
                        if (UsedAssetNames.Contains(BaseAssetName))
                        {
                            auto DupCount = int32{1};
                            do {
                                FinalAssetName = ck::Format_UE(TEXT("{}_DUP{}"), BaseAssetName, DupCount);
                                DupCount++;
                            } while (UsedAssetNames.Contains(FinalAssetName));

                            ck::angelscriptgenerator::Log(TEXT("Duplicate asset name: {} -> {}"), BaseAssetName, FinalAssetName);
                        }

                        UsedAssetNames.Add(FinalAssetName);
                        GloballyGeneratedAssets.Add(AssetPath);
                        AssetPathToFunctionName.Add(AssetPath, FinalAssetName);

                        if (IsEditorOnly)
                        { AssetFunction += TEXT("#if Editor\n"); }

                        AssetFunction += ck::Format_UE(TEXT("    TSoftObjectPtr<{}>"), AssetType);
                        AssetFunction += ck::Format_UE(TEXT(" {}() {{ return TSoftObjectPtr<{}>(FSoftObjectPath(\"{}\")); }}\n"),
                                                   FinalAssetName, AssetType, AssetPath);

                        if (IsEditorOnly)
                        { AssetFunction += TEXT("#endif\n"); }

                        auto LoadFunction = FString{};
                        if (IsEditorOnly)
                        { LoadFunction += TEXT("#if Editor\n"); }

                        LoadFunction += ck::Format_UE(TEXT("    {} {}()\n"), AssetType, FinalAssetName);
                        LoadFunction += TEXT("    {\n");
                        // Before engine-init, blocking loads are unsafe → return null so the query trips
                        // WasBlockingLoadQueriedWhileUnsafe and UCk_DeferredAssetInit_UE re-runs post-init.
                        // Report the premature call via the cheap aggregator (EnsureIfNot_PrematureAssetLoad —
                        // no per-call stack walks) so a stray premature assets::load::X() is still surfaced as
                        // one summary line. Gated on Get_IsRunningCommandlet() so it stays SILENT during cook.
                        LoadFunction += TEXT("        if (UCk_Utils_IO_UE::IsEngineSafeForBlockingLoads() == false)\n");
                        LoadFunction += TEXT("        {\n");
                        LoadFunction += ck::Format_UE(TEXT("            ck::EnsureIfNot_PrematureAssetLoad(UCk_Utils_IO_UE::Get_IsRunningCommandlet(), \"{}::load::{}() called before engine init. Use {}::{}() (soft ref) with UCk_DeferredConfig_UE instead.\");\n"), InConfig->Namespace, FinalAssetName, InConfig->Namespace, FinalAssetName);
                        LoadFunction += TEXT("            return nullptr;\n");
                        LoadFunction += TEXT("        }\n");
                        LoadFunction += ck::Format_UE(TEXT("        return System::LoadAsset_Blocking({}::{}());\n"), InConfig->Namespace, FinalAssetName);
                        LoadFunction += TEXT("    }\n");

                        if (IsEditorOnly)
                        { LoadFunction += TEXT("#endif\n"); }

                        if (IsBlueprint)
                        {
                            if (IsEditorOnly)
                            { LoadFunction += TEXT("#if Editor\n"); }

                            LoadFunction += ck::Format_UE(TEXT("    TSubclassOf<{}> {}_Class()\n"), AssetType, FinalAssetName);
                            LoadFunction += TEXT("    {\n");
                            // See note above: cheap premature-load report (no per-call stack walks).
                            LoadFunction += TEXT("        if (UCk_Utils_IO_UE::IsEngineSafeForBlockingLoads() == false)\n");
                            LoadFunction += TEXT("        {\n");
                            LoadFunction += ck::Format_UE(TEXT("            ck::EnsureIfNot_PrematureAssetLoad(UCk_Utils_IO_UE::Get_IsRunningCommandlet(), \"{}::load::{}_Class() called before engine init. Use {}::{}_Class() (soft ref) with UCk_DeferredConfig_UE instead.\");\n"), InConfig->Namespace, FinalAssetName, InConfig->Namespace, FinalAssetName);
                            LoadFunction += TEXT("            return nullptr;\n");
                            LoadFunction += TEXT("        }\n");
                            LoadFunction += ck::Format_UE(TEXT("        return System::LoadClassAsset_Blocking({}::{}_Class());\n"), InConfig->Namespace, FinalAssetName);
                            LoadFunction += TEXT("    }\n");

                            if (IsEditorOnly)
                            { LoadFunction += TEXT("#endif\n"); }
                        }

                        CollectedLoadFunctions->Add(LoadFunction);

                        if (IsBlueprint)
                        {
                            auto ClassPath = AssetPath + TEXT("_C");
                            
                            if (IsEditorOnly)
                            { AssetFunction += TEXT("#if Editor\n"); }
                            
                            AssetFunction += ck::Format_UE(TEXT("    TSoftClassPtr<{}>"), AssetType);
                            AssetFunction += ck::Format_UE(TEXT(" {}_Class() {{ return TSoftClassPtr<{}>(FSoftObjectPath(\"{}\")); }}\n"),
                                                           FinalAssetName, AssetType, ClassPath);
                            
                            if (IsEditorOnly)
                            { AssetFunction += TEXT("#endif\n"); }
                        }

                        (*GeneratedFunctionCount)++;
                    }
                    else
                    {
                        (*SkippedAssetCount)++;
                    }
                }
                else
                {
                    (*SkippedAssetCount)++;
                }

                CollectedFunctions->Add(AssetFunction);

                (*PendingAssets)--;
                (*ProcessedAssetCount)++;

                if (ActiveProgressNotification.IsValid())
                {
                    // TotalWorkDone is cumulative (not incremental); ProcessedAssetCount already tracks it.
                    FSlateNotificationManager::Get().UpdateProgressNotification(
                        ActiveProgressNotification, *ProcessedAssetCount, TotalAssets);
                }

                OnAssetRegistryProgress.Broadcast(*ProcessedAssetCount, TotalAssets);

                // Gate on DispatchComplete to suppress writes during the sync drain
                // (Race A causes ~1400 callbacks to fire inside this for-loop).
                if (*DispatchComplete && *PendingAssets <= 0)
                {
                    WriteCanonicalAndAdvance();
                }
            }));
    };

    // Why a per-frame ticker instead of a tight for-loop: nearly every asset now
    // resolves SYNCHRONOUSLY (sync linker walk / AssetData-tag parent resolution),
    // so a single for-loop would process the whole batch in one game-thread burst —
    // Slate never ticks, the status-bar progress bar can't repaint until the burst
    // ends, and the editor freezes for a second or two with the bar only flashing in
    // near 100%. Dispatching a time-boxed slice of assets per frame yields to Slate
    // between slices: the editor stays interactive and the bar fills live. (Resolution
    // touches UObject/reflection/AssetData, which is game-thread-only — no worker thread.)
    auto RemainingAssets = MakeShared<TArray<FAssetData>>(MoveTemp(DiscoveredAssets));
    auto NextIndex = MakeShared<int32>(0);

    Dismiss_GenerationTicker();
    GenerationTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
        [this, RemainingAssets, NextIndex, DispatchOneAsset, DispatchComplete, PendingAssets, WriteCanonicalAndAdvance]
        (float) -> bool
        {
            // ~8ms/frame budget keeps the editor interactive (knob: lower = smoother
            // editor + longer wall-clock; higher = faster finish + more hitch per frame).
            constexpr auto TickBudgetSeconds = 0.008;
            const auto SliceStartTime = FPlatformTime::Seconds();
            const auto Total = RemainingAssets->Num();

            while (*NextIndex < Total)
            {
                DispatchOneAsset((*RemainingAssets)[*NextIndex]);
                (*NextIndex)++;

                if ((FPlatformTime::Seconds() - SliceStartTime) >= TickBudgetSeconds)
                { break; }
            }

            if (*NextIndex < Total)
            { return true; } // more assets to dispatch next frame — keep ticking

            // All assets dispatched. Reset the handle BEFORE WriteCanonicalAndAdvance,
            // which may start the next queued config (and overwrite GenerationTickerHandle).
            // All-sync path: drain already complete, fire the write here.
            // All-async path: PendingAssets > 0, the last async callback owns the write.
            GenerationTickerHandle.Reset();
            *DispatchComplete = true;
            if (*PendingAssets <= 0)
            { WriteCanonicalAndAdvance(); }

            return false; // dispatch done — stop this ticker
        }));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Request_ProcessNextInQueue()
    -> void
{
    if (PendingGenerationQueue.IsEmpty())
    { return; }

    // Get the next config from the queue
    auto NextConfig = PendingGenerationQueue[0];
    PendingGenerationQueue.RemoveAt(0);

    if (ck::IsValid(NextConfig))
    {
        ck::angelscriptgenerator::Log(TEXT("Processing next queued config: {} ({} remaining in queue)"),
                                     NextConfig->GetDisplayName(), PendingGenerationQueue.Num());
        GenerateAssetRegistryForConfig_Internal(NextConfig);
    }
    else
    {
        ck::angelscriptgenerator::Warning(TEXT("Invalid config in queue, skipping to next"));
        // Recursively process the next one
        Request_ProcessNextInQueue();
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    OnAssetAdded(
        const FAssetData& AssetData)
    -> void
{
    if (AssetData.AssetClassPath.GetAssetName() == UCkAssetRegistryConfig::StaticClass()->GetFName())
    {
        ck::angelscriptgenerator::Log(TEXT("New Asset Registry config detected: {}"), AssetData.AssetName);
        Request_ScheduleRegeneration();
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    OnAssetRemoved(
        const FAssetData& AssetData)
    -> void
{
    if (AssetData.AssetClassPath.GetAssetName() == UCkAssetRegistryConfig::StaticClass()->GetFName())
    {
        ck::angelscriptgenerator::Log(TEXT("Asset Registry config removed: {}"), AssetData.AssetName);
        Request_ScheduleRegeneration();
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    OnAssetUpdated(
        const FAssetData& AssetData)
    -> void
{
    if (AssetData.AssetClassPath.GetAssetName() == UCkAssetRegistryConfig::StaticClass()->GetFName())
    {
        ck::angelscriptgenerator::Log(TEXT("Asset Registry config updated: {}"), AssetData.AssetName);
        Request_ScheduleRegeneration();
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Request_DiscoverAllConfigs()
    -> TArray<UCkAssetRegistryConfig*>
{
    auto Result = TArray<UCkAssetRegistryConfig*>{};

    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    const auto& AssetRegistry = AssetRegistryModule.Get();

    auto ConfigAssets = TArray<FAssetData>{};
    AssetRegistry.GetAssetsByClass(UCkAssetRegistryConfig::StaticClass()->GetClassPathName(), ConfigAssets);

    ck::angelscriptgenerator::Log(TEXT("Found {} Asset Registry config assets in project"), ConfigAssets.Num());

    for (const auto& AssetData : ConfigAssets)
    {
        if (auto Config = Cast<UCkAssetRegistryConfig>(AssetData.GetAsset()))
        { Result.Add(Config); }
    }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Request_DiscoverAssetsInPath(
        const FString& InRootPath)
    -> TArray<FAssetData>
{
    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    const auto& AssetRegistry = AssetRegistryModule.Get();

    auto DiscoveredAssets = TArray<FAssetData>{};

    AssetRegistry.GetAssetsByPath(FName(*InRootPath), DiscoveredAssets, true);

    ck::angelscriptgenerator::Log(TEXT("Discovered {} assets under path: {}"),
                                 DiscoveredAssets.Num(), InRootPath);

    return DiscoveredAssets;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Get_AssetTypeFromClass(
        UClass* InAssetClass)
    -> FString
{
    if (ck::Is_NOT_Valid(InAssetClass))
    { return FString{}; }

    return Get_CorrectClassNameWithPrefix(InAssetClass);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Get_CorrectClassNameWithPrefix(
        UClass* InClass)
    -> FString
{
    if (ck::Is_NOT_Valid(InClass))
    { return FString{}; }

    auto ClassName = InClass->GetName();
    auto ClassNameWithoutSuffix = FBlueprintEditorUtils::GetClassNameWithoutSuffix(InClass);

    ck::angelscriptgenerator::Log(TEXT("Processing class: {} -> {} (after suffix removal) (IsActor: {})"),
        ClassName, ClassNameWithoutSuffix, InClass->IsChildOf(AActor::StaticClass()));

    return Get_CorrectClassNameWithPrefix_String(ClassNameWithoutSuffix, InClass->IsChildOf(AActor::StaticClass()));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Get_CorrectClassNameWithPrefix_String(
        const FString& InClassName,
        bool IsActor)
    -> FString
{
    if (InClassName.IsEmpty())
    { return FString{}; }

    ck::angelscriptgenerator::Log(TEXT("Processing class name: {} (IsActor: {})"), InClassName, IsActor);

    if (InClassName == USER_DEFINED_STRUCT_CLASS)
    {
        return TEXT("UUserDefinedStruct");
    }

    if (IsActor)
    {
        auto Result = TEXT("A") + InClassName;
        ck::angelscriptgenerator::Log(TEXT("Actor class {} -> {}"), InClassName, Result);
        return Result;
    }
    else
    {
        auto Result = TEXT("U") + InClassName;
        ck::angelscriptgenerator::Log(TEXT("UObject class {} -> {}"), InClassName, Result);
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Get_CleanAssetName(
        const FString& InAssetName)
    -> FString
{
    auto Result = FString{};
    Result.Reserve(InAssetName.Len());

    // Sanitize: keep only alphanumeric characters and underscores
    for (int32 i = 0; i < InAssetName.Len(); i++)
    {
        if (const auto Char = InAssetName[i];
            FChar::IsAlnum(Char) || Char == TEXT('_'))
        {
            Result.AppendChar(Char);
        }
    }

    // Handle empty result (shouldn't happen, but just in case)
    if (Result.IsEmpty())
    {
        Result = TEXT("Asset");
    }

    // Add underscore prefix if name starts with a digit (invalid identifier in AngelScript)
    if (FChar::IsDigit(Result[0]))
    {
        Result = TEXT("_") + Result;
    }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Request_ScheduleRegeneration()
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(GEditor),
        TEXT("Cannot schedule regeneration - GEditor is not valid"))
    { return; }

    GEditor->GetTimerManager()->ClearTimer(RegenerationTimerHandle);

    constexpr auto Repeat = false;
    GEditor->GetTimerManager()->SetTimer(
        RegenerationTimerHandle,
        this,
        &UCkAssetRegistrySubsystem::ExecuteDelayedRegeneration,
        REGENERATION_DELAY_SECONDS,
        Repeat);

    ck::angelscriptgenerator::Log(TEXT("Scheduled asset registry regeneration in {} seconds"), REGENERATION_DELAY_SECONDS);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    ExecuteDelayedRegeneration()
    -> void
{
    ck::angelscriptgenerator::Log(TEXT("Executing delayed asset registry regeneration"));
    GenerateAllAssetRegistries();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Get_OutputDirectoryForRootPath(
        const FString& InRootPath)
    -> FString
{
    if (InRootPath.StartsWith(TEXT("/Game")))
    { return FPaths::ProjectDir() / TEXT("Script") / TEXT("Generated"); }

    if (InRootPath.StartsWith(TEXT("/")))
    {
        const auto PathWithoutLeadingSlash = InRootPath.Mid(1);

        if (const auto FirstSlashIndex = PathWithoutLeadingSlash.Find(TEXT("/"));
            FirstSlashIndex != INDEX_NONE)
        {
            auto PluginName = PathWithoutLeadingSlash.Left(FirstSlashIndex);

            if (const auto Plugin = IPluginManager::Get().FindPlugin(PluginName);
                Plugin.IsValid())
            {
                const auto& PluginDir = Plugin->GetBaseDir();
                return PluginDir / TEXT("Script") / TEXT("Generated");
            }

            ck::angelscriptgenerator::Warning(TEXT("Plugin [{}] not found for root path [{}], using project directory"), PluginName, InRootPath);
        }
    }

    ck::angelscriptgenerator::Warning(TEXT("Unrecognized root path format [{}], using project directory"), InRootPath);
    return FPaths::ProjectDir() / TEXT("Script") / TEXT("Generated");
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    BuildFileHeader(
        UCkAssetRegistryConfig* InConfig,
        const FString& InRootPath)
    -> FString
{
    auto Content = FString{};
    Content += TEXT("// Auto-generated Asset Registry\n");
    Content += TEXT("// DO NOT EDIT - This file is automatically regenerated\n");
    Content += ck::Format_UE(TEXT("// Source config: {}\n"), InConfig->GetDisplayName());
    Content += ck::Format_UE(TEXT("// Discovery root: {}\n\n"), InRootPath);
    Content += TEXT("// Soft references - for deferred loading\n");
    Content += ck::Format_UE(TEXT("namespace {}\n{{\n"), InConfig->Namespace);
    return Content;
}

// ====================================================================================================================
// AngelScript Asset Reference Tracking
// ====================================================================================================================

auto
    UCkAssetRegistrySubsystem::
    Get_ScriptDirectory()
    -> FString
{
    return FPaths::ProjectDir() / TEXT("Script");
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    HandleAngelscriptPostCompile()
    -> void
{
    ck::angelscriptgenerator::Log(TEXT("[AssetRegistry] AS compilation complete - rebuilding usage map"));
    ScanScriptFilesForUsage();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    SeedMapsFromGeneratedFiles()
    -> void
{
    // Discover configs to get namespaces and generated file paths
    auto Configs = Request_DiscoverAllConfigs();
    if (Configs.IsEmpty())
    { return; }

    for (const auto* Config : Configs)
    {
        if (ck::Is_NOT_Valid(Config))
        { continue; }

        ActiveNamespaces.Add(Config->Namespace);

        auto OutputDir = Get_OutputDirectoryForRootPath(Config->AssetDiscoveryRoot);
        auto OutputPath = OutputDir / Config->OutputFileName;

        auto FileContents = FString{};
        if (NOT FFileHelper::LoadFileToString(FileContents, *OutputPath))
        { continue; }

        // Parse lines matching: FSoftObjectPath("AssetPath")
        // and extract the function name from: FunctionName() {
        auto SoftPathPrefix = FString{TEXT("FSoftObjectPath(\"")};
        auto SearchStart = int32{0};

        while (true)
        {
            auto PathStart = FileContents.Find(SoftPathPrefix, ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchStart);
            if (PathStart == INDEX_NONE)
            { break; }

            auto AssetPathStart = PathStart + SoftPathPrefix.Len();
            auto AssetPathEnd = FileContents.Find(TEXT("\""), ESearchCase::CaseSensitive, ESearchDir::FromStart, AssetPathStart);
            if (AssetPathEnd == INDEX_NONE)
            { break; }

            auto AssetPath = FileContents.Mid(AssetPathStart, AssetPathEnd - AssetPathStart);

            // Skip _C class paths (Blueprint class refs are derived from the base entry)
            if (AssetPath.EndsWith(TEXT("_C")))
            {
                SearchStart = AssetPathEnd + 1;
                continue;
            }

            // Walk backwards from FSoftObjectPath to find the function name
            // Pattern: FunctionName() { return TSoftObjectPtr<...>(FSoftObjectPath("...
            // Look for the ") {" before this FSoftObjectPath call, then find "FunctionName("
            auto LineStart = FileContents.Find(TEXT("\n"), ESearchCase::CaseSensitive, ESearchDir::FromEnd, PathStart);
            if (LineStart == INDEX_NONE)
            { LineStart = 0; }

            auto LineContent = FileContents.Mid(LineStart, PathStart - LineStart);

            // Find the pattern: "> FunctionName() {" or just "FunctionName() {"
            // The function name is right before the first "()" on this line
            auto ParenIndex = LineContent.Find(TEXT("()"), ESearchCase::CaseSensitive);
            if (ParenIndex != INDEX_NONE)
            {
                // Walk backwards from paren to find the start of the function name
                auto NameEnd = ParenIndex;
                auto NameStart = NameEnd;
                while (NameStart > 0)
                {
                    auto Ch = LineContent[NameStart - 1];
                    if (FChar::IsAlnum(Ch) || Ch == TEXT('_'))
                    { NameStart--; }
                    else
                    { break; }
                }

                if (NameStart < NameEnd)
                {
                    auto FunctionName = LineContent.Mid(NameStart, NameEnd - NameStart);
                    AssetPathToFunctionName.Add(AssetPath, FunctionName);
                }
            }

            SearchStart = AssetPathEnd + 1;
        }
    }

    ck::angelscriptgenerator::Log(
        TEXT("[AssetRegistry] Seeded {} asset references from {} generated files ({} namespaces)"),
        AssetPathToFunctionName.Num(), Configs.Num(), ActiveNamespaces.Num());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    ScanScriptFilesForUsage()
    -> void
{
    FunctionUsageMap.Reset();

    // If Map 1 hasn't been populated by a generation pass yet, seed it from the existing generated files
    if (AssetPathToFunctionName.IsEmpty() || ActiveNamespaces.IsEmpty())
    {
        SeedMapsFromGeneratedFiles();
    }

    if (AssetPathToFunctionName.IsEmpty() || ActiveNamespaces.IsEmpty())
    { return; }

    auto ScriptDir = Get_ScriptDirectory();
    auto GeneratedDir = ScriptDir / TEXT("Generated");

    auto AsFiles = TArray<FString>{};
    IFileManager::Get().FindFilesRecursive(AsFiles, *ScriptDir, TEXT("*.as"), true, false);

    // Built once per scan — per-occurrence membership checks against the
    // generated function names would otherwise be linear map scans (FindKey)
    // repeated for every candidate in every script file.
    auto GeneratedFunctionNames = TSet<FString>{};
    GeneratedFunctionNames.Reserve(AssetPathToFunctionName.Num());
    for (const auto& [AssetPath, FunctionName] : AssetPathToFunctionName)
    {
        GeneratedFunctionNames.Add(FunctionName);
    }

    for (const auto& FilePath : AsFiles)
    {
        if (FilePath.StartsWith(GeneratedDir))
        { continue; }

        auto UsedFunctions = ScanSingleScriptFile(FilePath, GeneratedFunctionNames);
        for (const auto& FunctionName : UsedFunctions)
        {
            FunctionUsageMap.FindOrAdd(FunctionName).AddUnique(FilePath);
        }
    }

    ck::angelscriptgenerator::Log(
        TEXT("[AssetRegistry] Script usage scan complete: {} asset functions referenced from {} script files"),
        FunctionUsageMap.Num(), AsFiles.Num());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    ScanSingleScriptFile(
        const FString& FilePath,
        const TSet<FString>& InGeneratedFunctionNames) const
    -> TSet<FString>
{
    auto Result = TSet<FString>{};

    auto FileContents = FString{};
    if (NOT FFileHelper::LoadFileToString(FileContents, *FilePath))
    { return Result; }

    for (const auto& Namespace : ActiveNamespaces)
    {
        auto LoadPrefix = Namespace + TEXT("::load::");
        auto SoftPrefix = Namespace + TEXT("::");

        // Scan for assets::load::<name>( pattern
        int32 SearchStart = 0;
        while (true)
        {
            auto FoundIndex = FileContents.Find(LoadPrefix, ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchStart);
            if (FoundIndex == INDEX_NONE)
            { break; }

            auto NameStart = FoundIndex + LoadPrefix.Len();
            auto ParenIndex = FileContents.Find(TEXT("("), ESearchCase::CaseSensitive, ESearchDir::FromStart, NameStart);
            if (ParenIndex != INDEX_NONE && (ParenIndex - NameStart) < 128)
            {
                auto FunctionName = FileContents.Mid(NameStart, ParenIndex - NameStart).TrimStartAndEnd();
                if (FunctionName.Len() > 0 && InGeneratedFunctionNames.Contains(FunctionName))
                {
                    Result.Add(FunctionName);
                }
            }
            SearchStart = FoundIndex + 1;
        }

        // Scan for assets::<name>( pattern (soft refs), excluding assets::load::
        SearchStart = 0;
        while (true)
        {
            auto FoundIndex = FileContents.Find(SoftPrefix, ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchStart);
            if (FoundIndex == INDEX_NONE)
            { break; }

            // Skip if this is actually the load:: prefix
            if (FileContents.Mid(FoundIndex, LoadPrefix.Len()) == LoadPrefix)
            {
                SearchStart = FoundIndex + 1;
                continue;
            }

            auto NameStart = FoundIndex + SoftPrefix.Len();
            auto ParenIndex = FileContents.Find(TEXT("("), ESearchCase::CaseSensitive, ESearchDir::FromStart, NameStart);
            if (ParenIndex != INDEX_NONE && (ParenIndex - NameStart) < 128)
            {
                auto FunctionName = FileContents.Mid(NameStart, ParenIndex - NameStart).TrimStartAndEnd();
                if (FunctionName.Len() > 0 && FunctionName != TEXT("load")
                    && InGeneratedFunctionNames.Contains(FunctionName))
                {
                    Result.Add(FunctionName);
                }
            }
            SearchStart = FoundIndex + 1;
        }
    }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    HandleAssetsPreDelete(
        const TArray<UObject*>& AssetsToDelete)
    -> void
{
    if (AssetPathToFunctionName.IsEmpty() || FunctionUsageMap.IsEmpty())
    { return; }

    auto Warnings = TArray<TPair<FString, TArray<FString>>>{};

    for (const auto* Asset : AssetsToDelete)
    {
        if (ck::Is_NOT_Valid(Asset))
        { continue; }

        auto AssetPath = Asset->GetPathName();
        if (const auto* FunctionName = AssetPathToFunctionName.Find(AssetPath))
        {
            if (const auto* UsageFiles = FunctionUsageMap.Find(*FunctionName))
            {
                if (UsageFiles->Num() > 0)
                {
                    Warnings.Emplace(*FunctionName, *UsageFiles);
                }
            }
        }
    }

    if (Warnings.IsEmpty())
    { return; }

    auto Message = FString{TEXT("The following assets are actively referenced in AngelScript:\n\n")};
    for (const auto& [FunctionName, Files] : Warnings)
    {
        Message += ck::Format_UE(TEXT("  assets::{}() / assets::load::{}()\n"), FunctionName, FunctionName);
        for (const auto& File : Files)
        {
            auto RelativePath = File;
            FPaths::MakePathRelativeTo(RelativePath, *FPaths::ProjectDir());
            Message += ck::Format_UE(TEXT("    used in: {}\n"), RelativePath);
        }
        Message += TEXT("\n");
    }
    Message += TEXT("Deleting will cause runtime load failures in these scripts.");

    FMessageDialog::Open(
        EAppMsgType::Ok,
        FText::FromString(Message),
        FText::FromString(TEXT("AngelScript Asset Reference Warning")));
}

// --------------------------------------------------------------------------------------------------------------------
