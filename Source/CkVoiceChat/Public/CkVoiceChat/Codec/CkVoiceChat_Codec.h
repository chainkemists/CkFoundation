#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include <Containers/Array.h>
#include <Containers/ArrayView.h>
#include <Containers/Map.h>
#include <Misc/Optional.h>

// --------------------------------------------------------------------------------------------------------------------

// Wire header of one voice bundle: {Seq u16, ChannelIdx u8, AmplitudeQ8 u8, NumFrames u8}.
// NumFrames is derived from the frame list at pack time, never set by the caller.
struct CKVOICECHAT_API FCk_VoiceChat_BundleHeader
{
public:
    CK_GENERATED_BODY(FCk_VoiceChat_BundleHeader);

private:
    uint16 _Seq = 0;
    uint8 _ChannelIdx = 0;
    uint8 _AmplitudeQ8 = 0;

public:
    CK_PROPERTY_GET(_Seq);
    CK_PROPERTY_GET(_ChannelIdx);
    CK_PROPERTY_GET(_AmplitudeQ8);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_VoiceChat_BundleHeader, _Seq, _ChannelIdx, _AmplitudeQ8);
};

// --------------------------------------------------------------------------------------------------------------------

struct CKVOICECHAT_API FCk_VoiceChat_Bundle
{
public:
    CK_GENERATED_BODY(FCk_VoiceChat_Bundle);

private:
    FCk_VoiceChat_BundleHeader _Header;
    TArray<TArray<uint8>> _Frames;

public:
    CK_PROPERTY_GET(_Header);
    CK_PROPERTY_GET(_Frames);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_VoiceChat_Bundle, _Header, _Frames);
};

// --------------------------------------------------------------------------------------------------------------------

struct CKVOICECHAT_API FCk_VoiceChat_VadParams
{
public:
    CK_GENERATED_BODY(FCk_VoiceChat_VadParams);

private:
    float _Threshold = 0.07f;
    FCk_Time _Attack = FCk_Time{0.02};
    FCk_Time _Release = FCk_Time{0.2};

public:
    CK_PROPERTY(_Threshold);
    CK_PROPERTY(_Attack);
    CK_PROPERTY(_Release);
};

// --------------------------------------------------------------------------------------------------------------------

// Hysteresis gate over per-frame RMS: opens after _Threshold is held for Attack, closes after it
// is lost for Release. The accumulator resets whenever the signal crosses back over the
// threshold, so a single loud/quiet frame never flips the gate.
struct CKVOICECHAT_API FCk_VoiceChat_Vad
{
public:
    CK_GENERATED_BODY(FCk_VoiceChat_Vad);

public:
    auto
    Update(
        float InRms,
        FCk_Time InDeltaT,
        const FCk_VoiceChat_VadParams& InParams) -> bool;

private:
    bool _IsOpen = false;
    FCk_Time _TimeInOppositeBand = FCk_Time{0.0};

public:
    CK_PROPERTY_GET(_IsOpen);
};

// --------------------------------------------------------------------------------------------------------------------

struct CKVOICECHAT_API FCk_VoiceChat_JitterParams
{
public:
    CK_GENERATED_BODY(FCk_VoiceChat_JitterParams);

private:
    FCk_Time _FrameDuration = FCk_Time{0.02};
    FCk_Time _MinDepth = FCk_Time{0.02};
    FCk_Time _InitialTargetDepth = FCk_Time{0.06};
    FCk_Time _MaxDepth = FCk_Time{0.2};

    // Target depth = clamp(_JitterToDepthMultiplier x smoothed |inter-arrival - frame duration|,
    // Min, Max) once enough arrivals exist to trust the estimate.
    float _JitterToDepthMultiplier = 4.0f;
    float _JitterEwmaAlpha = 0.1f;

public:
    CK_PROPERTY(_FrameDuration);
    CK_PROPERTY(_MinDepth);
    CK_PROPERTY(_InitialTargetDepth);
    CK_PROPERTY(_MaxDepth);
    CK_PROPERTY(_JitterToDepthMultiplier);
    CK_PROPERTY(_JitterEwmaAlpha);
};

// --------------------------------------------------------------------------------------------------------------------

enum class ECk_VoiceChat_JitterPop : uint8
{
    // A real frame is returned
    Frame,

    // The expected frame is missing - caller should run packet-loss concealment for one frame
    Conceal,

    // Warming up (or drained): nothing to play yet
    Wait
};

// --------------------------------------------------------------------------------------------------------------------

struct CKVOICECHAT_API FCk_VoiceChat_JitterPopResult
{
public:
    CK_GENERATED_BODY(FCk_VoiceChat_JitterPopResult);

private:
    ECk_VoiceChat_JitterPop _Type = ECk_VoiceChat_JitterPop::Wait;
    TArray<uint8> _Frame;

public:
    CK_PROPERTY_GET(_Type);
    CK_PROPERTY_GET(_Frame);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_VoiceChat_JitterPopResult, _Type, _Frame);
};

// --------------------------------------------------------------------------------------------------------------------

// The unit-testable playout policy behind the voice synth: sequence reordering, gap concealment
// decisions, late-frame drops (voice is disposable - drop and count, never stash), and adaptive
// target depth from inter-arrival jitter. Sequence numbers wrap at u16; distances are signed
// 16-bit deltas. The caller pops once per frame duration from the audio pull.
struct CKVOICECHAT_API FCk_VoiceChat_JitterBuffer
{
public:
    CK_GENERATED_BODY(FCk_VoiceChat_JitterBuffer);

public:
    auto
    Push(
        uint16 InSeq,
        TArray<uint8> InFrame,
        FCk_Time InNow,
        const FCk_VoiceChat_JitterParams& InParams) -> void;

    auto
    Pop(
        const FCk_VoiceChat_JitterParams& InParams) -> FCk_VoiceChat_JitterPopResult;

public:
    auto
    Get_BufferedFrameCount() const -> int32;

    auto
    Get_TargetDepthFrames(
        const FCk_VoiceChat_JitterParams& InParams) const -> int32;

private:
    TMap<uint16, TArray<uint8>> _BufferedFrames;
    uint16 _NextPlayoutSeq = 0;
    bool _HasPlayoutCursor = false;
    bool _IsWarmedUp = false;

    FCk_Time _LastArrivalTime = FCk_Time{0.0};
    bool _HasLastArrivalTime = false;
    float _JitterEwmaSeconds = 0.0f;

    int32 _LateDropCount = 0;
    int32 _ConcealCount = 0;

public:
    CK_PROPERTY_GET(_LateDropCount);
    CK_PROPERTY_GET(_ConcealCount);
};

// --------------------------------------------------------------------------------------------------------------------

// One queued outbound bundle as the pacing policy sees it. _Id is opaque to the policy - the
// caller maps it back to its own queue entries.
struct CKVOICECHAT_API FCk_VoiceChat_PacingEntry
{
public:
    CK_GENERATED_BODY(FCk_VoiceChat_PacingEntry);

private:
    int32 _Id = 0;
    int32 _SizeBytes = 0;
    FCk_Time _Age = FCk_Time{0.0};

public:
    CK_PROPERTY_GET(_Id);
    CK_PROPERTY_GET(_SizeBytes);
    CK_PROPERTY_GET(_Age);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_VoiceChat_PacingEntry, _Id, _SizeBytes, _Age);
};

// --------------------------------------------------------------------------------------------------------------------

struct CKVOICECHAT_API FCk_VoiceChat_PacingResult
{
public:
    CK_GENERATED_BODY(FCk_VoiceChat_PacingResult);

private:
    TArray<int32> _SendIds;
    TArray<int32> _StaleDropIds;
    int32 _BytesSelected = 0;

public:
    CK_PROPERTY_GET(_SendIds);
    CK_PROPERTY_GET(_StaleDropIds);
    CK_PROPERTY_GET(_BytesSelected);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_VoiceChat_PacingResult, _SendIds, _StaleDropIds, _BytesSelected);
};

// --------------------------------------------------------------------------------------------------------------------

// Pure codec/wire/policy math - no UObjects, no ECS. The transport queues silently and never
// drops (Iris ordered-unreliable path), so freshness enforcement lives HERE, on the send side.
namespace ck::voice_chat::codec
{
    constexpr auto BundleHeaderBytes = 5;
    constexpr auto FrameSizePrefixBytes = 2;
    constexpr auto MaxFramesPerBundle = 255;
    constexpr auto MaxFrameBytes = 65535;

    // Wire form: [u16 Seq][u8 ChannelIdx][u8 AmplitudeQ8][u8 NumFrames] then per frame
    // [u16 EncodedSize][bytes], all little-endian. Returns an empty buffer (with an ensure) on
    // out-of-contract input.
    CKVOICECHAT_API auto
    Pack_Bundle(
        const FCk_VoiceChat_BundleHeader& InHeader,
        const TArray<TArray<uint8>>& InFrames) -> TArray<uint8>;

    // Strict inverse: returns unset on ANY malformed, truncated, or trailing-garbage input.
    CKVOICECHAT_API auto
    Unpack_Bundle(
        const TArray<uint8>& InBuffer) -> TOptional<FCk_VoiceChat_Bundle>;

    // Linear quantization of a 0..1 amplitude into the header's u8; out-of-range input clamps.
    CKVOICECHAT_API auto
    Quantize_Amplitude(
        float InAmplitude01) -> uint8;

    CKVOICECHAT_API auto
    Dequantize_Amplitude(
        uint8 InQuantized) -> float;

    // RMS of signed 16-bit PCM, normalized to 0..1. Empty input is 0.
    CKVOICECHAT_API auto
    Compute_Rms(
        TArrayView<const int16> InSamples) -> float;

    // Stale entries (age > InMaxAge) are dropped first - a late voice frame is worse than a
    // missing one; then the freshest-eligible queue sends oldest-first until the byte budget is
    // exhausted. Entries that don't fit stay queued for the next tick.
    CKVOICECHAT_API auto
    Select_BundlesToSend(
        const TArray<FCk_VoiceChat_PacingEntry>& InQueue,
        int32 InByteBudget,
        FCk_Time InMaxAge) -> FCk_VoiceChat_PacingResult;
}

// --------------------------------------------------------------------------------------------------------------------
