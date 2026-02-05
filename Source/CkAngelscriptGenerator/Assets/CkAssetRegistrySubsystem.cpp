#include "CkAssetRegistrySubsystem.h"

#include "CkAssetRegistryConfig.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"

#include "CkCore/IO/CkIO_Utils.h"
#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"

#include <AssetRegistry/AssetRegistryModule.h>
#include <Engine/Blueprint.h>
#include <Engine/Engine.h>
#include <GameFramework/Actor.h>
#include <HAL/FileManager.h>
#include <Interfaces/IPluginManager.h>
#include <Kismet2/BlueprintEditorUtils.h>
#include <Misc/FileHelper.h>
#include <Misc/Paths.h>
#include <TimerManager.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    IsEditorOnlyClass(
        const UClass* InClass)
    -> bool
{
    if (ck::Is_NOT_Valid(InClass))
    { return false; }

    auto Package = InClass->GetOutermost();
    if (ck::Is_NOT_Valid(Package))
    { return false; }

    const auto ModuleName = FPackageName::GetShortFName(Package->GetFName());

    // Check if module is loaded and if it's a known editor module
    if (FModuleManager::Get().IsModuleLoaded(ModuleName))
    {
        // Built-in editor modules
        static const TSet<FName> EditorModules = {
            TEXT("UnrealEd"),
            TEXT("ViewportInteraction"),
            TEXT("VREditor"),
            TEXT("Blutility"),
            TEXT("Kismet"),
            TEXT("AssetTools"),
            TEXT("PlacementMode"),
            TEXT("MaterialEditor"),
            TEXT("LandscapeEditor"),
        };

        if (EditorModules.Contains(ModuleName))
        { return true; }
    }

    // Check plugin modules
    for (const auto& Plugin : IPluginManager::Get().GetEnabledPlugins())
    {
        for (const auto& Module : Plugin->GetDescriptor().Modules)
        {
            if (Module.Name == ModuleName)
            { return Module.Type == EHostType::Editor; }
        }
    }

    return false;
}

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
                    if (IsEditorOnlyClass(NativeParentClass))
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
                        if (IsEditorOnlyClass(NativeParentClass))
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

    ck::angelscriptgenerator::Log(TEXT("Asset registry callbacks registered for real-time config discovery"));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Deinitialize()
    -> void
{
    ActiveSlowTask.Reset();
    IsGenerationInProgress = false;
    PendingGenerationQueue.Empty();

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
    GenerateAllAssetRegistries()
    -> void
{
    GloballyGeneratedAssets.Reset();

    ck::angelscriptgenerator::Log(TEXT("=== Generating All Asset Registries ==="));

    auto AllConfigs = Request_DiscoverAllConfigs();

    if (AllConfigs.IsEmpty())
    {
        ck::angelscriptgenerator::Warning(TEXT("No Asset Registry config assets found"));
        return;
    }

    ck::angelscriptgenerator::Log(TEXT("Found {} Asset Registry config assets"), AllConfigs.Num());

    auto GeneratedCount = int32{0};
    auto FailedCount = int32{0};

    for (auto Config : AllConfigs)
    {
        CK_ENSURE_IF_NOT(ck::IsValid(Config),
            TEXT("Invalid Asset Registry config found"))
        {
            FailedCount++;
            continue;
        }

        ck::angelscriptgenerator::Log(TEXT("Generating for config: {}"), Config->GetDisplayName());

        GenerateAssetRegistryForConfig_Internal(Config);
        GeneratedCount++;
    }

    ck::angelscriptgenerator::Log(TEXT("Asset Registry generation completed: {} succeeded, {} failed"),
                                 GeneratedCount, FailedCount);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    GenerateAssetRegistryForConfig(
        UCkAssetRegistryConfig* InConfig)
    -> void
{
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

    DiscoveredAssets.Sort([](const FAssetData &A, const FAssetData &B) {
        return A.GetSoftObjectPath().ToString() < B.GetSoftObjectPath().ToString();
    });

    auto TotalAssets = DiscoveredAssets.Num();
    ck::angelscriptgenerator::Log(TEXT("Processing {} assets with async loading"), TotalAssets);

    // Create slow task for progress status bar
    ActiveSlowTask = MakeShared<FScopedSlowTask>(
        static_cast<float>(TotalAssets),
        FText::FromString(ck::Format_UE(TEXT("Generating Asset Registry: {}"), InConfig->OutputFileName)),
        true
    );
    ActiveSlowTask->MakeDialog(false); // false = can't cancel

    auto Content = BuildFileHeader(InConfig, RootPath);
    auto GeneratedFunctionCount = MakeShared<int32>(0);
    auto SkippedAssetCount = MakeShared<int32>(0);
    auto ProcessedAssetCount = MakeShared<int32>(0);
    auto PendingAssets = MakeShared<int32>(0);
    auto CollectedFunctions = MakeShared<TArray<FString>>();
    auto CollectedLoadFunctions = MakeShared<TArray<FString>>();

    CollectedFunctions->Reserve(TotalAssets);
    CollectedLoadFunctions->Reserve(TotalAssets);

    for (const auto& AssetData : DiscoveredAssets)
    {
        (*PendingAssets)++;

        Get_AssetTypeFromAssetData(AssetData, FOnAssetTypeResolved::CreateLambda(
            [this, AssetData, PendingAssets, CollectedFunctions, CollectedLoadFunctions, GeneratedFunctionCount,
             SkippedAssetCount, ProcessedAssetCount, Content, InConfig, TotalAssets]
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

                        LoadFunction += ck::Format_UE(TEXT("    {} {}() {{ return System::LoadAsset_Blocking({}::{}()); }}\n"),
                            AssetType, FinalAssetName, InConfig->Namespace, FinalAssetName);

                        if (IsEditorOnly)
                        { LoadFunction += TEXT("#endif\n"); }

                        if (IsBlueprint)
                        {
                            if (IsEditorOnly)
                            { LoadFunction += TEXT("#if Editor\n"); }

                            LoadFunction += ck::Format_UE(TEXT("    TSubclassOf<{}> {}_Class() {{ return System::LoadClassAsset_Blocking({}::{}_Class()); }}\n"),
                                AssetType, FinalAssetName, InConfig->Namespace, FinalAssetName);

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

                // Update progress bar
                if (ActiveSlowTask.IsValid())
                {
                    ActiveSlowTask->EnterProgressFrame(1.0f);
                }

                OnAssetRegistryProgress.Broadcast(*ProcessedAssetCount, TotalAssets);

                if (*PendingAssets <= 0)
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

                    // Clean up slow task and reset flag
                    ActiveSlowTask.Reset();
                    IsGenerationInProgress = false;

                    OnAssetRegistryComplete.Broadcast(*GeneratedFunctionCount, *SkippedAssetCount, TotalAssets);

                    // Process next item in queue
                    Request_ProcessNextInQueue();
                }
            }));
    }
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

// --------------------------------------------------------------------------------------------------------------------
