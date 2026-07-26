#include "CkGoap/Action/CkGoap_Action_Utils.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/Planner/CkGoap_Planner_Fragment.h"
#include "CkGoap/Planner/CkGoap_Planner_Utils.h"  // resolve owning Planner
#include "CkGoap/Action/CkGoap_Action_Fragment.h"
#include "CkAStar/CkAStar_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Signal/CkSignal_Utils.inl.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_goap_action_utils
{
	// Invalid handle if the Action is orphaned (should not happen in well-formed graphs).
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

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_Goap_Action_UE::
	Has(const FCk_Handle& InHandle) -> bool
{
	return ck::IsValid(InHandle) && InHandle.Has<ck::FFragment_Goap_Action_Params>();
}

auto
	UCk_Utils_Goap_Action_UE::
	Get_HasCostProvider(const FCk_Handle_Goap_Action& InAction) -> bool
{
	return ck::IsValid(InAction) && InAction.Has<ck::FTag_Goap_Action_HasCostProvider>();
}

auto
	UCk_Utils_Goap_Action_UE::
	Get_PlanStatus(const FCk_Handle_Goap_Action& InAction) -> ECk_GoapPlanStatus
{
	if (NOT ck::IsValid(InAction)) { return ECk_GoapPlanStatus::Idle; }

	auto Owning = ck_goap_action_utils::ResolveOwningPlanner(InAction);
	if (NOT ck::IsValid(Owning)) { return ECk_GoapPlanStatus::Idle; }
	return Owning.Get<ck::FFragment_Goap_Planner_PlanState>().Get_PlanStatus();
}

auto
	UCk_Utils_Goap_Action_UE::
	Get_Plan(const FCk_Handle_Goap_Action& InAction) -> TArray<TSubclassOf<UCk_GoapAction_EntityScript>>
{
	if (NOT ck::IsValid(InAction)) { return {}; }
	auto Owning = ck_goap_action_utils::ResolveOwningPlanner(InAction);
	if (NOT ck::IsValid(Owning)) { return {}; }
	return Owning.Get<ck::FFragment_Goap_Planner_PlanState>().Get_PlanClasses();
}

auto
	UCk_Utils_Goap_Action_UE::
	Get_PlanCost(const FCk_Handle_Goap_Action& InAction) -> float
{
	if (NOT ck::IsValid(InAction)) { return 0.0f; }
	auto Owning = ck_goap_action_utils::ResolveOwningPlanner(InAction);
	if (NOT ck::IsValid(Owning)) { return 0.0f; }
	return Owning.Get<ck::FFragment_Goap_Planner_PlanState>().Get_PlanCost();
}

auto
	UCk_Utils_Goap_Action_UE::
	Get_WorldStateSource(const FCk_Handle_Goap_Action& InAction) -> FCk_Handle_Goap_WorldState
{
	if (NOT ck::IsValid(InAction)) { return {}; }
	// Action-side _Resolved is populated by AddAction + the activation walk, so it
	// is safe to read directly without resolving the owning Planner.
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
	auto Owning = ck_goap_action_utils::ResolveOwningPlanner(InAction);
	if (NOT ck::IsValid(Owning)) { return {}; }
	return Owning.Get<ck::FFragment_Goap_Planner_Goal>().Get_InvalidGoal();
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_Goap_Action_UE::
	Request_Plan(FCk_Handle_Goap_Action& InAction) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InAction)) { return InAction; }
	auto Owning = ck_goap_action_utils::ResolveOwningPlanner(InAction);
	CK_ENSURE_IF_NOT(ck::IsValid(Owning),
		TEXT("Action [{}] has no owning Planner; Request_Plan dropped."), InAction)
	{ return InAction; }
	auto& Reqs = Owning.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Planner_Plan{});
	return InAction;
}

auto
	UCk_Utils_Goap_Action_UE::
	Request_CancelPlan(FCk_Handle_Goap_Action& InAction) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InAction)) { return InAction; }
	auto Owning = ck_goap_action_utils::ResolveOwningPlanner(InAction);
	CK_ENSURE_IF_NOT(ck::IsValid(Owning),
		TEXT("Action [{}] has no owning Planner; Request_CancelPlan dropped."), InAction)
	{ return InAction; }
	auto& Reqs = Owning.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Planner_CancelPlan{});
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
	auto Owning = ck_goap_action_utils::ResolveOwningPlanner(InAction);
	CK_ENSURE_IF_NOT(ck::IsValid(Owning),
		TEXT("Action [{}] has no owning Planner; Request_SetActionCost dropped."), InAction)
	{ return InAction; }
	auto& Reqs = Owning.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Planner_SetActionCost{InActionClass, InCost});
	return InAction;
}

auto
	UCk_Utils_Goap_Action_UE::
	Request_SetReplanInterval(FCk_Handle_Goap_Action& InAction, float InSeconds) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InAction)) { return InAction; }
	auto Owning = ck_goap_action_utils::ResolveOwningPlanner(InAction);
	CK_ENSURE_IF_NOT(ck::IsValid(Owning),
		TEXT("Action [{}] has no owning Planner; Request_SetReplanInterval dropped."), InAction)
	{ return InAction; }
	auto& Reqs = Owning.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Planner_SetReplanInterval{InSeconds});
	return InAction;
}

auto
	UCk_Utils_Goap_Action_UE::
	Request_SetReplanPolicy(FCk_Handle_Goap_Action& InAction, ECk_Goap_ReplanPolicy InPolicy) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InAction)) { return InAction; }
	auto Owning = ck_goap_action_utils::ResolveOwningPlanner(InAction);
	CK_ENSURE_IF_NOT(ck::IsValid(Owning),
		TEXT("Action [{}] has no owning Planner; Request_SetReplanPolicy dropped."), InAction)
	{ return InAction; }
	auto& Reqs = Owning.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Planner_SetReplanPolicy{InPolicy});
	return InAction;
}

auto
	UCk_Utils_Goap_Action_UE::
	Request_SetSearchBudget(FCk_Handle_Goap_Action& InAction, int64 InMicroseconds) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InAction)) { return InAction; }
	auto Owning = ck_goap_action_utils::ResolveOwningPlanner(InAction);
	CK_ENSURE_IF_NOT(ck::IsValid(Owning),
		TEXT("Action [{}] has no owning Planner; Request_SetSearchBudget dropped."), InAction)
	{ return InAction; }
	auto& Reqs = Owning.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Planner_SetSearchBudget{InMicroseconds});
	return InAction;
}

auto
	UCk_Utils_Goap_Action_UE::
	Request_SetCostThreshold(FCk_Handle_Goap_Action& InAction, float InThreshold) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InAction)) { return InAction; }
	auto Owning = ck_goap_action_utils::ResolveOwningPlanner(InAction);
	CK_ENSURE_IF_NOT(ck::IsValid(Owning),
		TEXT("Action [{}] has no owning Planner; Request_SetCostThreshold dropped."), InAction)
	{ return InAction; }
	auto& Reqs = Owning.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Planner_SetCostThreshold{InThreshold});
	return InAction;
}

// --------------------------------------------------------------------------------------------------------------------
