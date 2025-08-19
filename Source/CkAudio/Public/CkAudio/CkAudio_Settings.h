#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkAudio_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKAUDIO_API UCk_Utils_AudioTrack_Settings : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_AudioTrack_Settings);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack|Settings",
              DisplayName = "[Ck][AudioTrack] Get Debug Preview All AudioTracks")
    static bool
    Get_DebugPreviewAllAudioTracks();

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack|Settings",
              DisplayName = "[Ck][AudioTrack] Get Debug Line Thickness")
    static float
    Get_DebugLineThickness();

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|AudioTrack|Settings",
              DisplayName = "[Ck][AudioTrack] Get Non-Spatial HUD Size")
    static float
    Get_NonSpatialHUDSize();
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::audio::settings
{
    CKAUDIO_API extern bool CVarAudio_DebugPreviewAllAudioTracks;
    CKAUDIO_API extern float CVarAudio_DebugLineThickness;
    CKAUDIO_API extern float CVarAudio_DebugNonSpatialHUDSize;
}

// --------------------------------------------------------------------------------------------------------------------