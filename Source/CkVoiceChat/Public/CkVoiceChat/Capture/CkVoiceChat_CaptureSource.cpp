#include "CkVoiceChat_CaptureSource.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkVoiceChat/CkVoiceChat_Log.h"

#include "Interfaces/VoiceCapture.h"
#include "VoiceModule.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_VoiceChat_CaptureSource_Engine::FCk_VoiceChat_CaptureSource_Engine(
    int32 InSampleRate)
    : _SampleRate(InSampleRate)
{
}

auto
    FCk_VoiceChat_CaptureSource_Engine::
    Start()
    -> bool
{
    if (_Capture.IsValid())
    { return _Capture->Start(); }

    constexpr auto NumChannels = 1;
    _Capture = FVoiceModule::Get().CreateVoiceCapture(FString{}, _SampleRate, NumChannels);

    const auto CaptureCreated = _Capture.IsValid();
    CK_ENSURE_IF_NOT(CaptureCreated,
        TEXT("Engine voice capture could not be created (sample rate [{}]). The engine Voice module "
             "is disabled or has no capture device - the host project needs [Voice] bEnabled=true "
             "in DefaultEngine.ini and a working microphone."), _SampleRate)
    {}
    if (NOT CaptureCreated)
    { return false; }

    return _Capture->Start();
}

auto
    FCk_VoiceChat_CaptureSource_Engine::
    Stop()
    -> void
{
    if (NOT _Capture.IsValid())
    { return; }

    _Capture->Stop();
}

auto
    FCk_VoiceChat_CaptureSource_Engine::
    DrainPcm(
        TArray<uint8>& OutPcm)
    -> void
{
    if (NOT _Capture.IsValid())
    { return; }

    auto AvailableBytes = 0u;
    const auto CaptureState = _Capture->GetCaptureState(AvailableBytes);

    if (CaptureState != EVoiceCaptureState::Ok || AvailableBytes == 0)
    { return; }

    const auto ExistingBytes = OutPcm.Num();
    OutPcm.AddUninitialized(static_cast<int32>(AvailableBytes));

    auto ReadBytes = 0u;
    _Capture->GetVoiceData(OutPcm.GetData() + ExistingBytes, AvailableBytes, ReadBytes);
    OutPcm.SetNum(ExistingBytes + static_cast<int32>(ReadBytes));
}

// --------------------------------------------------------------------------------------------------------------------

FCk_VoiceChat_CaptureSource_Fake::FCk_VoiceChat_CaptureSource_Fake(
    int32 InSampleRate)
    : _SampleRate(InSampleRate)
{
}

auto
    FCk_VoiceChat_CaptureSource_Fake::
    Start()
    -> bool
{
    _IsStarted = true;
    return true;
}

auto
    FCk_VoiceChat_CaptureSource_Fake::
    Stop()
    -> void
{
    _IsStarted = false;
    _PendingSeconds = 0.0;
}

auto
    FCk_VoiceChat_CaptureSource_Fake::
    Enqueue_Sine(
        FCk_Time InDuration,
        float InAmplitude01,
        float InFrequencyHz)
    -> void
{
    auto& Segment = _Segments.Emplace_GetRef();
    Segment._RemainingSamples = FMath::RoundToInt32(InDuration.Get_Seconds() * _SampleRate);
    Segment._Amplitude01 = InAmplitude01;
    Segment._FrequencyHz = InFrequencyHz;
}

auto
    FCk_VoiceChat_CaptureSource_Fake::
    Enqueue_Silence(
        FCk_Time InDuration)
    -> void
{
    Enqueue_Sine(InDuration, 0.0f, 0.0f);
}

auto
    FCk_VoiceChat_CaptureSource_Fake::
    AdvanceTime(
        FCk_Time InDeltaT)
    -> void
{
    if (NOT _IsStarted)
    { return; }

    _PendingSeconds += InDeltaT.Get_Seconds();
}

auto
    FCk_VoiceChat_CaptureSource_Fake::
    Tick(
        FCk_Time InDeltaT)
    -> void
{
    AdvanceTime(InDeltaT);
}

auto
    FCk_VoiceChat_CaptureSource_Fake::
    DrainPcm(
        TArray<uint8>& OutPcm)
    -> void
{
    if (NOT _IsStarted)
    { return; }

    auto SamplesToEmit = static_cast<int32>(_PendingSeconds * _SampleRate);
    if (SamplesToEmit <= 0)
    { return; }

    _PendingSeconds -= static_cast<double>(SamplesToEmit) / _SampleRate;

    while (SamplesToEmit > 0 && NOT _Segments.IsEmpty())
    {
        auto& Segment = _Segments[0];
        const auto SamplesFromSegment = FMath::Min(SamplesToEmit, Segment._RemainingSamples);

        const auto ExistingBytes = OutPcm.Num();
        OutPcm.AddUninitialized(SamplesFromSegment * 2);
        auto* Samples = reinterpret_cast<int16*>(OutPcm.GetData() + ExistingBytes);

        for (auto SampleIdx = 0; SampleIdx < SamplesFromSegment; ++SampleIdx)
        {
            if (Segment._FrequencyHz <= 0.0f || Segment._Amplitude01 <= 0.0f)
            {
                Samples[SampleIdx] = 0;
            }
            else
            {
                const auto Phase = 2.0 * PI * Segment._FrequencyHz * static_cast<double>(_SampleCounter) / _SampleRate;
                Samples[SampleIdx] = static_cast<int16>(Segment._Amplitude01 * 32767.0 * FMath::Sin(Phase));
            }
            ++_SampleCounter;
        }

        Segment._RemainingSamples -= SamplesFromSegment;
        SamplesToEmit -= SamplesFromSegment;

        if (Segment._RemainingSamples <= 0)
        { _Segments.RemoveAt(0); }
    }
}

// --------------------------------------------------------------------------------------------------------------------
