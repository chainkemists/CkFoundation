#include "CkVoiceChatSynth_Component.h"

#include "CkVoiceChat/Settings/CkVoiceChat_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_VoiceChat_SoundGenerator::FCk_VoiceChat_SoundGenerator(
    const TSharedPtr<FPcmQueue, ESPMode::ThreadSafe>& InPcmQueue)
    : _PcmQueue(InPcmQueue)
{
}

auto
    FCk_VoiceChat_SoundGenerator::
    OnGenerateAudio(
        float* OutAudio,
        int32 NumSamples)
    -> int32
{
    auto Written = 0;

    while (Written < NumSamples)
    {
        if (_PartialCursor < _Partial.Num())
        {
            const auto ToCopy = FMath::Min(NumSamples - Written, _Partial.Num() - _PartialCursor);
            FMemory::Memcpy(OutAudio + Written, _Partial.GetData() + _PartialCursor, ToCopy * sizeof(float));
            Written += ToCopy;
            _PartialCursor += ToCopy;
            continue;
        }

        if (NOT _PcmQueue.IsValid() || NOT _PcmQueue->Dequeue(_Partial))
        {
            // underrun (or warming up): silence
            FMemory::Memzero(OutAudio + Written, (NumSamples - Written) * sizeof(float));
            Written = NumSamples;
            break;
        }

        _PartialCursor = 0;
    }

    return NumSamples;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_VoiceChatSynthComponent_UE::
    Init(
        int32& SampleRate)
    -> bool
{
    SampleRate = UCk_Utils_VoiceChat_Settings_UE::Get_SampleRate();
    NumChannels = 1;

    if (NOT _PcmQueue.IsValid())
    {
        _PcmQueue = MakeShared<FCk_VoiceChat_SoundGenerator::FPcmQueue, ESPMode::ThreadSafe>();
    }

    return true;
}

auto
    UCk_VoiceChatSynthComponent_UE::
    CreateSoundGenerator(
        const FSoundGeneratorInitParams& InParams)
    -> ISoundGeneratorPtr
{
    if (NOT _PcmQueue.IsValid())
    {
        _PcmQueue = MakeShared<FCk_VoiceChat_SoundGenerator::FPcmQueue, ESPMode::ThreadSafe>();
    }

    return MakeShared<FCk_VoiceChat_SoundGenerator, ESPMode::ThreadSafe>(_PcmQueue);
}

auto
    UCk_VoiceChatSynthComponent_UE::
    Enqueue_DecodedPcm(
        TArrayView<const int16> InPcm)
    -> void
{
    if (NOT _PcmQueue.IsValid() || InPcm.IsEmpty())
    { return; }

    auto FloatPcm = TArray<float>{};
    FloatPcm.SetNumUninitialized(InPcm.Num());

    for (auto SampleIdx = 0; SampleIdx < InPcm.Num(); ++SampleIdx)
    {
        FloatPcm[SampleIdx] = static_cast<float>(InPcm[SampleIdx]) / 32768.0f;
    }

    _PcmQueue->Enqueue(MoveTemp(FloatPcm));
}

// --------------------------------------------------------------------------------------------------------------------
