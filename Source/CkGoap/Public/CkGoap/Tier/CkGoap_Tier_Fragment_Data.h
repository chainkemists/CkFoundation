#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkGoap/CkGoap_Fragment_Data.h"
#include "CkGoap/WorldState/CkGoap_WorldState_Fragment_Data.h"

#include "CkGoap_Tier_Fragment_Data.generated.h"

// ====================================================================================================================
// HANDLE
// ====================================================================================================================

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKGOAP_API FCk_Handle_Goap_Tier : public FCk_Handle_TypeSafe
{
	GENERATED_BODY()
	CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Goap_Tier);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Goap_Tier);

// ====================================================================================================================
// TIER PARAMS
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Fragment_Goap_TierParamsData
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Fragment_Goap_TierParamsData);

private:
	// Tier identity within an ActionSet. Strict equality with an Action's
	// _ActionTag drives the chain-update rule — when Plan[0]._ActionTag ==
	// SomeTier._TierTag, that tier becomes the current tier's sub-tier.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true, Categories = "Goap.Tier"))
	FGameplayTag _TierTag;

	// Optional WS override. If unset, this tier inherits its parent tier's
	// resolved WS at activation time. The ROOT tier MUST set this — there is
	// no parent for it to inherit from.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	FCk_Handle_Goap_WorldState _WorldStateSource_Override;

	// Root-only: initial goal world state. Ignored on non-root tiers (their
	// goal is injected from the parent action's Effects at activation).
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	TArray<FCk_GoapWS_Condition_Authored> _InitialGoal_RootOnly;

	// Per-tick A* time slice (microseconds). 0 = unbounded (finish in one tick).
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	int64 _SearchBudgetMicroseconds = 50000;

	// Early-out threshold on best A* frontier FScore. If > 0, planner returns
	// CostThresholdReached rather than committing to a plan exceeding it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	float _CostThreshold = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	ECk_Goap_ReplanPolicy _ReplanPolicy = ECk_Goap_ReplanPolicy::OnWorldStateDirty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
	float _MinReplanIntervalSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	bool _PlanOnStart = true;

public:
	CK_PROPERTY_GET(_TierTag);
	CK_PROPERTY(_WorldStateSource_Override);
	CK_PROPERTY(_InitialGoal_RootOnly);
	CK_PROPERTY(_SearchBudgetMicroseconds);
	CK_PROPERTY(_CostThreshold);
	CK_PROPERTY(_ReplanPolicy);
	CK_PROPERTY(_MinReplanIntervalSeconds);
	CK_PROPERTY(_PlanOnStart);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Goap_TierParamsData, _TierTag);
};

// ====================================================================================================================
// PER-TIER REQUESTS
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Tier_Plan
{
	GENERATED_BODY()
	CK_GENERATED_BODY(FCk_Request_Goap_Tier_Plan);
};

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Tier_CancelPlan
{
	GENERATED_BODY()
	CK_GENERATED_BODY(FCk_Request_Goap_Tier_CancelPlan);
};

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Tier_SetGoal
{
	GENERATED_BODY()
	CK_GENERATED_BODY(FCk_Request_Goap_Tier_SetGoal);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
	TArray<FCk_GoapWS_Condition_Authored> _Goal;

public:
	CK_PROPERTY_GET(_Goal);
	CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_Tier_SetGoal, _Goal);
};

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Tier_SetActionCost
{
	GENERATED_BODY()
	CK_GENERATED_BODY(FCk_Request_Goap_Tier_SetActionCost);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
	TSubclassOf<UCk_GoapAction_EntityScript> _ActionClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
	float _Cost = 0.0f;

public:
	CK_PROPERTY_GET(_ActionClass);
	CK_PROPERTY_GET(_Cost);
	CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_Tier_SetActionCost, _ActionClass, _Cost);
};

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Tier_SetReplanInterval
{
	GENERATED_BODY()
	CK_GENERATED_BODY(FCk_Request_Goap_Tier_SetReplanInterval);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
	float _MinReplanIntervalSeconds = 0.0f;

public:
	CK_PROPERTY_GET(_MinReplanIntervalSeconds);
	CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_Tier_SetReplanInterval, _MinReplanIntervalSeconds);
};

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Tier_SetReplanPolicy
{
	GENERATED_BODY()
	CK_GENERATED_BODY(FCk_Request_Goap_Tier_SetReplanPolicy);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
	ECk_Goap_ReplanPolicy _Policy = ECk_Goap_ReplanPolicy::OnWorldStateDirty;

public:
	CK_PROPERTY_GET(_Policy);
	CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_Tier_SetReplanPolicy, _Policy);
};

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Tier_SetSearchBudget
{
	GENERATED_BODY()
	CK_GENERATED_BODY(FCk_Request_Goap_Tier_SetSearchBudget);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true, ClampMin = "0"))
	int64 _SearchBudgetMicroseconds = 50000;

public:
	CK_PROPERTY_GET(_SearchBudgetMicroseconds);
	CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_Tier_SetSearchBudget, _SearchBudgetMicroseconds);
};

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Tier_SetCostThreshold
{
	GENERATED_BODY()
	CK_GENERATED_BODY(FCk_Request_Goap_Tier_SetCostThreshold);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
	float _CostThreshold = 0.0f;

public:
	CK_PROPERTY_GET(_CostThreshold);
	CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_Tier_SetCostThreshold, _CostThreshold);
};

// ====================================================================================================================
// SIGNAL PAYLOADS — Tier-scoped
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Goap_Payload_OnTierActivated
{
	GENERATED_BODY()
	CK_GENERATED_BODY(FCk_Goap_Payload_OnTierActivated);
};

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Goap_Payload_OnTierDeactivated
{
	GENERATED_BODY()
	CK_GENERATED_BODY(FCk_Goap_Payload_OnTierDeactivated);
};

// ====================================================================================================================
// DELEGATES — Tier-scoped
//
// FCk_Goap_Payload_OnPlanComplete / _OnPlanFailed already exist in
// CkGoap_Fragment_Data.h and are reused here — only the SOURCE handle type
// differs from the planner-era delegates.
// ====================================================================================================================

DECLARE_DYNAMIC_DELEGATE_TwoParams(
	FCk_Delegate_Goap_OnTierPlanComplete,
	FCk_Handle_Goap_Tier, InTier,
	FCk_Goap_Payload_OnPlanComplete, InPayload);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
	FCk_Delegate_Goap_OnTierPlanFailed,
	FCk_Handle_Goap_Tier, InTier,
	FCk_Goap_Payload_OnPlanFailed, InPayload);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
	FCk_Delegate_Goap_OnTierActivated,
	FCk_Handle_Goap_Tier, InTier,
	FCk_Goap_Payload_OnTierActivated, InPayload);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
	FCk_Delegate_Goap_OnTierDeactivated,
	FCk_Handle_Goap_Tier, InTier,
	FCk_Goap_Payload_OnTierDeactivated, InPayload);

