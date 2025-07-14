#include "CkIO_Utils.h"

#include "CkCore/CkCoreLog.h"
#include "CkCore/Ensure/CkEnsure.h"

#include <Interfaces/IPluginManager.h>
#include <Engine/AssetManager.h>
#include <Engine/Engine.h>
#include <Misc/ConfigCacheIni.h>
#include <Runtime/Engine/Classes/Kismet/BlueprintPathsLibrary.h>

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
        TEXT("Script/EngineSettings.GeneralProjectSettings"),
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

    // Get the asset registry
    auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    auto& AssetRegistry = AssetRegistryModule.Get();

    // Get search paths
    const auto& SearchPaths = Get_SearchPaths(SearchScope);

    // Create filter to search assets
    auto Filter = FARFilter{};
    Filter.bRecursivePaths = true;

    // Convert FName array to FString array for this legacy function
    auto SearchPathsAsStrings = TArray<FString>{};
    for (const auto& Path : SearchPaths)
    {
        SearchPathsAsStrings.Add(Path.ToString());
    }

    Filter.PackagePaths = SearchPaths;

    // Get assets
    auto AssetDataList = TArray<FAssetData>{};
    AssetRegistry.GetAssets(Filter, AssetDataList);

    // Look for matches
    for (const auto& AssetData : AssetDataList)
    {
        const auto& CurrentAssetName = AssetData.AssetName.ToString();
        if (CurrentAssetName.Contains(AssetName, ESearchCase::IgnoreCase))
        {
            const auto& AssetInfo = FString::Printf(TEXT("%s (%s) - %s"),
                *CurrentAssetName,
                *AssetData.GetSoftObjectPath().ToString(),
                *AssetData.GetSoftObjectPath().ToString());
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

    // OPTIMIZATION: Use fast exact lookups for ExactOnly and ExactThenFuzzy strategies
    if (SearchStrategy == ECk_AssetSearchStrategy::ExactOnly)
    {
        return DoFastExactLookup(AssetName, AssetClass, SearchScope);
    }

    if (SearchStrategy == ECk_AssetSearchStrategy::ExactThenFuzzy)
    {
        // Try fast exact lookup first
        auto ExactResults = DoFastExactLookup(AssetName, AssetClass, SearchScope);
        if (ExactResults.Get_Results().Num() > 0)
        {
            return ExactResults; // Found exact matches - stop here!
        }

        // No exact matches found - fall back to fuzzy search
        return DoFuzzySearch(AssetName, AssetClass, SearchScope);
    }

    // For FuzzyOnly and Both strategies, use the original full-scan approach
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

    // Get the asset registry
    auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    auto& AssetRegistry = AssetRegistryModule.Get();

    auto FoundAssets = TArray<FAssetData>{};

    // Try direct path lookup first (if it looks like a path)
    if (AssetName.StartsWith(TEXT("/")))
    {
        ck::core::VeryVerbose(TEXT("DoFastExactLookup: Attempting direct path lookup for: '{}'"), *AssetName);

        const auto& AssetPath = FSoftObjectPath(AssetName);
        const auto& AssetData = AssetRegistry.GetAssetByObjectPath(AssetPath);

        ck::core::VeryVerbose(TEXT("DoFastExactLookup: Direct path lookup - Valid: {}"), AssetData.IsValid());

        if (AssetData.IsValid())
        {
            // For exact paths, skip search scope validation - user knows where the asset is
            // Only validate class filter if specified
            auto ClassMatches = true;
            if (ck::IsValid(AssetClass, ck::IsValid_Policy_NullptrOnly{}))
            {
                if (NOT AssetData.GetClass()->IsChildOf(AssetClass))
                {
                    ck::core::Warning(TEXT("DoFastExactLookup: Asset '{}' found but wrong class"), *AssetName);
                    ClassMatches = false;
                }
            }

            if (ClassMatches)
            {
                // FAST PATH: Load immediately and return
                auto LoadedAsset = AssetData.GetAsset();
                if (ck::IsValid(LoadedAsset, ck::IsValid_Policy_NullptrOnly{}))
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

                    ck::core::VeryVerbose(TEXT("DoFastExactLookup: FAST PATH - Successfully loaded asset '{}' from direct path '{}'"),
                        *LoadedAsset->GetName(), *AssetName);

                    return Result; // IMMEDIATE RETURN - No further processing needed!
                }
            }
        }
    }

    // If no path-based match, try filtering all assets by name
    if (FoundAssets.Num() == 0)
    {
        // Create a filter to find assets by exact name
        auto NameFilter = FARFilter{};
        NameFilter.bRecursivePaths = true;
        NameFilter.PackagePaths = Get_SearchPaths(SearchScope);

        if (ck::IsValid(AssetClass, ck::IsValid_Policy_NullptrOnly{}))
        {
            NameFilter.ClassPaths.Add(AssetClass->GetClassPathName());
        }

        auto AllAssets = TArray<FAssetData>{};
        AssetRegistry.GetAssets(NameFilter, AllAssets);

        // Filter by exact name match
        for (const auto& AssetData : AllAssets)
        {
            if (AssetData.AssetName.ToString().Equals(AssetName, ESearchCase::IgnoreCase))
            {
                FoundAssets.Add(AssetData);
            }
        }

        ck::core::VeryVerbose(TEXT("DoFastExactLookup: Found {} assets by name lookup for '{}'"),
            FoundAssets.Num(), *AssetName);
    }

    // Filter results by search scope and class
    const auto& SearchPaths = Get_SearchPaths(SearchScope);
    auto FilteredAssets = TArray<FAssetData>{};

    for (const auto& AssetData : FoundAssets)
    {
        // Check if asset is in allowed search paths
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

        // Check class filter
        if (ck::IsValid(AssetClass, ck::IsValid_Policy_NullptrOnly{}))
        {
            if (NOT AssetData.GetClass()->IsChildOf(AssetClass))
            {
                continue;
            }
        }

        FilteredAssets.Add(AssetData);
    }

    // Load the filtered assets and build results
    for (const auto& AssetData : FilteredAssets)
    {
        auto LoadedAsset = AssetData.GetAsset();

        if (ck::IsValid(LoadedAsset, ck::IsValid_Policy_NullptrOnly{}))
        {
            auto SingleResult = FCk_Utils_Object_AssetSearchResult_Single{};
            SingleResult._Asset = LoadedAsset;
            SingleResult._AssetName = AssetData.AssetName.ToString();
            SingleResult._AssetPath = AssetData.GetSoftObjectPath().ToString();
            SingleResult._MatchType = ECk_AssetMatchType::ExactMatch;
            SingleResult._UniqueAsset = ECk_Unique::Unique; // Will be overridden by caller

            Result._Results.Add(SingleResult);

            ck::core::VeryVerbose(TEXT("DoFastExactLookup: Successfully loaded asset '{}' from path '{}'"),
                *LoadedAsset->GetName(), *AssetData.GetSoftObjectPath().ToString());
        }
        else
        {
            ck::core::Error(TEXT("DoFastExactLookup: Failed to load asset from path '{}'"),
                *AssetData.GetSoftObjectPath().ToString());
        }
    }

    // Set counts - all matches are exact
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
    // For fuzzy search, we need to scan all assets (can't optimize this easily)
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

    // Get the asset registry
    auto& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    auto& AssetRegistry = AssetRegistryModule.Get();

    // Ensure asset registry is loaded, especially for plugin content
    if (NOT AssetRegistry.IsLoadingAssets())
    {
        AssetRegistry.SearchAllAssets(true);
    }

    // Get search paths
    const auto& SearchPaths = Get_SearchPaths(SearchScope);

    // Create filter to search assets
    auto Filter = FARFilter{};
    Filter.bRecursivePaths = true;
    Filter.PackagePaths = SearchPaths;

    if (ck::IsValid(AssetClass, ck::IsValid_Policy_NullptrOnly{}))
    {
        Filter.ClassPaths.Add(AssetClass->GetClassPathName());
    }

    // Get assets
    auto AssetDataList = TArray<FAssetData>{};
    AssetRegistry.GetAssets(Filter, AssetDataList);

    // Debug logging for plugin search issues
    if (EnumHasAnyFlags(SearchScope, ECk_AssetSearchScope::Plugins))
    {
        ck::core::VeryVerbose(TEXT("DoFullAssetScan: Searching for '{}' in {} total assets across search paths"),
            *AssetName, AssetDataList.Num());

        for (const auto& Path : SearchPaths)
        {
            auto PathFilter = FARFilter{};
            PathFilter.bRecursivePaths = true;
            PathFilter.PackagePaths.Add(Path);

            auto PathAssets = TArray<FAssetData>{};
            AssetRegistry.GetAssets(PathFilter, PathAssets);

            ck::core::VeryVerbose(TEXT("DoFullAssetScan: Found {} assets in path '{}'"),
                PathAssets.Num(), *Path.ToString());
        }
    }

    auto ExactMatches = TArray<FAssetData>{};
    auto FuzzyMatches = TArray<FAssetData>{};

    // Separate exact and fuzzy matches
    for (const auto& AssetData : AssetDataList)
    {
        const auto& CurrentAssetName = AssetData.AssetName.ToString();
        if (CurrentAssetName.Equals(AssetName, ESearchCase::IgnoreCase))
        {
            ExactMatches.Add(AssetData);
        }
        else if (CurrentAssetName.Contains(AssetName, ESearchCase::IgnoreCase))
        {
            FuzzyMatches.Add(AssetData);
        }
    }

    // Apply search strategy
    auto AssetsToLoad = TArray<FAssetData>{};
    auto ExactMatchesToLoad = TArray<FAssetData>{};
    auto FuzzyMatchesToLoad = TArray<FAssetData>{};

    switch (SearchStrategy)
    {
        case ECk_AssetSearchStrategy::ExactOnly:
            AssetsToLoad = ExactMatches;
            ExactMatchesToLoad = ExactMatches;
            break;

        case ECk_AssetSearchStrategy::FuzzyOnly:
            AssetsToLoad = FuzzyMatches;
            FuzzyMatchesToLoad = FuzzyMatches;
            break;

        case ECk_AssetSearchStrategy::ExactThenFuzzy:
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

        case ECk_AssetSearchStrategy::Both:
            AssetsToLoad = ExactMatches;
            AssetsToLoad.Append(FuzzyMatches);
            ExactMatchesToLoad = ExactMatches;
            FuzzyMatchesToLoad = FuzzyMatches;
            break;
    }

    // Handle no results
    if (AssetsToLoad.Num() == 0)
    {
        const auto& ClassFilter = ck::IsValid(AssetClass, ck::IsValid_Policy_NullptrOnly{})
            ? FString::Printf(TEXT(" of class %s"), *AssetClass->GetName())
            : FString{};

        ck::core::Warning(TEXT("DoFullAssetScan: No assets{} found matching '{}'"), *ClassFilter, *AssetName);
        return FCk_Utils_Object_AssetSearchResult_Array{};
    }

    // Load assets and populate results
    for (const auto& AssetData : AssetsToLoad)
    {
        auto LoadedAsset = AssetData.GetAsset();

        if (ck::IsValid(LoadedAsset, ck::IsValid_Policy_NullptrOnly{}))
        {
            auto SingleResult = FCk_Utils_Object_AssetSearchResult_Single{};
            SingleResult._Asset = LoadedAsset;
            SingleResult._AssetName = AssetData.AssetName.ToString();
            SingleResult._AssetPath = AssetData.GetSoftObjectPath().ToString();

            // Determine match type
            const auto& IsExactMatch = ExactMatchesToLoad.ContainsByPredicate([&AssetData](const FAssetData& Data)
            {
                return Data.GetSoftObjectPath() == AssetData.GetSoftObjectPath();
            });

            SingleResult._MatchType = IsExactMatch
                ? ECk_AssetMatchType::ExactMatch
                : ECk_AssetMatchType::FuzzyMatch;

            // UniqueAsset will be set by the calling function
            SingleResult._UniqueAsset = ECk_Unique::Unique; // Default, will be overridden

            Result._Results.Add(SingleResult);

            ck::core::VeryVerbose(TEXT("DoFullAssetScan: Successfully loaded asset '{}' from path '{}'"),
                *LoadedAsset->GetName(), *AssetData.GetSoftObjectPath().ToString());
        }
        else
        {
            ck::core::Error(TEXT("DoFullAssetScan: Failed to load asset from path '{}'"),
                *AssetData.GetSoftObjectPath().ToString());
        }
    }

    // Set counts
    Result._ExactMatchCount = ExactMatchesToLoad.Num();
    Result._FuzzyMatchCount = FuzzyMatchesToLoad.Num();

    // Log multiple matches if necessary
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
        // Try both the general plugins path and more specific ones
        SearchPaths.Add(FName(TEXT("/Plugins")));

        // Also try common plugin content paths
        SearchPaths.Add(FName(TEXT("/PluginContent")));

        // Get all mounted plugin paths dynamically
        auto& PluginManager = IPluginManager::Get();
        auto EnabledPlugins = PluginManager.GetEnabledPlugins();

        for (const auto& Plugin : EnabledPlugins)
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

    // Fallback to Game if no scope specified
    if (SearchPaths.Num() == 0)
    {
        SearchPaths.Add(FName(TEXT("/Game")));
    }

    return SearchPaths;
}


// --------------------------------------------------------------------------------------------------------------------
