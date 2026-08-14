#include "CkVoiceChat_Codec.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voice_chat_codec
{
    auto
    Write_U16_LE(
        TArray<uint8>& InOutBuffer,
        uint16 InValue) -> void
    {
        InOutBuffer.Add(static_cast<uint8>(InValue & 0xFF));
        InOutBuffer.Add(static_cast<uint8>((InValue >> 8) & 0xFF));
    }

    auto
    Read_U16_LE(
        const TArray<uint8>& InBuffer,
        int32& InOutCursor) -> uint16
    {
        const auto Value = static_cast<uint16>(InBuffer[InOutCursor]) |
            (static_cast<uint16>(InBuffer[InOutCursor + 1]) << 8);
        InOutCursor += 2;
        return Value;
    }

    // Signed distance from InFrom to InTo in u16 sequence space (wrap-safe).
    auto
    Get_SeqDistance(
        uint16 InTo,
        uint16 InFrom) -> int32
    {
        return static_cast<int16>(static_cast<uint16>(InTo - InFrom));
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_VoiceChat_Vad::
    Update(
        float InRms,
        FCk_Time InDeltaT,
        const FCk_VoiceChat_VadParams& InParams)
    -> bool
{
    const auto IsAboveThreshold = InRms >= InParams.Get_Threshold();

    if (IsAboveThreshold == _IsOpen)
    {
        _TimeInOppositeBand = FCk_Time{0.0};
        return _IsOpen;
    }

    _TimeInOppositeBand = FCk_Time{_TimeInOppositeBand.Get_Seconds() + InDeltaT.Get_Seconds()};

    const auto& FlipAfter = _IsOpen ? InParams.Get_Release() : InParams.Get_Attack();

    if (_TimeInOppositeBand.Get_Seconds() >= FlipAfter.Get_Seconds())
    {
        _IsOpen = IsAboveThreshold;
        _TimeInOppositeBand = FCk_Time{0.0};
    }

    return _IsOpen;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_VoiceChat_JitterBuffer::
    Push(
        uint16 InSeq,
        TArray<uint8> InFrame,
        FCk_Time InNow,
        const FCk_VoiceChat_JitterParams& InParams)
    -> void
{
    const auto ArrivalGapSeconds = _HasLastArrivalTime
        ? InNow.Get_Seconds() - _LastArrivalTime.Get_Seconds()
        : 0.0;
    const auto SeqDistance = _HasPlayoutCursor
        ? ck_voice_chat_codec::Get_SeqDistance(InSeq, _NextPlayoutSeq)
        : 0;

    const auto IsDiscontinuity = _HasPlayoutCursor &&
        (ArrivalGapSeconds > InParams.Get_ArrivalGapReset().Get_Seconds() ||
         SeqDistance > InParams.Get_MaxSeqJumpFrames());

    if (IsDiscontinuity)
    {
        DoReseed(InSeq);
        ++_DiscontinuityCount;
    }
    else if (_HasLastArrivalTime)
    {
        const auto DeviationSeconds = FMath::Abs(ArrivalGapSeconds - InParams.Get_FrameDuration().Get_Seconds());
        const auto Alpha = InParams.Get_JitterEwmaAlpha();
        _JitterEwmaSeconds = Alpha * static_cast<float>(DeviationSeconds) + (1.0f - Alpha) * _JitterEwmaSeconds;
    }
    _LastArrivalTime = InNow;
    _HasLastArrivalTime = true;

    if (_HasPlayoutCursor && NOT IsDiscontinuity &&
        ck_voice_chat_codec::Get_SeqDistance(InSeq, _NextPlayoutSeq) < 0)
    {
        if (_IsWarmedUp)
        {
            ++_LateDropCount;
            return;
        }

        // still warming up: extend the playout window backward so a reordered-high first packet
        // cannot clip the utterance onset
        _NextPlayoutSeq = InSeq;
    }

    _BufferedFrames.Add(InSeq, MoveTemp(InFrame));

    if (NOT _HasPlayoutCursor)
    {
        _NextPlayoutSeq = InSeq;
        _HasPlayoutCursor = true;
    }
}

auto
    FCk_VoiceChat_JitterBuffer::
    DoReseed(
        uint16 InSeq)
    -> void
{
    _BufferedFrames.Reset();
    _NextPlayoutSeq = InSeq;
    _HasPlayoutCursor = true;
    _IsWarmedUp = false;
}

auto
    FCk_VoiceChat_JitterBuffer::
    Pop(
        const FCk_VoiceChat_JitterParams& InParams)
    -> FCk_VoiceChat_JitterPopResult
{
    if (NOT _HasPlayoutCursor)
    { return {}; }

    if (NOT _IsWarmedUp)
    {
        if (_BufferedFrames.Num() < Get_TargetDepthFrames(InParams))
        { return {}; }

        _IsWarmedUp = true;
    }

    if (_BufferedFrames.IsEmpty())
    {
        // Underrun: fall back to warmup so playout resumes only once the target depth rebuilds.
        _IsWarmedUp = false;
        return {};
    }

    auto Frame = TArray<uint8>{};
    if (_BufferedFrames.RemoveAndCopyValue(_NextPlayoutSeq, Frame))
    {
        ++_NextPlayoutSeq;
        return FCk_VoiceChat_JitterPopResult{ECk_VoiceChat_JitterPop::Frame, MoveTemp(Frame)};
    }

    ++_ConcealCount;
    ++_NextPlayoutSeq;
    return FCk_VoiceChat_JitterPopResult{ECk_VoiceChat_JitterPop::Conceal, {}};
}

auto
    FCk_VoiceChat_JitterBuffer::
    Get_BufferedFrameCount() const
    -> int32
{
    return _BufferedFrames.Num();
}

auto
    FCk_VoiceChat_JitterBuffer::
    Get_TargetDepthFrames(
        const FCk_VoiceChat_JitterParams& InParams) const
    -> int32
{
    const auto FrameDurationSeconds = InParams.Get_FrameDuration().Get_Seconds();

    const auto FrameDurationIsValid = FrameDurationSeconds > 0.0;
    CK_ENSURE_IF_NOT(FrameDurationIsValid,
        TEXT("JitterParams FrameDuration [{}] must be positive"), InParams.Get_FrameDuration())
    { return 1; }

    const auto DepthSeconds = _JitterEwmaSeconds <= 0.0f
        ? InParams.Get_InitialTargetDepth().Get_Seconds()
        : FMath::Clamp(
            static_cast<double>(InParams.Get_JitterToDepthMultiplier() * _JitterEwmaSeconds),
            InParams.Get_MinDepth().Get_Seconds(),
            InParams.Get_MaxDepth().Get_Seconds());

    return FMath::Max(1, FMath::CeilToInt32(DepthSeconds / FrameDurationSeconds));
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::voice_chat::codec
{
    auto
        Pack_Bundle(
            const FCk_VoiceChat_BundleHeader& InHeader,
            const TArray<TArray<uint8>>& InFrames)
        -> TArray<uint8>
    {
        const auto FramesAreInContract = InFrames.Num() <= MaxFramesPerBundle &&
            ck::algo::NoneOf(InFrames, [](const TArray<uint8>& InFrame)
            {
                return InFrame.Num() > MaxFrameBytes || InFrame.IsEmpty();
            });
        CK_ENSURE_IF_NOT(FramesAreInContract,
            TEXT("Pack_Bundle rejected: [{}] frames (max {}), each must be 1..{} bytes"),
            InFrames.Num(), MaxFramesPerBundle, MaxFrameBytes)
        { return {}; }

        auto TotalBytes = BundleHeaderBytes;
        for (const auto& Frame : InFrames)
        {
            TotalBytes += FrameSizePrefixBytes + Frame.Num();
        }

        auto Buffer = TArray<uint8>{};
        Buffer.Reserve(TotalBytes);

        ck_voice_chat_codec::Write_U16_LE(Buffer, InHeader.Get_Seq());
        Buffer.Add(InHeader.Get_ChannelIdx());
        Buffer.Add(InHeader.Get_AmplitudeQ8());
        Buffer.Add(static_cast<uint8>(InFrames.Num()));

        for (const auto& Frame : InFrames)
        {
            ck_voice_chat_codec::Write_U16_LE(Buffer, static_cast<uint16>(Frame.Num()));
            Buffer.Append(Frame);
        }

        return Buffer;
    }

    auto
        Unpack_Bundle(
            const TArray<uint8>& InBuffer)
        -> TOptional<FCk_VoiceChat_Bundle>
    {
        if (InBuffer.Num() < BundleHeaderBytes)
        { return {}; }

        auto Cursor = 0;
        const auto Seq = ck_voice_chat_codec::Read_U16_LE(InBuffer, Cursor);
        const auto ChannelIdx = InBuffer[Cursor++];
        const auto AmplitudeQ8 = InBuffer[Cursor++];
        const auto NumFrames = InBuffer[Cursor++];

        auto Frames = TArray<TArray<uint8>>{};
        Frames.Reserve(NumFrames);

        for (auto FrameIdx = 0; FrameIdx < NumFrames; ++FrameIdx)
        {
            if (InBuffer.Num() - Cursor < FrameSizePrefixBytes)
            { return {}; }

            const auto FrameSize = ck_voice_chat_codec::Read_U16_LE(InBuffer, Cursor);

            if (FrameSize == 0 || InBuffer.Num() - Cursor < FrameSize)
            { return {}; }

            auto& Frame = Frames.Emplace_GetRef();
            Frame.Append(InBuffer.GetData() + Cursor, FrameSize);
            Cursor += FrameSize;
        }

        if (Cursor != InBuffer.Num())
        { return {}; }

        return FCk_VoiceChat_Bundle{FCk_VoiceChat_BundleHeader{Seq, ChannelIdx, AmplitudeQ8}, MoveTemp(Frames)};
    }

    auto
        Quantize_Amplitude(
            float InAmplitude01)
        -> uint8
    {
        return static_cast<uint8>(FMath::Clamp(FMath::RoundToInt32(InAmplitude01 * 255.0f), 0, 255));
    }

    auto
        Dequantize_Amplitude(
            uint8 InQuantized)
        -> float
    {
        return static_cast<float>(InQuantized) / 255.0f;
    }

    auto
        Compute_Rms(
            TArrayView<const int16> InSamples)
        -> float
    {
        if (InSamples.IsEmpty())
        { return 0.0f; }

        auto SumOfSquares = 0.0;
        for (const auto Sample : InSamples)
        {
            const auto Normalized = static_cast<double>(Sample) / 32768.0;
            SumOfSquares += Normalized * Normalized;
        }

        return static_cast<float>(FMath::Sqrt(SumOfSquares / InSamples.Num()));
    }

    auto
        Select_BundlesToSend(
            const TArray<FCk_VoiceChat_PacingEntry>& InQueue,
            int32 InByteBudget,
            FCk_Time InMaxAge)
        -> FCk_VoiceChat_PacingResult
    {
        auto SendIds = TArray<int32>{};
        auto StaleDropIds = TArray<int32>{};
        auto BytesSelected = 0;

        auto FreshEntries = ck::algo::Filter(InQueue, [&](const FCk_VoiceChat_PacingEntry& InEntry)
        {
            if (InEntry.Get_Age().Get_Seconds() > InMaxAge.Get_Seconds())
            {
                StaleDropIds.Add(InEntry.Get_Id());
                return false;
            }
            return true;
        });

        // Oldest fresh entry first; the transport is ordered, so selection stops at the first
        // entry that does not fit instead of skip-fitting smaller later ones.
        ck::algo::Sort(FreshEntries, [](const FCk_VoiceChat_PacingEntry& InA, const FCk_VoiceChat_PacingEntry& InB)
        {
            return InA.Get_Age().Get_Seconds() > InB.Get_Age().Get_Seconds();
        });

        for (const auto& Entry : FreshEntries)
        {
            if (BytesSelected + Entry.Get_SizeBytes() > InByteBudget)
            { break; }

            SendIds.Add(Entry.Get_Id());
            BytesSelected += Entry.Get_SizeBytes();
        }

        return FCk_VoiceChat_PacingResult{MoveTemp(SendIds), MoveTemp(StaleDropIds), BytesSelected};
    }

    auto
        Select_TopNTalkers(
            const TArray<FCk_VoiceChat_TopNCandidate>& InCandidates,
            int32 InMaxTalkers)
        -> TArray<int32>
    {
        if (InMaxTalkers <= 0 || InCandidates.IsEmpty())
        { return {}; }

        constexpr auto LoudnessBuckets = 8;

        const auto Get_Bucket = [](const FCk_VoiceChat_TopNCandidate& InCandidate) -> int32
        {
            return FMath::Clamp(
                static_cast<int32>(InCandidate.Get_Envelope() * LoudnessBuckets), 0, LoudnessBuckets - 1);
        };

        auto Sorted = InCandidates;
        ck::algo::Sort(Sorted, [&](const FCk_VoiceChat_TopNCandidate& InA, const FCk_VoiceChat_TopNCandidate& InB)
        {
            const auto BucketA = Get_Bucket(InA);
            const auto BucketB = Get_Bucket(InB);

            if (BucketA != BucketB)
            { return BucketA > BucketB; }

            if (InA.Get_LastServedFrame() != InB.Get_LastServedFrame())
            { return InA.Get_LastServedFrame() < InB.Get_LastServedFrame(); }

            return InA.Get_Id() < InB.Get_Id();
        });

        auto Selected = TArray<int32>{};
        Selected.Reserve(FMath::Min(InMaxTalkers, Sorted.Num()));

        for (auto Idx = 0; Idx < Sorted.Num() && Idx < InMaxTalkers; ++Idx)
        {
            Selected.Add(Sorted[Idx].Get_Id());
        }

        return Selected;
    }
}

// --------------------------------------------------------------------------------------------------------------------
