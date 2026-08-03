#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Components/SynthComponent.h>
#include <Containers/Queue.h>
#include <Sound/SoundGenerator.h>

#include "CkVoiceChatSynth_Component.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Render-thread consumer: drains ready-to-play float PCM from the SPSC queue and zero-fills on
// underrun. Decode/jitter/PLC policy all run on the GAME thread (measured cheap at P1 — 27 us
// per 20 ms frame), so no decoder state, no UObjects, no locks and no new allocations live here.
// The generator holds the queue by shared ref, so it survives component Stop->Start untorn.
class CKVOICECHAT_API FCk_VoiceChat_SoundGenerator : public ISoundGenerator
{
public:
    using FPcmQueue = TQueue<TArray<float>, EQueueMode::Spsc>;

public:
    explicit FCk_VoiceChat_SoundGenerator(
        const TSharedPtr<FPcmQueue, ESPMode::ThreadSafe>& InPcmQueue);

public:
    auto
    OnGenerateAudio(
        float* OutAudio,
        int32 NumSamples) -> int32 override;

private:
    TSharedPtr<FPcmQueue, ESPMode::ThreadSafe> _PcmQueue;
    TArray<float> _Partial;
    int32 _PartialCursor = 0;
};

// --------------------------------------------------------------------------------------------------------------------

// Spatialized voice playback endpoint (ADR-3): one per audible talker on each listening machine.
// Fixed 48 kHz mono - the audio mixer's SRC owns device-rate conversion (review N6); never
// resample here. Game thread pushes decoded PCM via Enqueue_DecodedPcm.
UCLASS(NotBlueprintable)
class CKVOICECHAT_API UCk_VoiceChatSynthComponent_UE : public USynthComponent
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_VoiceChatSynthComponent_UE);

public:
    auto
    Init(
        int32& SampleRate) -> bool override;

    auto
    CreateSoundGenerator(
        const FSoundGeneratorInitParams& InParams) -> ISoundGeneratorPtr override;

public:
    // Game-thread only. 16-bit mono PCM at the module sample rate; converted to float here so
    // the render thread only ever copies.
    auto
    Enqueue_DecodedPcm(
        TArrayView<const int16> InPcm) -> void;

private:
    TSharedPtr<FCk_VoiceChat_SoundGenerator::FPcmQueue, ESPMode::ThreadSafe> _PcmQueue;
};

// --------------------------------------------------------------------------------------------------------------------
