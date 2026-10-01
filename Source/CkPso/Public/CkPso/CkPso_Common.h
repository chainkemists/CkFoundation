#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkPso/Drain/CkPso_DrainTracker.h"

#include "CkPso_Common.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Pso_DriverCacheHealth : uint8
{
    Healthy,
    SuspectedUnhealthy
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Pso_DriverCacheHealth);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Snapshot of the PSO subsystem's drain window. Counts are reflected as int32 and saturate at MAX_int32.
 * _ProgressRatio is 0..1 and never decreases within a window.
 */
USTRUCT(BlueprintType)
struct CKPSO_API FCk_Pso_DrainProgress
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Pso_DrainProgress);

private:
    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Pso_DrainState _State = ECk_Pso_DrainState::Idle;

    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Pso_DrainTimeoutReason _TimeoutReason = ECk_Pso_DrainTimeoutReason::None;

    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _NumRemaining = 0;

    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _NumPeak = 0;

    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _ProgressRatio = 0.0f;

    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Time _Elapsed;

public:
    CK_PROPERTY_GET(_State);
    CK_PROPERTY_GET(_TimeoutReason);
    CK_PROPERTY_GET(_NumRemaining);
    CK_PROPERTY_GET(_NumPeak);
    CK_PROPERTY_GET(_ProgressRatio);
    CK_PROPERTY_GET(_Elapsed);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Pso_DrainProgress, _State, _TimeoutReason, _NumRemaining, _NumPeak, _ProgressRatio, _Elapsed);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Runtime PSO creation counters for the current gameplay window (loading screen hidden -> now), as deltas from the
 * baseline taken when the screen last dropped. Before the first drop the baseline is the subsystem's creation.
 *
 * _DriverCacheHealth is the engine's current verdict, not a delta.
 *
 * The _FullPso* counters are only meaningful when _Validation is Enable: that requires a non-editor build with PSO
 * precache validation compiled in AND r.PSOPrecache.Validation set at launch (it is read-only at runtime).
 */
USTRUCT(BlueprintType)
struct CKPSO_API FCk_Pso_HitchReport
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Pso_HitchReport);

private:
    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _TotalPsoCreations = 0;

    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _GraphicsPsoHitches = 0;

    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _ComputePsoHitches = 0;

    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _PreviouslyPrecachedPsoHitches = 0;

    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _SuspectedUnhealthyDriverCachePsoHitches = 0;

    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Pso_DriverCacheHealth _DriverCacheHealth = ECk_Pso_DriverCacheHealth::Healthy;

    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _Validation = ECk_EnableDisable::Disable;

    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _FullPsoHits = 0;

    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _FullPsoMisses = 0;

    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _FullPsoTooLate = 0;

    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _FullPsoUntracked = 0;

public:
    CK_PROPERTY(_TotalPsoCreations);
    CK_PROPERTY(_GraphicsPsoHitches);
    CK_PROPERTY(_ComputePsoHitches);
    CK_PROPERTY(_PreviouslyPrecachedPsoHitches);
    CK_PROPERTY(_SuspectedUnhealthyDriverCachePsoHitches);
    CK_PROPERTY(_DriverCacheHealth);
    CK_PROPERTY(_Validation);
    CK_PROPERTY(_FullPsoHits);
    CK_PROPERTY(_FullPsoMisses);
    CK_PROPERTY(_FullPsoTooLate);
    CK_PROPERTY(_FullPsoUntracked);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_Pso_OnDrainProgressChanged,
    const FCk_Pso_DrainProgress&,
    InProgress);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_Pso_OnDrainProgressChanged_MC,
    const FCk_Pso_DrainProgress&,
    InProgress);

// --------------------------------------------------------------------------------------------------------------------
