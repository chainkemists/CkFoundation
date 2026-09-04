#include "CkGroundNav_CookedFieldIndex.h"

#include "CkCore/Format/CkFormat.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GroundNav_CookedFieldIndex_UE::
    Get_IsCompatibleWith(
        int32 InFormatVersion) const
    -> bool
{
    return _FormatVersion == InFormatVersion;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace cookedfieldindex_private
    {
        // /Game/Maps/TestMap -> <Root>/Maps/TestMap. The content-root prefix is dropped so a project
        // that cooks into its own root does not end up with /Game repeated inside the path.
        auto Get_LevelSubPath(
            const FString& InLevelPackageName) -> FString
        {
            auto SubPath = InLevelPackageName;
            SubPath.RemoveFromStart(TEXT("/Game"));

            return SubPath;
        }
    }

    auto
        Get_CookedIndexAssetPath(
            const FString& InCookedDataRootPath,
            const FString& InLevelPackageName,
            FName          InCookKey)
        -> FString
    {
        using namespace cookedfieldindex_private;

        return ck::Format_UE(TEXT("{}{}/GroundNavIndex_{}.GroundNavIndex_{}"),
            InCookedDataRootPath, Get_LevelSubPath(InLevelPackageName), InCookKey, InCookKey);
    }

    auto
        Get_CookedTileAssetPath(
            const FString& InCookedDataRootPath,
            const FString& InLevelPackageName,
            FName          InCookKey,
            FIntPoint      InTileCoord)
        -> FString
    {
        using namespace cookedfieldindex_private;

        return ck::Format_UE(TEXT("{}{}/GroundNavTile_{}_{}_{}.GroundNavTile_{}_{}_{}"),
            InCookedDataRootPath, Get_LevelSubPath(InLevelPackageName),
            InCookKey, InTileCoord.X, InTileCoord.Y,
            InCookKey, InTileCoord.X, InTileCoord.Y);
    }

    auto
        Get_PackageLookupKey(
            const FString& InPackageName)
        -> FName
    {
        // A no-op on a non-PIE name, so every caller is safe to funnel through it.
        return FName{*UWorld::RemovePIEPrefix(InPackageName)};
    }
}

// --------------------------------------------------------------------------------------------------------------------
