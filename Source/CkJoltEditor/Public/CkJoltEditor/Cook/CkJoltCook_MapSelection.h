#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Containers/Array.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt::cook
{
    /// Inputs for the packaging entry-map selection. The map entries are long package names, never
    /// filenames or object paths. This is deliberately world-free: validating selection must not
    /// load a map or mutate cooked data.
    struct CKJOLTEDITOR_API FCk_Jolt_PackagingMapSelectionInput
    {
        TArray<FString> _AuthoredMapsToCook;
        TArray<FString> _DirectoriesToAlwaysCook;
        TArray<FString> _DiscoveredAlwaysCookMapCandidates;
        TArray<FString> _DirectoriesToNeverCook;
        TArray<FString> _JoltExcludedMapPathPrefixes;
        FString _CookedDataRootPath;
        bool _bPackagingMaps = false;
        bool _bMap = false;
        bool _bAllMaps = false;
        bool _bCookAll = false;
    };

    struct CKJOLTEDITOR_API FCk_Jolt_PackagingMapSelectionResult
    {
        TArray<FString> _MapPackageNames;
        FString _Failure;
        bool _Success = false;
    };

    /// Tests a map or level package against configured package-directory exclusions. Relative paths
    /// are rooted at /Game; trailing slashes are ignored; matching respects path components.
    CKJOLTEDITOR_API auto
        Get_IsPackageExcluded(
            const FString& InPackageName,
            const TArray<FString>& InExcludedPackagePaths) -> bool;

    /// Resolves authored MapsToCook first, then lexically ordered UWorld candidates found under the configured
    /// DirectoriesToAlwaysCook. Exclusions skip either source; rejected command-line/settings input yields no plan.
    CKJOLTEDITOR_API auto
        Select_PackagingMaps(
            const FCk_Jolt_PackagingMapSelectionInput& InInput)
        -> FCk_Jolt_PackagingMapSelectionResult;
}

// --------------------------------------------------------------------------------------------------------------------
