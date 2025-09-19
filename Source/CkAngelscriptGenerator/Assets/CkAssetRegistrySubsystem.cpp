#include "CkAssetRegistrySubsystem.h"

#include "CkAssetRegistryConfig.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"

#include "CkCore/IO/CkIO_Utils.h"

#include <AssetRegistry/AssetRegistryModule.h>
#include <Engine/Blueprint.h>
#include <Engine/Engine.h>
#include <Engine/UserDefinedStruct.h>
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
    Get_AssetTypeFromAssetData_Immediate(
        const FAssetData& InAssetData) -> FString
{
    ck::angelscriptgenerator::Log(TEXT("Get_AssetTypeFromAssetData_Immediate called for: {} (AssetClass: {})"),
        InAssetData.AssetName, InAssetData.AssetClassPath.GetAssetName());

    // For Blueprint assets, try to get the parent class from TagsAndValues first
    if (InAssetData.AssetClassPath.GetAssetName() == TEXT("Blueprint"))
    {
        FString ParentClassPath;
        if (InAssetData.GetTagValue(TEXT("ParentClass"), ParentClassPath))
        {
            ck::angelscriptgenerator::Log(TEXT("Found ParentClass tag: {}"), ParentClassPath);

            // ParentClass format: "/Script/CoreUObject.Class'/Script/CkEcs.Ck_EntityScript_UE'"
            // We need to actually load and resolve the parent class to handle Blueprint inheritance

            // Extract the class path and try to load the actual UClass
            auto LastQuoteIndex = ParentClassPath.Find(TEXT("'"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
            if (LastQuoteIndex != INDEX_NONE)
            {
                auto FullClassPath = ParentClassPath.Mid(ParentClassPath.Find(TEXT("'")) + 1,
                    LastQuoteIndex - ParentClassPath.Find(TEXT("'")) - 1);

                ck::angelscriptgenerator::Log(TEXT("Attempting to load parent class: {}"), FullClassPath);

                // Try to load the parent class
                if (auto ParentClass = LoadClass<UObject>(nullptr, *FullClassPath))
                {
                    ck::angelscriptgenerator::Log(TEXT("Successfully loaded parent class: {}"), ParentClass->GetName());

                    // Find the native parent class
                    auto NativeParentClass = Get_NativeParentClass(ParentClass);
                    if (ck::IsValid(NativeParentClass))
                    {
                        auto Result = Get_CorrectClassNameWithPrefix(NativeParentClass);
                        ck::angelscriptgenerator::Log(TEXT("Immediate resolution successful: {}"), Result);
                        return Result;
                    }
                    else
                    {
                        ck::angelscriptgenerator::Warning(TEXT("Could not find native parent for loaded class: {}"), ParentClass->GetName());
                    }
                }
                else
                {
                    ck::angelscriptgenerator::Warning(TEXT("Failed to load parent class from path: {}"), FullClassPath);
                }
            }
        }
        else
        {
            ck::angelscriptgenerator::Log(TEXT("No ParentClass tag found - will need async loading"));
        }

        // If TagsAndValues not available or loading failed, return empty string to indicate async loading needed
        return FString{};
    }
    else
    {
        ck::angelscriptgenerator::Log(TEXT("Non-Blueprint asset, using class-based resolution"));
        // For non-Blueprint assets (DataAssets, etc.), get the class directly
        auto AssetClass = InAssetData.GetClass();
        if (ck::IsValid(AssetClass))
        {
            return Get_CorrectClassNameWithPrefix(AssetClass);
        }
        else
        {
            ck::angelscriptgenerator::Warning(TEXT("Could not get class for non-Blueprint asset: {}"), InAssetData.AssetName);
            return FString{};
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Get_AssetTypeFromAssetData_Async(
        const FAssetData& InAssetData,
        const FOnAssetTypeResolved& OnResolved) -> void
{
    ck::angelscriptgenerator::Log(TEXT("Get_AssetTypeFromAssetData_Async called for: {}"), InAssetData.AssetName);

    // Try immediate resolution first
    auto ImmediateType = Get_AssetTypeFromAssetData_Immediate(InAssetData);
    if (NOT ImmediateType.IsEmpty())
    {
        ck::angelscriptgenerator::Log(TEXT("Immediate resolution successful: {}"), ImmediateType);
        OnResolved.ExecuteIfBound(ImmediateType);
        return;
    }

    ck::angelscriptgenerator::Log(TEXT("Immediate resolution failed, trying async loading"));

    // Need async loading for Blueprint
    if (InAssetData.AssetClassPath.GetAssetName() == TEXT("Blueprint"))
    {
        auto AssetPath = InAssetData.GetSoftObjectPath();

        ck::angelscriptgenerator::Log(TEXT("Loading Blueprint asset asynchronously: {}"), InAssetData.AssetName);

        // Use StreamableManager to load the asset with lambda capture
        auto LoadHandle = StreamableManager.RequestAsyncLoad(
            AssetPath,
            FStreamableDelegate::CreateLambda([this, InAssetData, OnResolved]()
            {
                OnAssetLoaded(InAssetData, OnResolved);
            })
        );

        // The delegate will be called when loading completes
    }
    else
    {
        // This should not happen for non-Blueprint assets since immediate resolution should work
        CK_TRIGGER_ENSURE(TEXT("Non-Blueprint asset failed immediate resolution: {}"), InAssetData.AssetName);
        OnResolved.ExecuteIfBound(FString{});
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    OnAssetLoaded(
        const FAssetData& OriginalAssetData,
        const FOnAssetTypeResolved& OnResolved) -> void
{
    ck::angelscriptgenerator::Log(TEXT("OnAssetLoaded called for: {}"), OriginalAssetData.AssetName);

    auto LoadedAsset = OriginalAssetData.GetAsset();

    if (NOT ck::IsValid(LoadedAsset))
    {
        CK_TRIGGER_ENSURE(TEXT("LoadedAsset is null for: {}"), OriginalAssetData.AssetName);
        return;
    }

    ck::angelscriptgenerator::Log(TEXT("LoadedAsset type: {}"), LoadedAsset->GetClass()->GetName());

    if (auto LoadedBlueprint = Cast<UBlueprint>(LoadedAsset))
    {
        ck::angelscriptgenerator::Log(TEXT("Successfully cast to UBlueprint"));
        auto AssetType = Get_AssetTypeFromLoadedBlueprint(LoadedBlueprint);
        ck::angelscriptgenerator::Log(TEXT("Resolved Blueprint parent class: {} for {}"),
            AssetType, OriginalAssetData.AssetName);
        OnResolved.ExecuteIfBound(AssetType);
    }
    else
    {
        CK_TRIGGER_ENSURE(TEXT("Failed to cast to Blueprint, asset type: {}"), LoadedAsset->GetClass()->GetName());
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Get_AssetTypeFromLoadedBlueprint(
        UBlueprint* LoadedBlueprint) -> FString
{
    if (NOT ck::IsValid(LoadedBlueprint))
    {
        CK_TRIGGER_ENSURE(TEXT("LoadedBlueprint is null"));
        return FString{};
    }

    ck::angelscriptgenerator::Log(TEXT("LoadedBlueprint is valid, getting ParentClass"));

    auto ParentClass = LoadedBlueprint->ParentClass;
    if (NOT ck::IsValid(ParentClass))
    {
        CK_TRIGGER_ENSURE(TEXT("ParentClass is null for Blueprint"));
        return FString{};
    }

    ck::angelscriptgenerator::Log(TEXT("ParentClass found: {}"), ParentClass->GetName());

    // Find the native (non-Blueprint) parent class
    auto NativeParentClass = Get_NativeParentClass(ParentClass);
    if (NOT ck::IsValid(NativeParentClass))
    {
        CK_TRIGGER_ENSURE(TEXT("Could not find native parent class for: {}"), ParentClass->GetName());
        return FString{};
    }

    ck::angelscriptgenerator::Log(TEXT("Native parent class: {}"), NativeParentClass->GetName());

    // Use the systematic prefix resolution on the native class
    auto Result = Get_CorrectClassNameWithPrefix(NativeParentClass);
    ck::angelscriptgenerator::Log(TEXT("Final result after prefix resolution: {}"), Result);
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Get_NativeParentClass(
        UClass* InClass) -> UClass*
{
    if (NOT ck::IsValid(InClass))
    { return nullptr; }

    auto CurrentClass = InClass;

    // Walk up the inheritance chain until we find a native (non-Blueprint) class
    while (ck::IsValid(CurrentClass))
    {
        ck::angelscriptgenerator::Log(TEXT("Checking class: {} (HasAnyClassFlags(CLASS_CompiledFromBlueprint): {})"),
            CurrentClass->GetName(), CurrentClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint));

        // If this class is not compiled from Blueprint, it's native
        if (NOT CurrentClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
        {
            ck::angelscriptgenerator::Log(TEXT("Found native class: {}"), CurrentClass->GetName());
            return CurrentClass;
        }

        // Move up to the parent class
        CurrentClass = CurrentClass->GetSuperClass();
    }

    ck::angelscriptgenerator::Warning(TEXT("Could not find native parent class - reached top of hierarchy"));
    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Initialize(
        FSubsystemCollectionBase& Collection) -> void
{
    Super::Initialize(Collection);

    ck::angelscriptgenerator::Log(TEXT("Initialized Asset Registry Subsystem"));

    auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    auto& AssetRegistry = AssetRegistryModule.Get();

    AssetRegistry.OnAssetAdded().AddUObject(this, &UCkAssetRegistrySubsystem::OnAssetAdded);
    AssetRegistry.OnAssetRemoved().AddUObject(this, &UCkAssetRegistrySubsystem::OnAssetRemoved);
    AssetRegistry.OnAssetUpdated().AddUObject(this, &UCkAssetRegistrySubsystem::OnAssetUpdated);

    ck::angelscriptgenerator::Log(TEXT("Asset registry callbacks registered for real-time config discovery"));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Deinitialize() -> void
{
    if (FModuleManager::Get().IsModuleLoaded("AssetRegistry"))
    {
        auto& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
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
    GenerateAllAssetRegistries() -> void
{
    GloballyGeneratedAssets.Reset();

    ck::angelscriptgenerator::Log(TEXT("=== Generating All Asset Registries ==="));

    auto AllConfigs = Request_DiscoverAllConfigs();

    if (AllConfigs.Num() == 0)
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
        UCkAssetRegistryConfig* InConfig) -> void
{
    GloballyGeneratedAssets.Reset();
    GenerateAssetRegistryForConfig_Internal(InConfig);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    GenerateAssetRegistryForConfig_Internal(
        UCkAssetRegistryConfig* InConfig) -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InConfig),
        TEXT("Cannot generate asset registry for invalid config"))
    { return; }

    auto RootPath = InConfig->AssetDiscoveryRoot;
    auto OutputFileName = InConfig->OutputFileName;

    ck::angelscriptgenerator::Log(TEXT("=== Generating Asset Registry for Config: {} ==="), InConfig->GetDisplayName());

    CK_ENSURE_IF_NOT(NOT RootPath.IsEmpty(),
        TEXT("Asset discovery root path is empty for config [{}]"), InConfig->GetDisplayName())
    { return; }

    auto DiscoveredAssets = Request_DiscoverAssetsInPath(RootPath);

    if (DiscoveredAssets.Num() == 0)
    {
        ck::angelscriptgenerator::Warning(TEXT("No assets found under path: {}"), RootPath);
        return;
    }

    UsedAssetNames.Reset();

    // Sort assets for consistent output
    DiscoveredAssets.Sort([](const FAssetData& A, const FAssetData& B) {
        return A.AssetName.ToString() < B.AssetName.ToString();
    });

    ck::angelscriptgenerator::Log(TEXT("Processing {} assets synchronously with async loading"), DiscoveredAssets.Num());

    // Build the content synchronously but with async asset type resolution
    auto Content = FString{};
    Content += TEXT("// Auto-generated Asset Registry\n");
    Content += TEXT("// DO NOT EDIT - This file is automatically regenerated\n");
    Content += ck::Format_UE(TEXT("// Source config: {}\n"), InConfig->GetDisplayName());
    Content += ck::Format_UE(TEXT("// Discovery root: {}\n\n"), RootPath);
    Content += ck::Format_UE(TEXT("namespace {}\n{{\n"), InConfig->Namespace);

    // Process all assets and collect the functions
    auto GeneratedFunctionCount = int32{0};
    auto SkippedAssetCount = int32{0};
    auto PendingAssets = MakeShared<int32>(0);
    auto CollectedFunctions = MakeShared<TArray<FString>>();

    CollectedFunctions->Reserve(DiscoveredAssets.Num());

    for (const auto& AssetData : DiscoveredAssets)
    {
        (*PendingAssets)++;

        Get_AssetTypeFromAssetData_Async(AssetData, FOnAssetTypeResolved::CreateLambda(
            [this, AssetData, PendingAssets, CollectedFunctions, &GeneratedFunctionCount, &SkippedAssetCount, Content, InConfig]
            (const FString& AssetType)
            {
                // Generate the function for this asset
                auto AssetFunction = FString{};

                if (AssetData.AssetClassPath.GetAssetName() != TEXT("ObjectRedirector") &&
                    NOT UCk_Utils_IO_UE::Get_IsTemporaryAsset(AssetData.AssetName.ToString()))
                {
                    auto BaseAssetName = Get_CleanAssetName(AssetData.AssetName.ToString());
                    auto AssetPath = AssetData.GetSoftObjectPath().ToString();

                    if (NOT GloballyGeneratedAssets.Contains(AssetPath) && NOT AssetType.IsEmpty())
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

                        AssetFunction += ck::Format_UE(TEXT("    TSoftObjectPtr<{}>"), AssetType);
                        AssetFunction += ck::Format_UE(TEXT(" {}() {{ return TSoftObjectPtr<{}>(FSoftObjectPath(\"{}\")); }}\n"),
                                                   FinalAssetName, AssetType, AssetPath);

                        GeneratedFunctionCount++;

                        ck::angelscriptgenerator::Log(TEXT("Generated function for {}: {}"), AssetData.AssetName, AssetType);
                    }
                    else
                    {
                        SkippedAssetCount++;
                    }
                }
                else
                {
                    SkippedAssetCount++;
                }

                CollectedFunctions->Add(AssetFunction);

                // Decrement pending counter
                (*PendingAssets)--;

                // Check if all assets are processed
                if (*PendingAssets <= 0)
                {
                    // All assets processed, finalize the file
                    auto FinalContent = Content;

                    // Add all collected functions
                    for (const auto& Function : *CollectedFunctions)
                    {
                        if (NOT Function.IsEmpty())
                        {
                            FinalContent += Function;
                        }
                    }

                    FinalContent += TEXT("}\n");

                    auto OutputDir = Get_OutputDirectoryForRootPath(InConfig->AssetDiscoveryRoot);
                    auto OutputPath = OutputDir / InConfig->OutputFileName;

                    IFileManager::Get().MakeDirectory(*OutputDir, true);

                    if (FFileHelper::SaveStringToFile(FinalContent, *OutputPath))
                    {
                        ck::angelscriptgenerator::Log(TEXT("Generated: {} with {} functions ({} assets skipped)"),
                                                     InConfig->OutputFileName, GeneratedFunctionCount, SkippedAssetCount);
                    }
                    else
                    {
                        ck::angelscriptgenerator::Warning(TEXT("Failed to write file: {}"), OutputPath);
                    }
                }
            }));
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    OnAssetAdded(
        const FAssetData& AssetData) -> void
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
        const FAssetData& AssetData) -> void
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
        const FAssetData& AssetData) -> void
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
    Request_DiscoverAllConfigs() -> TArray<UCkAssetRegistryConfig*>
{
    auto Result = TArray<UCkAssetRegistryConfig*>{};

    auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    auto& AssetRegistry = AssetRegistryModule.Get();

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
        const FString& InRootPath) -> TArray<FAssetData>
{
    auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    auto& AssetRegistry = AssetRegistryModule.Get();

    auto DiscoveredAssets = TArray<FAssetData>{};

    AssetRegistry.GetAssetsByPath(FName(*InRootPath), DiscoveredAssets, true);

    ck::angelscriptgenerator::Log(TEXT("Discovered {} assets under path: {}"),
                                 DiscoveredAssets.Num(), InRootPath);

    return DiscoveredAssets;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Get_GeneratedAssetFunction(
        const FAssetData& InAssetData) -> FString
{
    // This method is now deprecated in favor of the async approach
    // Left here for backward compatibility but should not be used
    return Get_AssetTypeFromAssetData_Immediate(InAssetData);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Get_AssetTypeFromClass(
        UClass* InAssetClass) -> FString
{
    if (NOT ck::IsValid(InAssetClass))
    { return FString{}; }

    return Get_CorrectClassNameWithPrefix(InAssetClass);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Get_CorrectClassNameWithPrefix(
        UClass* InClass) -> FString
{
    if (NOT ck::IsValid(InClass))
    { return FString{}; }

    auto ClassName = InClass->GetName();

    // Remove _C suffix if it's a Blueprint-generated class
    auto ClassNameWithoutSuffix = FBlueprintEditorUtils::GetClassNameWithoutSuffix(InClass);

    ck::angelscriptgenerator::Log(TEXT("Processing class: {} -> {} (after suffix removal) (IsActor: {})"),
        ClassName, ClassNameWithoutSuffix, InClass->IsChildOf(AActor::StaticClass()));

    // Use the string-based version with Actor check
    return Get_CorrectClassNameWithPrefix_String(ClassNameWithoutSuffix, InClass->IsChildOf(AActor::StaticClass()));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Get_CorrectClassNameWithPrefix_String(
        const FString& InClassName,
        bool bIsActor) -> FString
{
    if (InClassName.IsEmpty())
    { return FString{}; }

    ck::angelscriptgenerator::Log(TEXT("Processing class name: {} (IsActor: {})"), InClassName, bIsActor);

    // Special case for UserDefinedStruct
    if (InClassName == TEXT("UserDefinedStruct"))
    {
        return TEXT("UUserDefinedStruct");
    }

    // Determine correct prefix based on Actor flag
    if (bIsActor)
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
        const FString& InAssetName) -> FString
{
    auto Result = InAssetName;

    Result = Result.Replace(TEXT(" "), TEXT("_"));
    Result = Result.Replace(TEXT("-"), TEXT("_"));
    Result = Result.Replace(TEXT("."), TEXT("_"));

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Request_ScheduleRegeneration() -> void
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
        RegenerationDelay,
        Repeat);

    ck::angelscriptgenerator::Log(TEXT("Scheduled asset registry regeneration in {} seconds"), RegenerationDelay);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    ExecuteDelayedRegeneration() -> void
{
    ck::angelscriptgenerator::Log(TEXT("Executing delayed asset registry regeneration"));
    GenerateAllAssetRegistries();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Get_OutputDirectoryForRootPath(
        const FString& InRootPath) -> FString
{
    if (InRootPath.StartsWith(TEXT("/Game")))
    { return FPaths::ProjectDir() / TEXT("Script") / TEXT("Generated"); }

    if (InRootPath.StartsWith(TEXT("/")))
    {
        auto PathWithoutLeadingSlash = InRootPath.Mid(1);
        auto FirstSlashIndex = PathWithoutLeadingSlash.Find(TEXT("/"));

        if (FirstSlashIndex != INDEX_NONE)
        {
            auto PluginName = PathWithoutLeadingSlash.Left(FirstSlashIndex);

            auto Plugin = IPluginManager::Get().FindPlugin(PluginName);
            if (Plugin.IsValid())
            {
                auto PluginDir = Plugin->GetBaseDir();
                return PluginDir / TEXT("Script") / TEXT("Generated");
            }
            else
            {
                ck::angelscriptgenerator::Warning(TEXT("Plugin [{}] not found for root path [{}], using project directory"),
                                                 PluginName, InRootPath);
            }
        }
    }

    ck::angelscriptgenerator::Warning(TEXT("Unrecognized root path format [{}], using project directory"), InRootPath);
    return FPaths::ProjectDir() / TEXT("Script") / TEXT("Generated");
}

// --------------------------------------------------------------------------------------------------------------------
