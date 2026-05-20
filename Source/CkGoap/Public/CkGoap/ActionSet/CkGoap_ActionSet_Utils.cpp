#include "CkGoap/ActionSet/CkGoap_ActionSet_Utils.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/ActionSet/CkGoap_ActionSet_Fragment.h"
#include "CkGoap/ActionSet/CkGoap_ActionSet_Record_Internal.h"  // FFragment_RecordOfGoapActionSets + utils struct
#include "CkGoap/ActionSet/CkGoap_ActionSet_Internal.h"         // DoCreateOrFindActionEntity
#include "CkGoap/Action/CkGoap_Action_Fragment.h"
#include "CkGoap/Action/CkGoap_Action_Record_Internal.h"        // FFragment_RecordOfGoapActions + utils struct
#include "CkGoap/WorldState/CkGoap_WorldState_Utils.h"          // Request_AddSubscriber
#include "CkGoap/CkGoap_Utils.h"  // UCk_Utils_Goap_UE::Find_ActionSet (uniqueness check)
#include "CkAStar/CkAStar_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Signal/CkSignal_Utils.inl.h"

#include "CkLabel/CkLabel_Utils.h"

// ====================================================================================================================
// SHARED INTERNAL HELPER — DoCreateOrFindActionEntity
//
// One entity-creation path shared by AddAction_ToActionSet and AddAction_ToAction.
// Does NOT manage active-chain seeding or tree edges; callers layer that on.
// ====================================================================================================================

auto
    ck::goap::internal_actionset::
    DoCreateOrFindActionEntity(
        FCk_Handle_Goap_ActionSet& InActionSet,
        const FCk_Fragment_Goap_ActionParamsData& InParams) -> FCk_Handle_Goap_Action
{
    CK_ENSURE_IF_NOT(ck::IsValid(InActionSet),
        TEXT("Invalid ActionSet handle in DoCreateOrFindActionEntity"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_ActionClass()),
        TEXT("Invalid _ActionClass in DoCreateOrFindActionEntity (ActionSet [{}])"), InActionSet)
    { return {}; }

    // Identity tag derived from the Action's class.
    const auto ActionTag = UCk_GoapAction_EntityScript::Get_ActionTagForClass(InParams.Get_ActionClass());
    CK_ENSURE_IF_NOT(ActionTag.IsValid(),
        TEXT("Could not derive a valid action tag for class [{}]"), InParams.Get_ActionClass())
    { return {}; }

    // Warn-and-return-existing: a given class can only be registered once per
    // ActionSet catalog. Callers wanting reuse get the existing handle back.
    if (auto Existing = UCk_Utils_Goap_ActionSet_UE::Find_Action(InActionSet, ActionTag);
        ck::IsValid(Existing))
    {
        ck::goap::Warning(
            TEXT("Action with tag [{}] (class [{}]) already exists in ActionSet [{}]; returning existing handle."),
            ActionTag, InParams.Get_ActionClass(), InActionSet);
        return Existing;
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
    Index.AddEntry(ActionTag, ActionEntity);

    // Catalog mutated → re-run ActionSet setup (cycle detection).
    InActionSet.AddOrGet<ck::FTag_Goap_ActionSet_RequiresSetup>();

    return ActionEntity;
}

// ====================================================================================================================

auto
	UCk_Utils_Goap_ActionSet_UE::
	AddActionSet(
		FCk_Handle_Goap& InGoap,
		const FCk_Fragment_Goap_ActionSetParamsData& InParams)
	-> FCk_Handle_Goap_ActionSet
{
	CK_ENSURE_IF_NOT(ck::IsValid(InGoap),
		TEXT("Invalid Goap root handle when adding ActionSet"))
	{ return {}; }

	CK_ENSURE_IF_NOT(InParams.Get_ActionSetTag().IsValid(),
		TEXT("ActionSet params has invalid _ActionSetTag (Goap root [{}])"), InGoap)
	{ return {}; }

	// Diagnostic: ActionSet-tag uniqueness within root.
	if (auto Existing = UCk_Utils_Goap_UE::Find_ActionSet(InGoap, InParams.Get_ActionSetTag());
		ck::IsValid(Existing))
	{
		ck::goap::Warning(
			TEXT("ActionSet with tag [{}] already exists on Goap root [{}]; AddActionSet rejected."),
			InParams.Get_ActionSetTag(), InGoap);
		return {};
	}

	auto ActionSetEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_Goap_ActionSet>(InGoap);

	// Records of ActionSets require GameplayLabels — label the ActionSet with its
	// declared tag. Same for actions in AddAction_ToActionSet.
	UCk_Utils_GameplayLabel_UE::Add(ActionSetEntity, InParams.Get_ActionSetTag());

	ActionSetEntity.Add<ck::FFragment_Goap_ActionSet_Params>(InParams);
	ActionSetEntity.Add<ck::FFragment_Goap_ActionSet_Current>();

	auto& Current = ActionSetEntity.Get<ck::FFragment_Goap_ActionSet_Current>();
	Current._EnableToggle = InParams.Get_InitialToggle();

	ActionSetEntity.Add<ck::FFragment_Goap_ActionSet_ActiveChain>();
	ActionSetEntity.Add<ck::FFragment_Goap_ActionSet_ActionCatalogIndex>();
	ActionSetEntity.Add<ck::FFragment_Goap_ActionSet_WorldStateSource>();
	ActionSetEntity.AddOrGet<ck::FTag_Goap_ActionSet_RequiresSetup>();

	// Register the ActionSet in the root's record.
	ck::goap::internal_root::FRecordOfGoapActionSets_Utils::Request_Connect(InGoap, ActionSetEntity);

	return ActionSetEntity;
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	Has(const FCk_Handle& InHandle) -> bool
{
	return ck::IsValid(InHandle) && InHandle.Has<ck::FFragment_Goap_ActionSet_Params>();
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	Find_Action(
		const FCk_Handle_Goap_ActionSet& InActionSet,
		FGameplayTag InActionTag) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InActionSet)) { return {}; }
	if (NOT InActionTag.IsValid()) { return {}; }

	const auto& Index = InActionSet.Get<ck::FFragment_Goap_ActionSet_ActionCatalogIndex>();
	const auto* Found = Index.Get_TagToAction().Find(InActionTag);
	return Found ? *Found : FCk_Handle_Goap_Action{};
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	Find_ActionByClass(
		const FCk_Handle_Goap_ActionSet& InActionSet,
		TSubclassOf<UCk_GoapAction_EntityScript> InActionClass) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InActionSet)) { return {}; }
	if (NOT ck::IsValid(InActionClass)) { return {}; }

	const auto ActionTag = UCk_GoapAction_EntityScript::Get_ActionTagForClass(InActionClass);
	if (NOT ActionTag.IsValid()) { return {}; }

	return Find_Action(InActionSet, ActionTag);
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	Get_ActiveChain(const FCk_Handle_Goap_ActionSet& InActionSet) -> TArray<FCk_Handle_Goap_Action>
{
	if (NOT ck::IsValid(InActionSet)) { return {}; }
	return InActionSet.Get<ck::FFragment_Goap_ActionSet_ActiveChain>().Get_Chain();
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	Get_EnableToggle(const FCk_Handle_Goap_ActionSet& InActionSet) -> ECk_EnableDisable
{
	if (NOT ck::IsValid(InActionSet)) { return ECk_EnableDisable::Disable; }
	return InActionSet.Get<ck::FFragment_Goap_ActionSet_Current>().Get_EnableToggle();
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	Get_DependencyCycles(const FCk_Handle_Goap_ActionSet& InActionSet) -> TArray<FCk_GoapDiagnostic_DependencyCycle>
{
	if (NOT ck::IsValid(InActionSet)) { return {}; }
	return InActionSet.Get<ck::FFragment_Goap_ActionSet_Current>().Get_DependencyCycles();
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	Get_RootAction(const FCk_Handle_Goap_ActionSet& InActionSet) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InActionSet)) { return {}; }
	return InActionSet.Get<ck::FFragment_Goap_ActionSet_Current>().Get_RootAction();
}

// ====================================================================================================================
// CONSTRUCTION — Root + child actions
// ====================================================================================================================

auto
	UCk_Utils_Goap_ActionSet_UE::
	SetRootAction(
		FCk_Handle_Goap_ActionSet& InActionSet,
		const FCk_Fragment_Goap_ActionParamsData& InRootParams,
		FCk_Handle_Goap_WorldState& InInitialWorldState) -> FCk_Handle_Goap_Action
{
	CK_ENSURE_IF_NOT(ck::IsValid(InActionSet),
		TEXT("Invalid ActionSet handle in SetRootAction"))
	{ return {}; }

	CK_ENSURE_IF_NOT(ck::IsValid(InRootParams.Get_ActionClass()),
		TEXT("Invalid root action class in SetRootAction (ActionSet [{}])"), InActionSet)
	{ return {}; }

	CK_ENSURE_IF_NOT(ck::IsValid(InInitialWorldState),
		TEXT("Invalid initial WorldState handle in SetRootAction (ActionSet [{}])"), InActionSet)
	{ return {}; }

	// Store the WS source on the ActionSet.
	auto& WSFragment = InActionSet.Get<ck::FFragment_Goap_ActionSet_WorldStateSource>();
	WSFragment.Set_WorldStateSource(InInitialWorldState);

	// Create the root Action entity (or reuse if already in the catalog).
	auto RootHandle = ck::goap::internal_actionset::DoCreateOrFindActionEntity(InActionSet, InRootParams);

	if (NOT ck::IsValid(RootHandle))
	{ return {}; }

	// Mark as the ActionSet's root.
	auto& Current = InActionSet.Get<ck::FFragment_Goap_ActionSet_Current>();
	Current._RootAction = RootHandle;

	// Resolve WS source synchronously for the root (no parent to inherit from).
	auto& RootCurrent = RootHandle.Get<ck::FFragment_Goap_Action_Current>();
	RootCurrent._WorldStateSource_Resolved = InInitialWorldState;

	// Subscribe root action to its WS so value-changes flip the dirty tag and
	// AutoReplan fires. Non-root actions get this hook-up in the ActionSet
	// ChainUpdate processor at activation time.
	UCk_Utils_Goap_WorldState_UE::Request_AddSubscriber(InInitialWorldState, RootHandle);

	// Seed the active chain with the root.
	auto& ActiveChain = InActionSet.Get<ck::FFragment_Goap_ActionSet_ActiveChain>();
	ActiveChain._Chain.Reset();
	ActiveChain._Chain.Add(RootHandle);

	return RootHandle;
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	AddAction_ToActionSet(
		FCk_Handle_Goap_ActionSet& InActionSet,
		const FCk_Fragment_Goap_ActionParamsData& InParams) -> FCk_Handle_Goap_Action
{
	auto ActionEntity = ck::goap::internal_actionset::DoCreateOrFindActionEntity(InActionSet, InParams);

	if (NOT ck::IsValid(ActionEntity))
	{ return {}; }

	// First AddAction on an ActionSet = the implicit root action. Seed the
	// active chain. Callers preferring explicit root semantics should use
	// SetRootAction; this path preserves the pre-U2 implicit-root behaviour.
	auto& ActiveChain = InActionSet.Get<ck::FFragment_Goap_ActionSet_ActiveChain>();
	if (ActiveChain._Chain.IsEmpty())
	{
		auto& Current = InActionSet.Get<ck::FFragment_Goap_ActionSet_Current>();
		Current._RootAction = ActionEntity;

		// Validate root has a WS override (no parent to inherit from).
		const auto ActionTag = UCk_GoapAction_EntityScript::Get_ActionTagForClass(InParams.Get_ActionClass());
		if (NOT ck::IsValid(InParams.Get_WorldStateSource_Override()))
		{
			ck::goap::Warning(
				TEXT("Root action [{}] in ActionSet [{}] has no _WorldStateSource_Override; planning will not run until one is set."),
				ActionTag, InActionSet);
		}
		else
		{
			// Resolve WS source synchronously for the root.
			auto& ActionCurrent = ActionEntity.Get<ck::FFragment_Goap_Action_Current>();
			ActionCurrent._WorldStateSource_Resolved = InParams.Get_WorldStateSource_Override();

			// Subscribe root action to its WS.
			auto WS = ActionCurrent._WorldStateSource_Resolved;
			UCk_Utils_Goap_WorldState_UE::Request_AddSubscriber(WS, ActionEntity);
		}

		ActiveChain._Chain.Add(ActionEntity);
	}

	return ActionEntity;
}

// ====================================================================================================================
// REQUESTS
// ====================================================================================================================

auto
	UCk_Utils_Goap_ActionSet_UE::
	Request_SetEnableToggle(
		FCk_Handle_Goap_ActionSet& InActionSet,
		ECk_EnableDisable InToggle) -> FCk_Handle_Goap_ActionSet
{
	CK_ENSURE_IF_NOT(ck::IsValid(InActionSet),
		TEXT("Invalid ActionSet handle in Request_SetEnableToggle"))
	{ return InActionSet; }

	auto& Current = InActionSet.Get<ck::FFragment_Goap_ActionSet_Current>();
	Current._EnableToggle = InToggle;
	return InActionSet;
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	Request_SetRootAction(
		FCk_Handle_Goap_ActionSet& InActionSet,
		const FCk_Fragment_Goap_ActionParamsData& InRootParams,
		FCk_Handle_Goap_WorldState& InInitialWorldState) -> FCk_Handle_Goap_ActionSet
{
	// TODO(U3): U3 will move this to an enqueued FFragment_Goap_ActionSet_Requests
	// variant + a dedicated handler processor. For Phase U2 we mirror the existing
	// Request_SetEnableToggle / Request_ResetActiveChain direct-mutation pattern.
	(void)SetRootAction(InActionSet, InRootParams, InInitialWorldState);
	return InActionSet;
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	Request_ResetActiveChain(FCk_Handle_Goap_ActionSet& InActionSet) -> FCk_Handle_Goap_ActionSet
{
	CK_ENSURE_IF_NOT(ck::IsValid(InActionSet),
		TEXT("Invalid ActionSet handle in Request_ResetActiveChain"))
	{ return InActionSet; }

	auto& ActiveChain = InActionSet.Get<ck::FFragment_Goap_ActionSet_ActiveChain>();
	if (ActiveChain._Chain.Num() <= 1) { return InActionSet; }

	// Tear down every Action past the root (index 0) in reverse order, mirroring
	// the per-action cleanup that FProcessor_Goap_ActionSet_ChainUpdate::
	// DoTruncateChainFrom performs during normal chain divergence:
	//   1. Unsubscribe the Action from its resolved WorldState.
	//   2. Reset live state (goal, plan, parent reference, WS source, status).
	//   3. Broadcast OnActionDeactivated.
	for (auto i = ActiveChain._Chain.Num() - 1; i >= 1; --i)
	{
		auto Action = ActiveChain._Chain[i];
		if (NOT ck::IsValid(Action)) { continue; }

		// 1. WS unsubscribe via the public WorldState util.
		auto& Current = Action.Get<ck::FFragment_Goap_Action_Current>();
		if (ck::IsValid(Current._WorldStateSource_Resolved))
		{
			auto ActionHandle = FCk_Handle{Action};
			UCk_Utils_Goap_WorldState_UE::Request_RemoveSubscriber(
				Current._WorldStateSource_Resolved, ActionHandle);
		}

		// 2. Reset live Action state.
		Current._Goal.Reset();
		Current._InvalidGoal.Reset();
		Current._ActiveParentAction = nullptr;
		Current._WorldStateSource_Resolved = {};
		Current._Plan.Reset();
		Current._PlanStatus = ECk_GoapPlanStatus::Idle;

		// 3. Fire deactivation signal.
		ck::UUtils_Signal_OnGoap_Action_Deactivated::Broadcast(
			Action, ck::MakePayload(Action, FCk_Goap_Payload_OnActionDeactivated{}));
	}

	ActiveChain._Chain.SetNum(1);
	return InActionSet;
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	BindTo_OnActiveChainChanged(
		FCk_Handle_Goap_ActionSet& InActionSet,
		const FCk_Delegate_Goap_OnActiveChainChanged& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior) -> FCk_Handle_Goap_ActionSet
{
	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoap_ActionSet_ActiveChainChanged,
		InActionSet, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InActionSet;
}

auto
	UCk_Utils_Goap_ActionSet_UE::
	UnbindFrom_OnActiveChainChanged(
		FCk_Handle_Goap_ActionSet& InActionSet,
		const FCk_Delegate_Goap_OnActiveChainChanged& InDelegate) -> FCk_Handle_Goap_ActionSet
{
	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoap_ActionSet_ActiveChainChanged,
		InActionSet, InDelegate);
	return InActionSet;
}
