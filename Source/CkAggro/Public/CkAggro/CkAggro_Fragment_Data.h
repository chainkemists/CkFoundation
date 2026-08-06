#pragma once

#include "CkAggro/CkAggroTarget_Fragment_Data.h"

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkAggro_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// What happens when a new target is admitted while the tracked-target cap is full.
UENUM(BlueprintType)
enum class ECk_Aggro_EvictionPolicy : uint8
{
    // Forget the lowest-threat currently-tracked target to make room.
    EvictLowestThreat,
    // Reject the incoming target; the cap holds.
    RejectNew
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Aggro_EvictionPolicy);

// --------------------------------------------------------------------------------------------------------------------

// Whether a threat request may create a not-yet-tracked target, or requires it to already exist.
UENUM(BlueprintType)
enum class ECk_Aggro_TargetCreationPolicy : uint8
{
    CreateIfMissing,
    RequireExisting
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Aggro_TargetCreationPolicy);

// --------------------------------------------------------------------------------------------------------------------

// Why a target was forgotten — carried in the OnAggroTargetForgotten payload.
UENUM(BlueprintType)
enum class ECk_Aggro_ForgetReason : uint8
{
    ThreatDepleted,
    ThreatTimeout,
    MaxAgeReached,
    MaxLifetimeReached,
    TargetInvalid,
    Evicted,
    Requested
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Aggro_ForgetReason);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKAGGRO_API FCk_Handle_Aggro : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Aggro); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Aggro);

// --------------------------------------------------------------------------------------------------------------------

// Owner-wide active-target selection + hysteresis tuning.
USTRUCT(BlueprintType)
struct CKAGGRO_API FCk_Aggro_SelectionParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Aggro_SelectionParams);

private:
    // A challenger must beat the incumbent's biased score by at least this factor to trigger a switch.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1.0"))
    float _TargetSwitchThreshold = 1.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _TargetSwitchCooldown = FCk_Time{2.0};

    // Incumbent stickiness — its score is multiplied by this when compared against challengers.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1.0"))
    float _CurrentTargetBias = 1.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _MinimumTargetScore = 0.0f;

    // Minimum time an active target is held before hysteresis allows a switch away from it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _MinimumAggroDuration = FCk_Time{1.0};

public:
    CK_PROPERTY(_TargetSwitchThreshold);
    CK_PROPERTY(_TargetSwitchCooldown);
    CK_PROPERTY(_CurrentTargetBias);
    CK_PROPERTY(_MinimumTargetScore);
    CK_PROPERTY(_MinimumAggroDuration);
};

// --------------------------------------------------------------------------------------------------------------------

// Owner-wide target-set capacity + eviction. Per-target forget rules (duration/grace/age) live on the AggroTarget.
USTRUCT(BlueprintType)
struct CKAGGRO_API FCk_Aggro_CapParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Aggro_CapParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _TargetCapMode = ECk_EnableDisable::Enable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_TargetCapMode == ECk_EnableDisable::Enable", EditConditionHides, ClampMin = "1"))
    int32 _MaximumTrackedTargets = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_TargetCapMode == ECk_EnableDisable::Enable", EditConditionHides))
    ECk_Aggro_EvictionPolicy _EvictionPolicy = ECk_Aggro_EvictionPolicy::EvictLowestThreat;

public:
    CK_PROPERTY(_TargetCapMode);
    CK_PROPERTY(_MaximumTrackedTargets);
    CK_PROPERTY(_EvictionPolicy);
};

// --------------------------------------------------------------------------------------------------------------------

// Owner-wide evaluation cadence. Interval + a per-rearm jitter in [0, Jitter] staggers the fleet.
USTRUCT(BlueprintType)
struct CKAGGRO_API FCk_Aggro_EvaluationParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Aggro_EvaluationParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _EvaluationInterval = FCk_Time{0.25};

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _EvaluationJitter = FCk_Time{0.1};

public:
    CK_PROPERTY(_EvaluationInterval);
    CK_PROPERTY(_EvaluationJitter);
};

// --------------------------------------------------------------------------------------------------------------------

// Aggregate owner params. Aggro is the accelerant on top of AggroTarget: _DefaultTargetParams is the template it
// stamps onto new targets (CreateTarget); Selection/Cap/Evaluation are the owner-level concerns.
USTRUCT(BlueprintType)
struct CKAGGRO_API FCk_Aggro_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Aggro_Spec);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_AggroTarget_Spec _DefaultTargetParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Aggro_SelectionParams _SelectionParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Aggro_CapParams _CapParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Aggro_EvaluationParams _EvaluationParams;

public:
    CK_PROPERTY(_DefaultTargetParams);
    CK_PROPERTY(_SelectionParams);
    CK_PROPERTY(_CapParams);
    CK_PROPERTY(_EvaluationParams);
};

// --------------------------------------------------------------------------------------------------------------------

// The DamageResolution one-liner: add threat from InstigatorTarget onto the owner's table, creating the tracked
// target on demand (per CreatePolicy).
USTRUCT(BlueprintType)
struct CKAGGRO_API FCk_Request_Aggro_AddThreat : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Aggro_AddThreat);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Aggro_AddThreat);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _TrackedEntity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _ThreatAmount = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Instigator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Source;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Aggro_TargetCreationPolicy _CreatePolicy = ECk_Aggro_TargetCreationPolicy::CreateIfMissing;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_AggroTarget_Spec _TargetParams;

public:
    CK_PROPERTY_GET(_TrackedEntity);
    CK_PROPERTY_GET(_ThreatAmount);
    CK_PROPERTY(_Instigator);
    CK_PROPERTY(_Source);
    CK_PROPERTY(_CreatePolicy);
    CK_PROPERTY(_TargetParams);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Aggro_AddThreat, _TrackedEntity, _ThreatAmount);
};

// --------------------------------------------------------------------------------------------------------------------

// Forget a specific tracked target by its tracked entity.
USTRUCT(BlueprintType)
struct CKAGGRO_API FCk_Request_Aggro_RemoveTarget : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Aggro_RemoveTarget);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Aggro_RemoveTarget);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _TrackedEntity;

public:
    CK_PROPERTY_GET(_TrackedEntity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Aggro_RemoveTarget, _TrackedEntity);
};

// --------------------------------------------------------------------------------------------------------------------

// Forget every tracked target.
USTRUCT(BlueprintType)
struct CKAGGRO_API FCk_Request_Aggro_ClearAllTargets : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Aggro_ClearAllTargets);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Aggro_ClearAllTargets);
};

// --------------------------------------------------------------------------------------------------------------------

// Force the active target (taunt) — bypasses selection hysteresis. The target must already be tracked.
USTRUCT(BlueprintType)
struct CKAGGRO_API FCk_Request_Aggro_SetActiveTarget : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Aggro_SetActiveTarget);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Aggro_SetActiveTarget);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _TrackedEntity;

public:
    CK_PROPERTY_GET(_TrackedEntity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Aggro_SetActiveTarget, _TrackedEntity);
};

// --------------------------------------------------------------------------------------------------------------------

// Clear the active target without forgetting it; selection re-runs next evaluation.
USTRUCT(BlueprintType)
struct CKAGGRO_API FCk_Request_Aggro_ClearActiveTarget : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Aggro_ClearActiveTarget);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Aggro_ClearActiveTarget);
};

// --------------------------------------------------------------------------------------------------------------------

// OnAggroTargetForgotten payload — outlives the dying AggroTarget entity, so it carries copies rather than the handle.
USTRUCT(BlueprintType)
struct CKAGGRO_API FCk_Aggro_TargetForgottenInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Aggro_TargetForgottenInfo);

private:
    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _TrackedEntity;

    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _FinalThreat = 0.0f;

    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Aggro_ForgetReason _Reason = ECk_Aggro_ForgetReason::ThreatDepleted;

public:
    CK_PROPERTY_GET(_TrackedEntity);
    CK_PROPERTY_GET(_FinalThreat);
    CK_PROPERTY_GET(_Reason);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Aggro_TargetForgottenInfo, _TrackedEntity, _FinalThreat, _Reason);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Aggro_OnTargetAcquired,
    FCk_Handle_Aggro, InAggro,
    FCk_Handle_AggroTarget, InTarget);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_Aggro_OnActiveTargetChanged,
    FCk_Handle_Aggro, InAggro,
    FCk_Handle_AggroTarget, InPreviousTarget,
    FCk_Handle_AggroTarget, InNewTarget);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Aggro_OnTargetForgotten,
    FCk_Handle_Aggro, InAggro,
    FCk_Aggro_TargetForgottenInfo, InForgottenInfo);

// --------------------------------------------------------------------------------------------------------------------
