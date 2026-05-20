#include "CkGoap/Action/CkGoap_Action_Utils.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/ActionSet/CkGoap_ActionSet_Fragment.h"
#include "CkGoap/Action/CkGoap_Action_Fragment.h"
#include "CkGoap/Action/CkGoap_Action_Record_Internal.h"  // FFragment_RecordOfGoapActions + utils struct
#include "CkGoap/ActionSet/CkGoap_ActionSet_Utils.h"  // Find_Action (uniqueness check)
#include "CkGoap/WorldState/CkGoap_WorldState_Utils.h"  // Request_AddSubscriber
#include "CkAStar/CkAStar_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Signal/CkSignal_Utils.inl.h"

#include "CkLabel/CkLabel_Utils.h"

// ====================================================================================================================
// CONSTRUCTION
// ====================================================================================================================

auto
	UCk_Utils_Goap_Action_UE::
	AddAction_ToActionSet(
		FCk_Handle_Goap_ActionSet& InActionSet,
		const FCk_Fragment_Goap_ActionParamsData& InParams)
	-> FCk_Handle_Goap_Action
{
	CK_ENSURE_IF_NOT(ck::IsValid(InActionSet),
		TEXT("Invalid ActionSet handle when adding action"))
	{ return {}; }

	CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_ActionClass()),
		TEXT("Action params has invalid _ActionClass (ActionSet [{}])"), InActionSet)
	{ return {}; }

	// Identity tag derived from the Action's class.
	const auto ActionTag = UCk_GoapAction_EntityScript::Get_ActionTagForClass(InParams.Get_ActionClass());
	CK_ENSURE_IF_NOT(ActionTag.IsValid(),
		TEXT("Could not derive a valid action tag for class [{}]"), InParams.Get_ActionClass())
	{ return {}; }

	// Diagnostic: action-tag uniqueness within ActionSet.
	if (auto Existing = UCk_Utils_Goap_ActionSet_UE::Find_Action(InActionSet, ActionTag);
		ck::IsValid(Existing))
	{
		ck::goap::Warning(
			TEXT("Action with tag [{}] already exists in ActionSet [{}]; AddAction_ToActionSet rejected."),
			ActionTag, InActionSet);
		return {};
	}

	auto ActionEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_Goap_Action>(InActionSet);

	// Records of actions require GameplayLabels — label the action with its
	// derived tag.
	UCk_Utils_GameplayLabel_UE::Add(ActionEntity, ActionTag);

	ActionEntity.Add<ck::FFragment_Goap_Action_Params>(InParams);
	ActionEntity.Add<ck::FFragment_Goap_Action_Current>();
	ActionEntity.Add<ck::FFragment_Goap_Action_ActionClasses>();
	ActionEntity.Add<ck::FFragment_Goap_Action_Definition>();
	ActionEntity.Add<ck::FFragment_Goap_Action_Tree>();
	ActionEntity.Add<ck::FFragment_Goap_Action_Requests>();

	auto& Throttle = ActionEntity.AddOrGet<ck::FFragment_Goap_Action_ReplanThrottle>();
	(void)Throttle;  // throttle's interval is read from params at Setup time

	ActionEntity.Add<ck::FFragment_Goap_Action_SearchState>();
	ActionEntity.Add<ck::FFragment_Goap_Action_Result>();
	ActionEntity.Add<ck::FFragment_Goap_Action_PlanContext>();

	// A* params mirror the action's planning knobs.
	auto AStarParams = ck::FFragment_AStar_Params{};
	AStarParams.Set_BudgetMicroseconds(InParams.Get_SearchBudgetMicroseconds());
	AStarParams.Set_CostThreshold(InParams.Get_CostThreshold());
	ActionEntity.Add<ck::FFragment_AStar_Params>(AStarParams);
	ActionEntity.Add<ck::FFragment_AStar_Debug>();

	// Mark for one-shot setup.
	ActionEntity.AddOrGet<ck::FTag_Goap_Action_RequiresSetup>();

	if (InParams.Get_PlanOnStart())
	{
		ActionEntity.AddOrGet<ck::FTag_Goap_Action_RequiresInitialPlan>();
	}

	// Register in the ActionSet's catalog record + tag→action index.
	ck::goap::internal_action::FRecordOfGoapActions_Utils::AddIfMissing(InActionSet);
	ck::goap::internal_action::FRecordOfGoapActions_Utils::Request_Connect(InActionSet, ActionEntity);

	auto& Index = InActionSet.Get<ck::FFragment_Goap_ActionSet_ActionCatalogIndex>();
	Index._TagToAction.Add(ActionTag, ActionEntity);

	// Catalog mutated → re-run ActionSet setup (cycle detection).
	InActionSet.AddOrGet<ck::FTag_Goap_ActionSet_RequiresSetup>();

	// First AddAction on an ActionSet = the root action. Seed the active chain.
	auto& ActiveChain = InActionSet.Get<ck::FFragment_Goap_ActionSet_ActiveChain>();
	if (ActiveChain._Chain.IsEmpty())
	{
		// Validate root has a WS override (no parent to inherit from).
		if (NOT ck::IsValid(InParams.Get_WorldStateSource_Override()))
		{
			ck::goap::Warning(
				TEXT("Root action [{}] in ActionSet [{}] has no _WorldStateSource_Override; planning will not run until one is set."),
				ActionTag, InActionSet);
		}
		else
		{
			// Resolve WS source synchronously for the root.
			auto& Current = ActionEntity.Get<ck::FFragment_Goap_Action_Current>();
			Current._WorldStateSource_Resolved = InParams.Get_WorldStateSource_Override();

			// Subscribe root action to its WS so value-changes flip the dirty
			// tag and AutoReplan fires. Non-root actions get this hook-up in
			// the ActionSet ChainUpdate processor at activation time.
			auto WS = Current._WorldStateSource_Resolved;
			UCk_Utils_Goap_WorldState_UE::Request_AddSubscriber(WS, ActionEntity);
		}

		ActiveChain._Chain.Add(ActionEntity);
	}

	return ActionEntity;
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
