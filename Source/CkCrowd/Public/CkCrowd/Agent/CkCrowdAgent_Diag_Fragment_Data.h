#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>

#include "CkCrowdAgent_Diag_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_CrowdAgent_DiagRecorder;
}

class UCk_Utils_CrowdAgent_Diag_UE;

// --------------------------------------------------------------------------------------------------------------------
// Diagnostic recorder data — per-agent breadcrumb trail of (t, x, y, speed, dir) samples plus
// running per-cycle metrics (min-sep, dir-reversals, max angular delta, time-to-goal).
//
// Populated by FProcessor_CrowdAgent_DiagRecorder at the rate set by ck.Crowd.DiagSampleHz
// (default 10Hz). Consumed by:
//   - The diagnostic gym's EmitDigest at end of each cycle → grep-friendly log lines.
//   - The debugger's world-draw overlay → in-viewport breadcrumb polyline so devs can
//     see what an agent actually did on the path it tried to walk.
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCROWD_API FCk_CrowdDiag_PathSample
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_CrowdDiag_PathSample);

public:
    // Seconds since tracking started on this agent.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag")
    float _T = 0.0f;

    // World-space position at sample time. Stored as full FVector (not just x/y) so the breadcrumb
    // overlay shows Z-lift bugs without us having to add a separate Z field later.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag")
    FVector _Pos = FVector::ZeroVector;

    // Linear speed in cm/s (magnitude of desired velocity).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag")
    float _Speed = 0.0f;

    // Heading in radians, atan2(VelY, VelX). Zero when speed is zero.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag")
    float _DirRad = 0.0f;

public:
    CK_PROPERTY_GET(_T);
    CK_PROPERTY_GET(_Pos);
    CK_PROPERTY_GET(_Speed);
    CK_PROPERTY_GET(_DirRad);
};

// --------------------------------------------------------------------------------------------------------------------

// Per-agent recorder data. Lives on agent entities tagged with FTag_CrowdDiag_Tracked. The gym
// adds both via UCk_Utils_CrowdAgent_Diag_UE::Track at spawn time and supplies start/goal so
// efficiency (path_len / straight) can be computed at digest time without re-querying the agent.
USTRUCT(BlueprintType)
struct CKCROWD_API FCk_Fragment_CrowdAgent_DiagRecorderData
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Fragment_CrowdAgent_DiagRecorderData);

    friend class ck::FProcessor_CrowdAgent_DiagRecorder;
    friend class ::UCk_Utils_CrowdAgent_Diag_UE;

private:
    // Append-only sample buffer. ck.Crowd.DiagSampleHz controls the cadence; default 10Hz over
    // a 9s diag-gym cycle = ~90 samples per agent. Cheap to keep in memory; trimmed when the
    // agent destructs at cycle end.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    TArray<FCk_CrowdDiag_PathSample> _Samples;

    // Seconds since tracking started. Sample t = _ElapsedSec at sample time. Incremented every
    // tick by InDeltaT so it matches the recorder's notion of cycle time without needing absolute
    // world-time queries.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    float _ElapsedSec = 0.0f;

    // Accumulator for the next-sample timer; ticks against ck.Crowd.DiagSampleHz interval.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    float _SecsSinceLastSample = 0.0f;

    // Initial spawn position. Captured at Track() time. Used by Task E digest to compute the
    // straight-line distance for efficiency (path_len / straight).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    FVector _StartPos = FVector::ZeroVector;

    // Goal location at Track() time. Used to detect "reached" + compute straight distance.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    FVector _GoalPos = FVector::ZeroVector;

    // Per-cycle metrics — accumulated by the recorder as it samples.

    // Min distance to nearest neighbor across the whole tracked window (across the cluster's
    // 600cm radius the head-on hot moment is what matters; this catches clipping events).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    float _MinSepAcrossCycle = TNumericLimits<float>::Max();

    // Sample t at which _MinSepAcrossCycle was observed.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    float _MinSepTime = 0.0f;

    // Number of times the heading flipped > 90° between consecutive samples — proxy for the
    // "vibrating" / "dancing" failure mode.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    int32 _DirReversalCount = 0;

    // Max angular delta (degrees) between any two consecutive samples. Captures direction
    // jitter even when not full reversals.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    float _MaxAngularDeltaDeg = 0.0f;

    // True once the agent crossed within an arrival window of _GoalPos. Sticky.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    bool _Reached = false;

    // Sample t at which _Reached first became true.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    float _TimeToGoal = 0.0f;

public:
    CK_PROPERTY_GET(_Samples);
    CK_PROPERTY_GET(_ElapsedSec);
    CK_PROPERTY_GET(_StartPos);
    CK_PROPERTY_GET(_GoalPos);
    CK_PROPERTY_GET(_MinSepAcrossCycle);
    CK_PROPERTY_GET(_MinSepTime);
    CK_PROPERTY_GET(_DirReversalCount);
    CK_PROPERTY_GET(_MaxAngularDeltaDeg);
    CK_PROPERTY_GET(_Reached);
    CK_PROPERTY_GET(_TimeToGoal);
};

// --------------------------------------------------------------------------------------------------------------------
