#include "CkGoap/Planner/CkGoap_Planner_Utils.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/Planner/CkGoap_Planner_Fragment.h"
#include "CkGoap/Planner/CkGoap_Planner_Record_Internal.h"  // FFragment_RecordOfGoapPlanners + utils struct
#include "CkGoap/Planner/CkGoap_Planner_Internal.h"         // DoCreateOrFindActionEntity
#include "CkGoap/Action/CkGoap_Action_Fragment.h"
#include "CkGoap/Action/CkGoap_Action_Record_Internal.h"        // FFragment_RecordOfGoapActions + utils struct
#include "CkGoap/WorldState/CkGoap_WorldState_Utils.h"          // Request_AddSubscriber
#include "CkAStar/CkAStar_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Signal/CkSignal_Utils.inl.h"

#include "CkLabel/CkLabel_Utils.h"

// ====================================================================================================================
// SHARED INTERNAL HELPER — DoCreateOrFindActionEntity
//
// One entity-creation path shared by AddAction and AddAction_ToAction.
// Does NOT manage active-chain seeding or tree edges; callers layer that on.
// ====================================================================================================================

auto
    ck::goap::internal_planner::
    DoCreateOrFindActionEntity(
        FCk_Handle_Goap_Planner& InPlanner,
        const FCk_Fragment_Goap_ActionParamsData& InParams) -> FCk_Handle_Goap_Action
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
        TEXT("Invalid ActionSet handle in DoCreateOrFindActionEntity"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_ActionClass()),
        TEXT("Invalid _ActionClass in DoCreateOrFindActionEntity (ActionSet [{}])"), InPlanner)
    { return {}; }

    // Identity tag derived from the Action's class.
    const auto ActionTag = UCk_GoapAction_EntityScript::Get_ActionTagForClass(InParams.Get_ActionClass());
    CK_ENSURE_IF_NOT(ActionTag.IsValid(),
        TEXT("Could not derive a valid action tag for class [{}]"), InParams.Get_ActionClass())
    { return {}; }

    // Warn-and-return-existing: a given class can only be registered once per
    // ActionSet catalog. Callers wanting reuse get the existing handle back.
    if (auto Existing = UCk_Utils_Goap_Planner_UE::Find_Action(InPlanner, ActionTag);
        ck::IsValid(Existing))
    {
        ck::goap::Warning(
            TEXT("Action with tag [{}] (class [{}]) already exists in ActionSet [{}]; returning existing handle."),
            ActionTag, InParams.Get_ActionClass(), InPlanner);
        return Existing;
    }

    auto ActionEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_Goap_Action>(InPlanner);

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
    ck::goap::internal_action::FRecordOfGoapActions_Utils::AddIfMissing(InPlanner);
    ck::goap::internal_action::FRecordOfGoapActions_Utils::Request_Connect(InPlanner, ActionEntity);

    auto& Index = InPlanner.Get<ck::FFragment_Goap_Planner_ActionCatalogIndex>();
    Index.AddEntry(ActionTag, ActionEntity);

    // Catalog mutated → re-run ActionSet setup (cycle detection).
    InPlanner.AddOrGet<ck::FTag_Goap_Planner_RequiresSetup>();

    return ActionEntity;
}

// ====================================================================================================================

auto
	UCk_Utils_Goap_Planner_UE::
	Add(
		FCk_Handle& InOwner,
		const FCk_Fragment_Goap_PlannerParamsData& InParams)
	-> FCk_Handle_Goap_Planner
{
	CK_ENSURE_IF_NOT(ck::IsValid(InOwner),
		TEXT("Invalid owner handle when adding Planner"))
	{ return {}; }

	CK_ENSURE_IF_NOT(InParams.Get_PlannerTag().IsValid(),
		TEXT("Planner params has invalid _PlannerTag (owner [{}])"), InOwner)
	{ return {}; }

	// Diagnostic: Planner-tag uniqueness within owner.
	if (auto Existing = Find_Planner(InOwner, InParams.Get_PlannerTag());
		ck::IsValid(Existing))
	{
		ck::goap::Warning(
			TEXT("Planner with tag [{}] already exists on owner [{}]; Add rejected."),
			InParams.Get_PlannerTag(), InOwner);
		return {};
	}

	auto PlannerEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_Goap_Planner>(InOwner);

	// Records of Planners require GameplayLabels — label the Planner with its
	// declared tag. Same for actions in AddAction.
	UCk_Utils_GameplayLabel_UE::Add(PlannerEntity, InParams.Get_PlannerTag());

	PlannerEntity.Add<ck::FFragment_Goap_Planner_Params>(InParams);
	PlannerEntity.Add<ck::FFragment_Goap_Planner_Current>();

	auto& Current = PlannerEntity.Get<ck::FFragment_Goap_Planner_Current>();
	Current._EnableToggle = InParams.Get_InitialToggle();

	PlannerEntity.Add<ck::FFragment_Goap_Planner_ActiveChain>();
	PlannerEntity.Add<ck::FFragment_Goap_Planner_ActionCatalogIndex>();
	PlannerEntity.Add<ck::FFragment_Goap_Planner_WorldStateSource>();
	PlannerEntity.AddOrGet<ck::FTag_Goap_Planner_RequiresSetup>();

	// Register the Planner in the owner's record.
	ck::goap::internal_planner_record::FRecordOfGoapPlanners_Utils::AddIfMissing(InOwner);
	ck::goap::internal_planner_record::FRecordOfGoapPlanners_Utils::Request_Connect(InOwner, PlannerEntity);

	return PlannerEntity;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Create(
		FCk_Handle& InOwner,
		FGameplayTag InPlannerTag,
		const FCk_Fragment_Goap_PlannerParamsData& InParams)
	-> FCk_Handle_Goap_Planner
{
	auto ParamsCopy = InParams;
	ParamsCopy.Set_PlannerTag(InPlannerTag);
	return Add(InOwner, ParamsCopy);
}

auto
	UCk_Utils_Goap_Planner_UE::
	Find_Planner(
		const FCk_Handle& InOwner,
		FGameplayTag InPlannerTag) -> FCk_Handle_Goap_Planner
{
	auto Result = FCk_Handle_Goap_Planner{};
	if (NOT ck::IsValid(InOwner)) { return Result; }
	if (NOT InPlannerTag.IsValid()) { return Result; }

	auto MutableOwner = InOwner;
	if (NOT MutableOwner.Has<ck::FFragment_RecordOfGoapPlanners>()) { return Result; }

	ck::goap::internal_planner_record::FRecordOfGoapPlanners_Utils::ForEach_ValidEntry(
		MutableOwner,
		[&](FCk_Handle_Goap_Planner InPlanner)
		{
			if (NOT ck::IsValid(InPlanner)) { return; }
			const auto& Params = InPlanner.Get<ck::FFragment_Goap_Planner_Params>();
			if (Params.Get_PlannerTag() == InPlannerTag)
			{
				Result = InPlanner;
			}
		});

	return Result;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Has(const FCk_Handle& InHandle) -> bool
{
	return ck::IsValid(InHandle) && InHandle.Has<ck::FFragment_Goap_Planner_Params>();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Find_Action(
		const FCk_Handle_Goap_Planner& InPlanner,
		FGameplayTag InActionTag) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InPlanner)) { return {}; }
	if (NOT InActionTag.IsValid()) { return {}; }

	const auto& Index = InPlanner.Get<ck::FFragment_Goap_Planner_ActionCatalogIndex>();
	const auto* Found = Index.Get_TagToAction().Find(InActionTag);
	return Found ? *Found : FCk_Handle_Goap_Action{};
}

auto
	UCk_Utils_Goap_Planner_UE::
	Find_ActionByClass(
		const FCk_Handle_Goap_Planner& InPlanner,
		TSubclassOf<UCk_GoapAction_EntityScript> InActionClass) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InPlanner)) { return {}; }
	if (NOT ck::IsValid(InActionClass)) { return {}; }

	const auto ActionTag = UCk_GoapAction_EntityScript::Get_ActionTagForClass(InActionClass);
	if (NOT ActionTag.IsValid()) { return {}; }

	return Find_Action(InPlanner, ActionTag);
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_ActiveChain(const FCk_Handle_Goap_Planner& InPlanner) -> TArray<FCk_Handle_Goap_Action>
{
	if (NOT ck::IsValid(InPlanner)) { return {}; }
	return InPlanner.Get<ck::FFragment_Goap_Planner_ActiveChain>().Get_Chain();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_EnableToggle(const FCk_Handle_Goap_Planner& InPlanner) -> ECk_EnableDisable
{
	if (NOT ck::IsValid(InPlanner)) { return ECk_EnableDisable::Disable; }
	return InPlanner.Get<ck::FFragment_Goap_Planner_Current>().Get_EnableToggle();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_DependencyCycles(const FCk_Handle_Goap_Planner& InPlanner) -> TArray<FCk_GoapDiagnostic_DependencyCycle>
{
	if (NOT ck::IsValid(InPlanner)) { return {}; }
	return InPlanner.Get<ck::FFragment_Goap_Planner_Current>().Get_DependencyCycles();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_RootAction(const FCk_Handle_Goap_Planner& InPlanner) -> FCk_Handle_Goap_Action
{
	if (NOT ck::IsValid(InPlanner)) { return {}; }
	return InPlanner.Get<ck::FFragment_Goap_Planner_Current>().Get_RootAction();
}

// ====================================================================================================================
// CONSTRUCTION — Root + child actions
// ====================================================================================================================

auto
	UCk_Utils_Goap_Planner_UE::
	SetRootAction(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Fragment_Goap_ActionParamsData& InRootParams,
		FCk_Handle_Goap_WorldState& InInitialWorldState) -> FCk_Handle_Goap_Action
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid ActionSet handle in SetRootAction"))
	{ return {}; }

	CK_ENSURE_IF_NOT(ck::IsValid(InRootParams.Get_ActionClass()),
		TEXT("Invalid root action class in SetRootAction (ActionSet [{}])"), InPlanner)
	{ return {}; }

	CK_ENSURE_IF_NOT(ck::IsValid(InInitialWorldState),
		TEXT("Invalid initial WorldState handle in SetRootAction (ActionSet [{}])"), InPlanner)
	{ return {}; }

	// Store the WS source on the ActionSet.
	auto& WSFragment = InPlanner.Get<ck::FFragment_Goap_Planner_WorldStateSource>();
	WSFragment.Set_WorldStateSource(InInitialWorldState);

	// Create the root Action entity (or reuse if already in the catalog).
	auto RootHandle = ck::goap::internal_planner::DoCreateOrFindActionEntity(InPlanner, InRootParams);

	if (NOT ck::IsValid(RootHandle))
	{ return {}; }

	// Mark as the ActionSet's root.
	auto& Current = InPlanner.Get<ck::FFragment_Goap_Planner_Current>();
	Current._RootAction = RootHandle;

	// Resolve WS source synchronously for the root (no parent to inherit from).
	auto& RootCurrent = RootHandle.Get<ck::FFragment_Goap_Action_Current>();
	RootCurrent._WorldStateSource_Resolved = InInitialWorldState;

	// Subscribe root action to its WS so value-changes flip the dirty tag and
	// AutoReplan fires. Non-root actions get this hook-up in the ActionSet
	// ChainUpdate processor at activation time.
	UCk_Utils_Goap_WorldState_UE::Request_AddSubscriber(InInitialWorldState, RootHandle);

	// Seed the active chain with the root.
	auto& ActiveChain = InPlanner.Get<ck::FFragment_Goap_Planner_ActiveChain>();
	ActiveChain._Chain.Reset();
	ActiveChain._Chain.Add(RootHandle);

	return RootHandle;
}

auto
	UCk_Utils_Goap_Planner_UE::
	AddAction(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Fragment_Goap_ActionParamsData& InParams) -> FCk_Handle_Goap_Action
{
	auto ActionEntity = ck::goap::internal_planner::DoCreateOrFindActionEntity(InPlanner, InParams);

	if (NOT ck::IsValid(ActionEntity))
	{ return {}; }

	// First AddAction on an ActionSet = the implicit root action. Seed the
	// active chain. Callers preferring explicit root semantics should use
	// SetRootAction; this path preserves the pre-U2 implicit-root behaviour.
	auto& ActiveChain = InPlanner.Get<ck::FFragment_Goap_Planner_ActiveChain>();
	if (ActiveChain._Chain.IsEmpty())
	{
		auto& Current = InPlanner.Get<ck::FFragment_Goap_Planner_Current>();
		Current._RootAction = ActionEntity;

		// Validate root has a WS override (no parent to inherit from).
		const auto ActionTag = UCk_GoapAction_EntityScript::Get_ActionTagForClass(InParams.Get_ActionClass());
		if (NOT ck::IsValid(InParams.Get_WorldStateSource_Override()))
		{
			ck::goap::Warning(
				TEXT("Root action [{}] in ActionSet [{}] has no _WorldStateSource_Override; planning will not run until one is set."),
				ActionTag, InPlanner);
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
	UCk_Utils_Goap_Planner_UE::
	Request_SetEnableToggle(
		FCk_Handle_Goap_Planner& InPlanner,
		ECk_EnableDisable InToggle) -> FCk_Handle_Goap_Planner
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid ActionSet handle in Request_SetEnableToggle"))
	{ return InPlanner; }

	auto& Current = InPlanner.Get<ck::FFragment_Goap_Planner_Current>();
	Current._EnableToggle = InToggle;
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Request_SetRootAction(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Fragment_Goap_ActionParamsData& InRootParams,
		FCk_Handle_Goap_WorldState& InInitialWorldState) -> FCk_Handle_Goap_Planner
{
	// TODO(U3): U3 will move this to an enqueued FFragment_Goap_ActionSet_Requests
	// variant + a dedicated handler processor. For Phase U2 we mirror the existing
	// Request_SetEnableToggle / Request_ResetActiveChain direct-mutation pattern.
	(void)SetRootAction(InPlanner, InRootParams, InInitialWorldState);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Request_ResetActiveChain(FCk_Handle_Goap_Planner& InPlanner) -> FCk_Handle_Goap_Planner
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid ActionSet handle in Request_ResetActiveChain"))
	{ return InPlanner; }

	auto& ActiveChain = InPlanner.Get<ck::FFragment_Goap_Planner_ActiveChain>();
	if (ActiveChain._Chain.Num() <= 1) { return InPlanner; }

	// Tear down every Action past the root (index 0) in reverse order, mirroring
	// the per-action cleanup that FProcessor_Goap_Planner_ChainUpdate::
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
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	BindTo_OnActiveChainChanged(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnActiveChainChanged& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior) -> FCk_Handle_Goap_Planner
{
	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoap_Planner_ActiveChainChanged,
		InPlanner, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	UnbindFrom_OnActiveChainChanged(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnActiveChainChanged& InDelegate) -> FCk_Handle_Goap_Planner
{
	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoap_Planner_ActiveChainChanged,
		InPlanner, InDelegate);
	return InPlanner;
}
