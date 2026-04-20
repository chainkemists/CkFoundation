#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkGoap_Fragment_Data.generated.h"

// ====================================================================================================================

class UCk_GoapAction_EntityScript;
class UCk_GoapGoal_EntityScript;

// ====================================================================================================================
// HANDLES
// ====================================================================================================================

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKGOAP_API FCk_Handle_Goap : public FCk_Handle_TypeSafe
{
	GENERATED_BODY()
	CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Goap);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Goap);

// ====================================================================================================================
// PLAN STATUS
// ====================================================================================================================

UENUM(BlueprintType)
enum class ECk_GoapPlanStatus : uint8
{
	Idle,
	Planning,
	PlanFound,
	PlanFailed,
	CostThresholdReached
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GoapPlanStatus);

// ====================================================================================================================
// REPLAN POLICY
// ====================================================================================================================

// Controls when the planner re-plans automatically. Dirty-tracking is
// value-change-based: setting a world-state key to its current value, or an
// action cost to its current value, does NOT trigger a replan.
UENUM(BlueprintType)
enum class ECk_Goap_ReplanPolicy : uint8
{
	// Re-plan only when consumer explicitly calls Request_Plan. World-state
	// and cost changes flag dirty state but are never acted on.
	Explicit,

	// Re-plan when any registered world-state key value changes.
	OnWorldStateDirty,

	// Re-plan when any action's cost changes.
	OnCostDirty,

	// Re-plan on either trigger.
	OnEitherDirty
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Goap_ReplanPolicy);

// ====================================================================================================================
// PARAMS
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Fragment_Goap_ParamsData
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Fragment_Goap_ParamsData);

private:
	// Per-tick time slice for A* iteration work. 0 = unbounded (finish in one
	// tick). Default 50ms is a generous one-shot ceiling; real-time AI should
	// lower this.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	int64 _SearchBudgetMicroseconds = 50000;

	// Early-out threshold on the best A* frontier FScore. If > 0, planner
	// returns CostThresholdReached (surfaced as PlanFailed) rather than
	// committing to a plan exceeding this cost. Useful for "not worth it"
	// behavior: AI abandons an expensive goal and the caller picks another.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	float _CostThreshold = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	ECk_Goap_ReplanPolicy _ReplanPolicy = ECk_Goap_ReplanPolicy::Explicit;

	// Minimum seconds between auto-replans. 0 = no throttle. Dirty events
	// inside the window coalesce into a single replan fired at window end.
	// Typical LOD usage: short interval for nearby AI, long for distant AI.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
	float _MinReplanIntervalSeconds = 0.0f;

	// If true, fire an initial Request_Plan automatically after setup
	// completes. Saves consumers from manually enqueuing a plan request in
	// DoConstruct.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	bool _PlanOnStart = true;

public:
	CK_PROPERTY(_SearchBudgetMicroseconds);
	CK_PROPERTY(_CostThreshold);
	CK_PROPERTY(_ReplanPolicy);
	CK_PROPERTY(_MinReplanIntervalSeconds);
	CK_PROPERTY(_PlanOnStart);
};

// ====================================================================================================================
// SIGNAL PAYLOADS
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Goap_Payload_OnPlanComplete
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Goap_Payload_OnPlanComplete);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	TArray<TSubclassOf<UCk_GoapAction_EntityScript>> _Actions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	float _TotalCost = 0.0f;

public:
	CK_PROPERTY_GET(_Actions);
	CK_PROPERTY_GET(_TotalCost);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Goap_Payload_OnPlanComplete, _Actions, _TotalCost);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Goap_Payload_OnPlanFailed
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Goap_Payload_OnPlanFailed);
};

// ====================================================================================================================
// DELEGATES
// ====================================================================================================================

DECLARE_DYNAMIC_DELEGATE_TwoParams(
	FCk_Delegate_Goap_OnPlanComplete,
	FCk_Handle_Goap, InHandle,
	FCk_Goap_Payload_OnPlanComplete, InPayload);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
	FCk_Delegate_Goap_OnPlanFailed,
	FCk_Handle_Goap, InHandle,
	FCk_Goap_Payload_OnPlanFailed, InPayload);

// ====================================================================================================================
// DIAGNOSTICS
// ====================================================================================================================

// A (tag, required-value) pair — describes a goal condition or an
// unreachable requirement in diagnostic output.
USTRUCT(BlueprintType)
struct CKGOAP_API FCk_GoapDiagnostic_ConditionPair
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_GoapDiagnostic_ConditionPair);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	FGameplayTag _Key;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	bool _Value = false;

public:
	CK_PROPERTY_GET(_Key);
	CK_PROPERTY_GET(_Value);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_GoapDiagnostic_ConditionPair, _Key, _Value);
};

// A strongly-connected component in the action dependency graph. Every action
// listed transitively depends on every other via an effect→precondition chain,
// so none of their effects can be produced from empty state. If the starting
// world state doesn't seed at least one condition in the cycle, actions inside
// it are unreachable.
USTRUCT(BlueprintType)
struct CKGOAP_API FCk_GoapDiagnostic_DependencyCycle
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_GoapDiagnostic_DependencyCycle);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	TArray<TSubclassOf<UCk_GoapAction_EntityScript>> _ActionsInCycle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	TArray<FGameplayTag> _CycleConditions;

public:
	CK_PROPERTY_GET(_ActionsInCycle);
	CK_PROPERTY_GET(_CycleConditions);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_GoapDiagnostic_DependencyCycle, _ActionsInCycle, _CycleConditions);
};

// ====================================================================================================================
// REQUESTS
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_Plan
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Request_Goap_Plan);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	TSubclassOf<UCk_GoapGoal_EntityScript> _SpecificGoalClass;

public:
	CK_PROPERTY(_SpecificGoalClass);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_SetWorldState
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Request_Goap_SetWorldState);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	FGameplayTag _Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	bool _Value = false;

public:
	CK_PROPERTY_GET(_Key);
	CK_PROPERTY_GET(_Value);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_SetWorldState, _Key, _Value);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_CancelPlan
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Request_Goap_CancelPlan);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_SetReplanInterval
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Request_Goap_SetReplanInterval);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
	float _MinReplanIntervalSeconds = 0.0f;

public:
	CK_PROPERTY_GET(_MinReplanIntervalSeconds);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_SetReplanInterval, _MinReplanIntervalSeconds);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_SetReplanPolicy
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Request_Goap_SetReplanPolicy);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	ECk_Goap_ReplanPolicy _ReplanPolicy = ECk_Goap_ReplanPolicy::Explicit;

public:
	CK_PROPERTY_GET(_ReplanPolicy);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_SetReplanPolicy, _ReplanPolicy);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_SetSearchBudget
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Request_Goap_SetSearchBudget);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	int64 _SearchBudgetMicroseconds = 50000;

public:
	CK_PROPERTY_GET(_SearchBudgetMicroseconds);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_SetSearchBudget, _SearchBudgetMicroseconds);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_SetCostThreshold
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Request_Goap_SetCostThreshold);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
	float _CostThreshold = 0.0f;

public:
	CK_PROPERTY_GET(_CostThreshold);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_SetCostThreshold, _CostThreshold);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Request_Goap_SetActionCost
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Request_Goap_SetActionCost);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	TSubclassOf<UCk_GoapAction_EntityScript> _ActionClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	float _NewCost = 1.0f;

public:
	CK_PROPERTY_GET(_ActionClass);
	CK_PROPERTY_GET(_NewCost);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Request_Goap_SetActionCost, _ActionClass, _NewCost);
};

// ====================================================================================================================
