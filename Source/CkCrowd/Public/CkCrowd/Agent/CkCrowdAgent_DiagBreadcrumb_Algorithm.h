#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::crowd_diag_breadcrumb
{
    inline constexpr int32 SegmentsPerChunk = 128;
    inline constexpr int32 MaximumRetainedChunks = 8;
    inline constexpr int32 MaximumSamplesPerUpdate = 256;
    inline constexpr float MinimumSegmentLength = 1.0f;
    inline constexpr int32 DefaultMaximumRecordedSamples = 4096;
    inline constexpr int32 MinimumMaximumRecordedSamples = 256;

    inline auto
        GetRecorderTrimCount(
            int32 InSampleCount,
            int32 InMaximumSamples)
        -> int32
    {
        const auto MaximumSamples = FMath::Max(MinimumMaximumRecordedSamples, InMaximumSamples);
        if (InSampleCount <= MaximumSamples)
        { return 0; }

        const auto TrimTarget = MaximumSamples - MaximumSamples / 4;
        return InSampleCount - TrimTarget;
    }

    struct FHistoryState
    {
        int32 _TrackGeneration = INDEX_NONE;
        int32 _NextSampleIndex = 0;
        FVector _PreviousPosition = FVector::ZeroVector;
        bool _HasPreviousPosition = false;
        int32 _ActiveChunkSegments = 0;
        int32 _RetainedChunkCount = 0;
    };

    struct FUpdateRange
    {
        int32 _BeginSampleIndex = 0;
        int32 _EndSampleIndex = 0;
        bool _NeedsGeometryReset = false;
    };

    inline auto
        PrepareUpdate(
            int32 InSampleCount,
            int32 InTrackGeneration,
            const FVector& InTrackStart,
            FHistoryState& InOutState)
        -> FUpdateRange
    {
        auto NeedsReset = InOutState._TrackGeneration != InTrackGeneration ||
                          InOutState._NextSampleIndex > InSampleCount;
        if (NeedsReset)
        {
            InOutState = FHistoryState{};
            InOutState._TrackGeneration = InTrackGeneration;
            InOutState._PreviousPosition = InTrackStart;
            InOutState._HasPreviousPosition = NOT InTrackStart.ContainsNaN();
        }

        const auto Begin = InOutState._NextSampleIndex;
        const auto End = FMath::Min(InSampleCount, Begin + MaximumSamplesPerUpdate);
        return FUpdateRange{Begin, End, NeedsReset};
    }

    struct FSegmentPlan
    {
        FHistoryState _NextState;
        FVector _Start = FVector::ZeroVector;
        FVector _End = FVector::ZeroVector;
        bool _ShouldAppend = false;
        bool _ShouldStartChunk = false;
        bool _ShouldEvictOldestChunk = false;
    };

    inline auto
        PlanSample(
            const FVector& InSamplePosition,
            const FHistoryState& InState)
        -> FSegmentPlan
    {
        auto Result = FSegmentPlan{};
        Result._NextState = InState;
        ++Result._NextState._NextSampleIndex;

        if (InSamplePosition.ContainsNaN())
        { return Result; }

        if (NOT InState._HasPreviousPosition || InState._PreviousPosition.ContainsNaN())
        {
            Result._NextState._PreviousPosition = InSamplePosition;
            Result._NextState._HasPreviousPosition = true;
            return Result;
        }

        Result._Start = InState._PreviousPosition;
        Result._End = InSamplePosition;
        Result._NextState._PreviousPosition = InSamplePosition;

        if (FVector::DistSquared(Result._Start, Result._End) < FMath::Square(MinimumSegmentLength))
        { return Result; }

        Result._ShouldAppend = true;
        Result._ShouldStartChunk = InState._RetainedChunkCount == 0 ||
                                   InState._ActiveChunkSegments >= SegmentsPerChunk;
        if (Result._ShouldStartChunk)
        {
            Result._NextState._ActiveChunkSegments = 0;
            ++Result._NextState._RetainedChunkCount;
            if (Result._NextState._RetainedChunkCount > MaximumRetainedChunks)
            {
                Result._ShouldEvictOldestChunk = true;
                Result._NextState._RetainedChunkCount = MaximumRetainedChunks;
            }
        }

        ++Result._NextState._ActiveChunkSegments;
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
