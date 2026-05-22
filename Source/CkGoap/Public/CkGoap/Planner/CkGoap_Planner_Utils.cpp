#include "CkGoap/Planner/CkGoap_Planner_Utils.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/Planner/CkGoap_Planner_Fragment.h"
#include "CkGoap/Planner/CkGoap_Planner_Processor.h"        // FProcessor_Goap_Planner_UpdateActivation::DoDeactivatePlanner
#include "CkGoap/Planner/CkGoap_Planner_Record_Internal.h"  // FFragment_RecordOfGoapPlanners + utils struct
#include "CkGoap/Planner/CkGoap_Planner_Internal.h"         // DoCreateOrFindActionEntity
#include "CkGoap/Action/CkGoap_Action_Fragment.h"
#include "CkGoap/Action/CkGoap_Action_Utils.h"             // U11.3: Action Has() for promotion validation
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

    // Planner-role fragments — every Action entity is dual-role (carries its own
    // Action-role _Definition + its own per-Action planner state). See
    // CkGoap_Planner_Fragment.h for the split rationale (U11.0).
    ActionEntity.Add<ck::FFragment_Goap_Planner_PlanState>();
    ActionEntity.Add<ck::FFragment_Goap_Planner_Goal>();
    ActionEntity.Add<ck::FFragment_Goap_Planner_WorldStateSource>();

    // U11.2: Action-as-Planner-role gets per-Planner activation state. Starts
    // _IsActive=false; SetRootAction flips the root Action's _IsActive=true,
    // and mid-tier Actions get flipped on by their parent's UpdateActivation.
    ActionEntity.Add<ck::FFragment_Goap_Planner_Activation>();

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

	PlannerEntity.Add<ck::FFragment_Goap_Planner_ActionCatalogIndex>();
	PlannerEntity.Add<ck::FFragment_Goap_Planner_WorldStateSource>();
	// U11.0 split: top-level Planner is also dual-role; stamp PlanState + Goal
	// alongside the existing Planner-role fragments.
	PlannerEntity.Add<ck::FFragment_Goap_Planner_PlanState>();
	{
		auto GoalFrag = ck::FFragment_Goap_Planner_Goal{};
		// U11.1: PlannerParams._Goal is the source-of-truth at construction.
		// Setup resolves _GoalAuthored → _Goal at the Planner's tier; for the
		// root Action that actually runs A* in the current model, SetRootAction
		// propagates this same authored goal down (see SetRootAction below).
		GoalFrag._GoalAuthored = InParams.Get_Goal();
		PlannerEntity.Add<ck::FFragment_Goap_Planner_Goal>(GoalFrag);
	}

	// U11.2: top-level Planners are always active by construction (no parent
	// to activate them). Mid-tier sub-Planners are activated by their parent's
	// UpdateActivation pass.
	{
		auto ActivationFrag = ck::FFragment_Goap_Planner_Activation{};
		ActivationFrag._IsActive = true;
		PlannerEntity.Add<ck::FFragment_Goap_Planner_Activation>(ActivationFrag);
	}

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

	// U11.2: the active chain is no longer stored in a fragment — it's derived
	// on demand by walking Plan[0]s from the root Action down. Each composite
	// step is appended; the walk terminates at an atomic Action or when a
	// child sub-Planner is not yet marked _IsActive (next frame's
	// UpdateActivation will fix that).
	auto Result = TArray<FCk_Handle_Goap_Action>{};

	const auto& Current = InPlanner.Get<ck::FFragment_Goap_Planner_Current>();
	auto Curr = Current.Get_RootAction();
	if (NOT ck::IsValid(Curr)) { return Result; }

	// Cap depth defensively (matches the planner's dependency-cycle
	// diagnostics — a cyclic catalog can theoretically loop the walk).
	constexpr auto MaxDepth = 64;
	auto Seen = TSet<FCk_Handle_Goap_Action>{};

	Result.Add(Curr);
	Seen.Add(Curr);

	for (auto Depth = 0; Depth < MaxDepth; ++Depth)
	{
		const auto& PlanState = Curr.Get<ck::FFragment_Goap_Planner_PlanState>();
		const auto& Plan = PlanState.Get_Plan();
		if (Plan.IsEmpty()) { break; }

		const auto Next = Plan[0];
		if (NOT ck::IsValid(Next)) { break; }

		// Only composite sub-Planners participate in the active-chain walk —
		// atomic Plan[0] entries are terminal (consumer drives the leaf).
		const auto& NextTree = Next.Get<ck::FFragment_Goap_Action_Tree>();
		if (NextTree.Get_ChildActions().IsEmpty()) { break; }

		// Only include in the chain if the sub-Planner is actually active.
		// (UpdateActivation may not have flipped _IsActive yet on the same
		// frame the plan resolved — caller polls Get_ActiveChain across
		// frames in that case, mirroring U10/U11.0 chain-extension timing.)
		const auto& NextActivation = Next.Get<ck::FFragment_Goap_Planner_Activation>();
		if (NOT NextActivation.Get_IsActive()) { break; }

		if (Seen.Contains(Next)) { break; }
		Seen.Add(Next);
		Result.Add(Next);
		Curr = Next;
	}

	return Result;
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
	auto& RootWSSource = RootHandle.Get<ck::FFragment_Goap_Planner_WorldStateSource>();
	RootWSSource._Resolved = InInitialWorldState;

	// U11.1: propagate the Planner's authored goal onto the root Action's
	// planner-role goal fragment. In the current (pre-U11.2) model, the root
	// Action is the entity that actually runs A* on behalf of this Planner —
	// it needs the same authored goal so its own Setup resolves it correctly.
	{
		const auto& PlannerGoal = InPlanner.Get<ck::FFragment_Goap_Planner_Goal>();
		auto& RootGoal = RootHandle.Get<ck::FFragment_Goap_Planner_Goal>();
		RootGoal._GoalAuthored = PlannerGoal.Get_GoalAuthored();
	}

	// Subscribe root action to its WS so value-changes flip the dirty tag and
	// AutoReplan fires. Non-root actions get this hook-up in UpdateActivation
	// at activation time.
	UCk_Utils_Goap_WorldState_UE::Request_AddSubscriber(InInitialWorldState, RootHandle);

	// U11.2: the root Action is the entry-point for the activation walk. Flip
	// its _IsActive=true so the UpdateActivation processor picks it up. (Its
	// own parent-Planner activation pass would have done this, but the top-
	// level Planner doesn't run A* in the transitional model so there's no
	// upstream Plan[0] to activate the root via.)
	{
		auto& RootActivation = RootHandle.Get<ck::FFragment_Goap_Planner_Activation>();
		RootActivation._IsActive = true;
	}

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

	// First AddAction on an ActionSet = the implicit root action. Callers
	// preferring explicit root semantics should use SetRootAction; this path
	// preserves the pre-U2 implicit-root behaviour.
	auto& Current = InPlanner.Get<ck::FFragment_Goap_Planner_Current>();
	if (NOT ck::IsValid(Current._RootAction))
	{
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
			auto& ActionWSSource = ActionEntity.Get<ck::FFragment_Goap_Planner_WorldStateSource>();
			ActionWSSource._Resolved = InParams.Get_WorldStateSource_Override();

			// Subscribe root action to its WS.
			auto WS = ActionWSSource._Resolved;
			UCk_Utils_Goap_WorldState_UE::Request_AddSubscriber(WS, ActionEntity);
		}

		// U11.2: implicit-root entry-point for the activation walk.
		auto& RootActivation = ActionEntity.Get<ck::FFragment_Goap_Planner_Activation>();
		RootActivation._IsActive = true;
	}

	return ActionEntity;
}

auto
	UCk_Utils_Goap_Planner_UE::
	PromoteActionToPlanner(
		FCk_Handle_Goap_Action& InAction,
		const FCk_Fragment_Goap_PlannerParamsData& InParams) -> FCk_Handle_Goap_Planner
{
	CK_ENSURE_IF_NOT(ck::IsValid(InAction),
		TEXT("Invalid Action handle in PromoteActionToPlanner"))
	{ return {}; }

	// Must be an Action — Promotion takes an existing Action role and adds the
	// Planner role on top.
	CK_ENSURE_IF_NOT(UCk_Utils_Goap_Action_UE::Has(InAction),
		TEXT("Handle [{}] does not have the Action role; PromoteActionToPlanner requires an Action entity."), InAction)
	{ return {}; }

	CK_ENSURE_IF_NOT(InParams.Get_PlannerTag().IsValid(),
		TEXT("Promote params has invalid _PlannerTag (Action [{}])"), InAction)
	{ return {}; }

	// If already a Planner, treat as a no-op promotion: return the existing
	// Planner-cast and warn. Re-stamping Params/Current would clobber catalog
	// state.
	if (UCk_Utils_Goap_Planner_UE::Has(InAction))
	{
		ck::goap::Warning(
			TEXT("Handle [{}] is already a Planner; PromoteActionToPlanner is a no-op (returning existing Planner-cast)."), InAction);
		return UCk_Utils_Goap_Planner_UE::Cast(InAction);
	}

	// Stamp the Planner-role discriminator fragments. The remaining planner-role
	// fragment cluster (PlanState, Goal, WorldStateSource, Activation) was
	// already added by DoCreateOrFindActionEntity when this entity was created
	// as an Action (Path A — transitional dual-role default).
	InAction.Add<ck::FFragment_Goap_Planner_Params>(InParams);
	InAction.Add<ck::FFragment_Goap_Planner_Current>();
	InAction.Add<ck::FFragment_Goap_Planner_ActionCatalogIndex>();

	auto& Current = InAction.Get<ck::FFragment_Goap_Planner_Current>();
	Current._EnableToggle = InParams.Get_InitialToggle();

	// U11.1: the Planner's authored goal is independent of the Action role's
	// effects. Overwrite the existing _GoalAuthored (which may have been set
	// when this entity was the root of a higher-level planner) so the promoted
	// Planner plans toward what its own InParams says.
	{
		auto& GoalFrag = InAction.Get<ck::FFragment_Goap_Planner_Goal>();
		GoalFrag._GoalAuthored = InParams.Get_Goal();
		// Clear _Goal — Setup will re-resolve _GoalAuthored → _Goal using this
		// entity's WS source.
		GoalFrag._Goal = {};
		GoalFrag._InvalidGoal = {};
	}

	// Re-run setup so cycle detection and goal resolution pick up the new
	// Planner-role config.
	InAction.AddOrGet<ck::FTag_Goap_Planner_RequiresSetup>();

	return UCk_Utils_Goap_Planner_UE::Cast(InAction);
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

	// U11.2: walk the current active chain (root → leaf) and deactivate every
	// sub-Planner past the root. DoDeactivatePlanner handles WS unsubscribe,
	// live-state reset, _IsActive flip, OnPlannerDeactivated broadcast, and
	// recursive descendant teardown.
	const auto Chain = Get_ActiveChain(InPlanner);
	if (Chain.Num() <= 1) { return InPlanner; }

	// Deactivate in reverse order (leaf → child-of-root). The root itself
	// stays active.
	for (auto i = Chain.Num() - 1; i >= 1; --i)
	{
		auto Action = Chain[i];
		if (NOT ck::IsValid(Action)) { continue; }
		ck::FProcessor_Goap_Planner_UpdateActivation::DoDeactivatePlanner(Action);
	}

	// The root Action's cached _LastActivatedPlan0 still points at the old
	// child; clear it so the next UpdateActivation tick sees this as a fresh
	// state transition (rather than no-op'ing because OldStep0 == NewStep0).
	{
		auto RootAction = Chain[0];
		if (ck::IsValid(RootAction))
		{
			auto& RootActivation = RootAction.Get<ck::FFragment_Goap_Planner_Activation>();
			RootActivation._LastActivatedPlan0 = {};
		}
	}

	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Request_SetGoal(
		FCk_Handle_Goap_Planner& InPlanner,
		const TArray<FCk_GoapWS_Condition_Authored>& InGoal) -> FCk_Handle_Goap_Planner
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Request_SetGoal"))
	{ return InPlanner; }

	// Also stamp the Planner's own _GoalAuthored so any future query of the
	// Planner's authored goal reads the current source-of-truth. The actual
	// resolution + replan happens on the root Action (which runs the A*).
	{
		auto& PlannerGoal = InPlanner.Get<ck::FFragment_Goap_Planner_Goal>();
		PlannerGoal._GoalAuthored = InGoal;
	}

	const auto& Current = InPlanner.Get<ck::FFragment_Goap_Planner_Current>();
	auto RootAction = Current.Get_RootAction();

	CK_ENSURE_IF_NOT(ck::IsValid(RootAction),
		TEXT("Planner [{}] has no root Action; Request_SetGoal requires a root to dispatch through. Call SetRootAction first."),
		InPlanner)
	{ return InPlanner; }

	auto& Requests = RootAction.AddOrGet<ck::FFragment_Goap_Action_Requests>();
	Requests._Requests.Add(FCk_Request_Goap_Planner_SetGoal{InGoal});
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
