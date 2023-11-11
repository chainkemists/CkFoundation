#include "CkIO_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

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

    FString OutProjectVersion;
    GConfig->GetString
    (
        TEXT("Script/EngineSettings.GeneralProjectSettings"),
        TEXT("ProjectVersion"),
        OutProjectVersion,
        GGameIni
    );

    return OutProjectVersion;
}

// --------------------------------------------------------------------------------------------------------------------
