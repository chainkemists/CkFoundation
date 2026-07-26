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
// Diagnostic recorder data — a per-agent breadcrumb trail plus running per-cycle metrics, populated
// by FProcessor_CrowdAgent_DiagRecorder and consumed by the diag gym's digest + the debugger overlay.
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

    // World-space position at sample time.
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

// Per-agent recorder data, added alongside FTag_CrowdDiag_Tracked by Track() at spawn time.
USTRUCT(BlueprintType)
struct CKCROWD_API FCk_Fragment_CrowdAgent_DiagRecorderData
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Fragment_CrowdAgent_DiagRecorderData);

    friend class ck::FProcessor_CrowdAgent_DiagRecorder;
    friend class ::UCk_Utils_CrowdAgent_Diag_UE;

private:
    // Append-only sample buffer; cadence set by ck.Crowd.SampleHz.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    TArray<FCk_CrowdDiag_PathSample> _Samples;

    // Seconds since tracking started; a sample's _T is this value at sample time.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    float _ElapsedSec = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    float _SecsSinceLastSample = 0.0f;

    // Spawn position, captured at Track() time.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    FVector _StartPos = FVector::ZeroVector;

    // Goal location at Track() time.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    FVector _GoalPos = FVector::ZeroVector;

    // Min distance to the nearest neighbour across the tracked window — catches clipping events.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    float _MinSepAcrossCycle = TNumericLimits<float>::Max();

    // Sample t at which _MinSepAcrossCycle was observed.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    float _MinSepTime = 0.0f;

    // Heading flips > 90° between consecutive samples — proxy for the "vibrating" failure mode.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ck|CrowdDiag",
              meta = (AllowPrivateAccess = true))
    int32 _DirReversalCount = 0;

    // Max angular delta (degrees) between consecutive samples — catches jitter short of a reversal.
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
