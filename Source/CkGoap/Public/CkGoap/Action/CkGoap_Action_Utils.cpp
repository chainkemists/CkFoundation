#include "CkGoap/Action/CkGoap_Action_Utils.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/Planner/CkGoap_Planner_Fragment.h"
#include "CkGoap/Planner/CkGoap_Planner_Utils.h"            // Find_ActionByClass + CastChecked
#include "CkGoap/Planner/CkGoap_Planner_Internal.h"         // DoCreateOrFindActionEntity
#include "CkGoap/Action/CkGoap_Action_Fragment.h"
#include "CkGoap/Action/CkGoap_Action_Record_Internal.h"        // FFragment_RecordOfGoapActions + utils struct
#include "CkGoap/WorldState/CkGoap_WorldState_Utils.h"          // Request_AddSubscriber
#include "CkAStar/CkAStar_Fragment.h"

#include "CkEcs/ContextOwner/CkContextOwner_Utils.h"            // owner-chain walk
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Signal/CkSignal_Utils.inl.h"

#include "CkLabel/CkLabel_Utils.h"

// ====================================================================================================================
// CONSTRUCTION
// ====================================================================================================================

auto
	UCk_Utils_Goap_Action_UE::
	AddAction_ToAction(
		FCk_Handle_Goap_Action& InParentAction,
		const FCk_Fragment_Goap_ActionParamsData& InParams)
	-> FCk_Handle_Goap_Action
{
	CK_ENSURE_IF_NOT(ck::IsValid(InParentAction),
		TEXT("Invalid parent Action handle [{}] in AddAction_ToAction"), InParentAction)
	{ return {}; }

	CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_ActionClass()),
		TEXT("Invalid _ActionClass in AddAction_ToAction (parent [{}])"), InParentAction)
	{ return {}; }

	// Walk up the lifetime-owner chain to find the containing ActionSet. Actions
	// are created with the ActionSet as their lifetime owner (see
	// DoCreateOrFindActionEntity, which calls Request_CreateEntity_AsTypeSafe
	// with InPlanner as the owner handle). Lifetime owner is not the same as
	// context owner: context owner is inherited from the owner's context owner
	// (which may be the test entity or NPC entity), while lifetime owner is the
	// direct parent entity that created this action — the ActionSet.
	auto OwnerEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InParentAction);
	auto ActionSetHandle = UCk_Utils_Goap_Planner_UE::CastChecked(OwnerEntity);

	CK_ENSURE_IF_NOT(ck::IsValid(ActionSetHandle),
		TEXT("Parent Action [{}] has no owning ActionSet"), InParentAction)
	{ return {}; }

	// Create / find the child entity in the ActionSet's catalog.
	auto ChildHandle = ck::goap::internal_planner::DoCreateOrFindActionEntity(ActionSetHandle, InParams);

	if (NOT ck::IsValid(ChildHandle))
	{ return {}; }

	// Wire up tree edges. v1 enforces a strict tree: a given Action entity
	// cannot be re-parented once it has a parent.
	auto& ChildTree = ChildHandle.Get<ck::FFragment_Goap_Action_Tree>();
	CK_ENSURE_IF_NOT(NOT ck::IsValid(ChildTree.Get_ParentAction()),
		TEXT("Action [{}] already has parent [{}]; cannot reparent (v1 enforces tree)."),
		ChildHandle, ChildTree.Get_ParentAction())
	{ return ChildHandle; }

	ChildTree._ParentAction = InParentAction;

	auto& ParentTree = InParentAction.Get<ck::FFragment_Goap_Action_Tree>();
	ParentTree._ChildActions.AddUnique(ChildHandle);

	// Eagerly resolve the child's WS source so its Setup processor can run
	// (and populate _CachedActionDef) BEFORE any parent plan is requested.
	// Without this, child Actions that haven't been activated via ChainUpdate
	// still have no resolved WS — their Setup defers indefinitely, leaving
	// _CachedActionDef empty, so the parent's planner sees no usable
	// candidate operators and the search fails immediately.
	//
	// Resolution order mirrors ChainUpdate's DoResolveAndAssignWorldStateSource:
	//   1. Child's own override (if set).
	//   2. Inherit from parent's already-resolved WS.
	//   3. Fall back to the ActionSet-level default WS source.
	//
	// We DO NOT subscribe the child to the WS here — subscription is only for
	// active (in-chain) Actions and is managed by ChainUpdate at activation.
	{
		auto& ChildCurrent = ChildHandle.Get<ck::FFragment_Goap_Action_Current>();
		if (NOT ck::IsValid(ChildCurrent.Get_WorldStateSource_Resolved()))
		{
			const auto& ChildParams = ChildHandle.Get<ck::FFragment_Goap_Action_Params>();
			const auto Override = ChildParams.Get_WorldStateSource_Override();
			if (ck::IsValid(Override))
			{
				ChildCurrent._WorldStateSource_Resolved = Override;
			}
			else
			{
				const auto& ParentCurrent = InParentAction.Get<ck::FFragment_Goap_Action_Current>();
				const auto ParentWS = ParentCurrent.Get_WorldStateSource_Resolved();
				if (ck::IsValid(ParentWS))
				{
					ChildCurrent._WorldStateSource_Resolved = ParentWS;
				}
				else if (ActionSetHandle.Has<ck::FFragment_Goap_Planner_WorldStateSource>())
				{
					const auto& SetWS = ActionSetHandle.Get<ck::FFragment_Goap_Planner_WorldStateSource>();
					if (ck::IsValid(SetWS.Get_WorldStateSource()))
					{
						ChildCurrent._WorldStateSource_Resolved = SetWS.Get_WorldStateSource();
					}
				}
			}
		}
	}

	return ChildHandle;
}

// ====================================================================================================================
// QUERY
// ====================================================================================================================

auto
	UCk_Utils_Goap_Action_UE::
	Has(const FCk_Handle& InHandle) -> bool
{
	return ck::IsValid(InHandle) && InHandle.Has<ck::FFragment_Goap_Action_Params>();
}

auto
	UCk_Utils_Goap_Action_UE::
	Get_PlanStatus(const FCk_Handle_Goap_Action& InAction) -> ECk_GoapPlanStatus
{
	if (NOT ck::IsValid(InAction)) { return ECk_GoapPlanStatus::Idle; }
	return InAction.Get<ck::FFragment_Goap_Action_Current>().Get_PlanStatus();
}

auto
	UCk_Utils_Goap_Action_UE::
	Get_Plan(const FCk_Handle_Goap_Action& InAction) -> TArray<TSubclassOf<UCk_GoapAction_EntityScript>>
{
	if (NOT ck::IsValid(InAction)) { return {}; }
	return InAction.Get<ck::FFragment_Goap_Action_Current>().Get_PlanClasses();
}

auto
	UCk_Utils_Goap_Action_UE::
	Get_PlanCost(const FCk_Handle_Goap_Action& InAction) -> float
{
	if (NOT ck::IsValid(InAction)) { return 0.0f; }
	return InAction.Get<ck::FFragment_Goap_Action_Current>().Get_PlanCost();
}

auto
	UCk_Utils_Goap_Action_UE::
	Get_WorldStateSource(const FCk_Handle_Goap_Action& InAction) -> FCk_Handle_Goap_WorldState
{
	if (NOT ck::IsValid(InAction)) { return {}; }
	return InAction.Get<ck::FFragment_Goap_Action_Current>().Get_WorldStateSource_Resolved();
}

auto
	UCk_Utils_Goap_Action_UE::
	Get_ActiveParentAction(const FCk_Handle_Goap_Action& InAction) -> TSubclassOf<UCk_GoapAction_EntityScript>
{
	if (NOT ck::IsValid(InAction)) { return nullptr; }
	return InAction.Get<ck::FFragment_Goap_Action_Current>().Get_ActiveParentAction();
}

auto
	UCk_Utils_Goap_Action_UE::
	Get_InvalidGoal(const FCk_Handle_Goap_Action& InAction) -> TArray<FCk_GoapWS_Condition_Authored>
{
	if (NOT ck::IsValid(InAction)) { return {}; }
	return InAction.Get<ck::FFragment_Goap_Action_Current>().Get_InvalidGoal();
}

// ====================================================================================================================
// REQUESTS — append to per-action request queue, processors drain.
// Inlined per-verb (UCk_Utils_Goap_Action_UE is friended to access _Requests,
// but a free helper in another namespace isn't).
// ====================================================================================================================

auto
	UCk_Utils_Goap_Action_UE::
	Request_SetGoalWorldState(
		FCk_Handle_Goap_Action& InAction,
		const TArray<FCk_GoapWS_Condition_Authored>& InGoal) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InAction)) { return InAction; }
	auto& Reqs = InAction.AddOrGet<ck::FFragment_Goap_Action_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Action_SetGoal{InGoal});
	return InAction;
}

auto
	UCk_Utils_Goap_Action_UE::
	Request_Plan(FCk_Handle_Goap_Action& InAction) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InAction)) { return InAction; }
	auto& Reqs = InAction.AddOrGet<ck::FFragment_Goap_Action_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Action_Plan{});
	return InAction;
}

auto
	UCk_Utils_Goap_Action_UE::
	Request_CancelPlan(FCk_Handle_Goap_Action& InAction) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InAction)) { return InAction; }
	auto& Reqs = InAction.AddOrGet<ck::FFragment_Goap_Action_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Action_CancelPlan{});
	return InAction;
}

auto
	UCk_Utils_Goap_Action_UE::
	Request_SetActionCost(
		FCk_Handle_Goap_Action& InAction,
		TSubclassOf<UCk_GoapAction_EntityScript> InActionClass,
		float InCost) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InAction)) { return InAction; }
	auto& Reqs = InAction.AddOrGet<ck::FFragment_Goap_Action_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Action_SetActionCost{InActionClass, InCost});
	return InAction;
}

auto
	UCk_Utils_Goap_Action_UE::
	Request_SetReplanInterval(FCk_Handle_Goap_Action& InAction, float InSeconds) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InAction)) { return InAction; }
	auto& Reqs = InAction.AddOrGet<ck::FFragment_Goap_Action_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Action_SetReplanInterval{InSeconds});
	return InAction;
}

auto
	UCk_Utils_Goap_Action_UE::
	Request_SetReplanPolicy(FCk_Handle_Goap_Action& InAction, ECk_Goap_ReplanPolicy InPolicy) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InAction)) { return InAction; }
	auto& Reqs = InAction.AddOrGet<ck::FFragment_Goap_Action_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Action_SetReplanPolicy{InPolicy});
	return InAction;
}

auto
	UCk_Utils_Goap_Action_UE::
	Request_SetSearchBudget(FCk_Handle_Goap_Action& InAction, int64 InMicroseconds) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InAction)) { return InAction; }
	auto& Reqs = InAction.AddOrGet<ck::FFragment_Goap_Action_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Action_SetSearchBudget{InMicroseconds});
	return InAction;
}

auto
	UCk_Utils_Goap_Action_UE::
	Request_SetCostThreshold(FCk_Handle_Goap_Action& InAction, float InThreshold) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InAction)) { return InAction; }
	auto& Reqs = InAction.AddOrGet<ck::FFragment_Goap_Action_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Action_SetCostThreshold{InThreshold});
	return InAction;
}

// ====================================================================================================================
// SIGNAL BINDING
// ====================================================================================================================

auto
	UCk_Utils_Goap_Action_UE::
	BindTo_OnPlanComplete(
		FCk_Handle_Goap_Action& InAction,
		const FCk_Delegate_Goap_OnActionPlanComplete& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior) -> FCk_Handle_Goap_Action
{
	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoap_Action_PlanComplete,
		InAction, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InAction;
}

auto
	UCk_Utils_Goap_Action_UE::
	UnbindFrom_OnPlanComplete(
		FCk_Handle_Goap_Action& InAction,
		const FCk_Delegate_Goap_OnActionPlanComplete& InDelegate) -> FCk_Handle_Goap_Action
{
	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoap_Action_PlanComplete, InAction, InDelegate);
	return InAction;
}

auto
	UCk_Utils_Goap_Action_UE::
	BindTo_OnPlanFailed(
		FCk_Handle_Goap_Action& InAction,
		const FCk_Delegate_Goap_OnActionPlanFailed& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior) -> FCk_Handle_Goap_Action
{
	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoap_Action_PlanFailed,
		InAction, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InAction;
}

auto
	UCk_Utils_Goap_Action_UE::
	UnbindFrom_OnPlanFailed(
		FCk_Handle_Goap_Action& InAction,
		const FCk_Delegate_Goap_OnActionPlanFailed& InDelegate) -> FCk_Handle_Goap_Action
{
	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoap_Action_PlanFailed, InAction, InDelegate);
	return InAction;
}

auto
	UCk_Utils_Goap_Action_UE::
	BindTo_OnActionActivated(
		FCk_Handle_Goap_Action& InAction,
		const FCk_Delegate_Goap_OnActionActivated& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior) -> FCk_Handle_Goap_Action
{
	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoap_Action_Activated,
		InAction, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InAction;
}

auto
	UCk_Utils_Goap_Action_UE::
	UnbindFrom_OnActionActivated(
		FCk_Handle_Goap_Action& InAction,
		const FCk_Delegate_Goap_OnActionActivated& InDelegate) -> FCk_Handle_Goap_Action
{
	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoap_Action_Activated, InAction, InDelegate);
	return InAction;
}

auto
	UCk_Utils_Goap_Action_UE::
	BindTo_OnActionDeactivated(
		FCk_Handle_Goap_Action& InAction,
		const FCk_Delegate_Goap_OnActionDeactivated& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior) -> FCk_Handle_Goap_Action
{
	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoap_Action_Deactivated,
		InAction, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InAction;
}

auto
	UCk_Utils_Goap_Action_UE::
	UnbindFrom_OnActionDeactivated(
		FCk_Handle_Goap_Action& InAction,
		const FCk_Delegate_Goap_OnActionDeactivated& InDelegate) -> FCk_Handle_Goap_Action
{
	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoap_Action_Deactivated, InAction, InDelegate);
	return InAction;
}
