#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkAggroTarget_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKAGGRO_API FCk_Handle_AggroTarget : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_AggroTarget); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_AggroTarget);

// --------------------------------------------------------------------------------------------------------------------

// Self-sufficient threat model for one target — the full behavior the Evaluate processor reads, carried on the target
// itself (not the owner). Every field has a sensible standalone default; the Aggro accelerant seeds these from its
// owner-wide defaults at CreateTarget time.
USTRUCT(BlueprintType)
struct CKAGGRO_API FCk_AggroTarget_ThreatParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_AggroTarget_ThreatParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _InitialThreat = 1.0f;

    // Scales every incoming threat delta before it is applied to this target.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _ThreatMultiplier = 1.0f;

    // Threat lost per second while perceived and within retention. 0 = threat never decays.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _ThreatDecayRate = 0.0f;

    // Decay accelerator applied while the target is unperceived (and past its lost-sight grace).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1.0"))
    float _UnperceivedThreatDecayMultiplier = 2.0f;

    // Below this, the target is forgotten (ThreatDepleted).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _MinimumTrackedThreat = 0.01f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_FloatRange _ThreatClampRange = FCk_FloatRange(0.0, 10000.0);

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _MaximumThreatOverrideMode = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_MaximumThreatOverrideMode == ECk_EnableDisable::Enable", EditConditionHides))
    float _MaximumThreatOverride = 0.0f;

public:
    CK_PROPERTY(_InitialThreat);
    CK_PROPERTY(_ThreatMultiplier);
    CK_PROPERTY(_ThreatDecayRate);
    CK_PROPERTY(_UnperceivedThreatDecayMultiplier);
    CK_PROPERTY(_MinimumTrackedThreat);
    CK_PROPERTY(_ThreatClampRange);
    CK_PROPERTY(_MaximumThreatOverrideMode);
    CK_PROPERTY(_MaximumThreatOverride);
};

// --------------------------------------------------------------------------------------------------------------------

// Self-sufficient spatial tuning for one target (distances in cm). Retention is clamped >= Acquisition at Add time.
USTRUCT(BlueprintType)
struct CKAGGRO_API FCk_AggroTarget_SpatialParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_AggroTarget_SpatialParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _AcquisitionDistance = 3000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _RetentionDistance = 4500.0f;

    // Decay accelerator applied while the target is beyond retention.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1.0"))
    float _OutOfRangeDecayMultiplier = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _DistanceFalloffHalfDistance = 1500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _DistanceFalloffExponent = 2.0f;

    // Optional bounded "prefer whoever is in my face" band. Disabled by default.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _NearbyPreference = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_NearbyPreference == ECk_EnableDisable::Enable", EditConditionHides))
    float _NearbyPreferenceDistance = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_NearbyPreference == ECk_EnableDisable::Enable", EditConditionHides))
    float _NearbyPreferenceMultiplier = 1.5f;

public:
    CK_PROPERTY(_AcquisitionDistance);
    CK_PROPERTY(_RetentionDistance);
    CK_PROPERTY(_OutOfRangeDecayMultiplier);
    CK_PROPERTY(_DistanceFalloffHalfDistance);
    CK_PROPERTY(_DistanceFalloffExponent);
    CK_PROPERTY(_NearbyPreference);
    CK_PROPERTY(_NearbyPreferenceDistance);
    CK_PROPERTY(_NearbyPreferenceMultiplier);
};

// --------------------------------------------------------------------------------------------------------------------

// Self-sufficient forget tuning for one target (inactivity/age rules). Capacity/eviction is owner-level (CkAggro).
USTRUCT(BlueprintType)
struct CKAGGRO_API FCk_AggroTarget_ForgetParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_AggroTarget_ForgetParams);

private:
    // Forget the target this long after its last threat/perception refresh (ThreatTimeout).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _ForgetDuration = FCk_Time{10.0};

    // After losing perception, threat keeps decaying at the perceived rate for this long before the unperceived
    // accelerator kicks in.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _LostSightGraceDuration = FCk_Time{3.0};

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _MaximumTargetAgeMode = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_MaximumTargetAgeMode == ECk_EnableDisable::Enable", EditConditionHides))
    FCk_Time _MaximumTargetAge;

public:
    CK_PROPERTY(_ForgetDuration);
    CK_PROPERTY(_LostSightGraceDuration);
    CK_PROPERTY(_MaximumTargetAgeMode);
    CK_PROPERTY(_MaximumTargetAge);
};

// --------------------------------------------------------------------------------------------------------------------

// Per-target score adjustment — the game-heuristic escape hatch (bias/multiplier applied to the computed score).
USTRUCT(BlueprintType)
struct CKAGGRO_API FCk_AggroTarget_ScoreParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_AggroTarget_ScoreParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _ScoreBias = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _ScoreMultiplier = 1.0f;

public:
    CK_PROPERTY(_ScoreBias);
    CK_PROPERTY(_ScoreMultiplier);
};

// --------------------------------------------------------------------------------------------------------------------

// Per-target lifetime/eligibility tuning.
USTRUCT(BlueprintType)
struct CKAGGRO_API FCk_AggroTarget_LifetimeParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_AggroTarget_LifetimeParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _MaximumLifetimeMode = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_MaximumLifetimeMode == ECk_EnableDisable::Enable", EditConditionHides))
    FCk_Time _MaximumLifetime;

    // Enable => this target may be selected as the active target. Disable => tracked but never made active.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _CanBecomeActiveTarget = ECk_EnableDisable::Enable;

    // Enable => this target may be forgotten by threat/age rules. Disable => sticky (only explicit Forget/invalid drops it).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _CanBeForgotten = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY(_MaximumLifetimeMode);
    CK_PROPERTY(_MaximumLifetime);
    CK_PROPERTY(_CanBecomeActiveTarget);
    CK_PROPERTY(_CanBeForgotten);
};

// --------------------------------------------------------------------------------------------------------------------

// Aggregate params for one target. All optional — a default-constructed value is a valid standalone target that tracks
// itself; _TrackedEntity names the subject (Create sets it; Add defaults it to the entity the feature is added to).
USTRUCT(BlueprintType)
struct CKAGGRO_API FCk_Fragment_AggroTarget_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_AggroTarget_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _TrackedEntity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Instigator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Source;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_AggroTarget_ThreatParams _ThreatParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_AggroTarget_SpatialParams _SpatialParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_AggroTarget_ForgetParams _ForgetParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_AggroTarget_ScoreParams _ScoreParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_AggroTarget_LifetimeParams _LifetimeParams;

public:
    CK_PROPERTY(_TrackedEntity);
    CK_PROPERTY(_Instigator);
    CK_PROPERTY(_Source);
    CK_PROPERTY(_ThreatParams);
    CK_PROPERTY(_SpatialParams);
    CK_PROPERTY(_ForgetParams);
    CK_PROPERTY(_ScoreParams);
    CK_PROPERTY(_LifetimeParams);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_AggroTarget_ParamsData, _TrackedEntity);
};

// --------------------------------------------------------------------------------------------------------------------

// Per-section override for Aggro::CreateTarget_WithParams. Each section is applied only when its toggle is on;
// otherwise the owner's DefaultTargetParams value is used.
USTRUCT(BlueprintType)
struct CKAGGRO_API FCk_AggroTarget_ParamOverrides
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_AggroTarget_ParamOverrides);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, InlineEditConditionToggle))
    bool _OverrideThreat = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_OverrideThreat"))
    FCk_AggroTarget_ThreatParams _ThreatParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, InlineEditConditionToggle))
    bool _OverrideSpatial = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_OverrideSpatial"))
    FCk_AggroTarget_SpatialParams _SpatialParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, InlineEditConditionToggle))
    bool _OverrideForget = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_OverrideForget"))
    FCk_AggroTarget_ForgetParams _ForgetParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, InlineEditConditionToggle))
    bool _OverrideScore = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_OverrideScore"))
    FCk_AggroTarget_ScoreParams _ScoreParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, InlineEditConditionToggle))
    bool _OverrideLifetime = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_OverrideLifetime"))
    FCk_AggroTarget_LifetimeParams _LifetimeParams;

public:
    CK_PROPERTY(_OverrideThreat);
    CK_PROPERTY(_ThreatParams);
    CK_PROPERTY(_OverrideSpatial);
    CK_PROPERTY(_SpatialParams);
    CK_PROPERTY(_OverrideForget);
    CK_PROPERTY(_ForgetParams);
    CK_PROPERTY(_OverrideScore);
    CK_PROPERTY(_ScoreParams);
    CK_PROPERTY(_OverrideLifetime);
    CK_PROPERTY(_LifetimeParams);
};

// --------------------------------------------------------------------------------------------------------------------

// Add (or subtract, when negative) threat on this target. Analytic decay is advanced to Now before the delta lands.
USTRUCT(BlueprintType)
struct CKAGGRO_API FCk_Request_AggroTarget_AddThreat : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AggroTarget_AddThreat);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_AggroTarget_AddThreat);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _ThreatDelta = 0.0f;

public:
    CK_PROPERTY_GET(_ThreatDelta);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_AggroTarget_AddThreat, _ThreatDelta);
};

// --------------------------------------------------------------------------------------------------------------------

// Overwrite the target's threat to an absolute value (clamped).
USTRUCT(BlueprintType)
struct CKAGGRO_API FCk_Request_AggroTarget_SetThreat : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AggroTarget_SetThreat);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_AggroTarget_SetThreat);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _Threat = 0.0f;

public:
    CK_PROPERTY_GET(_Threat);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_AggroTarget_SetThreat, _Threat);
};

// --------------------------------------------------------------------------------------------------------------------

// One sense reports this target perceived. Counted — N MarkPerceived need N MarkUnperceived. Optionally carries a
// known location; by default the handler resolves the tracked entity's transform.
USTRUCT(BlueprintType)
struct CKAGGRO_API FCk_Request_AggroTarget_MarkPerceived : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AggroTarget_MarkPerceived);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_AggroTarget_MarkPerceived);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _KnownLocationMode = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_KnownLocationMode == ECk_EnableDisable::Enable", EditConditionHides))
    FVector _KnownLocation = FVector::ZeroVector;

public:
    CK_PROPERTY(_KnownLocationMode);
    CK_PROPERTY(_KnownLocation);
};

// --------------------------------------------------------------------------------------------------------------------

// One sense reports this target no longer perceived. Counted decrement; a no-op at count 0.
USTRUCT(BlueprintType)
struct CKAGGRO_API FCk_Request_AggroTarget_MarkUnperceived : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AggroTarget_MarkUnperceived);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_AggroTarget_MarkUnperceived);
};

// --------------------------------------------------------------------------------------------------------------------

// Safety valve — clears the perception count outright regardless of how many senses had voted.
USTRUCT(BlueprintType)
struct CKAGGRO_API FCk_Request_AggroTarget_ResetPerception : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AggroTarget_ResetPerception);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_AggroTarget_ResetPerception);
};

// --------------------------------------------------------------------------------------------------------------------

// Explicit forget — always honored, bypasses CannotBeForgotten.
USTRUCT(BlueprintType)
struct CKAGGRO_API FCk_Request_AggroTarget_Forget : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_AggroTarget_Forget);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_AggroTarget_Forget);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_AggroTarget_OnThreatChanged,
    FCk_Handle_AggroTarget, InTarget,
    float, InOldThreat,
    float, InNewThreat);

// --------------------------------------------------------------------------------------------------------------------
