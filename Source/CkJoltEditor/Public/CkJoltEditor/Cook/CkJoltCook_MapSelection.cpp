#include "CkJoltCook_MapSelection.h"

#include <Misc/PackageName.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::cook
{
    namespace ck_jolt_cook_map_selection
    {
        static auto IsEqualToOrUnderPath(
            const FString& InPackageName,
            const FString& InPath) -> bool
        {
            if (InPath.IsEmpty())
            { return false; }

            auto NormalizedPath = InPath;

            while (NormalizedPath.Len() > 1 && NormalizedPath.EndsWith(TEXT("/")))
            { NormalizedPath.LeftChopInline(1); }

            // UProjectPackagingSettings also permits a path relative to /Game.
            if (NOT NormalizedPath.StartsWith(TEXT("/")))
            { NormalizedPath = TEXT("/Game/") + NormalizedPath; }

            return InPackageName == NormalizedPath
                || InPackageName.StartsWith(NormalizedPath + TEXT("/"));
        }

        static auto Get_ExcludedPath(
            const FString& InMapPackageName,
            const FCk_Jolt_PackagingMapSelectionInput& InInput) -> FString
        {
            if (Get_IsPackageExcluded(InMapPackageName, InInput._DirectoriesToNeverCook))
            { return TEXT("DirectoriesToNeverCook"); }

            if (Get_IsPackageExcluded(InMapPackageName, InInput._JoltExcludedMapPathPrefixes))
            { return TEXT("CkJolt CookExcludedMapPathPrefixes"); }

            if (Get_IsPackageExcluded(InMapPackageName, TArray<FString>{InInput._CookedDataRootPath}))
            { return InInput._CookedDataRootPath; }

            return {};
        }

        static auto IsUnderAlwaysCookDirectory(
            const FString& InMapPackageName,
            const FCk_Jolt_PackagingMapSelectionInput& InInput) -> bool
        {
            return InInput._DirectoriesToAlwaysCook.ContainsByPredicate(
                [&](const FString& InDirectory)
                {
                    return IsEqualToOrUnderPath(InMapPackageName, InDirectory);
                });
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_IsPackageExcluded(
            const FString& InPackageName,
            const TArray<FString>& InExcludedPackagePaths) -> bool
    {
        using namespace ck_jolt_cook_map_selection;

        return InExcludedPackagePaths.ContainsByPredicate(
            [&](const FString& InExcludedPath)
            {
                return IsEqualToOrUnderPath(InPackageName, InExcludedPath);
            });
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Select_PackagingMaps(
            const FCk_Jolt_PackagingMapSelectionInput& InInput)
        -> FCk_Jolt_PackagingMapSelectionResult
    {
        using namespace ck_jolt_cook_map_selection;

        auto Result = FCk_Jolt_PackagingMapSelectionResult{};

        if (NOT InInput._bPackagingMaps)
        {
            Result._Failure = TEXT("-PackagingMaps was not requested");
            return Result;
        }

        if (InInput._bMap || InInput._bAllMaps)
        {
            Result._Failure = TEXT("-PackagingMaps cannot be combined with -Map or -AllMaps");
            return Result;
        }

        if (InInput._bCookAll)
        {
            Result._Failure = TEXT("ProjectPackagingSettings bCookAll is unsupported with -PackagingMaps");
            return Result;
        }

        auto SelectedMaps = TArray<FString>{};

        for (const auto& AuthoredMap : InInput._AuthoredMapsToCook)
        {
            constexpr auto IncludeReadOnlyRoots = true;
            if (NOT FPackageName::IsValidLongPackageName(AuthoredMap, IncludeReadOnlyRoots))
            {
                Result._Failure = FString::Printf(
                    TEXT("ProjectPackagingSettings MapsToCook entry [%s] is not a valid long package name"),
                    *AuthoredMap);
                return Result;
            }

            if (NOT Get_ExcludedPath(AuthoredMap, InInput).IsEmpty())
            { continue; }

            SelectedMaps.AddUnique(AuthoredMap);
        }

        auto SortedCandidates = InInput._DiscoveredAlwaysCookMapCandidates;
        SortedCandidates.Sort();

        for (const auto& CandidateMap : SortedCandidates)
        {
            constexpr auto IncludeReadOnlyRoots = true;
            if (NOT FPackageName::IsValidLongPackageName(CandidateMap, IncludeReadOnlyRoots))
            { continue; }

            if (NOT IsUnderAlwaysCookDirectory(CandidateMap, InInput))
            { continue; }

            if (NOT Get_ExcludedPath(CandidateMap, InInput).IsEmpty())
            { continue; }

            SelectedMaps.AddUnique(CandidateMap);
        }

        Result._MapPackageNames = MoveTemp(SelectedMaps);
        Result._Success = true;
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
