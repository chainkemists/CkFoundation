#include "CkGoap/Action/CkGoap_Action_Utils.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/Planner/CkGoap_Planner_Fragment.h"
#include "CkGoap/Planner/CkGoap_Planner_Utils.h"  // PR-B.1b Stage 3: resolve owning Planner
#include "CkGoap/Action/CkGoap_Action_Fragment.h"
#include "CkAStar/CkAStar_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Signal/CkSignal_Utils.inl.h"

// ====================================================================================================================
// LOCAL HELPERS — PR-B.1b Stage 3 owning-Planner resolution.
// ====================================================================================================================

namespace
{
	// Resolve the owning Planner for an Action handle. Returns an invalid
	// handle if the Action is orphaned (should not happen in well-formed
	// graphs).
	auto ResolveOwningPlanner(const FCk_Handle_Goap_Action& InAction) -> FCk_Handle_Goap_Planner
	{
		if (NOT ck::IsValid(InAction)) { return {}; }

		if (UCk_Utils_Goap_Planner_UE::Has(InAction))
		{
			return UCk_Utils_Goap_Planner_UE::CastChecked(InAction);
		}

		auto Owner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAction);
		if (UCk_Utils_Goap_Planner_UE::Has(Owner))
		{
			return UCk_Utils_Goap_Planner_UE::CastChecked(Owner);
		}

		return {};
	}
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

	// PR-B.1b Stage 3: PlanState lives on the owning Planner now. Resolve and
	// read from there. The Action-side stamp (still present from dual-stamp)
	// is no longer authoritative.
	auto Owning = ResolveOwningPlanner(InAction);
	if (NOT ck::IsValid(Owning)) { return ECk_GoapPlanStatus::Idle; }
	return Owning.Get<ck::FFragment_Goap_Planner_PlanState>().Get_PlanStatus();
}

auto
	UCk_Utils_Goap_Action_UE::
	Get_Plan(const FCk_Handle_Goap_Action& InAction) -> TArray<TSubclassOf<UCk_GoapAction_EntityScript>>
{
	if (NOT ck::IsValid(InAction)) { return {}; }
	auto Owning = ResolveOwningPlanner(InAction);
	if (NOT ck::IsValid(Owning)) { return {}; }
	return Owning.Get<ck::FFragment_Goap_Planner_PlanState>().Get_PlanClasses();
}

auto
	UCk_Utils_Goap_Action_UE::
	Get_PlanCost(const FCk_Handle_Goap_Action& InAction) -> float
{
	if (NOT ck::IsValid(InAction)) { return 0.0f; }
	auto Owning = ResolveOwningPlanner(InAction);
	if (NOT ck::IsValid(Owning)) { return 0.0f; }
	return Owning.Get<ck::FFragment_Goap_Planner_PlanState>().Get_PlanCost();
}

auto
	UCk_Utils_Goap_Action_UE::
	Get_WorldStateSource(const FCk_Handle_Goap_Action& InAction) -> FCk_Handle_Goap_WorldState
{
	if (NOT ck::IsValid(InAction)) { return {}; }
	// Action-side _Resolved is still populated by activation walk + AddAction
	// for the implicit-root; safe to read here.
	return InAction.Get<ck::FFragment_Goap_Planner_WorldStateSource>().Get_Resolved();
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
	auto Owning = ResolveOwningPlanner(InAction);
	if (NOT ck::IsValid(Owning)) { return {}; }
	return Owning.Get<ck::FFragment_Goap_Planner_Goal>().Get_InvalidGoal();
}

// ====================================================================================================================
// REQUESTS — PR-B.1b Stage 3: resolve the owning Planner and enqueue on its
// request queue (FFragment_Goap_Planner_Requests, alias to the same underlying
// type). The Planner-on-Planner HandleRequests drains the queue.
// ====================================================================================================================

auto
	UCk_Utils_Goap_Action_UE::
	Request_Plan(FCk_Handle_Goap_Action& InAction) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InAction)) { return InAction; }
	auto Owning = ResolveOwningPlanner(InAction);
	CK_ENSURE_IF_NOT(ck::IsValid(Owning),
		TEXT("Action [{}] has no owning Planner; Request_Plan dropped."), InAction)
	{ return InAction; }
	auto& Reqs = Owning.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Action_Plan{});
	return InAction;
}

auto
	UCk_Utils_Goap_Action_UE::
	Request_CancelPlan(FCk_Handle_Goap_Action& InAction) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InAction)) { return InAction; }
	auto Owning = ResolveOwningPlanner(InAction);
	CK_ENSURE_IF_NOT(ck::IsValid(Owning),
		TEXT("Action [{}] has no owning Planner; Request_CancelPlan dropped."), InAction)
	{ return InAction; }
	auto& Reqs = Owning.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
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
	auto Owning = ResolveOwningPlanner(InAction);
	CK_ENSURE_IF_NOT(ck::IsValid(Owning),
		TEXT("Action [{}] has no owning Planner; Request_SetActionCost dropped."), InAction)
	{ return InAction; }
	auto& Reqs = Owning.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Action_SetActionCost{InActionClass, InCost});
	return InAction;
}

auto
	UCk_Utils_Goap_Action_UE::
	Request_SetReplanInterval(FCk_Handle_Goap_Action& InAction, float InSeconds) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InAction)) { return InAction; }
	auto Owning = ResolveOwningPlanner(InAction);
	CK_ENSURE_IF_NOT(ck::IsValid(Owning),
		TEXT("Action [{}] has no owning Planner; Request_SetReplanInterval dropped."), InAction)
	{ return InAction; }
	auto& Reqs = Owning.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Action_SetReplanInterval{InSeconds});
	return InAction;
}

auto
	UCk_Utils_Goap_Action_UE::
	Request_SetReplanPolicy(FCk_Handle_Goap_Action& InAction, ECk_Goap_ReplanPolicy InPolicy) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InAction)) { return InAction; }
	auto Owning = ResolveOwningPlanner(InAction);
	CK_ENSURE_IF_NOT(ck::IsValid(Owning),
		TEXT("Action [{}] has no owning Planner; Request_SetReplanPolicy dropped."), InAction)
	{ return InAction; }
	auto& Reqs = Owning.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Action_SetReplanPolicy{InPolicy});
	return InAction;
}

auto
	UCk_Utils_Goap_Action_UE::
	Request_SetSearchBudget(FCk_Handle_Goap_Action& InAction, int64 InMicroseconds) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InAction)) { return InAction; }
	auto Owning = ResolveOwningPlanner(InAction);
	CK_ENSURE_IF_NOT(ck::IsValid(Owning),
		TEXT("Action [{}] has no owning Planner; Request_SetSearchBudget dropped."), InAction)
	{ return InAction; }
	auto& Reqs = Owning.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Action_SetSearchBudget{InMicroseconds});
	return InAction;
}

auto
	UCk_Utils_Goap_Action_UE::
	Request_SetCostThreshold(FCk_Handle_Goap_Action& InAction, float InThreshold) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InAction)) { return InAction; }
	auto Owning = ResolveOwningPlanner(InAction);
	CK_ENSURE_IF_NOT(ck::IsValid(Owning),
		TEXT("Action [{}] has no owning Planner; Request_SetCostThreshold dropped."), InAction)
	{ return InAction; }
	auto& Reqs = Owning.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Action_SetCostThreshold{InThreshold});
	return InAction;
}

// ====================================================================================================================
// SIGNAL BINDING — moved to UCk_Utils_Goap_Planner_UE in PR-B.1b Stage 0
// (spec §3.5: per-Planner signals have FCk_Handle_Goap_Planner source).
// ====================================================================================================================
