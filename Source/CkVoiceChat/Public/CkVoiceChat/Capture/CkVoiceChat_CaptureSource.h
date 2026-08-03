#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include <Containers/Array.h>
#include <Templates/SharedPointer.h>

class IVoiceCapture;

// --------------------------------------------------------------------------------------------------------------------

// The capture seam (spec §7.6): the ONLY surface the talker pipeline sees for microphone input,
// so every automated test runs the full pipeline against the fake and only the final
// "real microphone works" check needs a human. 16-bit mono PCM at the configured sample rate.
class CKVOICECHAT_API ICk_VoiceChat_CaptureSource
{
public:
    virtual ~ICk_VoiceChat_CaptureSource() = default;

    virtual auto Start() -> bool = 0;
    virtual auto Stop() -> void = 0;

    // Called on the game thread each tick before DrainPcm; scripted sources advance their clock
    // here, device-backed sources have nothing to do.
    virtual auto Tick(FCk_Time InDeltaT) -> void {}

    // Appends all PCM available since the last drain. Called on the game thread each tick.
    virtual auto DrainPcm(TArray<uint8>& OutPcm) -> void = 0;
};

// --------------------------------------------------------------------------------------------------------------------

// Engine microphone capture via FVoiceModule::CreateVoiceCapture. Start() fails LOUDLY (one
// ensure naming the fix) when the engine Voice module is disabled — [Voice] bEnabled=true must
// exist in the host project's DefaultEngine.ini for real capture; there is no silent fallback.
class CKVOICECHAT_API FCk_VoiceChat_CaptureSource_Engine : public ICk_VoiceChat_CaptureSource
{
public:
    CK_GENERATED_BODY(FCk_VoiceChat_CaptureSource_Engine);

    explicit FCk_VoiceChat_CaptureSource_Engine(
        int32 InSampleRate);

public:
    auto Start() -> bool override;
    auto Stop() -> void override;
    auto DrainPcm(TArray<uint8>& OutPcm) -> void override;

private:
    int32 _SampleRate = 48000;
    TSharedPtr<IVoiceCapture> _Capture;
};

// --------------------------------------------------------------------------------------------------------------------

// Deterministic scripted capture for tests and the gym: a FIFO of segments (sine or silence)
// emitted at real-time pace against the drain calls' accumulated time. Enqueue more segments at
// any point; when the script runs dry the source emits nothing (an open mic with a quiet room is
// silence SEGMENTS, not absence of data — model it explicitly).
class CKVOICECHAT_API FCk_VoiceChat_CaptureSource_Fake : public ICk_VoiceChat_CaptureSource
{
public:
    CK_GENERATED_BODY(FCk_VoiceChat_CaptureSource_Fake);

    explicit FCk_VoiceChat_CaptureSource_Fake(
        int32 InSampleRate);

public:
    auto Start() -> bool override;
    auto Stop() -> void override;
    auto Tick(FCk_Time InDeltaT) -> void override;
    auto DrainPcm(TArray<uint8>& OutPcm) -> void override;

public:
    auto
    Enqueue_Sine(
        FCk_Time InDuration,
        float InAmplitude01,
        float InFrequencyHz) -> void;

    auto
    Enqueue_Silence(
        FCk_Time InDuration) -> void;

    // Advances the fake clock; the next DrainPcm emits this much more audio.
    auto
    AdvanceTime(
        FCk_Time InDeltaT) -> void;

private:
    struct FSegment
    {
        int32 _RemainingSamples = 0;
        float _Amplitude01 = 0.0f;
        float _FrequencyHz = 0.0f;   // 0 = silence
    };

private:
    int32 _SampleRate = 48000;
    bool _IsStarted = false;
    double _PendingSeconds = 0.0;
    uint64 _SampleCounter = 0;
    TArray<FSegment> _Segments;
};

// --------------------------------------------------------------------------------------------------------------------
