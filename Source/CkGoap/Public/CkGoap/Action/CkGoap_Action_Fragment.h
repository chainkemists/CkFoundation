#pragma once

#include "CkGoap/Action/CkGoap_Action_Fragment_Data.h"

#include "CkGoap/Algorithm/CkGoap_WorldState.h"
#include "CkGoap/Algorithm/CkGoap_Types.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

// Forward decls in global scope so friend lookups bind correctly.
class UCk_Utils_Goap_Action_UE;
class UCk_Utils_Goap_Planner_UE;
class UCk_GoapAction_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
	class FProcessor_Goap_Action_Setup;
	class FProcessor_Goap_Planner_UpdateActivation;
	class FProcessor_Goap_Planner_AutoReplan;
	class FProcessor_Goap_Planner_HandleRequests;
	class FProcessor_Goap_Planner_HandleResult;

// --------------------------------------------------------------------------------------------------------------------

	CK_DEFINE_ECS_TAG(FTag_Goap_Action_RequiresSetup);

	// Marks an Action whose _Cost is driven by an external provider (pushed via
	// Request_SetChildActionCost from the consumer's change-signal). Declarative /
	// introspection only — the planner still reads _Cost; this does not gate planning.
	CK_DEFINE_ECS_TAG(FTag_Goap_Action_HasCostProvider);

// --------------------------------------------------------------------------------------------------------------------

	using FFragment_Goap_Action_Params = FCk_Goap_Action_Spec;

// --------------------------------------------------------------------------------------------------------------------

	struct CKGOAP_API FFragment_Goap_Action_Current
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Action_Current);

		friend class ::UCk_Utils_Goap_Action_UE;
		friend class ::UCk_Utils_Goap_Planner_UE;
		friend class FProcessor_Goap_Planner_UpdateActivation;

	private:
		// nullptr for the root.
		TSubclassOf<UCk_GoapAction_EntityScript>         _ActiveParentAction;

	public:
		CK_PROPERTY_GET(_ActiveParentAction);
	};

// --------------------------------------------------------------------------------------------------------------------

	struct CKGOAP_API FFragment_Goap_Action_Definition
	{
	public:
		CK_GENERATED_BODY(FFragment_Goap_Action_Definition);

		friend class ::UCk_Utils_Goap_Action_UE;
		friend class FProcessor_Goap_Action_Setup;
		friend class FProcessor_Goap_Planner_UpdateActivation;
		// SetActionCost mutates a child Action's _CachedActionDef.
		friend class FProcessor_Goap_Planner_HandleRequests;

	private:
		// This Action's own def — extracted once from the CDO at Setup time.
		TArray<ck::goap::FWorldStateCondition_Raw> _Preconditions;
		TArray<ck::goap::FWorldStateEffect_Raw>    _Effects;
		float _Cost = 1.0f;

		// Pre-resolved form of _Effects, used as this Action's goal when active.
		TArray<FCk_GoapWS_Condition_Authored> _GoalFromEffects;

		// Effect keys not registered in the resolved WS; surfaced via Get_InvalidGoal.
		TArray<FCk_GoapWS_Condition_Authored> _InvalidGoal;

		// Cached candidate operator consumed by the parent Planner's A* search.
		ck::goap::FActionDef _CachedActionDef;

	public:
		CK_PROPERTY_GET(_Preconditions);
		CK_PROPERTY_GET(_Effects);
		CK_PROPERTY_GET(_Cost);
		CK_PROPERTY_GET(_GoalFromEffects);
		CK_PROPERTY_GET(_InvalidGoal);

		auto AsActionDef() const -> const ck::goap::FActionDef& { return _CachedActionDef; }
	};

// --------------------------------------------------------------------------------------------------------------------

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

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck
