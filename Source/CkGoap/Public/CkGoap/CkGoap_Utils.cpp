#include "CkGoap_Utils.h"

#include "CkGoap/EntityScripts/CkGoapAction_EntityScript.h"
#include "CkGoap/EntityScripts/CkGoapGoal_EntityScript.h"

#include "CkGoap/CkGoap_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Signal/CkSignal_Utils.inl.h"

// ====================================================================================================================
// CREATION
// ====================================================================================================================

auto
	UCk_Utils_Goap_UE::
	Add(
		FCk_Handle& InOwner)
	-> FCk_Handle_Goap
{
	CK_ENSURE_IF_NOT(ck::IsValid(InOwner),
		TEXT("Invalid owner handle when creating GOAP planner"))
	{ return {}; }

	auto GoapEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner);

	UCk_Utils_Handle_UE::Set_DebugName(GoapEntity,
		TEXT("GOAP Planner"));

	GoapEntity.Add<ck::FTag_Goap_RequiresSetup>();
	GoapEntity.Add<ck::FFragment_Goap_WorldState>();
	GoapEntity.Add<ck::FFragment_Goap_Current>();
	GoapEntity.Add<ck::FFragment_Goap_ActionClasses>();
	GoapEntity.Add<ck::FFragment_Goap_Actions>();
	GoapEntity.Add<ck::FFragment_Goap_GoalClasses>();
	GoapEntity.Add<ck::FFragment_Goap_Goals>();
	GoapEntity.Add<ck::FFragment_Goap_SearchState>();
	GoapEntity.Add<ck::FFragment_Goap_Result>();
	GoapEntity.Add<ck::FFragment_Goap_PlanContext>();
	GoapEntity.Add<ck::FFragment_Goap_Diagnostics>();
	GoapEntity.Add<ck::FFragment_AStar_Params>();
	GoapEntity.Add<ck::FFragment_AStar_Debug>();

	return Cast(GoapEntity);
}

// ====================================================================================================================
// ACTION & GOAL REGISTRATION
// ====================================================================================================================

auto
	UCk_Utils_Goap_UE::
	AddAction(
		FCk_Handle_Goap& InGoap,
		TSubclassOf<UCk_GoapAction_EntityScript> InActionClass)
	-> FCk_Handle_Goap
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap),
		TEXT("Invalid GOAP handle when adding action"))
	{ return InGoap; }

	CK_ENSURE_IF_NOT(ck::IsValid(InActionClass),
		TEXT("Invalid action class when adding to GOAP"))
	{ return InGoap; }

	auto& ActionClasses = InGoap.Get<ck::FFragment_Goap_ActionClasses>();
	ActionClasses._Classes.AddUnique(InActionClass);

	return InGoap;
}

auto
	UCk_Utils_Goap_UE::
	AddGoal(
		FCk_Handle_Goap& InGoap,
		TSubclassOf<UCk_GoapGoal_EntityScript> InGoalClass)
	-> FCk_Handle_Goap
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap),
		TEXT("Invalid GOAP handle when adding goal"))
	{ return InGoap; }

	CK_ENSURE_IF_NOT(ck::IsValid(InGoalClass),
		TEXT("Invalid goal class when adding to GOAP"))
	{ return InGoap; }

	auto& GoalClasses = InGoap.Get<ck::FFragment_Goap_GoalClasses>();
	GoalClasses._Classes.AddUnique(InGoalClass);

	return InGoap;
}

// ====================================================================================================================
// WORLD STATE
// ====================================================================================================================

auto
	UCk_Utils_Goap_UE::
	Set_WorldStateValue(
		FCk_Handle_Goap& InGoap,
		FGameplayTag InKey,
		bool InValue)
	-> FCk_Handle_Goap
{
	return DoAddRequest(InGoap, FCk_Request_Goap_SetWorldState{InKey, InValue});
}

auto
	UCk_Utils_Goap_UE::
	Get_WorldStateValue(
		const FCk_Handle_Goap& InGoap,
		FGameplayTag InKey)
	-> bool
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap),
		TEXT("Invalid GOAP handle in Get_WorldStateValue"))
	{ return false; }

	const auto& WorldState = InGoap.Get<ck::FFragment_Goap_WorldState>();
	return WorldState.Get_WorldState().Get(InKey);
}

// ====================================================================================================================
// DYNAMIC COST UPDATE
// ====================================================================================================================

auto
	UCk_Utils_Goap_UE::
	Set_ActionCost(
		FCk_Handle_Goap& InGoap,
		TSubclassOf<UCk_GoapAction_EntityScript> InActionClass,
		float InNewCost)
	-> FCk_Handle_Goap
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap),
		TEXT("Invalid GOAP handle in Set_ActionCost"))
	{ return InGoap; }

	auto& Actions = InGoap.Get<ck::FFragment_Goap_Actions>();
	for (auto& ActionDef : Actions._ActionDefs)
	{
		if (ActionDef.ActionClass == InActionClass)
		{
			ActionDef.Cost = InNewCost;
			break;
		}
	}

	return InGoap;
}

auto
	UCk_Utils_Goap_UE::
	Get_ActionCost(
		const FCk_Handle_Goap& InGoap,
		TSubclassOf<UCk_GoapAction_EntityScript> InActionClass)
	-> float
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap),
		TEXT("Invalid GOAP handle in Get_ActionCost"))
	{ return 0.0f; }

	const auto& Actions = InGoap.Get<ck::FFragment_Goap_Actions>();
	for (const auto& ActionDef : Actions.Get_ActionDefs())
	{
		if (ActionDef.ActionClass == InActionClass)
		{
			return ActionDef.Cost;
		}
	}

	return 0.0f;
}

// ====================================================================================================================
// PLANNING
// ====================================================================================================================

auto
	UCk_Utils_Goap_UE::
	Request_Plan(
		FCk_Handle_Goap& InGoap)
	-> FCk_Handle_Goap
{
	return DoAddRequest(InGoap, FCk_Request_Goap_Plan{});
}

auto
	UCk_Utils_Goap_UE::
	Request_PlanForGoal(
		FCk_Handle_Goap& InGoap,
		TSubclassOf<UCk_GoapGoal_EntityScript> InGoalClass)
	-> FCk_Handle_Goap
{
	auto Request = FCk_Request_Goap_Plan{};
	Request.Set_SpecificGoalClass(InGoalClass);
	return DoAddRequest(InGoap, Request);
}

auto
	UCk_Utils_Goap_UE::
	Request_CancelPlan(
		FCk_Handle_Goap& InGoap)
	-> FCk_Handle_Goap
{
	return DoAddRequest(InGoap, FCk_Request_Goap_CancelPlan{});
}

// ====================================================================================================================
// QUERY
// ====================================================================================================================

auto
	UCk_Utils_Goap_UE::
	Has(
		const FCk_Handle& InHandle)
	-> bool
{
	return ck::IsValid(InHandle)
		&& InHandle.Has<ck::FFragment_Goap_Current>();
}

auto
	UCk_Utils_Goap_UE::
	Get_PlanStatus(
		const FCk_Handle_Goap& InGoap)
	-> ECk_GoapPlanStatus
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap),
		TEXT("Invalid GOAP handle in Get_PlanStatus"))
	{ return ECk_GoapPlanStatus::Idle; }

	return InGoap.Get<ck::FFragment_Goap_Current>().Get_PlanStatus();
}

auto
	UCk_Utils_Goap_UE::
	Get_Plan(
		const FCk_Handle_Goap& InGoap)
	-> TArray<TSubclassOf<UCk_GoapAction_EntityScript>>
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap),
		TEXT("Invalid GOAP handle in Get_Plan"))
	{ return {}; }

	return InGoap.Get<ck::FFragment_Goap_Current>().Get_Plan();
}

auto
	UCk_Utils_Goap_UE::
	Get_PlanCost(
		const FCk_Handle_Goap& InGoap)
	-> float
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap),
		TEXT("Invalid GOAP handle in Get_PlanCost"))
	{ return 0.0f; }

	return InGoap.Get<ck::FFragment_Goap_Current>().Get_PlanCost();
}

// ====================================================================================================================
// SIGNAL BINDING
// ====================================================================================================================

auto
	UCk_Utils_Goap_UE::
	BindTo_OnPlanComplete(
		FCk_Handle_Goap& InGoap,
		const FCk_Delegate_Goap_OnPlanComplete& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior)
	-> FCk_Handle_Goap
{
	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoapPlanComplete, InGoap, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InGoap;
}

auto
	UCk_Utils_Goap_UE::
	UnbindFrom_OnPlanComplete(
		FCk_Handle_Goap& InGoap,
		const FCk_Delegate_Goap_OnPlanComplete& InDelegate)
	-> FCk_Handle_Goap
{
	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoapPlanComplete, InGoap, InDelegate);
	return InGoap;
}

auto
	UCk_Utils_Goap_UE::
	BindTo_OnPlanFailed(
		FCk_Handle_Goap& InGoap,
		const FCk_Delegate_Goap_OnPlanFailed& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior)
	-> FCk_Handle_Goap
{
	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoapPlanFailed, InGoap, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InGoap;
}

auto
	UCk_Utils_Goap_UE::
	UnbindFrom_OnPlanFailed(
		FCk_Handle_Goap& InGoap,
		const FCk_Delegate_Goap_OnPlanFailed& InDelegate)
	-> FCk_Handle_Goap
{
	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoapPlanFailed, InGoap, InDelegate);
	return InGoap;
}

// ====================================================================================================================
// DIAGNOSTICS
// ====================================================================================================================

auto
	UCk_Utils_Goap_UE::
	Get_DependencyCycles(
		const FCk_Handle_Goap& InGoap)
	-> TArray<FCk_GoapDiagnostic_DependencyCycle>
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap),
		TEXT("Invalid GOAP handle in Get_DependencyCycles"))
	{ return {}; }

	if (NOT InGoap.Has<ck::FFragment_Goap_Diagnostics>()) { return {}; }
	return InGoap.Get<ck::FFragment_Goap_Diagnostics>().Get_DependencyCycles();
}

auto
	UCk_Utils_Goap_UE::
	Get_LastUnreachableGoalConditions(
		const FCk_Handle_Goap& InGoap)
	-> TArray<FCk_GoapDiagnostic_ConditionPair>
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap),
		TEXT("Invalid GOAP handle in Get_LastUnreachableGoalConditions"))
	{ return {}; }

	if (NOT InGoap.Has<ck::FFragment_Goap_Diagnostics>()) { return {}; }
	return InGoap.Get<ck::FFragment_Goap_Diagnostics>().Get_LastUnreachableGoalConditions();
}

auto
	UCk_Utils_Goap_UE::
	Has_DiagnosticWarnings(
		const FCk_Handle_Goap& InGoap)
	-> bool
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap),
		TEXT("Invalid GOAP handle in Has_DiagnosticWarnings"))
	{ return false; }

	if (NOT InGoap.Has<ck::FFragment_Goap_Diagnostics>()) { return false; }
	const auto& Diag = InGoap.Get<ck::FFragment_Goap_Diagnostics>();
	return Diag.Get_DependencyCycles().Num() > 0
		|| Diag.Get_LastUnreachableGoalConditions().Num() > 0;
}

// ====================================================================================================================
// CAST
// ====================================================================================================================

auto
	UCk_Utils_Goap_UE::
	DoCast(
		FCk_Handle& InHandle,
		ECk_SucceededFailed& OutResult)
	-> FCk_Handle_Goap
{
	if (Has(InHandle))
	{
		OutResult = ECk_SucceededFailed::Succeeded;
		return Cast(InHandle);
	}

	OutResult = ECk_SucceededFailed::Failed;
	return {};
}

auto
	UCk_Utils_Goap_UE::
	DoCastChecked(
		FCk_Handle InHandle)
	-> FCk_Handle_Goap
{
	return CastChecked(InHandle);
}

// ====================================================================================================================
// INTERNALS
// ====================================================================================================================

auto
	UCk_Utils_Goap_UE::
	DoAddRequest(
		FCk_Handle_Goap& InGoap,
		const auto& InRequest)
	-> FCk_Handle_Goap
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap),
		TEXT("Invalid GOAP handle when adding request"))
	{ return InGoap; }

	auto& Requests = InGoap.AddOrGet<ck::FFragment_Goap_Requests>();
	Requests._Requests.Add(InRequest);

	return InGoap;
}

// ====================================================================================================================
