#include "CkIO_Utils.h"

#include "CkCore/CkCoreLog.h"
#include "CkCore/Ensure/CkEnsure.h"

#include <CoreGlobals.h>
#include <Misc/CommandLine.h>
#include <Interfaces/IPluginManager.h>
#include <Engine/AssetManager.h>
#include <Engine/Engine.h>
#include <Misc/ConfigCacheIni.h>
#include <Misc/CoreDelegates.h>
#include <Runtime/Engine/Classes/Kismet/BlueprintPathsLibrary.h>

// --------------------------------------------------------------------------------------------------------------------
// The blocking-load safety flag flips once FEngineLoop::Init() completes and every subsystem
// (UTypedElementRegistry included) exists. Before that, System::LoadAsset_Blocking can crash on
// packages containing UActorComponent exports.
// --------------------------------------------------------------------------------------------------------------------

namespace ck_io_utils
{
    bool GIsEngineSafeForBlockingLoads          = false;
    bool GBlockingLoadWasQueriedWhileUnsafe     = false;

    // ---- Premature-load diagnostic aggregator state ----
    int32   GPrematureAssetLoadCount        = 0;
    FString GFirstPrematureAssetLoadMessage;

    struct FBlockingLoadSafetyRegistrar
    {
        FBlockingLoadSafetyRegistrar()
        {
            // A late module load (plugin load, editor hot-reload, DLL reload) can miss
            // OnFEngineLoopInitComplete entirely, leaving the flag false forever — catch it up front.
            if (GEngine != nullptr && GIsRunning)
            {
                GIsEngineSafeForBlockingLoads = true;
                return;
            }

            FCoreDelegates::OnFEngineLoopInitComplete.AddLambda([]()
            {
                GIsEngineSafeForBlockingLoads = true;
            });
        }
    };

    static FBlockingLoadSafetyRegistrar GBlockingLoadSafetyRegistrar;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IO_UE::
    IsEngineSafeForBlockingLoads()
    -> bool
{
    if (NOT ck_io_utils::GIsEngineSafeForBlockingLoads)
    { ck_io_utils::GBlockingLoadWasQueriedWhileUnsafe = true; }

    return ck_io_utils::GIsEngineSafeForBlockingLoads;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IO_UE::
    Get_IsRunningCommandlet()
    -> bool
{
    // IsRunningCommandlet()'s flag is only set during PreInit, and very-early Angelscript CDO
    // construction runs before that — so also sniff the command line, which is populated from process
    // start. Keeps the diagnostic loud in editor/PIE/game while never failing a cook.
    if (::IsRunningCommandlet() || ::IsRunningCookCommandlet())
    { return true; }

    // Deliberately re-read every call: the first early-init query can precede a fully populated
    // command line, and caching that read would stick at false for the rest of the cook.
    const auto CmdLine = FString{FCommandLine::Get()};
    return CmdLine.Contains(TEXT("-run="), ESearchCase::IgnoreCase)
        || CmdLine.Contains(TEXT("-TargetPlatform="), ESearchCase::IgnoreCase);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IO_UE::
    MarkEngineSafeForBlockingLoads()
    -> void
{
    ck_io_utils::GIsEngineSafeForBlockingLoads = true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IO_UE::
    WasBlockingLoadQueriedWhileUnsafe()
    -> bool
{
    return ck_io_utils::GBlockingLoadWasQueriedWhileUnsafe;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IO_UE::
    Get_IsEngineSafeForBlockingLoads_Peek()
    -> bool
{
    return ck_io_utils::GIsEngineSafeForBlockingLoads;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IO_UE::
    Report_PrematureAssetLoad(
        const FString& InMessage)
    -> void
{
    if (ck_io_utils::GPrematureAssetLoadCount == 0)
    { ck_io_utils::GFirstPrematureAssetLoadMessage = InMessage; }

    ++ck_io_utils::GPrematureAssetLoadCount;
}

auto
    UCk_Utils_IO_UE::
    Get_PrematureAssetLoadCount()
    -> int32
{
    return ck_io_utils::GPrematureAssetLoadCount;
}

auto
    UCk_Utils_IO_UE::
    Get_FirstPrematureAssetLoadMessage()
    -> FString
{
    return ck_io_utils::GFirstPrematureAssetLoadMessage;
}

auto
    UCk_Utils_IO_UE::
    Reset_PrematureAssetLoadReport()
    -> void
{
    ck_io_utils::GPrematureAssetLoadCount = 0;
    ck_io_utils::GFirstPrematureAssetLoadMessage.Reset();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IO_UE::
    Get_Engine_DefaultTextFont(
        ECk_Engine_TextFontSize InFontSize)
    -> UFont*
{
    if (ck::Is_NOT_Valid(GEngine))
    { return {}; }

    switch (InFontSize)
    {
        case ECk_Engine_TextFontSize::Subtitle:
        {
            return GEngine->GetSubtitleFont();
        }
        case ECk_Engine_TextFontSize::Tiny:
        {
            return GEngine->GetTinyFont();
        }
        case ECk_Engine_TextFontSize::Small:
        {
            return GEngine->GetSmallFont();
        }
        case ECk_Engine_TextFontSize::Medium:
        {
            return GEngine->GetMediumFont();
        }
        case ECk_Engine_TextFontSize::Large:
        {
            return GEngine->GetLargeFont();
        }
        default:
        {
            CK_INVALID_ENUM(InFontSize);
            return {};
        }
    }
}

auto
    UCk_Utils_IO_UE::
    Get_ProjectVersion()
    -> FString
{
    if (ck::Is_NOT_Valid(GConfig, ck::IsValid_Policy_NullptrOnly{}))
    { return {}; }

    FString ProjectVersion;
    GConfig->GetString
    (
        TEXT("/Script/EngineSettings.GeneralProjectSettings"),
        TEXT("ProjectVersion"),
        ProjectVersion,
        GGameIni
    );

    return ProjectVersion;
}

auto
    UCk_Utils_IO_UE::
    Get_ProjectDir()
    -> FString
{
    return UBlueprintPathsLibrary::ProjectDir();
}

auto
    UCk_Utils_IO_UE::
    Get_PluginsDir(
        const FString& InPluginName)
    -> FString
{
    const auto& PluginInstalledInGame   = Get_ProjectPluginsDir() + InPluginName;
    const auto& PluginInstalledInEngine = Get_EnginePluginsDir()  + InPluginName;

    return UBlueprintPathsLibrary::DirectoryExists(PluginInstalledInGame)
        ? PluginInstalledInGame
        : PluginInstalledInEngine;
}

auto
    UCk_Utils_IO_UE::
    Get_ProjectContentDir()
    -> FString
{
    return UBlueprintPathsLibrary::ProjectContentDir();
}

auto
    UCk_Utils_IO_UE::
    Get_ProjectPluginsDir()
    -> FString
{
    return UBlueprintPathsLibrary::ProjectPluginsDir();
}

auto
    UCk_Utils_IO_UE::
    Get_EnginePluginsDir()
    -> FString
{
    return UBlueprintPathsLibrary::EnginePluginsDir();
}

auto
    UCk_Utils_IO_UE::
    Get_AssetInfoFromPath(
        const FString& InAssetPath)
    -> FCk_Utils_IO_AssetInfoFromPath_Result
{
    const auto& AssetManager = UAssetManager::Get();

    FAssetData AssetData;
    const auto& AssetFound = AssetManager.GetAssetDataForPath(InAssetPath, AssetData);

    const auto AssetInfo = FCk_Utils_IO_AssetInfoFromPath_Result{}
                           .Set_AssetFound(AssetFound)
                           .Set_AssetData(AssetData);

    return AssetInfo;
}

auto
    UCk_Utils_IO_UE::
    Get_AssetLocalRoot(
        const FString& InAssetPath)
    -> ECk_AssetLocalRootType
{
    auto PackageRoot = FString{};
    auto PackagePath = FString{};
    auto PackageName = FString{};

    constexpr auto StripRootLeadingSlash = true;
    if (FPackageName::SplitLongPackageName(InAssetPath, PackageRoot, PackagePath, PackageName, StripRootLeadingSlash))
    {
        constexpr auto GameRootName = TEXT("Game/");
        constexpr auto EngineRootName = TEXT("Engine/");
        constexpr auto ScriptRootName = TEXT("Script/");

        const auto IsInGame = PackageRoot == GameRootName;

        if (IsInGame)
        { return ECk_AssetLocalRootType::Project; }

        const auto IsInEngine = PackageRoot == EngineRootName;
        const auto IsInPlugin = NOT IsInGame && NOT IsInEngine && PackageRoot != ScriptRootName;

        if (IsInEngine && NOT IsInPlugin)
        { return ECk_AssetLocalRootType::Engine; }

        auto PluginName = PackageRoot;
        PluginName.RemoveFromEnd(TEXT("/"));

        if (const auto& Plugin = IPluginManager::Get().FindPlugin(PluginName);
            Plugin.IsValid())
        {
            switch (Plugin->GetLoadedFrom())
            {
                case EPluginLoadedFrom::Engine: return ECk_AssetLocalRootType::EnginePlugin;
                case EPluginLoadedFrom::Project: return ECk_AssetLocalRootType::ProjectPlugin;
            }
        }
    }

    return ECk_AssetLocalRootType::Invalid;
}

auto
    UCk_Utils_IO_UE::
    Get_ExtractPath(
        const FString& InFullPath)
    -> FString
{
    return FPaths::GetPath(InFullPath);
}

auto
    UCk_Utils_IO_UE::
    Get_SoftObjectAssetName(
        const TSoftObjectPtr<>& InSoftObject)
    -> FString
{
    return InSoftObject.GetAssetName();
}

auto
    UCk_Utils_IO_UE::
    Get_SoftObjectAssetPath(
        const TSoftObjectPtr<>& InSoftObject)
    -> FString
{
    return InSoftObject.ToSoftObjectPath().GetAssetPathString();
}

auto
    UCk_Utils_IO_UE::
    Get_IsTemporaryAsset(const FString& InAssetName)
    -> bool
{
    return InAssetName.StartsWith(TEXT("REINST_")) ||
           InAssetName.StartsWith(TEXT("SKEL_")) ||
           InAssetName.StartsWith(TEXT("TRASHCLASS_")) ||
           InAssetName.StartsWith(TEXT("DEADCLASS_")) ||
           InAssetName.StartsWith(TEXT("LIVECODING_")) ||
           InAssetName.Contains(TEXT("_INST_")) ||
           InAssetName.Contains(TEXT("_REPLACED_"));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IO_UE::
    LoadAssetsByName(
        const FString& AssetName,
        ECk_AssetSearchScope SearchScope,
        ECk_AssetSearchStrategy SearchStrategy)
    -> FCk_Utils_Object_AssetSearchResult_Array
{
    return DoLoadAssetsByName(AssetName, nullptr, SearchScope, SearchStrategy);
}

auto
    UCk_Utils_IO_UE::
    LoadAssetsByName_FilterByClass(
        const FString& AssetName,
        TSubclassOf<UObject> AssetClass,
        ECk_AssetSearchScope SearchScope,
        ECk_AssetSearchStrategy SearchStrategy)
    -> FCk_Utils_Object_AssetSearchResult_Array
{
    CK_ENSURE_IF_NOT(ck::IsValid(AssetClass), TEXT("Asset class cannot be null"))
    {
        return FCk_Utils_Object_AssetSearchResult_Array{};
    }

    return DoLoadAssetsByName(AssetName, AssetClass.Get(), SearchScope, SearchStrategy);
}

auto
    UCk_Utils_IO_UE::
    LoadAssetByName(
        const FString& AssetName,
        ECk_AssetSearchScope SearchScope,
        ECk_AssetSearchStrategy SearchStrategy)
    -> FCk_Utils_Object_AssetSearchResult_Single
{
    const auto& ArrayResult = LoadAssetsByName(AssetName, SearchScope, SearchStrategy);

    auto SingleResult = FCk_Utils_Object_AssetSearchResult_Single{};

    if (ArrayResult.Get_Results().Num() > 0)
    {
        const auto& FirstResult = ArrayResult.Get_Results()[0];
        SingleResult._Asset = FirstResult.Get_Asset();
        SingleResult._AssetName = FirstResult.Get_AssetName();
        SingleResult._AssetPath = FirstResult.Get_AssetPath();
        SingleResult._MatchType = FirstResult.Get_MatchType();
        SingleResult._UniqueAsset = ArrayResult.Get_Results().Num() == 1 ? ECk_Unique::Unique : ECk_Unique::NotUnique;
    }
    else
    {
        SingleResult._UniqueAsset = ECk_Unique::DoesNotExist;
    }

    return SingleResult;
}

auto
    UCk_Utils_IO_UE::
    LoadAssetByName_FilterByClass(
        const FString& AssetName,
        TSubclassOf<UObject> AssetClass,
        ECk_AssetSearchScope SearchScope,
        ECk_AssetSearchStrategy SearchStrategy)
    -> FCk_Utils_Object_AssetSearchResult_Single
{
    const auto& ArrayResult = LoadAssetsByName_FilterByClass(AssetName, AssetClass, SearchScope, SearchStrategy);

    auto SingleResult = FCk_Utils_Object_AssetSearchResult_Single{};

    if (ArrayResult.Get_Results().Num() > 0)
    {
        const auto& FirstResult = ArrayResult.Get_Results()[0];
        SingleResult._Asset = FirstResult.Get_Asset();
        SingleResult._AssetName = FirstResult.Get_AssetName();
        SingleResult._AssetPath = FirstResult.Get_AssetPath();
        SingleResult._MatchType = FirstResult.Get_MatchType();
        SingleResult._UniqueAsset = ArrayResult.Get_Results().Num() == 1 ? ECk_Unique::Unique : ECk_Unique::NotUnique;
    }
    else
    {
        SingleResult._UniqueAsset = ECk_Unique::DoesNotExist;
    }

    return SingleResult;
}

auto
    UCk_Utils_IO_UE::
    FindAssetsByName(
        const FString& AssetName,
        ECk_AssetSearchScope SearchScope)
    -> TArray<FString>
{
    auto Results = TArray<FString>{};

    if (AssetName.IsEmpty())
    {
        ck::core::Warning(TEXT("FindAssetsByName: Asset name cannot be empty"));
        return Results;
    }

    auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    auto& AssetRegistry = AssetRegistryModule.Get();

    const auto& SearchPaths = Get_SearchPaths(SearchScope);

    auto Filter = FARFilter{};
    Filter.bRecursivePaths = true;
    Filter.PackagePaths = SearchPaths;
    // In-memory asset enumeration is game-thread-only (EnumerateMemoryAssetsHelper asserts off it), and
    // these lookups can run off it during threaded AngelScript init.
    Filter.bIncludeOnlyOnDiskAssets = NOT IsInGameThread();

    auto AssetDataList = TArray<FAssetData>{};
    AssetRegistry.GetAssets(Filter, AssetDataList);

    for (const auto& AssetData : AssetDataList)
    {
        if (const auto& CurrentAssetName = AssetData.AssetName.ToString();
            CurrentAssetName.Contains(AssetName, ESearchCase::IgnoreCase))
        {
            const auto& AssetInfo = ck::Format_UE(TEXT("{} ({})"), CurrentAssetName, AssetData);
            Results.Add(AssetInfo);
        }
    }

    return Results;
}

auto
    UCk_Utils_IO_UE::
    DoLoadAssetsByName(
        const FString& AssetName,
        UClass* AssetClass,
        ECk_AssetSearchScope SearchScope,
        ECk_AssetSearchStrategy SearchStrategy)
    -> FCk_Utils_Object_AssetSearchResult_Array
{
    if (AssetName.IsEmpty())
    {
        ck::core::Error(TEXT("DoLoadAssetsByName: Asset name cannot be empty"));
        return FCk_Utils_Object_AssetSearchResult_Array{};
    }

    if (SearchStrategy == ECk_AssetSearchStrategy::ExactOnly)
    {
        return DoFastExactLookup(AssetName, AssetClass, SearchScope);
    }

    if (SearchStrategy == ECk_AssetSearchStrategy::ExactThenFuzzy)
    {
        if (auto ExactResults = DoFastExactLookup(AssetName, AssetClass, SearchScope);
            ExactResults.Get_Results().Num() > 0)
        {
            return ExactResults;
        }

        return DoFuzzySearch(AssetName, AssetClass, SearchScope);
    }

    return DoFullAssetScan(AssetName, AssetClass, SearchScope, SearchStrategy);
}

auto
    UCk_Utils_IO_UE::
    DoFastExactLookup(
        const FString& AssetName,
        UClass* AssetClass,
        ECk_AssetSearchScope SearchScope)
    -> FCk_Utils_Object_AssetSearchResult_Array
{
    auto Result = FCk_Utils_Object_AssetSearchResult_Array{};

    const auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    const auto& AssetRegistry = AssetRegistryModule.Get();

    auto FoundAssets = TArray<FAssetData>{};

    if (AssetName.StartsWith(TEXT("/")))
    {
        ck::core::VeryVerbose(TEXT("DoFastExactLookup: Attempting direct path lookup for: '{}'"), AssetName);

        const auto& AssetPath = FSoftObjectPath(AssetName);
        const auto& AssetData = AssetRegistry.GetAssetByObjectPath(AssetPath);

        ck::core::VeryVerbose(TEXT("DoFastExactLookup: Direct path lookup - Valid: {}"), ck::IsValid(AssetData));

        if (AssetData.IsValid())
        {
            // An exact path skips scope validation — the caller already said where the asset is.
            auto ClassMatches = true;
            if (ck::IsValid(AssetClass, ck::IsValid_Policy_NullptrOnly{}))
            {
                if (NOT AssetData.GetClass()->IsChildOf(AssetClass))
                {
                    ck::core::Warning(TEXT("DoFastExactLookup: Asset '{}' found but wrong class"), AssetName);
                    ClassMatches = false;
                }
            }

            if (ClassMatches)
            {
                if (auto LoadedAsset = AssetData.GetAsset();
                    ck::IsValid(LoadedAsset, ck::IsValid_Policy_NullptrOnly{}))
                {
                    auto SingleResult = FCk_Utils_Object_AssetSearchResult_Single{};
                    SingleResult._Asset = LoadedAsset;
                    SingleResult._AssetName = AssetData.AssetName.ToString();
                    SingleResult._AssetPath = AssetData.GetSoftObjectPath().ToString();
                    SingleResult._MatchType = ECk_AssetMatchType::ExactMatch;
                    SingleResult._UniqueAsset = ECk_Unique::Unique;

                    Result._Results.Add(SingleResult);
                    Result._ExactMatchCount = 1;
                    Result._FuzzyMatchCount = 0;

                    ck::core::VeryVerbose(TEXT("DoFastExactLookup: FAST PATH - Successfully loaded asset '{}' from direct path '{}'"), LoadedAsset, AssetName);

                    return Result;
                }
            }
        }
    }

    if (FoundAssets.IsEmpty())
    {
        auto NameFilter = FARFilter{};
        NameFilter.bRecursivePaths = true;
        NameFilter.PackagePaths = Get_SearchPaths(SearchScope);
        // On-disk-only off the game thread — the path that crashed during threaded AS init.
        NameFilter.bIncludeOnlyOnDiskAssets = NOT IsInGameThread();

        if (ck::IsValid(AssetClass, ck::IsValid_Policy_NullptrOnly{}))
        {
            NameFilter.ClassPaths.Add(AssetClass->GetClassPathName());
        }

        auto AllAssets = TArray<FAssetData>{};
        AssetRegistry.GetAssets(NameFilter, AllAssets);

        for (const auto& AssetData : AllAssets)
        {
            if (AssetData.AssetName.ToString().Equals(AssetName, ESearchCase::IgnoreCase))
            {
                FoundAssets.Add(AssetData);
            }
        }

        ck::core::VeryVerbose(TEXT("DoFastExactLookup: Found {} assets by name lookup for '{}'"), FoundAssets.Num(), AssetName);
    }

    const auto& SearchPaths = Get_SearchPaths(SearchScope);
    auto FilteredAssets = TArray<FAssetData>{};

    for (const auto& AssetData : FoundAssets)
    {
        auto AssetInScope = false;
        const auto& AssetPath = AssetData.PackageName.ToString();

        for (const auto& SearchPath : SearchPaths)
        {
            if (AssetPath.StartsWith(SearchPath.ToString()))
            {
                AssetInScope = true;
                break;
            }
        }

        if (NOT AssetInScope)
        {
            continue;
        }

        if (ck::IsValid(AssetClass, ck::IsValid_Policy_NullptrOnly{}))
        {
            if (NOT AssetData.GetClass()->IsChildOf(AssetClass))
            { continue; }
        }

        FilteredAssets.Add(AssetData);
    }

    for (const auto& AssetData : FilteredAssets)
    {
        if (auto LoadedAsset = AssetData.GetAsset();
            ck::IsValid(LoadedAsset, ck::IsValid_Policy_NullptrOnly{}))
        {
            auto SingleResult = FCk_Utils_Object_AssetSearchResult_Single{};
            SingleResult._Asset = LoadedAsset;
            SingleResult._AssetName = AssetData.AssetName.ToString();
            SingleResult._AssetPath = AssetData.GetSoftObjectPath().ToString();
            SingleResult._MatchType = ECk_AssetMatchType::ExactMatch;
            SingleResult._UniqueAsset = ECk_Unique::Unique; // Will be overridden by caller

            Result._Results.Add(SingleResult);

            ck::core::VeryVerbose(TEXT("DoFastExactLookup: Successfully loaded asset '{}' from path '{}'"), LoadedAsset, AssetData);
        }
        else
        {
            ck::core::Error(TEXT("DoFastExactLookup: Failed to load asset from path '{}'"), AssetData);
        }
    }

    Result._ExactMatchCount = Result._Results.Num();
    Result._FuzzyMatchCount = 0;

    return Result;
}

auto
    UCk_Utils_IO_UE::
    DoFuzzySearch(
        const FString& AssetName,
        UClass* AssetClass,
        ECk_AssetSearchScope SearchScope)
    -> FCk_Utils_Object_AssetSearchResult_Array
{
    return DoFullAssetScan(AssetName, AssetClass, SearchScope, ECk_AssetSearchStrategy::FuzzyOnly);
}

auto
    UCk_Utils_IO_UE::
    DoFullAssetScan(
        const FString& AssetName,
        UClass* AssetClass,
        ECk_AssetSearchScope SearchScope,
        ECk_AssetSearchStrategy SearchStrategy)
    -> FCk_Utils_Object_AssetSearchResult_Array
{
    auto Result = FCk_Utils_Object_AssetSearchResult_Array{};

    auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    auto& AssetRegistry = AssetRegistryModule.Get();

    if (NOT AssetRegistry.IsLoadingAssets())
    {
        AssetRegistry.SearchAllAssets(true);
    }

    const auto& SearchPaths = Get_SearchPaths(SearchScope);

    auto Filter = FARFilter{};
    Filter.bRecursivePaths = true;
    Filter.PackagePaths = SearchPaths;
    // On-disk-only off the game thread avoids the AssetRegistry's game-thread assert.
    Filter.bIncludeOnlyOnDiskAssets = NOT IsInGameThread();

    if (ck::IsValid(AssetClass, ck::IsValid_Policy_NullptrOnly{}))
    {
        Filter.ClassPaths.Add(AssetClass->GetClassPathName());
    }

    auto AssetDataList = TArray<FAssetData>{};
    AssetRegistry.GetAssets(Filter, AssetDataList);

    if (EnumHasAnyFlags(SearchScope, ECk_AssetSearchScope::Plugins))
    {
        ck::core::VeryVerbose(TEXT("DoFullAssetScan: Searching for '{}' in {} total assets across search paths"), AssetName, AssetDataList.Num());

        for (const auto& Path : SearchPaths)
        {
            auto PathFilter = FARFilter{};
            PathFilter.bRecursivePaths = true;
            PathFilter.PackagePaths.Add(Path);
            PathFilter.bIncludeOnlyOnDiskAssets = NOT IsInGameThread();

            auto PathAssets = TArray<FAssetData>{};
            AssetRegistry.GetAssets(PathFilter, PathAssets);

            ck::core::VeryVerbose(TEXT("DoFullAssetScan: Found {} assets in path '{}'"), PathAssets.Num(), Path);
        }
    }

    auto ExactMatches = TArray<FAssetData>{};
    auto FuzzyMatches = TArray<FAssetData>{};

    for (const auto& AssetData : AssetDataList)
    {
        if (const auto& CurrentAssetName = AssetData.AssetName.ToString();
            CurrentAssetName.Equals(AssetName, ESearchCase::IgnoreCase))
        {
            ExactMatches.Add(AssetData);
        }
        else if (CurrentAssetName.Contains(AssetName, ESearchCase::IgnoreCase))
        {
            FuzzyMatches.Add(AssetData);
        }
    }

    auto AssetsToLoad = TArray<FAssetData>{};
    auto ExactMatchesToLoad = TArray<FAssetData>{};
    auto FuzzyMatchesToLoad = TArray<FAssetData>{};

    switch (SearchStrategy)
    {
        case ECk_AssetSearchStrategy::ExactOnly:
        {
            AssetsToLoad = ExactMatches;
            ExactMatchesToLoad = ExactMatches;
            break;
        }
        case ECk_AssetSearchStrategy::FuzzyOnly:
        {
            AssetsToLoad = FuzzyMatches;
            FuzzyMatchesToLoad = FuzzyMatches;
            break;
        }
        case ECk_AssetSearchStrategy::ExactThenFuzzy:
        {
            if (ExactMatches.Num() > 0)
            {
                AssetsToLoad = ExactMatches;
                ExactMatchesToLoad = ExactMatches;
            }
            else
            {
                AssetsToLoad = FuzzyMatches;
                FuzzyMatchesToLoad = FuzzyMatches;
            }
            break;
        }
        case ECk_AssetSearchStrategy::Both:
        {
            AssetsToLoad = ExactMatches;
            AssetsToLoad.Append(FuzzyMatches);
            ExactMatchesToLoad = ExactMatches;
            FuzzyMatchesToLoad = FuzzyMatches;
            break;
        }
    }

    if (AssetsToLoad.Num() == 0)
    {
        const auto& ClassFilter = ck::IsValid(AssetClass, ck::IsValid_Policy_NullptrOnly{})
            ? FString::Printf(TEXT(" of class %s"), *AssetClass->GetName())
            : FString{};

        ck::core::Warning(TEXT("DoFullAssetScan: No assets{} found matching '{}'"), ClassFilter, AssetName);
        return FCk_Utils_Object_AssetSearchResult_Array{};
    }

    for (const auto& AssetData : AssetsToLoad)
    {
        if (auto LoadedAsset = AssetData.GetAsset();
            ck::IsValid(LoadedAsset, ck::IsValid_Policy_NullptrOnly{}))
        {
            auto SingleResult = FCk_Utils_Object_AssetSearchResult_Single{};
            SingleResult._Asset = LoadedAsset;
            SingleResult._AssetName = AssetData.AssetName.ToString();
            SingleResult._AssetPath = AssetData.GetSoftObjectPath().ToString();

            const auto& IsExactMatch = ExactMatchesToLoad.ContainsByPredicate([&AssetData](const FAssetData& Data)
            {
                return Data.GetSoftObjectPath() == AssetData.GetSoftObjectPath();
            });

            SingleResult._MatchType = IsExactMatch
                ? ECk_AssetMatchType::ExactMatch
                : ECk_AssetMatchType::FuzzyMatch;

            SingleResult._UniqueAsset = ECk_Unique::Unique; // Overridden by the calling function

            Result._Results.Add(SingleResult);

            ck::core::VeryVerbose(TEXT("DoFullAssetScan: Successfully loaded asset '{}' from path '{}'"), LoadedAsset, AssetData);
        }
        else
        {
            ck::core::Error(TEXT("DoFullAssetScan: Failed to load asset from path '{}'"), AssetData);
        }
    }

    Result._ExactMatchCount = ExactMatchesToLoad.Num();
    Result._FuzzyMatchCount = FuzzyMatchesToLoad.Num();

    if (Result._Results.Num() > 1)
    {
        const auto& ClassFilter = ck::IsValid(AssetClass, ck::IsValid_Policy_NullptrOnly{})
            ? FString::Printf(TEXT(" %s"), *AssetClass->GetName())
            : FString{};

        auto ErrorMsg = FString::Printf(TEXT("DoFullAssetScan: Multiple%s assets found matching '%s':"),
            *ClassFilter, *AssetName);

        for (const auto& SingleResult : Result._Results)
        {
            ErrorMsg += FString::Printf(TEXT("\n  - %s (%s)"),
                *SingleResult.Get_AssetName(),
                *SingleResult.Get_AssetPath());
        }

        ck::core::Warning(TEXT("{}"), *ErrorMsg);
    }

    return Result;
}

auto
    UCk_Utils_IO_UE::
    Get_SearchPaths(
        ECk_AssetSearchScope SearchScope)
    -> TArray<FName>
{
    auto SearchPaths = TArray<FName>{};

    if (EnumHasAnyFlags(SearchScope, ECk_AssetSearchScope::Game))
    {
        SearchPaths.Add(FName(TEXT("/Game")));
    }

    if (EnumHasAnyFlags(SearchScope, ECk_AssetSearchScope::Plugins))
    {
        SearchPaths.Add(FName(TEXT("/Plugins")));
        SearchPaths.Add(FName(TEXT("/PluginContent")));

        auto& PluginManager = IPluginManager::Get();

        for (const auto& EnabledPlugins = PluginManager.GetEnabledPlugins();
            const auto& Plugin : EnabledPlugins)
        {
            if (Plugin->CanContainContent())
            {
                const auto& PluginContentPath = FString::Printf(TEXT("/%s"), *Plugin->GetName());
                SearchPaths.AddUnique(FName(*PluginContentPath));
            }
        }
    }

    if (EnumHasAnyFlags(SearchScope, ECk_AssetSearchScope::Engine))
    {
        SearchPaths.Add(FName(TEXT("/Engine")));
    }

    if (SearchPaths.IsEmpty())
    {
        SearchPaths.Add(FName(TEXT("/Game")));
    }

    return SearchPaths;
}

// --------------------------------------------------------------------------------------------------------------------
