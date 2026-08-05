#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkVoiceChat_Settings.generated.h"

class USoundAttenuation;

// --------------------------------------------------------------------------------------------------------------------

// Fallbacks when the settings CDO is unavailable. The numeric defaults are the canonical voice
// wire recipe: 48 kHz mono Opus, 20 ms frames, 2-3 frames per RPC, adaptive jitter starting
// ~60 ms, 8 audible speakers.
namespace ck::voice_chat::defaults
{
    constexpr int32 SampleRate = 48000;
    constexpr int32 BitrateBps = 24000;
    constexpr int32 FrameSizeMs = 20;
    constexpr int32 FramesPerRpc = 3;
    // S5-measured (2026-08-03, editor net-PIE, Iris, project config): the connection's saturated
    // unreliable-RPC drain is ~708 B/tick, stable across windows. 640 covers the 8-talker steady
    // worst case (8 x 240 B / 3.6 ticks ~= 533 B/tick) yet stays UNDER the drain, so sustained
    // overage drops at the counted budget layer instead of silently queueing (S2). Re-measure on
    // packaged/dedicated config before ship-tuning.
    constexpr int32 MaxVoiceBytesPerConnectionPerTick = 640;
    constexpr int32 MaxAudibleSpeakers = 8;
    constexpr float ProximityHysteresisMarginCm = 100.0f;
    constexpr float JitterBufferMinSeconds = 0.02f;
    constexpr float JitterBufferTargetSeconds = 0.06f;
    constexpr float JitterBufferMaxSeconds = 0.2f;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Voice Chat"))
class CKVOICECHAT_API UCk_VoiceChat_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_VoiceChat_ProjectSettings_UE);

private:
    UPROPERTY(Config, EditDefaultsOnly, Category = "Codec",
        meta = (AllowPrivateAccess = true, ClampMin = "8000", ClampMax = "48000",
            ToolTip = "Capture and codec sample rate in Hz. 48000 is the canonical wire recipe; the engine Voice factories are created at this rate."))
    int32 _SampleRate = ck::voice_chat::defaults::SampleRate;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Codec",
        meta = (AllowPrivateAccess = true, ClampMin = "6000", ClampMax = "128000",
            ToolTip = "Opus target bitrate in bits per second. ~60 bytes per 20 ms frame at 24 kbps."))
    int32 _BitrateBps = ck::voice_chat::defaults::BitrateBps;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Codec",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Variable bitrate encoding."))
    ECk_EnableDisable _Vbr = ECk_EnableDisable::Enable;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Codec",
        meta = (AllowPrivateAccess = true, ClampMin = "10", ClampMax = "60",
            ToolTip = "Opus frame size in milliseconds. Valid values: 10, 20, 40, 60. 20 ms is the smallest frame for which packet-loss concealment works well."))
    int32 _FrameSizeMs = ck::voice_chat::defaults::FrameSizeMs;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Transport",
        meta = (AllowPrivateAccess = true, ClampMin = "1", ClampMax = "8",
            ToolTip = "Encoded frames bundled per voice RPC. 2-3 frames -> ~16-25 RPCs/s while speaking. Larger bundles trade per-RPC overhead for added mouth-to-ear latency."))
    int32 _FramesPerRpc = ck::voice_chat::defaults::FramesPerRpc;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Transport",
        meta = (AllowPrivateAccess = true, ClampMin = "256",
            ToolTip = "Per-connection byte budget per tick for outbound voice, drained by the pacing processor. Keep BELOW the connection's measured unreliable drain rate (Ck.VoiceChat.Net.DrainBudgetMeasure prints it) so overage drops at this counted layer instead of silently queueing in the transport."))
    int32 _MaxVoiceBytesPerConnectionPerTick = ck::voice_chat::defaults::MaxVoiceBytesPerConnectionPerTick;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Routing",
        meta = (AllowPrivateAccess = true, ClampMin = "1", ClampMax = "32",
            ToolTip = "Top-N audible talkers forwarded per listener, ranked by reported amplitude. Server-only cap - clients only ever receive what the server forwards."))
    int32 _MaxAudibleSpeakers = ck::voice_chat::defaults::MaxAudibleSpeakers;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Routing",
        meta = (AllowPrivateAccess = true, ClampMin = "0.0",
            ToolTip = "Hysteresis margin in cm applied around a channel's audible range so speakers don't pop mid-word at the boundary."))
    float _ProximityHysteresisMarginCm = ck::voice_chat::defaults::ProximityHysteresisMarginCm;

    UPROPERTY(Config, EditDefaultsOnly, Category = "Playback",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Adaptive jitter buffer minimum depth."))
    FCk_Time _JitterBufferMin = FCk_Time{ck::voice_chat::defaults::JitterBufferMinSeconds};

    UPROPERTY(Config, EditDefaultsOnly, Category = "Playback",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Adaptive jitter buffer starting depth (~3 frames at 20 ms)."))
    FCk_Time _JitterBufferTarget = FCk_Time{ck::voice_chat::defaults::JitterBufferTargetSeconds};

    UPROPERTY(Config, EditDefaultsOnly, Category = "Playback",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Adaptive jitter buffer maximum depth."))
    FCk_Time _JitterBufferMax = FCk_Time{ck::voice_chat::defaults::JitterBufferMaxSeconds};

    UPROPERTY(Config, EditDefaultsOnly, Category = "Playback",
        meta = (AllowPrivateAccess = true,
            ToolTip = "Attenuation used by Positional3D channels that do not specify their own."))
    TSoftObjectPtr<USoundAttenuation> _DefaultAttenuation;

public:
    CK_PROPERTY_GET(_SampleRate);
    CK_PROPERTY_GET(_BitrateBps);
    CK_PROPERTY_GET(_Vbr);
    CK_PROPERTY_GET(_FrameSizeMs);
    CK_PROPERTY_GET(_FramesPerRpc);
    CK_PROPERTY_GET(_MaxVoiceBytesPerConnectionPerTick);
    CK_PROPERTY_GET(_MaxAudibleSpeakers);
    CK_PROPERTY_GET(_ProximityHysteresisMarginCm);
    CK_PROPERTY_GET(_JitterBufferMin);
    CK_PROPERTY_GET(_JitterBufferTarget);
    CK_PROPERTY_GET(_JitterBufferMax);
    CK_PROPERTY_GET(_DefaultAttenuation);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKVOICECHAT_API UCk_Utils_VoiceChat_Settings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_VoiceChat_Settings_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Settings")
    static int32
    Get_SampleRate();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Settings")
    static int32
    Get_BitrateBps();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Settings")
    static ECk_EnableDisable
    Get_Vbr();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Settings")
    static int32
    Get_FrameSizeMs();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Settings")
    static int32
    Get_FramesPerRpc();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Settings")
    static int32
    Get_MaxVoiceBytesPerConnectionPerTick();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Settings")
    static int32
    Get_MaxAudibleSpeakers();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Settings")
    static float
    Get_ProximityHysteresisMarginCm();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Settings")
    static FCk_Time
    Get_JitterBufferMin();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Settings")
    static FCk_Time
    Get_JitterBufferTarget();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Settings")
    static FCk_Time
    Get_JitterBufferMax();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|VoiceChat|Settings")
    static TSoftObjectPtr<USoundAttenuation>
    Get_DefaultAttenuation();
};

// --------------------------------------------------------------------------------------------------------------------
