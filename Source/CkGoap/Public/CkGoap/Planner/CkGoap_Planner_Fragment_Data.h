#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkCore/Macros/CkMacros.h"

// FCk_Handle_Goap_Action, for the OnActiveChainChanged payload's TArray<>.
#include "CkGoap/Action/CkGoap_Action_Fragment_Data.h"
#include "CkGoap/CkGoap_Fragment_Data.h"  // FCk_GoapWS_Condition_Authored
#include "CkGoap/WorldState/CkGoap_WorldState_Fragment_Data.h"  // FCk_Handle_Goap_WorldState

#include "CkGoap_Planner_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKGOAP_API FCk_Handle_Goap_Planner : public FCk_Handle_TypeSafe
{
	GENERATED_BODY()
	CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Goap_Planner);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Goap_Planner);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKGOAP_API FCk_Goap_Planner_Spec
{
	GENERATED_BODY()

public:
	CK_GENERATED_BODY(FCk_Goap_Planner_Spec);

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

	// The goal this Planner plans toward — independent of any Action-role effects
	// on this entity. May be empty at construction; set later via Request_SetGoal.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	TArray<FCk_GoapWS_Condition_Authored> _Goal;

	// Required for top-level Planners. Optional for promoted mid-tier Planners —
	// if unset they inherit the parent's resolved WS at activation time.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	FCk_Handle_Goap_WorldState _WorldStateSource;

	// Per-tick A* time slice (microseconds). 0 = unbounded (finish in one tick).
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	int64 _SearchBudgetMicroseconds = 50000;

	// Early-out threshold on best A* frontier FScore. If > 0, planner returns
	// CostThresholdReached rather than committing to a plan exceeding it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	float _CostThreshold = 0.0f;

	// Replan trigger policy — when AutoReplan fires a Plan request.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	ECk_Goap_ReplanPolicy _ReplanPolicy = ECk_Goap_ReplanPolicy::OnWorldStateDirty;

	// Throttle window for AutoReplan dirty triggers. Coalesces multiple dirty
	// events within the window into one replan.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
	float _MinReplanIntervalSeconds = 0.0f;

	// Fire an initial Plan request after activation — top-level Planners immediately,
	// promoted mid-tier Planners when their parent picks them as Plan[0].
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		meta = (AllowPrivateAccess = true))
	bool _PlanOnStart = true;

	// Default false → Setup verifies this catalog has an unconditional fallback
	// Action and ensures if not. Set true ONLY when PlanFailed is the test subject;
	// game-content Planners must leave it false (see CkGoap CLAUDE.md design tenets).
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
	CK_DEFINE_CONSTRUCTORS(FCk_Goap_Planner_Spec, _PlannerTag);
};

// --------------------------------------------------------------------------------------------------------------------

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

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
	FCk_Delegate_Goap_OnActiveChainChanged,
	FCk_Handle_Goap_Planner, InPlanner,
	FCk_Goap_Payload_OnActiveChainChanged, InPayload);

// Per-Planner signal delegates. Source type is FCk_Handle_Goap_Planner.

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

