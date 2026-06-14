#pragma once

#include "CkGoap/Action/CkGoap_Action_Fragment_Data.h"

#include "CkGoap/Algorithm/CkGoap_WorldState.h"
#include "CkGoap/Algorithm/CkGoap_Types.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

// ====================================================================================================================

// Forward decls in global scope so friend lookups bind correctly.
class UCk_Utils_Goap_Action_UE;
class UCk_Utils_Goap_Planner_UE;
class UCk_GoapAction_EntityScript;

// ====================================================================================================================

namespace ck
{
	class FProcessor_Goap_Action_Setup;
	class FProcessor_Goap_Planner_UpdateActivation;
	// PR-B.1b Stage 3: Planner-side A*-pipeline processors.
	class FProcessor_Goap_Planner_AutoReplan;
	class FProcessor_Goap_Planner_HandleRequests;
	class FProcessor_Goap_Planner_HandleResult;

// ====================================================================================================================
// TAGS — action-scoped lifecycle
// ====================================================================================================================

	CK_DEFINE_ECS_TAG(FTag_Goap_Action_RequiresSetup);

	// Marks an Action whose _Cost is driven by an external provider (pushed via
	// Request_SetChildActionCost from the consumer's change-signal). Declarative /
	// introspection only — the planner still reads _Cost; this does not gate planning.
	CK_DEFINE_ECS_TAG(FTag_Goap_Action_HasCostProvider);

// ====================================================================================================================
// PARAMS — alias to BlueprintType data
// ====================================================================================================================

	using FFragment_Goap_Action_Params = FCk_Fragment_Goap_ActionParamsData;

// ====================================================================================================================
// CURRENT — residual Action-role state. Post-U11.0 split, _Plan / _PlanCost /
// _PlanStatus / _PlanAttemptCount moved to FFragment_Goap_Planner_PlanState;
// _Goal / _InvalidGoal moved to FFragment_Goap_Planner_Goal;
// _WorldStateSource_Resolved moved to FFragment_Goap_Planner_WorldStateSource
// (as its new _Resolved field). What remains: the "active parent" record —
// the class of the parent Action that injected this Action's current goal.
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_Action_Current
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Action_Current);

		friend class ::UCk_Utils_Goap_Action_UE;
		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class FProcessor_Goap_Planner_UpdateActivation;

	private:
		// The action class on the PARENT action that injected this action's
		// current goal. nullptr for the root.
		TSubclassOf<UCk_GoapAction_EntityScript>         _ActiveParentAction;

	public:
		CK_PROPERTY_GET(_ActiveParentAction);
	};

// ====================================================================================================================
// DEFINITION — This Action's own def, CDO-extracted at Setup time.
// In the unified model, every Action entity carries its OWN def (one def per
// Action). The parent's planner consumes its children's defs as candidate
// operators.
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_Action_Definition
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Action_Definition);

		friend class ::UCk_Utils_Goap_Action_UE;
		friend class FProcessor_Goap_Action_Setup;
		friend class FProcessor_Goap_Planner_UpdateActivation;
		// PR-B.1b Stage 3: SetActionCost mutates child Action's _CachedActionDef
		// from the Planner-on-Planner HandleRequests processor.
		friend class FProcessor_Goap_Planner_HandleRequests;

	private:
		// This Action's own def — extracted once from the CDO at Setup time.
		TArray<ck::goap::FWorldStateCondition_Raw> _Preconditions;
		TArray<ck::goap::FWorldStateEffect_Raw>    _Effects;
		float _Cost = 1.0f;

		// Pre-resolved form of _Effects as the Action's _Goal when active.
		// Populated at Setup; read at chain extension. (Will be filled by U3.)
		TArray<FCk_GoapWS_Condition_Authored> _GoalFromEffects;

		// Effect keys not registered in the resolved WS — populated at Setup,
		// surfaced via Get_InvalidGoal. (Will be filled by U5.)
		TArray<FCk_GoapWS_Condition_Authored> _InvalidGoal;

		// Pre-built FActionDef cache used by parent's planner as a candidate
		// operator. Built once at Setup time. (Will be filled by U3.)
		ck::goap::FActionDef _CachedActionDef;

	public:
		CK_PROPERTY_GET(_Preconditions);
		CK_PROPERTY_GET(_Effects);
		CK_PROPERTY_GET(_Cost);
		CK_PROPERTY_GET(_GoalFromEffects);
		CK_PROPERTY_GET(_InvalidGoal);

		auto AsActionDef() const -> const ck::goap::FActionDef& { return _CachedActionDef; }
	};

// ====================================================================================================================
// TREE — Parent/child relationships between Action entities. Populated by
// AddAction (PR-A unified construction verb) and ChainUpdate time.
// ====================================================================================================================

	struct CKGOAP_API FFragment_Goap_Action_Tree
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Action_Tree);

		friend class ::UCk_Utils_Goap_Action_UE;
		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class FProcessor_Goap_Action_Setup;
		friend class FProcessor_Goap_Planner_HandleResult;
		friend class FProcessor_Goap_Planner_UpdateActivation;

	private:
		FCk_Handle_Goap_Action _ParentAction;   // invalid for top-level Actions
		TArray<FCk_Handle_Goap_Action> _ChildActions;

	public:
		CK_PROPERTY_GET(_ParentAction);
		CK_PROPERTY_GET(_ChildActions);
	};

// ====================================================================================================================

} // namespace ck
