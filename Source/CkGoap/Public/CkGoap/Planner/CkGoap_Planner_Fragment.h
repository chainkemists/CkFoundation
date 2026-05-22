#pragma once

#include "CkGoap/Planner/CkGoap_Planner_Fragment_Data.h"
#include "CkGoap/CkGoap_Fragment_Data.h"  // FCk_GoapDiagnostic_DependencyCycle
#include "CkGoap/Action/CkGoap_Action_Fragment_Data.h"  // FCk_Handle_Goap_Action, FCk_GoapWS_Condition_Authored, ECk_GoapPlanStatus
#include "CkGoap/Algorithm/CkGoap_WorldState.h"  // goap::FWorldStateCondition
#include "CkGoap/WorldState/CkGoap_WorldState_Fragment_Data.h"  // FCk_Handle_Goap_WorldState

#include "CkEcs/Signal/CkSignal_Macros.h"

// ====================================================================================================================

// Forward decls in global scope so friend lookups bind correctly.
class UCk_Utils_Goap_Planner_UE;
class UCk_Utils_Goap_Action_UE;
class UCk_GoapAction_EntityScript;

// ====================================================================================================================

namespace ck
{
	class FProcessor_Goap_Planner_Setup;
	class FProcessor_Goap_Planner_ChainUpdate;
	class FProcessor_Goap_Action_Setup;
	class FProcessor_Goap_Action_HandleRequests;
	class FProcessor_Goap_Action_HandleResult;

// ====================================================================================================================
// TAGS
// ====================================================================================================================

	CK_DEFINE_ECS_TAG(FTag_Goap_Planner_RequiresSetup);

	// Set whenever any action in the ActionSet completes a plan. Consumed +
	// removed by ChainUpdate. Optimization to skip walking inert ActionSets.
	CK_DEFINE_ECS_TAG(FTag_Goap_Planner_RequiresChainUpdate);

// ====================================================================================================================
// PARAMS — alias to the BlueprintType data shape
// ====================================================================================================================

	using FFragment_Goap_Planner_Params = FCk_Fragment_Goap_PlannerParamsData;

// ====================================================================================================================
// CURRENT FRAGMENT — Runtime ActionSet state (enable toggle, diagnostics)
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_Planner_Current
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Planner_Current);

		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class FProcessor_Goap_Planner_Setup;
		friend class FProcessor_Goap_Planner_ChainUpdate;

	private:
		ECk_EnableDisable _EnableToggle = ECk_EnableDisable::Enable;
		TArray<FCk_GoapDiagnostic_DependencyCycle> _DependencyCycles;

		// The root Action entity for this ActionSet. Established by SetRootAction
		// (Phase U2) or implicitly by the first AddAction call.
		FCk_Handle_Goap_Action _RootAction;

	public:
		CK_PROPERTY_GET(_EnableToggle);
		CK_PROPERTY_GET(_DependencyCycles);
		CK_PROPERTY_GET(_RootAction);
	};

// ====================================================================================================================
// ACTIVE CHAIN — Ordered chain of currently-active actions. [0] is the root.
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_Planner_ActiveChain
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Planner_ActiveChain);

		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class ::UCk_Utils_Goap_Action_UE;
		friend class FProcessor_Goap_Planner_ChainUpdate;

	private:
		TArray<FCk_Handle_Goap_Action> _Chain;

	public:
		CK_PROPERTY_GET(_Chain);
	};

// ====================================================================================================================
// ACTION CATALOG INDEX — O(1) tag-to-action lookup. Populated at AddAction
// time; read by ChainUpdate when resolving Plan[0]'s action tag.
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_Planner_ActionCatalogIndex
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Planner_ActionCatalogIndex);

		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class ::UCk_Utils_Goap_Action_UE;
		friend class FProcessor_Goap_Planner_ChainUpdate;

	private:
		TMap<FGameplayTag, FCk_Handle_Goap_Action> _TagToAction;

	public:
		CK_PROPERTY_GET(_TagToAction);

		// Public mutator used by the shared entity-creation helper
		// ck::goap::internal_planner::DoCreateOrFindActionEntity. Private-
		// field-access via friendship is class-scoped and doesn't reach
		// namespace-level free functions, so the helper goes through this.
		auto AddEntry(FGameplayTag InTag, FCk_Handle_Goap_Action InAction) -> void
		{
			_TagToAction.Add(InTag, InAction);
		}
	};

// ====================================================================================================================
// WORLD STATE SOURCE — ActionSet-level default WS source. Used by the unified
// ChainUpdate logic when an Action does not provide its own override.
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_Planner_WorldStateSource
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Planner_WorldStateSource);

		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class ::UCk_Utils_Goap_Action_UE;
		friend class FProcessor_Goap_Planner_ChainUpdate;
		friend class FProcessor_Goap_Action_Setup;
		friend class FProcessor_Goap_Action_HandleRequests;

	private:
		// ActionSet-level default WS (set by SetRootAction). Also used on Action
		// entities to store the original override (if any). For the unified split
		// model, Action entities carry this fragment alongside their PlanState /
		// Goal. _Resolved is the per-Action eager-resolved source; _WorldStateSource
		// is unused on Action entities (Override lives on Params).
		FCk_Handle_Goap_WorldState _WorldStateSource;

		// Resolved at activation: override-if-set, else parent's resolved, else
		// ActionSet WS. Lives here so the Action-role fragments need not duplicate.
		FCk_Handle_Goap_WorldState _Resolved;

	public:
		CK_PROPERTY_GET(_WorldStateSource);
		CK_PROPERTY_SET(_WorldStateSource);
		CK_PROPERTY_GET(_Resolved);
	};

// ====================================================================================================================
// PLAN STATE — Planner-role fragment: live plan + status + cost + attempt count
// for the planner running on this entity. Lives on every Action entity (because
// every Action runs its own planner in the unified model) and on the top-level
// Planner entity.
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_Planner_PlanState
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Planner_PlanState);

		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class ::UCk_Utils_Goap_Action_UE;
		friend class FProcessor_Goap_Planner_ChainUpdate;
		friend class FProcessor_Goap_Action_Setup;
		friend class FProcessor_Goap_Action_HandleRequests;
		friend class FProcessor_Goap_Action_HandleResult;

	private:
		ECk_GoapPlanStatus                               _PlanStatus = ECk_GoapPlanStatus::Idle;

		// The planner emits a sequence of Action *entities*. Get_PlanClasses() is
		// a convenience mapping each entity back to its EntityScript class.
		TArray<FCk_Handle_Goap_Action>                   _Plan;

		float                                            _PlanCost = 0.0f;
		int32                                            _PlanAttemptCount = 0;

	public:
		CK_PROPERTY_GET(_PlanStatus);
		CK_PROPERTY_GET(_Plan);
		CK_PROPERTY_GET(_PlanCost);
		CK_PROPERTY_GET(_PlanAttemptCount);

		// Map each Action entity in the plan back to its EntityScript class.
		// Defined here (inline) to avoid creating a CkGoap_Planner_Fragment.cpp
		// just for one helper. Depends on FFragment_Goap_Action_Params so the
		// declaration lives in the .h that already includes that header.
		auto Get_PlanClasses() const -> TArray<TSubclassOf<UCk_GoapAction_EntityScript>>;
	};

// ====================================================================================================================
// GOAL — Planner-role fragment: effective goal world state for this planner.
// Root action: populated from _InitialGoal_RootOnly at Setup. Non-root: injected
// from parent action's Effects at activation. _InvalidGoal carries validation
// fallout (effect / goal tags not in the resolved WS registry).
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_Planner_Goal
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Planner_Goal);

		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class ::UCk_Utils_Goap_Action_UE;
		friend class FProcessor_Goap_Planner_ChainUpdate;
		friend class FProcessor_Goap_Action_Setup;
		friend class FProcessor_Goap_Action_HandleRequests;

	private:
		TArray<goap::FWorldStateCondition>               _Goal;
		TArray<FCk_GoapWS_Condition_Authored>            _InvalidGoal;

	public:
		CK_PROPERTY_GET(_Goal);
		CK_PROPERTY_GET(_InvalidGoal);
	};

// ====================================================================================================================
// SIGNALS — ActionSet-scoped
// ====================================================================================================================

	CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
		CKGOAP_API,
		OnGoap_Planner_ActiveChainChanged,
		FCk_Delegate_Goap_OnActiveChainChanged,
		FCk_Handle_Goap_Planner,
		FCk_Goap_Payload_OnActiveChainChanged);

// ====================================================================================================================

} // namespace ck
