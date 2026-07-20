#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkGoap/WorldState/CkGoap_WorldState_Fragment_Data.h"

#include "CkGoap_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_GoapAction_EntityScript;
class UCk_Utils_Goap_Planner_UE;

namespace ck { class FProcessor_Goap_Planner_HandleRequests; }

// --------------------------------------------------------------------------------------------------------------------

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

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Goap_ReplanPolicy : uint8
{
	// Re-plan only when consumer explicitly calls Request_Plan.
	Explicit,

	// Re-plan when any registered WS key value changes.
	OnWorldStateDirty,

	// Re-plan when any action's cost changes.
	OnCostDirty,

	// Re-plan on either trigger.
	OnEitherDirty
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Goap_ReplanPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Goap_ReplanOrigin : uint8
{
	// No replan recorded yet.
	None,

	// First plan after activation (_PlanOnStart).
	PlanOnStart,

	// Consumer called Request_Plan directly.
	Explicit,

	// AutoReplan fired on a world-state change.
	WorldStateDirty,

	// AutoReplan fired on an action-cost change.
	CostDirty,

	// AutoReplan fired with both dirty flags set in the same window.
	WorldStateAndCostDirty,

	// Request_SetGoal replans with the new goal.
	GoalChanged,

	// AddAction / Request_RemoveAction rebuilt the operator set.
	CatalogChanged
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Goap_ReplanOrigin);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Goap_WorldStateMutator : uint8
{
	SetValue,
	OverridePush,
	OverridePop,
	OverrideClear
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Goap_WorldStateMutator);

// --------------------------------------------------------------------------------------------------------------------
// One recorded world-state mutation whose EFFECTIVE value changed. The ring on
// the WorldState entity keeps the most recent entries (see
// FFragment_Goap_WorldState_ChangeLog); the debugger's timeline lane and the
// Planner's replan-cause attribution both read it.

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Goap_WorldStateChange
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Goap_WorldStateChange);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	FGameplayTag _Key;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	bool _OldValue = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	bool _NewValue = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	int64 _FrameNumber = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	ECk_Goap_WorldStateMutator _Mutator = ECk_Goap_WorldStateMutator::SetValue;

public:
	CK_PROPERTY_GET(_Key);
	CK_PROPERTY_GET(_OldValue);
	CK_PROPERTY_GET(_NewValue);
	CK_PROPERTY_GET(_FrameNumber);
	CK_PROPERTY_GET(_Mutator);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Goap_WorldStateChange, _Key, _OldValue, _NewValue, _FrameNumber, _Mutator);
};

// --------------------------------------------------------------------------------------------------------------------
// Why the last replan fired: origin + the world-state changes recorded since
// the previous replan. ChangedKeys.Num() > 1 means the throttle window
// coalesced several dirty events into this one replan.

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Goap_ReplanCauseInfo
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Goap_ReplanCauseInfo);

	friend class ck::FProcessor_Goap_Planner_HandleRequests;
	friend class ::UCk_Utils_Goap_Planner_UE;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	ECk_Goap_ReplanOrigin _Origin = ECk_Goap_ReplanOrigin::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	TArray<FCk_Goap_WorldStateChange> _ChangedKeys;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	int32 _AttemptNumber = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	int64 _FrameNumber = 0;

public:
	CK_PROPERTY_GET(_Origin);
	CK_PROPERTY_GET(_ChangedKeys);
	CK_PROPERTY_GET(_AttemptNumber);
	CK_PROPERTY_GET(_FrameNumber);
};

// --------------------------------------------------------------------------------------------------------------------
// Post-search statistics for the Planner's most recent A* run.

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Goap_SearchStats
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Goap_SearchStats);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	ECk_GoapPlanStatus _PlanStatus = ECk_GoapPlanStatus::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	int32 _Iterations = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	int64 _ElapsedMicroseconds = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	int32 _StatePoolSize = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	int32 _PlanLength = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	float _PlanCost = 0.0f;

public:
	CK_PROPERTY_GET(_PlanStatus);
	CK_PROPERTY_GET(_Iterations);
	CK_PROPERTY_GET(_ElapsedMicroseconds);
	CK_PROPERTY_GET(_StatePoolSize);
	CK_PROPERTY_GET(_PlanLength);
	CK_PROPERTY_GET(_PlanCost);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Goap_SearchStats, _PlanStatus, _Iterations, _ElapsedMicroseconds, _StatePoolSize, _PlanLength, _PlanCost);
};

// --------------------------------------------------------------------------------------------------------------------
// BlueprintType-friendly (tag, bool) pair for declaring
// goal / initial-state entries from editor or AngelScript. The Setup processor
// resolves these into the internal FCk_GoapKey-indexed form against the
// WorldState registry.

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_GoapWS_Condition_Authored
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_GoapWS_Condition_Authored);

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true, Categories = "Goap"))
	FGameplayTag _Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	bool _Value = false;

public:
	CK_PROPERTY_GET(_Key);
	CK_PROPERTY_GET(_Value);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_GoapWS_Condition_Authored, _Key, _Value);
};

// --------------------------------------------------------------------------------------------------------------------
// Reused by Action-scoped signals declared in Action/CkGoap_Action_Fragment_Data.h

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

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Goap_Payload_OnPlanFailed
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Goap_Payload_OnPlanFailed);
};

// --------------------------------------------------------------------------------------------------------------------

// A (tag, required-value) pair — describes a goal condition or an unreachable
// requirement in diagnostic output.
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
// listed transitively depends on every other via an effect→precondition chain.
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

// --------------------------------------------------------------------------------------------------------------------
// One regressive-search node from the last completed search, in discovery
// order: the constraint set, the action whose reverse application produced it,
// and how many of its constraints the seed-time world state left unsatisfied.
// (Declared after FCk_GoapWS_Condition_Authored — UHT needs in-file order.)

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Goap_SearchDebugRow
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Goap_SearchDebugRow);

	friend class ::UCk_Utils_Goap_Planner_UE;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	TArray<FCk_GoapWS_Condition_Authored> _Conditions;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	TSubclassOf<UCk_GoapAction_EntityScript> _ViaActionClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	int32 _UnsatisfiedCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
		meta = (AllowPrivateAccess = true))
	bool _SatisfiedByWorldState = false;

public:
	CK_PROPERTY_GET(_Conditions);
	CK_PROPERTY_GET(_ViaActionClass);
	CK_PROPERTY_GET(_UnsatisfiedCount);
	CK_PROPERTY_GET(_SatisfiedByWorldState);
};
