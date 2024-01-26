#include "CkIO_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

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
    Get_ExtractPath(
        const FString& InFullPath)
    -> FString
{
    return FPaths::GetPath(InFullPath);
}

// --------------------------------------------------------------------------------------------------------------------
