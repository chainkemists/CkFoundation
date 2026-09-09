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

        auto Get_ProfileSubPath(
            FGameplayTag InProfileTag) -> FString
        {
            if (NOT InProfileTag.IsValid())
            { return {}; }

            // Gameplay tags are dot-hierarchical, but their underlying names are not a package-path
            // grammar. Encode each hierarchy segment so no character in an authored tag can introduce
            // an extra directory or collide with a separator we add here.
            auto Segments = TArray<FString>{};
            InProfileTag.ToString().ParseIntoArray(Segments, TEXT("."), false);

            auto Result = FString{TEXT("/Profiles")};

            for (const auto& Segment : Segments)
            {
                Result.Append(TEXT("/S_"));

                for (const auto Character : Segment)
                {
                    const auto IsAsciiAlphaNumeric =
                        (Character >= TEXT('A') && Character <= TEXT('Z')) ||
                        (Character >= TEXT('a') && Character <= TEXT('z')) ||
                        (Character >= TEXT('0') && Character <= TEXT('9'));

                    if (IsAsciiAlphaNumeric)
                    { Result.AppendChar(Character); }
                    else
                    { Result.Appendf(TEXT("_%04X"), static_cast<uint32>(Character)); }
                }
            }

            return Result;
        }

        auto Get_ProfileLevelPath(
            const FString& InLevelPackageName,
            FGameplayTag   InProfileTag) -> FString
        {
            auto LevelComponents = TArray<FString>{};
            InLevelPackageName.ParseIntoArray(LevelComponents, TEXT("/"), true);

            // The component count makes the raw package-name boundary explicit. Without it, a nested
            // level path ending in Profiles/A could occupy the same variant directory as a shallower
            // level with the A profile.
            return ck::Format_UE(TEXT("/__CkGroundNavProfiles/Level_{}{}{}"),
                LevelComponents.Num(), InLevelPackageName, Get_ProfileSubPath(InProfileTag));
        }

        auto Get_SelectorSubPath(
            const FCk_GroundNav_DataLayerSelector& InDataLayerSelector) -> FString
        {
            if (InDataLayerSelector.Get_IsAll())
            { return {}; }

            // The complete canonical names are encoded instead of reduced to a hash. A selector is a
            // geometry identity, so two sets must never share a cooked asset path merely because a
            // short digest collided; the manifest repeats these names for load-time validation.
            auto Result = ck::Format_UE(TEXT("/__CkGroundNavLayers/Count_{}"),
                InDataLayerSelector.Get_LayerNames().Num());

            for (const auto& LayerName : InDataLayerSelector.Get_LayerNames())
            {
                Result.Append(TEXT("/L_"));

                for (const auto Character : LayerName.ToString())
                {
                    const auto IsAsciiAlphaNumeric =
                        (Character >= TEXT('A') && Character <= TEXT('Z')) ||
                        (Character >= TEXT('a') && Character <= TEXT('z')) ||
                        (Character >= TEXT('0') && Character <= TEXT('9'));

                    if (IsAsciiAlphaNumeric)
                    { Result.AppendChar(Character); }
                    else
                    { Result.Appendf(TEXT("_%04X"), static_cast<uint32>(Character)); }
                }
            }

            return Result;
        }
    }

    auto
        Get_CookedIndexAssetPath(
            const FString& InCookedDataRootPath,
            const FString& InLevelPackageName,
            FName          InCookKey,
            FGameplayTag   InProfileTag,
            const FCk_GroundNav_DataLayerSelector& InDataLayerSelector)
        -> FString
    {
        using namespace cookedfieldindex_private;

        if (NOT InDataLayerSelector.Get_IsCanonical())
        { return {}; }

        if (NOT InProfileTag.IsValid())
        {
            return ck::Format_UE(TEXT("{}{}{}/GroundNavIndex_{}.GroundNavIndex_{}"),
                InCookedDataRootPath, Get_LevelSubPath(InLevelPackageName),
                Get_SelectorSubPath(InDataLayerSelector), InCookKey, InCookKey);
        }

        return ck::Format_UE(TEXT("{}{}{}/GroundNavProfileIndex_{}.GroundNavProfileIndex_{}"),
            InCookedDataRootPath, Get_ProfileLevelPath(InLevelPackageName, InProfileTag),
            Get_SelectorSubPath(InDataLayerSelector), InCookKey, InCookKey);
    }

    auto
        Get_CookedTileAssetPath(
            const FString& InCookedDataRootPath,
            const FString& InLevelPackageName,
            FName          InCookKey,
            FIntPoint      InTileCoord,
            FGameplayTag   InProfileTag,
            const FCk_GroundNav_DataLayerSelector& InDataLayerSelector)
        -> FString
    {
        using namespace cookedfieldindex_private;

        if (NOT InDataLayerSelector.Get_IsCanonical())
        { return {}; }

        if (NOT InProfileTag.IsValid())
        {
            return ck::Format_UE(TEXT("{}{}{}/GroundNavTile_{}_{}_{}.GroundNavTile_{}_{}_{}"),
                InCookedDataRootPath, Get_LevelSubPath(InLevelPackageName),
                Get_SelectorSubPath(InDataLayerSelector),
                InCookKey, InTileCoord.X, InTileCoord.Y,
                InCookKey, InTileCoord.X, InTileCoord.Y);
        }

        return ck::Format_UE(TEXT("{}{}{}/GroundNavProfileTile_{}_{}_{}.GroundNavProfileTile_{}_{}_{}"),
            InCookedDataRootPath, Get_ProfileLevelPath(InLevelPackageName, InProfileTag),
            Get_SelectorSubPath(InDataLayerSelector),
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
