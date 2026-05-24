#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkCore/Macros/CkMacros.h"

// Need FCk_Handle_Goap_Action for the OnActiveChainChanged payload's TArray<>.
// Also pulls FCk_GoapWS_Condition_Authored for the Planner's _Goal field.
#include "CkGoap/Action/CkGoap_Action_Fragment_Data.h"
#include "CkGoap/CkGoap_Fragment_Data.h"  // FCk_GoapWS_Condition_Authored
#include "CkGoap/WorldState/CkGoap_WorldState_Fragment_Data.h"  // FCk_Handle_Goap_WorldState

#include "CkGoap_Planner_Fragment_Data.generated.h"

// ====================================================================================================================
// HANDLE
// ====================================================================================================================

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKGOAP_API FCk_Handle_Goap_Planner : public FCk_Handle_TypeSafe
{
	GENERATED_BODY()
	CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Goap_Planner);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Goap_Planner);

// ====================================================================================================================
// ACTIONSET PARAMS
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Fragment_Goap_PlannerParamsData
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Fragment_Goap_PlannerParamsData);

private:
	// ActionSet identity within a Goap root. Unique per root entity.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true, Categories = "Goap.Planner"))
	FGameplayTag _PlannerTag;

	// Initial enable/disable state. Disabled ActionSets skip per-Action
	// planning and chain-update. Toggle at runtime via Request_SetEnableToggle.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	ECk_EnableDisable _InitialToggle = ECk_EnableDisable::Enable;

	// U11.1: the goal this Planner plans toward. Independent of any Action role
	// effects this entity may carry. May be empty at construction and set later
	// via Request_SetGoal. Stamped onto the Planner's FFragment_Goap_Planner_Goal
	// at construction (and propagated to the implicit-root Action's planner-role
	// goal by the first AddAction call on a top-level Planner).
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	TArray<FCk_GoapWS_Condition_Authored> _Goal;

	// PR-A: the WorldState this Planner reads. Required for top-level Planners
	// (where the Planner entity itself doesn't carry an Action role to inherit
	// from). Optional for promoted mid-tier Planners — if unset, the promoted
	// Planner inherits its parent's resolved WS at activation time. Replaces
	// the WS argument the old SetRootAction verb took.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	FCk_Handle_Goap_WorldState _WorldStateSource;

	// Per-tick A* time slice (microseconds). 0 = unbounded (finish in one tick).
	// PR-B.1b: lifted from FCk_Fragment_Goap_ActionParamsData (spec §3.2 has always
	// shown this as a Planner-level knob).
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	int64 _SearchBudgetMicroseconds = 50000;

	// Early-out threshold on best A* frontier FScore. If > 0, planner returns
	// CostThresholdReached rather than committing to a plan exceeding it.
	// PR-B.1b: lifted from FCk_Fragment_Goap_ActionParamsData.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	float _CostThreshold = 0.0f;

	// Replan trigger policy — when AutoReplan fires a Plan request.
	// PR-B.1b Stage 5: lifted from FCk_Fragment_Goap_ActionParamsData; goal
	// independence (spec §3.2) means policy is a Planner-tier concern.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	ECk_Goap_ReplanPolicy _ReplanPolicy = ECk_Goap_ReplanPolicy::OnWorldStateDirty;

	// Throttle window for AutoReplan dirty triggers. Coalesces multiple dirty
	// events within the window into one replan. PR-B.1b Stage 5: lifted.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
	float _MinReplanIntervalSeconds = 0.0f;

	// If true, the Planner fires an initial Plan request after activation
	// (top-level Planners: immediately; promoted mid-tier Planners: when their
	// parent picks them as Plan[0]). PR-B.1b Stage 5: lifted from ActionParams.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	bool _PlanOnStart = true;

	// "PlanFailed is a misconfiguration, not a normal state" tenet enforcement.
	// Default false → the framework's Setup pass verifies this Planner's catalog
	// contains at least one Action with no preconditions whose effects cover
	// every goal condition (an unconditional fallback like WaitForEnemy /
	// StandWatch / Idle). If the check fails, CK_ENSURE_IF_NOT fires. The
	// runtime check in HandleResult also ensures on Failed / CostThresholdReached.
	//
	// Set to true ONLY when PlanFailed is the test/research subject (e.g.
	// CkAutoTest_Goap_Planner_InvalidGoal, the MakeTea gym station that demos
	// PlanFailed when ingredients are missing). Game-content Planners must
	// leave this false — see CkGoap/CLAUDE.md § "Design tenets".
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	bool _AllowPlanFailed = false;

public:
	CK_PROPERTY(_PlannerTag);
	CK_PROPERTY(_InitialToggle);
	CK_PROPERTY(_Goal);
	CK_PROPERTY(_WorldStateSource);
	CK_PROPERTY(_SearchBudgetMicroseconds);
	CK_PROPERTY(_CostThreshold);
	CK_PROPERTY(_ReplanPolicy);
	CK_PROPERTY(_MinReplanIntervalSeconds);
	CK_PROPERTY(_PlanOnStart);
	CK_PROPERTY(_AllowPlanFailed);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Goap_PlannerParamsData, _PlannerTag);
};

// ====================================================================================================================
// SIGNAL PAYLOADS — ActionSet-scoped
// ====================================================================================================================

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Goap_Payload_OnActiveChainChanged
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Goap_Payload_OnActiveChainChanged);

private:
	// Snapshot of the chain BEFORE this frame's mutation. Current chain is
	// readable via UCk_Utils_Goap_Planner_UE::Get_ActiveChain in the handler.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	TArray<FCk_Handle_Goap_Action> _OldChain;

public:
	CK_PROPERTY_GET(_OldChain);

public:
	CK_DEFINE_CONSTRUCTORS(FCk_Goap_Payload_OnActiveChainChanged, _OldChain);
};

// ====================================================================================================================
// DELEGATES — ActionSet-scoped
// ====================================================================================================================

DECLARE_DYNAMIC_DELEGATE_TwoParams(
	FCk_Delegate_Goap_OnActiveChainChanged,
	FCk_Handle_Goap_Planner, InPlanner,
	FCk_Goap_Payload_OnActiveChainChanged, InPayload);

// PR-B.1b Stage 0 — per-Planner signal delegates (spec §3.5). Source type is
// FCk_Handle_Goap_Planner. Moved from CkGoap_Action_Fragment_Data.h and
// renamed (OnActionPlan* → OnPlan*) to align with the new signal class names.

DECLARE_DYNAMIC_DELEGATE_TwoParams(
	FCk_Delegate_Goap_OnPlanComplete,
	FCk_Handle_Goap_Planner, InPlanner,
	FCk_Goap_Payload_OnPlanComplete, InPayload);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
	FCk_Delegate_Goap_OnPlanFailed,
	FCk_Handle_Goap_Planner, InPlanner,
	FCk_Goap_Payload_OnPlanFailed, InPayload);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
	FCk_Delegate_Goap_OnPlannerActivated,
	FCk_Handle_Goap_Planner, InPlanner,
	FCk_Goap_Payload_OnPlannerActivated, InPayload);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
	FCk_Delegate_Goap_OnPlannerDeactivated,
	FCk_Handle_Goap_Planner, InPlanner,
	FCk_Goap_Payload_OnPlannerDeactivated, InPayload);

