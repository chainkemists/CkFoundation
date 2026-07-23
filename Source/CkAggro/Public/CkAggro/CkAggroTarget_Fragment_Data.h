#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkAggroTarget_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Whether a per-target value overrides the owner-wide default, or inherits it.
UENUM(BlueprintType)
enum class ECk_Aggro_OverridePolicy : uint8
{
    UseOwnerDefault,
    Override
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Aggro_OverridePolicy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKAGGRO_API FCk_Handle_AggroTarget : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_AggroTarget); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_AggroTarget);

// --------------------------------------------------------------------------------------------------------------------

// Per-target threat tuning. All optional — default-constructed means "inherit the owner's ThreatParams".
USTRUCT(BlueprintType)
struct CKAGGRO_API FCk_AggroTarget_ThreatParams
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_AggroTarget_ThreatParams);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Aggro_OverridePolicy _InitialThreatMode = ECk_Aggro_OverridePolicy::UseOwnerDefault;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_InitialThreatMode == ECk_Aggro_OverridePolicy::Override", EditConditionHides))
    float _InitialThreat = 1.0f;

    // Scales every incoming threat delta before it is applied to this target.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _ThreatMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _MaximumThreatOverrideMode = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_MaximumThreatOverrideMode == ECk_EnableDisable::Enable", EditConditionHides))
    float _MaximumThreatOverride = 0.0f;

public:
    CK_PROPERTY(_InitialThreatMode);
    CK_PROPERTY(_InitialThreat);
    CK_PROPERTY(_ThreatMultiplier);
    CK_PROPERTY(_MaximumThreatOverrideMode);
    CK_PROPERTY(_MaximumThreatOverride);
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

// Aggregate creation params for one tracked target. _TrackedEntity is the only essential — a default-constructed-
// except-TrackedEntity value means "inherit every owner default".
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
    FCk_AggroTarget_ScoreParams _ScoreParams;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_AggroTarget_LifetimeParams _LifetimeParams;

public:
    CK_PROPERTY_GET(_TrackedEntity);
    CK_PROPERTY(_Instigator);
    CK_PROPERTY(_Source);
    CK_PROPERTY(_ThreatParams);
    CK_PROPERTY(_ScoreParams);
    CK_PROPERTY(_LifetimeParams);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_AggroTarget_ParamsData, _TrackedEntity);
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
