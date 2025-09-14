#include "CkAssetRegistrySubsystem.h"

#include "CkAssetRegistryConfig.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"

#include "CkCore/IO/CkIO_Utils.h"

#include <AssetRegistry/AssetRegistryModule.h>
#include <Engine/Engine.h>
#include <Engine/UserDefinedStruct.h>
#include <HAL/FileManager.h>
#include <Interfaces/IPluginManager.h>
#include <Misc/FileHelper.h>
#include <Misc/Paths.h>
#include <TimerManager.h>

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

        GenerateAssetRegistryForConfig(Config);
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

    auto OutputDir = Get_OutputDirectoryForRootPath(RootPath);
    auto OutputPath = OutputDir / OutputFileName;

    IFileManager::Get().MakeDirectory(*OutputDir, true);

    auto Content = FString{};
    Content += TEXT("// Auto-generated Asset Registry\n");
    Content += TEXT("// DO NOT EDIT - This file is automatically regenerated\n");
    Content += ck::Format_UE(TEXT("// Source config: {}\n"), InConfig->GetDisplayName());
    Content += ck::Format_UE(TEXT("// Discovery root: {}\n\n"), RootPath);

    Content += ck::Format_UE(TEXT("namespace {}\n{{\n"), InConfig->Namespace);

    auto GeneratedFunctionCount = int32{0};
    auto SkippedAssetCount = int32{0};

    UsedAssetNames.Reset();

    DiscoveredAssets.Sort([](const FAssetData& A, const FAssetData& B) {
        return A.AssetName.ToString() < B.AssetName.ToString();
    });

    auto TotalAssets = DiscoveredAssets.Num();
    auto ProcessedAssets = int32{0};

    while (ProcessedAssets < TotalAssets)
    {
        auto BatchEnd = FMath::Min(ProcessedAssets + AssetProcessingBatchSize, TotalAssets);

        for (auto I = ProcessedAssets; I < BatchEnd; I++)
        {
            const auto& AssetData = DiscoveredAssets[I];
            auto AssetFunction = Get_GeneratedAssetFunction(AssetData);

            if (NOT AssetFunction.IsEmpty())
            {
                Content += AssetFunction;
                GeneratedFunctionCount++;
            }
            else
            {
                SkippedAssetCount++;
            }
        }

        ProcessedAssets = BatchEnd;

        if (ProcessedAssets < TotalAssets)
        {
            constexpr auto YieldTimeSeconds = 0.001f;
            FPlatformProcess::Sleep(YieldTimeSeconds);
        }
    }

    Content += TEXT("}\n");

    if (FFileHelper::SaveStringToFile(Content, *OutputPath))
    {
        ck::angelscriptgenerator::Log(TEXT("Generated: {} with {} functions ({} assets skipped)"),
                                     OutputFileName, GeneratedFunctionCount, SkippedAssetCount);
    }
    else
    {
        ck::angelscriptgenerator::Warning(TEXT("Failed to write file: {}"), OutputPath);
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
    if (InAssetData.AssetClassPath.GetAssetName() == TEXT("ObjectRedirector"))
    { return FString{}; }

    auto BaseAssetName = Get_CleanAssetName(InAssetData.AssetName.ToString());

    if (UCk_Utils_IO_UE::Get_IsTemporaryAsset(BaseAssetName))
    { return FString{}; }

    auto AssetType = Get_AssetTypeFromClass(InAssetData.GetClass());
    auto AssetPath = InAssetData.GetSoftObjectPath().ToString();

    if (AssetType.IsEmpty())
    {
        ck::angelscriptgenerator::Warning(TEXT("Could not determine asset type for: {}"), BaseAssetName);
        return FString{};
    }

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

    auto Result = FString{};
    Result += ck::Format_UE(TEXT("    TSoftObjectPtr<{}>"), AssetType);
    Result += ck::Format_UE(TEXT(" {}() {{ return TSoftObjectPtr<{}>(FSoftObjectPath(\"{}\")); }}\n"),
                           FinalAssetName, AssetType, AssetPath);

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkAssetRegistrySubsystem::
    Get_AssetTypeFromClass(
        UClass* InAssetClass) -> FString
{
    if (NOT ck::IsValid(InAssetClass))
    { return FString{}; }

    if (auto CachedType = AssetTypeCache.Find(InAssetClass))
    { return *CachedType; }

    auto ClassName = InAssetClass->GetName();

    if (InAssetClass->IsChildOf(UUserDefinedStruct::StaticClass()))
    { ClassName = TEXT("UUserDefinedStruct"); }
    else if (NOT ClassName.StartsWith(TEXT("U")))
    { ClassName = TEXT("U") + ClassName; }

    AssetTypeCache.Add(InAssetClass, ClassName);
    return ClassName;
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
