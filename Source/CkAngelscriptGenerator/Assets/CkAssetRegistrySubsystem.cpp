#include "CkAssetRegistrySubsystem.h"

#include "CkAssetRegistryConfig.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"

#include "CkCore/IO/CkIO_Utils.h"
#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"

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
    Get_AssetTypeFromAssetData(
        const FAssetData& InAssetData,
        const FOnAssetTypeResolved& OnResolved)
    -> void
{
    ck::angelscriptgenerator::Log(TEXT("Get_AssetTypeFromAssetData called for: {}"), InAssetData.AssetName);

    if (InAssetData.AssetClassPath.GetAssetName() == BLUEPRINT_CLASS_NAME)
    {
        ck::angelscriptgenerator::Log(TEXT("Loading Blueprint asset asynchronously: {}"), InAssetData.AssetName);

        auto AssetPath = InAssetData.GetSoftObjectPath();
        auto AssetName = InAssetData.AssetName.ToString();

        auto LoadHandle = StreamableManager.RequestAsyncLoad(
            AssetPath,
            FStreamableDelegate::CreateLambda([this, AssetName, OnResolved, AssetPath]()
            {
                auto LoadedAsset = AssetPath.ResolveObject();

                if (NOT ck::IsValid(LoadedAsset))
                {
                    ck::angelscriptgenerator::Warning(TEXT("Failed to load Blueprint asset: {}"), AssetName);

                    auto MessageSegments = FCk_MessageSegments{
                        {FCk_TokenizedMessage{ck::Format_UE(TEXT("Failed to load Blueprint asset: {}"), AssetName)}}
                    };
                    auto Params = FCk_Utils_EditorOnly_PushNewEditorMessage_Params{
                        TEXT("AssetRegistry"), MessageSegments
                    };
                    Params.Set_MessageSeverity(ECk_EditorMessage_Severity::Warning);
                    UCk_Utils_EditorOnly_UE::Request_PushNewEditorMessage(Params);

                    OnResolved.ExecuteIfBound(FString{});
                    return;
                }

                if (auto LoadedBlueprint = Cast<UBlueprint>(LoadedAsset))
                {
                    auto ParentClass = LoadedBlueprint->ParentClass;
                    if (NOT ck::IsValid(ParentClass))
                    {
                        ck::angelscriptgenerator::Warning(TEXT("Blueprint has no parent class: {}"), AssetName);
                        OnResolved.ExecuteIfBound(FString{});
                        return;
                    }

                    auto NativeParentClass = Get_NativeParentClass(ParentClass);
                    if (ck::IsValid(NativeParentClass))
                    {
                        auto Result = Get_CorrectClassNameWithPrefix(NativeParentClass);
                        ck::angelscriptgenerator::Log(TEXT("Resolved Blueprint parent class: {} for {}"), Result, AssetName);
                        OnResolved.ExecuteIfBound(Result);
                    }
                    else
                    {
                        ck::angelscriptgenerator::Warning(TEXT("Could not find native parent class for: {}"), AssetName);
                        OnResolved.ExecuteIfBound(FString{});
                    }
                }
                else
                {
                    ck::angelscriptgenerator::Warning(TEXT("Asset is not a Blueprint: {}"), AssetName);
                    OnResolved.ExecuteIfBound(FString{});
                }
            })
        );

        if (NOT LoadHandle.IsValid())
        {
            ck::angelscriptgenerator::Warning(TEXT("Failed to start async load for Blueprint: {}"), AssetName);
            OnResolved.ExecuteIfBound(FString{});
        }
    }
    else
    {
        ck::angelscriptgenerator::Log(TEXT("Non-Blueprint asset, using class-based resolution"));

        auto AssetClass = InAssetData.GetClass();
        if (ck::IsValid(AssetClass))
        {
            auto Result = Get_CorrectClassNameWithPrefix(AssetClass);
            OnResolved.ExecuteIfBound(Result);
        }
        else
        {
            ck::angelscriptgenerator::Warning(TEXT("Could not get class for non-Blueprint asset: {}"), InAssetData.AssetName);
            OnResolved.ExecuteIfBound(FString{});
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Get_NativeParentClass(
        UClass* InClass)
    -> UClass*
{
    if (NOT ck::IsValid(InClass))
    { return nullptr; }

    auto CurrentClass = InClass;

    while (ck::IsValid(CurrentClass))
    {
        ck::angelscriptgenerator::Log(TEXT("Checking class: {} (HasAnyClassFlags(CLASS_CompiledFromBlueprint): {})"),
            CurrentClass->GetName(), CurrentClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint));

        if (NOT CurrentClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint))
        {
            ck::angelscriptgenerator::Log(TEXT("Found native class: {}"), CurrentClass->GetName());
            return CurrentClass;
        }

        CurrentClass = CurrentClass->GetSuperClass();
    }

    ck::angelscriptgenerator::Warning(TEXT("Could not find native parent class - reached top of hierarchy"));
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
    Deinitialize()
    -> void
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
    GenerateAllAssetRegistries()
    -> void
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
        OnAssetRegistryComplete.Broadcast(0, 0, 0);
        return;
    }

    UsedAssetNames.Reset();

    DiscoveredAssets.Sort([](const FAssetData& A, const FAssetData& B) {
        return A.AssetName.ToString() < B.AssetName.ToString();
    });

    auto TotalAssets = DiscoveredAssets.Num();
    ck::angelscriptgenerator::Log(TEXT("Processing {} assets with async loading"), TotalAssets);

    auto Content = BuildFileHeader(InConfig, RootPath);
    auto GeneratedFunctionCount = MakeShared<int32>(0);
    auto SkippedAssetCount = MakeShared<int32>(0);
    auto ProcessedAssetCount = MakeShared<int32>(0);
    auto PendingAssets = MakeShared<int32>(0);
    auto CollectedFunctions = MakeShared<TArray<FString>>();

    CollectedFunctions->Reserve(TotalAssets);

    for (const auto& AssetData : DiscoveredAssets)
    {
        (*PendingAssets)++;

        Get_AssetTypeFromAssetData(AssetData, FOnAssetTypeResolved::CreateLambda(
            [this, AssetData, PendingAssets, CollectedFunctions, GeneratedFunctionCount, SkippedAssetCount, ProcessedAssetCount, Content, InConfig, TotalAssets]
            (const FString& AssetType)
            {
                auto AssetFunction = FString{};

                if (AssetData.AssetClassPath.GetAssetName() != OBJECT_REDIRECTOR_CLASS &&
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

                        (*GeneratedFunctionCount)++;

                        ck::angelscriptgenerator::Log(TEXT("Generated function for {}: {}"), AssetData.AssetName, AssetType);
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

                // Report progress
                OnAssetRegistryProgress.Broadcast(*ProcessedAssetCount, TotalAssets);

                if (*PendingAssets <= 0)
                {
                    auto FinalContent = Content;

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
                                                     InConfig->OutputFileName, *GeneratedFunctionCount, *SkippedAssetCount);
                    }
                    else
                    {
                        ck::angelscriptgenerator::Warning(TEXT("Failed to write file: {}"), OutputPath);
                    }

                    // Report completion
                    OnAssetRegistryComplete.Broadcast(*GeneratedFunctionCount, *SkippedAssetCount, TotalAssets);
                }
            }));
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
        const FString& InRootPath)
    -> TArray<FAssetData>
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
    Get_AssetTypeFromClass(
        UClass* InAssetClass)
    -> FString
{
    if (NOT ck::IsValid(InAssetClass))
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
    if (NOT ck::IsValid(InClass))
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
        auto Char = InAssetName[i];
        if (FChar::IsAlnum(Char) || Char == TEXT('_'))
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
    Content += ck::Format_UE(TEXT("namespace {}\n{{\n"), InConfig->Namespace);
    return Content;
}

// --------------------------------------------------------------------------------------------------------------------
