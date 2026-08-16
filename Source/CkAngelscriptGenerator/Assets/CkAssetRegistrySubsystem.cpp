#include "CkAssetRegistrySubsystem.h"

#include "CkAssetRegistryConfig.h"
#include "CkAngelscriptGenerator/Assets/CkAssetRegistry_ClassResolver.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_RegenOwnership.h"

#include "CkCore/IO/CkIO_Utils.h"
#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"
#include "CkCore/Reference/CkAssetReferenceProvider.h"
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
#include <Misc/CoreDelegates.h>
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

    // Sync resolve runs BEFORE the async load so the result lands before the regen-completion
    // event; a late async callback drops its accessor from the output and kicks the self-heal
    // synth/cleanup loop. The BP gate is load-bearing: embedded `_C` exports (an AnimMontage's
    // AnimNotifyStates) would fool the linker walk, and GetClass() is authoritative for non-BPs.
    if (const auto AssetClass = InAssetData.GetClass(); ck::IsValid(AssetClass))
    {
        const auto IsBlueprintAsset = AssetClass->IsChildOf<UBlueprint>();

        if (IsBlueprintAsset)
        {
            const auto SyncResolved = ck::angelscriptgenerator::FCkAssetRegistry_ClassResolver
                ::Resolve_ViaPackageReader(AssetPath.ToString());

            if (NOT SyncResolved.ClassName.IsEmpty())
            {
                // A null ResolvedClass (AS reload window) correctly defaults to false: AS classes
                // are runtime by definition, and editor-utility parents are loaded at startup.
                const auto IsEditorOnly = ck::IsValid(SyncResolved.ResolvedClass)
                    && UCk_Utils_Reflection_UE::Is_EditorOnlyClass(SyncResolved.ResolvedClass);

                ck::angelscriptgenerator::Log(TEXT("Sync-resolved BP class via FPackageReader linker: {} for {} (IsEditorOnly: {})"),
                    SyncResolved.ClassName, AssetName, IsEditorOnly);

                constexpr auto IsBlueprintLike = true;
                OnResolved.ExecuteIfBound(SyncResolved.ClassName, IsBlueprintLike, IsEditorOnly);
                return;
            }

            // A Blueprint with a deleted/renamed parent must NOT fall through to the async load:
            // its package load is cancelled, the streamable delegate never fires, *PendingAssets
            // never drains, and the whole batch stalls forever. Resolving from AR tags instead
            // needs no load, and either outcome fires the callback and returns.
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
            // Trades the async path's instance-level LoadedAsset->IsEditorOnly() for the
            // class-level check, which is the load-bearing one on game assets.
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
                if (const auto AssetClass = LoadedAsset->GetClass();
                    ck::IsValid(AssetClass))
                {
                    if (const auto NativeParentClass = Get_NonBlueprintParentClass(AssetClass);
                        ck::IsValid(NativeParentClass))
                    {
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

        if (NOT IsBlueprint)
        {
            ck::angelscriptgenerator::Log(TEXT("Found non blueprint class: {}"), CurrentClass->GetName());
            return CurrentClass;
        }

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

    PreDeleteDelegateHandle = FEditorDelegates::OnAssetsPreDelete.AddUObject(
        this, &UCkAssetRegistrySubsystem::HandleAssetsPreDelete);

#if WITH_ANGELSCRIPT_CK
    PostCompileDelegateHandle = FAngelscriptCodeModule::GetPostCompile().AddUObject(
        this, &UCkAssetRegistrySubsystem::HandleAngelscriptPostCompile);
#endif

    SeedMapsFromGeneratedFiles();
    ScanScriptFilesForUsage();

    // Declares this module's script references to anything that reasons about whether an asset is reachable. The
    // lambda is weak on purpose: the registry outlives an editor subsystem, and a stale binding would be a query
    // answering from a dead `this` at exactly the moment a tool is deciding whether to delete something.
    FCk_AssetReferenceProviderRegistry::Get().Request_Register(
        Get_ScriptReferenceProviderId(),
        FCk_Delegate_AssetReference_Query::CreateWeakLambda(this,
            [this](const FSoftObjectPath& InAsset) -> TArray<FString>
            {
                return Get_ScriptReferencersOfAsset(InAsset);
            }));

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

    // Unregistered EXPLICITLY rather than left to the weak binding. A weak lambda whose owner died answers "no script
    // referencers", which is indistinguishable from a correct negative — so a consumer would read `Get_HasAnyProvider`
    // as true and trust an answer nobody is giving. Removing the entry makes the silence visible.
    FCk_AssetReferenceProviderRegistry::Get().Request_Unregister(Get_ScriptReferenceProviderId());

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
    // Pre-init guard, and the single choke point for every regen trigger. The sweep resolves
    // classes via LoadObject, and loading a package with UActorComponent exports before
    // UEngine::Init asserts fatally ("Element type 'Components' has not been registered!").
    // A fast headless boot satisfies the settle-tickers' idleness gates PRE-init, so this race
    // is reachable. IsEngineSafeForBlockingLoads() flips exactly on OnFEngineLoopInitComplete.
    if (NOT UCk_Utils_IO_UE::IsEngineSafeForBlockingLoads())
    {
        ck::angelscriptgenerator::Log(
            TEXT("GenerateAllAssetRegistries requested pre-engine-init — deferring to OnFEngineLoopInitComplete."));

        FCoreDelegates::OnFEngineLoopInitComplete.AddWeakLambda(this, [this]()
        {
            Request_ScheduleRegeneration();
        });
        return;
    }

    // In-flight guard: generation spans multiple frames, so a re-trigger landing mid-run must
    // NOT reset the shared dedup/name state under the active batch — that corrupts its output
    // and can kick a self-heal regen cycle. The reschedule retries once the run drains.
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

    // Only dispatch counts are knowable here — _Internal may queue instead of generating, and
    // per-config success is reported by OnAssetRegistryComplete.
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
    // In-flight guard — see GenerateAllAssetRegistries.
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

    // Single-writer gate over every *Assets.as canonical write. A secondary writes nothing: it
    // clears the queue and broadcasts a did-nothing completion so listeners aren't left hanging,
    // and it must never flip IsGenerationInProgress, which a deferred ticker reads.
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

    if (IsGenerationInProgress)
    {
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

    // Status-bar notification, NOT FScopedSlowTask::MakeDialog — it must not block editor
    // interaction. Headless boots register no status-bar handler, so it no-ops silently there.
    // It auto-dismisses once work-done reaches total; the completion sites Cancel to drop the
    // entry from the status bar's tracking array.
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

    // Exactly one write per config: fired by the last async callback to drain, or post-loop
    // when every asset resolved synchronously.
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

        // A 0-accessor regen over a populated canonical means AR has not finished cataloging
        // this mount; refuse the write and let the next pass retry.
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

    // Called by the per-frame ticker below on a slice of assets at a time.
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
                        // The emitted null return trips WasBlockingLoadQueriedWhileUnsafe so
                        // UCk_DeferredAssetInit_UE re-runs post-init. EnsureIfNot_PrematureAssetLoad
                        // aggregates (no per-call stack walks) and stays silent during cook.
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

                // DispatchComplete suppresses writes during the sync drain, where every callback
                // fires inside the dispatch loop itself.
                if (*DispatchComplete && *PendingAssets <= 0)
                {
                    WriteCanonicalAndAdvance();
                }
            }));
    };

    // A ticker rather than a for-loop because nearly every asset resolves SYNCHRONOUSLY now:
    // one loop would burn the whole batch in a single game-thread burst, freezing Slate for
    // seconds. Time-boxed slices yield between frames. Resolution is game-thread-only
    // (UObject/reflection/AssetData), so a worker thread is not an option.
    auto RemainingAssets = MakeShared<TArray<FAssetData>>(MoveTemp(DiscoveredAssets));
    auto NextIndex = MakeShared<int32>(0);

    Dismiss_GenerationTicker();
    GenerationTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
        [this, RemainingAssets, NextIndex, DispatchOneAsset, DispatchComplete, PendingAssets, WriteCanonicalAndAdvance]
        (float) -> bool
        {
            // Tuning knob: lower = smoother editor + longer wall-clock; higher = the reverse.
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

            // Reset BEFORE WriteCanonicalAndAdvance, which may start the next queued config and
            // overwrite GenerationTickerHandle. All-sync fires the write here; all-async leaves
            // PendingAssets > 0 and the last callback owns it.
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

    for (int32 i = 0; i < InAssetName.Len(); i++)
    {
        if (const auto Char = InAssetName[i];
            FChar::IsAlnum(Char) || Char == TEXT('_'))
        {
            Result.AppendChar(Char);
        }
    }

    if (Result.IsEmpty())
    {
        Result = TEXT("Asset");
    }

    // An AngelScript identifier cannot start with a digit.
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

        // Parses the emitted shape: `FunctionName() { return TSoftObjectPtr<X>(FSoftObjectPath("Y"))`
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

            // Blueprint class refs are derived from the base entry, so `_C` paths are skipped.
            if (AssetPath.EndsWith(TEXT("_C")))
            {
                SearchStart = AssetPathEnd + 1;
                continue;
            }

            // The function name is the identifier immediately before the first "()" on the line.
            auto LineStart = FileContents.Find(TEXT("\n"), ESearchCase::CaseSensitive, ESearchDir::FromEnd, PathStart);
            if (LineStart == INDEX_NONE)
            { LineStart = 0; }

            auto LineContent = FileContents.Mid(LineStart, PathStart - LineStart);

            auto ParenIndex = LineContent.Find(TEXT("()"), ESearchCase::CaseSensitive);
            if (ParenIndex != INDEX_NONE)
            {
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

    // Built once per scan: the alternative is a linear FindKey per candidate per script file.
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
    Get_ScriptReferenceProviderId()
    -> FName
{
    return FName{TEXT("AngelScript")};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Get_ScriptReferencersOfAsset(
        const FSoftObjectPath& InAsset) const
    -> TArray<FString>
{
    if (InAsset.IsNull())
    { return {}; }

    // `AssetPathToFunctionName` is keyed on the object path string `UObject::GetPathName` produces, which is what
    // `FSoftObjectPath::ToString` produces for a top-level asset. Keeping ONE key shape is what lets this query and
    // the pre-delete warning agree — they read the same map, so they cannot disagree about an asset.
    const auto* FunctionName = AssetPathToFunctionName.Find(InAsset.ToString());

    if (FunctionName == nullptr)
    { return {}; }

    const auto* UsageFiles = FunctionUsageMap.Find(*FunctionName);

    if (UsageFiles == nullptr)
    { return {}; }

    auto Referencers = *UsageFiles;

    // Project-relative and sorted. Absolute paths carry the machine's checkout root, which makes two readers'
    // copies of the same list differ in a way neither of them caused.
    for (auto& Referencer : Referencers)
    { FPaths::MakePathRelativeTo(Referencer, *FPaths::ProjectDir()); }

    Referencers.Sort([](const FString& InLhs, const FString& InRhs) -> bool
    {
        return InLhs.Compare(InRhs, ESearchCase::IgnoreCase) < 0;
    });

    return Referencers;
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
