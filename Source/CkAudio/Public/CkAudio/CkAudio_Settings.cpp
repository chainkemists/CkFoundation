#include "CkAudio_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::audio::settings
{
    bool CVarAudio_DebugPreviewAllAudioTracks = false;
    static FAutoConsoleVariableRef CVarAudio_DebugPreviewAllAudioTracks_Ref(
        TEXT("ck.Audio.DebugPreviewAllAudioTracks"),
        CVarAudio_DebugPreviewAllAudioTracks,
        TEXT("Enable debug visualization for all AudioTracks"),
        ECVF_Default);

    float CVarAudio_DebugLineThickness = 3.0f;
    static FAutoConsoleVariableRef CVarAudio_DebugLineThickness_Ref(
        TEXT("ck.Audio.DebugLineThickness"),
        CVarAudio_DebugLineThickness,
        TEXT("Line thickness for AudioTrack debug drawing"),
        ECVF_Default);

    float CVarAudio_DebugNonSpatialHUDSize = 150.0f;
    static FAutoConsoleVariableRef CVarAudio_DebugNonSpatialHUDSize_Ref(
        TEXT("ck.Audio.DebugNonSpatialHUDSize"),
        CVarAudio_DebugNonSpatialHUDSize,
        TEXT("Size of HUD elements for non-spatial AudioTrack debug visualization"),
        ECVF_Default);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AudioTrack_Settings::
    Get_DebugPreviewAllAudioTracks()
        -> bool
{
    return ck::audio::settings::CVarAudio_DebugPreviewAllAudioTracks;
}

auto
    UCk_Utils_AudioTrack_Settings::
    Get_DebugLineThickness()
        -> float
{
    return ck::audio::settings::CVarAudio_DebugLineThickness;
}

auto
    UCk_Utils_AudioTrack_Settings::
    Get_NonSpatialHUDSize()
        -> float
{
    return ck::audio::settings::CVarAudio_DebugNonSpatialHUDSize;
}