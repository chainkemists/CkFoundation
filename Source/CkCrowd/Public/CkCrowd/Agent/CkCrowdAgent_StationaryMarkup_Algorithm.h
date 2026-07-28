#pragma once

#include "CoreMinimal.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::ck_crowd_agent_stationary_markup_algorithm
{
    inline auto IsValidSpeedThreshold(const float InSpeedThresholdUuPerSec) -> bool
    {
        return FMath::IsFinite(InSpeedThresholdUuPerSec) &&
            InSpeedThresholdUuPerSec >= 0.0f;
    }

    // A coarse position window classifies physical movement more reliably than the instantaneous
    // velocity fragment here: push-apart can produce a one-frame command spike while a queue member
    // remains effectively planted.
    inline auto IsStationarySample(
        const FVector& InSampleStart,
        const FVector& InSampleEnd,
        const float InSampleSeconds,
        const float InSpeedThresholdUuPerSec) -> bool
    {
        const auto InputsAreValid =
            NOT InSampleStart.ContainsNaN() &&
            NOT InSampleEnd.ContainsNaN() &&
            FMath::IsFinite(InSampleSeconds) &&
            InSampleSeconds > 0.0f &&
            IsValidSpeedThreshold(InSpeedThresholdUuPerSec);
        CK_ENSURE_IF_NOT(
            InputsAreValid,
            TEXT("Invalid stationary-markup movement sample "
                 "(start [{}], end [{}], seconds [{}], speed_threshold [{}])"),
            InSampleStart,
            InSampleEnd,
            InSampleSeconds,
            InSpeedThresholdUuPerSec)
        {}
        if (NOT InputsAreValid)
        { return false; }

        const auto WindowedSpeedUuPerSec =
            FVector::Dist2D(InSampleStart, InSampleEnd) / InSampleSeconds;
        return WindowedSpeedUuPerSec <= InSpeedThresholdUuPerSec;
    }
}

// --------------------------------------------------------------------------------------------------------------------
