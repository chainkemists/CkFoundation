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
// Single entity-creation primitive used by AddAction. Does NOT manage
// implicit-root seeding or tree edges; callers layer that on.
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
    ActionEntity.Add<ck::FFragment_Goap_Action_Definition>();
    ActionEntity.Add<ck::FFragment_Goap_Action_Tree>();

    // PR-B.1b Stage 5: atomic leaf Actions are lean — they carry only Action-
    // role fragments (_Definition, _Params, _Tree, _Current) plus
    // _WorldStateSource (read by FProcessor_Goap_Action_Setup to resolve
    // candidate-operator preconditions/effects against the active WS registry,
    // and by UCk_Utils_Goap_Action_UE::Get_WorldStateSource(InAction)).
    //
    // Sub-Planners (composite Actions promoted via PromoteActionToPlanner)
    // pick up the Planner-role cluster (PlanState, Goal, Activation, Requests,
    // ReplanThrottle, SearchState, Result, PlanContext, AStar_Params,
    // AStar_Debug) through that promotion's AddOrGet pass.
    ActionEntity.Add<ck::FFragment_Goap_Planner_WorldStateSource>();

    // Mark for one-shot setup.
    ActionEntity.AddOrGet<ck::FTag_Goap_Action_RequiresSetup>();

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

	// PR-A: stamp the Planner's WorldStateSource at construction. Was previously
	// supplied via SetRootAction's separate WS argument; now read off the
	// PlannerParams' _WorldStateSource field. Top-level Planners must set this;
	// promoted mid-tier Planners may leave it unset (inherits parent's resolved
	// WS at activation time).
	//
	// PR-B.1b Stage 3: also seed _Resolved with the same value for top-level
	// Planners so the Planner-tier Setup processor (goal resolution) and
	// HandleRequests (A* seeding) can read it without going through the
	// implicit-root Action. Promoted mid-tier Planners go through activation
	// (UpdateActivation::DoResolveAndAssignWorldStateSource) — same as before.
	{
		auto WSFrag = ck::FFragment_Goap_Planner_WorldStateSource{};
		WSFrag._WorldStateSource = InParams.Get_WorldStateSource();
		WSFrag._Resolved          = InParams.Get_WorldStateSource();
		PlannerEntity.Add<ck::FFragment_Goap_Planner_WorldStateSource>(WSFrag);
	}

	// U11.0 split: top-level Planner is also dual-role; stamp PlanState + Goal
	// alongside the existing Planner-role fragments.
	PlannerEntity.Add<ck::FFragment_Goap_Planner_PlanState>();
	{
		auto GoalFrag = ck::FFragment_Goap_Planner_Goal{};
		// U11.1: PlannerParams._Goal is the source-of-truth at construction.
		// Setup resolves _GoalAuthored → _Goal at the Planner's tier; the first
		// AddAction call on a top-level Planner propagates this same authored
		// goal down to the implicit-root Action (the entity that actually runs
		// A* in the transitional Path A model).
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

	// PR-B.1b Stage 2/3a — dual-stamp the full Planner-role A*-pipeline cluster
	// on the Planner entity. Stage 3 retargets the processors to read these
	// Planner-side fragments authoritatively. Stage 3a (this commit) seeds
	// FFragment_AStar_Params with the Planner's own budget/cost-threshold knobs
	// (lifted from per-Action params per spec §3.2). Without this, Stage 3b's
	// processor flip would regress the A* budget 100x (default 500us vs
	// PlannerParams' 50000us).
	{
		auto AStarParams = ck::FFragment_AStar_Params{};
		AStarParams.Set_BudgetMicroseconds(InParams.Get_SearchBudgetMicroseconds());
		AStarParams.Set_CostThreshold(InParams.Get_CostThreshold());
		PlannerEntity.Add<ck::FFragment_AStar_Params>(AStarParams);
	}
	PlannerEntity.Add<ck::FFragment_AStar_Debug>();

	// Planner-side A* pipeline fragments. The aliases (see CkGoap_Planner_Fragment.h)
	// resolve to the Action-side types under the hood; Stage 5 promotes them to
	// first-class types. EnTT treats Planner vs Action as distinct entities, so
	// each carries its own copy of these fragments.
	PlannerEntity.Add<ck::FFragment_Goap_Planner_SearchState>();
	PlannerEntity.Add<ck::FFragment_Goap_Planner_Result>();
	PlannerEntity.Add<ck::FFragment_Goap_Planner_PlanContext>();
	PlannerEntity.Add<ck::FFragment_Goap_Planner_Requests>();
	PlannerEntity.AddOrGet<ck::FFragment_Goap_Planner_ReplanThrottle>();

	PlannerEntity.AddOrGet<ck::FTag_Goap_Planner_RequiresSetup>();

	// PR-B.1b Stage 5: subscribe the top-level Planner to its WS source so
	// WS-dirty tags land on the Planner for AutoReplan to pick up. Previously
	// done by AddAction's implicit-root branch.
	if (ck::IsValid(InParams.Get_WorldStateSource()))
	{
		auto PlannerAsGeneric = static_cast<FCk_Handle>(PlannerEntity);
		auto WS = InParams.Get_WorldStateSource();
		UCk_Utils_Goap_WorldState_UE::Request_AddSubscriber(WS, PlannerAsGeneric);
	}

	// PR-B.1b Stage 5: _PlanOnStart lifted to PlannerParams. Stamp the
	// initial-plan tag so AutoReplan fires the first plan after Setup.
	if (InParams.Get_PlanOnStart())
	{
		PlannerEntity.AddOrGet<ck::FTag_Goap_Planner_RequiresInitialPlan>();
	}

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

	// PR-B.1b Stage 5: there is no implicit-root prefix. The chain starts
	// with Plan[0] of the Planner's own PlanState. Subsequent entries are
	// produced by walking Plan[0] of each composite + active sub-Planner.
	auto Result = TArray<FCk_Handle_Goap_Action>{};

	const auto& PlannerPlanState = InPlanner.Get<ck::FFragment_Goap_Planner_PlanState>();
	const auto& InitialPlan = PlannerPlanState.Get_Plan();
	if (InitialPlan.IsEmpty()) { return Result; }

	auto Curr = InitialPlan[0];
	if (NOT ck::IsValid(Curr)) { return Result; }

	// First chain entry: if it carries the Planner-role Activation fragment
	// (i.e. it's a sub-Planner) it must be active. Atomic Actions (no
	// Activation fragment) are always included as the leaf chain step.
	if (Curr.Has<ck::FFragment_Goap_Planner_Activation>())
	{
		const auto& CurrActivation = Curr.Get<ck::FFragment_Goap_Planner_Activation>();
		if (NOT CurrActivation.Get_IsActive()) { return Result; }
	}

	auto Seen = TSet<FCk_Handle_Goap_Action>{};
	Seen.Add(Curr);
	Result.Add(Curr);

	constexpr auto MaxDepth = 64;
	for (auto Depth = 0; Depth < MaxDepth; ++Depth)
	{
		// Only composite + active sub-Planners contribute a further step.
		if (NOT Curr.Has<ck::FFragment_Goap_Action_Tree>()) { break; }
		const auto& CurrTree = Curr.Get<ck::FFragment_Goap_Action_Tree>();
		if (CurrTree.Get_ChildActions().IsEmpty()) { break; }

		if (NOT Curr.Has<ck::FFragment_Goap_Planner_Activation>()) { break; }
		const auto& CurrActivation = Curr.Get<ck::FFragment_Goap_Planner_Activation>();
		if (NOT CurrActivation.Get_IsActive()) { break; }

		if (NOT Curr.Has<ck::FFragment_Goap_Planner_PlanState>()) { break; }
		const auto& PlanState = Curr.Get<ck::FFragment_Goap_Planner_PlanState>();
		const auto& Plan = PlanState.Get_Plan();
		if (Plan.IsEmpty()) { break; }

		const auto Next = Plan[0];
		if (NOT ck::IsValid(Next)) { break; }
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

// --------------------------------------------------------------------------------------------------------------------
// PR-B.1b Stage 3: Planner-API getters read Planner-side fragments directly.
// The A* pipeline now writes to the Planner entity's own PlanState/Goal/
// WorldStateSource — no _RootAction indirection needed.
// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_Goap_Planner_UE::
	Get_PlanStatus(const FCk_Handle_Goap_Planner& InPlanner) -> ECk_GoapPlanStatus
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_PlanStatus"))
	{ return ECk_GoapPlanStatus::Idle; }

	return InPlanner.Get<ck::FFragment_Goap_Planner_PlanState>().Get_PlanStatus();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_Plan(const FCk_Handle_Goap_Planner& InPlanner) -> TArray<FCk_Handle_Goap_Action>
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_Plan"))
	{ return {}; }

	return InPlanner.Get<ck::FFragment_Goap_Planner_PlanState>().Get_Plan();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_PlanClasses(const FCk_Handle_Goap_Planner& InPlanner) -> TArray<TSubclassOf<UCk_GoapAction_EntityScript>>
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_PlanClasses"))
	{ return {}; }

	return InPlanner.Get<ck::FFragment_Goap_Planner_PlanState>().Get_PlanClasses();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_PlanCost(const FCk_Handle_Goap_Planner& InPlanner) -> float
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_PlanCost"))
	{ return 0.0f; }

	return InPlanner.Get<ck::FFragment_Goap_Planner_PlanState>().Get_PlanCost();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_PlanAttemptCount(const FCk_Handle_Goap_Planner& InPlanner) -> int32
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_PlanAttemptCount"))
	{ return 0; }

	return InPlanner.Get<ck::FFragment_Goap_Planner_PlanState>().Get_PlanAttemptCount();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_WorldStateSource(const FCk_Handle_Goap_Planner& InPlanner) -> FCk_Handle_Goap_WorldState
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_WorldStateSource"))
	{ return {}; }

	const auto& WSSource = InPlanner.Get<ck::FFragment_Goap_Planner_WorldStateSource>();
	const auto Resolved = WSSource.Get_Resolved();
	if (ck::IsValid(Resolved)) { return Resolved; }
	return WSSource.Get_WorldStateSource();
}

auto
	UCk_Utils_Goap_Planner_UE::
	Get_InvalidGoal(const FCk_Handle_Goap_Planner& InPlanner) -> TArray<FCk_GoapWS_Condition_Authored>
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Get_InvalidGoal"))
	{ return {}; }

	return InPlanner.Get<ck::FFragment_Goap_Planner_Goal>().Get_InvalidGoal();
}

// ====================================================================================================================
// CONSTRUCTION — AddAction (the only construction verb for children)
//
// PR-B.1b Stage 5: there is no implicit-root concept. Every AddAction creates
// a direct child of the Planner. Two host shapes still exist:
//
// * Top-level Planner (no Action role on the host entity): child is parented
//   directly to the Planner; its WS is resolved from the Planner's _Resolved
//   (or the child's own override).
//
// * Promoted mid-tier Planner (host entity carries the Action role): child
//   is parented to the host (which is also an Action entity).
//
// In both shapes the parent that owns the child's Tree edge is whatever
// Action entity "owns" the operator catalog at this tier:
//   - Top-level Planner: there's no such Action — we leave the child's
//     _ParentAction invalid and the candidate set is read straight from the
//     Planner's ActionCatalogIndex.
//   - Promoted mid-tier Planner: the host-cast-to-Action is the parent.
// ====================================================================================================================

// PR-A shared helper. Declared in CkGoap_Planner_Internal.h; befriended on
// FFragment_Goap_Planner_WorldStateSource so it can write _Resolved directly.
auto
	ck::goap::internal_planner::
	DoResolveChildWorldStateFromParent(
		FCk_Handle_Goap_Action& InChild,
		const FCk_Handle_Goap_Action& InParentAction) -> void
{
	// Eagerly resolve the child's WS so its Setup processor can run
	// (and populate _CachedActionDef) BEFORE any parent plan is requested.
	// Resolution order:
	//   1. Child's own override.
	//   2. Inherit from parent's already-resolved WS (when there IS a parent
	//      Action — i.e. promoted mid-tier).
	//   3. Fall back to the Planner-level default WS source on the lifetime
	//      owner (the top-level Planner entity).
	auto& ChildWSSource = InChild.Get<ck::FFragment_Goap_Planner_WorldStateSource>();
	if (ck::IsValid(ChildWSSource.Get_Resolved())) { return; }

	const auto& ChildParams = InChild.Get<ck::FFragment_Goap_Action_Params>();
	const auto Override = ChildParams.Get_WorldStateSource_Override();
	if (ck::IsValid(Override))
	{
		ChildWSSource._Resolved = Override;
		return;
	}

	if (ck::IsValid(InParentAction))
	{
		const auto& ParentWSSource = InParentAction.Get<ck::FFragment_Goap_Planner_WorldStateSource>();
		const auto ParentWS = ParentWSSource.Get_Resolved();
		if (ck::IsValid(ParentWS))
		{
			ChildWSSource._Resolved = ParentWS;
			return;
		}
	}

	auto OwnerEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InChild);
	if (OwnerEntity.Has<ck::FFragment_Goap_Planner_WorldStateSource>())
	{
		const auto& OwnerWS = OwnerEntity.Get<ck::FFragment_Goap_Planner_WorldStateSource>();
		if (ck::IsValid(OwnerWS.Get_Resolved()))
		{
			ChildWSSource._Resolved = OwnerWS.Get_Resolved();
			return;
		}
		if (ck::IsValid(OwnerWS.Get_WorldStateSource()))
		{
			ChildWSSource._Resolved = OwnerWS.Get_WorldStateSource();
		}
	}
}

auto
	UCk_Utils_Goap_Planner_UE::
	AddAction(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Fragment_Goap_ActionParamsData& InParams) -> FCk_Handle_Goap_Action
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in AddAction"))
	{ return {}; }

	CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_ActionClass()),
		TEXT("Invalid _ActionClass in AddAction (Planner [{}])"), InPlanner)
	{ return {}; }

	auto ActionEntity = ck::goap::internal_planner::DoCreateOrFindActionEntity(InPlanner, InParams);

	if (NOT ck::IsValid(ActionEntity))
	{ return {}; }

	// Discriminate Planner host shape: promoted mid-tier Planners carry the
	// Action role (and therefore a Tree fragment); top-level Planners don't.
	const auto IsPromotedMidTier = InPlanner.Has<ck::FFragment_Goap_Action_Tree>();

	auto& ChildTree = ActionEntity.Get<ck::FFragment_Goap_Action_Tree>();
	if (ck::IsValid(ChildTree.Get_ParentAction()))
	{
		// Already parented — DoCreateOrFindActionEntity returned an existing
		// entry. Keep existing edges intact.
		return ActionEntity;
	}

	if (IsPromotedMidTier)
	{
		// Promoted Planner host — the host IS an Action. Wire the new child
		// as its direct tree child.
		auto HostAsAction = UCk_Utils_Goap_Action_UE::CastChecked(InPlanner);
		ChildTree._ParentAction = HostAsAction;

		auto& HostTree = InPlanner.Get<ck::FFragment_Goap_Action_Tree>();
		HostTree._ChildActions.AddUnique(ActionEntity);

		ck::goap::internal_planner::DoResolveChildWorldStateFromParent(ActionEntity, HostAsAction);
		return ActionEntity;
	}

	// Top-level Planner: the host is not an Action. The child has no Action
	// parent (its _ParentAction stays invalid); the Planner's
	// ActionCatalogIndex IS the candidate set, populated by
	// DoCreateOrFindActionEntity. Resolve the child's WS against the
	// Planner's _Resolved (or override).
	{
		auto InvalidParent = FCk_Handle_Goap_Action{};
		ck::goap::internal_planner::DoResolveChildWorldStateFromParent(ActionEntity, InvalidParent);
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

	// PR-B.1b Stage 4: stamp the Planner-role discriminator fragments + the
	// rest of the Planner-role cluster the host now needs. After Stage 4,
	// DoCreateOrFindActionEntity stamps only the lean Action-role set
	// (_Definition, _Params, _Tree, _Current, _ActionClasses, _WorldStateSource);
	// every additional Planner-side fragment is added here via AddOrGet so the
	// host carries the full cluster regardless of whether DoCreateOrFindActionEntity
	// previously seeded any of them.
	InAction.Add<ck::FFragment_Goap_Planner_Params>(InParams);
	InAction.Add<ck::FFragment_Goap_Planner_Current>();
	InAction.Add<ck::FFragment_Goap_Planner_ActionCatalogIndex>();

	auto& Current = InAction.Get<ck::FFragment_Goap_Planner_Current>();
	Current._EnableToggle = InParams.Get_InitialToggle();

	// PR-B.1b Stage 4: PlanState / Goal / Activation no longer get stamped by
	// DoCreateOrFindActionEntity, so the promotion path must add them. _Goal
	// is initialized from InParams.Get_Goal() to honour the U11.1 invariant
	// that the Planner's authored goal is independent of the Action role's
	// effects.
	InAction.AddOrGet<ck::FFragment_Goap_Planner_PlanState>();
	InAction.AddOrGet<ck::FFragment_Goap_Planner_Activation>();

	{
		auto& GoalFrag = InAction.AddOrGet<ck::FFragment_Goap_Planner_Goal>();
		GoalFrag._GoalAuthored = InParams.Get_Goal();
		GoalFrag._Goal = {};
		GoalFrag._InvalidGoal = {};
	}

	// PR-A: if the promoted Planner explicitly supplies a WS source, stamp it
	// onto the WorldStateSource fragment. (Optional — promoted Planners may
	// leave it unset and inherit the parent's resolved WS at activation time.)
	if (ck::IsValid(InParams.Get_WorldStateSource()))
	{
		auto& WSFragment = InAction.Get<ck::FFragment_Goap_Planner_WorldStateSource>();
		WSFragment._WorldStateSource = InParams.Get_WorldStateSource();
	}

	// PR-B.1b Stage 2 — dual-stamp the Planner-role A*-pipeline cluster on the
	// promoted host. The host is already an Action and therefore already carries
	// the Action-side copies of these fragments (stamped by
	// DoCreateOrFindActionEntity). Because the Planner-side aliases resolve to
	// the same underlying types in this transitional model, we use AddOrGet to
	// keep the existing fragment instance — calling Add would duplicate-assert.
	// Stage 5 splits the aliases into first-class types; until then this is a
	// defensive guarantee that the host carries the cluster, regardless of
	// which entry point created it.
	//
	// Stage 3a — overwrite the existing AStarParams (seeded from this entity's
	// ActionParams when it was first created) with values from the new
	// PlannerParams. The two structs share defaults (50000us / 0.0f) so most
	// callers won't notice; explicit overrides at promotion time win.
	{
		auto& AStarParams = InAction.AddOrGet<ck::FFragment_AStar_Params>();
		AStarParams.Set_BudgetMicroseconds(InParams.Get_SearchBudgetMicroseconds());
		AStarParams.Set_CostThreshold(InParams.Get_CostThreshold());
	}
	InAction.AddOrGet<ck::FFragment_AStar_Debug>();

	InAction.AddOrGet<ck::FFragment_Goap_Planner_SearchState>();
	InAction.AddOrGet<ck::FFragment_Goap_Planner_Result>();
	InAction.AddOrGet<ck::FFragment_Goap_Planner_PlanContext>();
	InAction.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	InAction.AddOrGet<ck::FFragment_Goap_Planner_ReplanThrottle>();

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
	Request_ResetActiveChain(FCk_Handle_Goap_Planner& InPlanner) -> FCk_Handle_Goap_Planner
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid ActionSet handle in Request_ResetActiveChain"))
	{ return InPlanner; }

	// PR-B.1b Stage 5: walk the entire active chain (Plan[0]-derived) and
	// deactivate every entry. There's no implicit root to skip — the chain
	// starts at Plan[0] of the Planner. DoDeactivatePlanner handles WS
	// unsubscribe, live-state reset, _IsActive flip, OnPlannerDeactivated
	// broadcast, and recursive descendant teardown.
	const auto Chain = Get_ActiveChain(InPlanner);
	if (Chain.Num() == 0) { return InPlanner; }

	// Deactivate in reverse order (leaf → outermost).
	for (auto i = Chain.Num() - 1; i >= 0; --i)
	{
		auto Action = Chain[i];
		if (NOT ck::IsValid(Action)) { continue; }
		ck::FProcessor_Goap_Planner_UpdateActivation::DoDeactivatePlanner(Action);
	}

	// The Planner's cached _LastActivatedPlan0 still points at the old Chain[0];
	// clear it so the next UpdateActivation tick sees this as a fresh state
	// transition (rather than no-op'ing because OldStep0 == NewStep0).
	{
		auto& PlannerActivation = InPlanner.Get<ck::FFragment_Goap_Planner_Activation>();
		PlannerActivation._LastActivatedPlan0 = {};
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

	// PR-B.1b Stage 3: Request_SetGoal enqueues on the Planner's own request
	// queue. The Planner-on-Planner HandleRequests resolves the goal against
	// the Planner's own WS source.
	auto& Requests = InPlanner.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Requests._Requests.Add(FCk_Request_Goap_Planner_SetGoal{InGoal});
	return InPlanner;
}

// --------------------------------------------------------------------------------------------------------------------
// PR-B.1b Stage 3: Planner-API request verbs enqueue on the Planner's own
// request queue. The Planner-on-Planner HandleRequests drains them.
// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_Goap_Planner_UE::
	Request_Plan(FCk_Handle_Goap_Planner& InPlanner) -> FCk_Handle_Goap_Planner
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Request_Plan"))
	{ return InPlanner; }

	auto& Reqs = InPlanner.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Planner_Plan{});
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Request_CancelPlan(FCk_Handle_Goap_Planner& InPlanner) -> FCk_Handle_Goap_Planner
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Request_CancelPlan"))
	{ return InPlanner; }

	auto& Reqs = InPlanner.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Planner_CancelPlan{});
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Request_SetReplanInterval(
		FCk_Handle_Goap_Planner& InPlanner,
		float InSeconds) -> FCk_Handle_Goap_Planner
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Request_SetReplanInterval"))
	{ return InPlanner; }

	auto& Reqs = InPlanner.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Planner_SetReplanInterval{InSeconds});
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Request_SetReplanPolicy(
		FCk_Handle_Goap_Planner& InPlanner,
		ECk_Goap_ReplanPolicy InPolicy) -> FCk_Handle_Goap_Planner
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Request_SetReplanPolicy"))
	{ return InPlanner; }

	auto& Reqs = InPlanner.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Planner_SetReplanPolicy{InPolicy});
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Request_SetSearchBudget(
		FCk_Handle_Goap_Planner& InPlanner,
		int64 InMicroseconds) -> FCk_Handle_Goap_Planner
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Request_SetSearchBudget"))
	{ return InPlanner; }

	auto& Reqs = InPlanner.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Planner_SetSearchBudget{InMicroseconds});
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Request_SetCostThreshold(
		FCk_Handle_Goap_Planner& InPlanner,
		float InThreshold) -> FCk_Handle_Goap_Planner
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Request_SetCostThreshold"))
	{ return InPlanner; }

	auto& Reqs = InPlanner.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Planner_SetCostThreshold{InThreshold});
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Request_SetChildActionCost(
		FCk_Handle_Goap_Planner& InPlanner,
		TSubclassOf<UCk_GoapAction_EntityScript> InChildClass,
		float InCost) -> FCk_Handle_Goap_Planner
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Request_SetChildActionCost"))
	{ return InPlanner; }

	auto& Reqs = InPlanner.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Planner_SetActionCost{InChildClass, InCost});
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Request_RegisterActionCostProvider(
		FCk_Handle_Goap_Planner& InPlanner,
		TSubclassOf<UCk_GoapAction_EntityScript> InChildClass) -> FCk_Handle_Goap_Planner
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Request_RegisterActionCostProvider"))
	{ return InPlanner; }

	auto& Reqs = InPlanner.AddOrGet<ck::FFragment_Goap_Planner_Requests>();
	Reqs._Requests.Add(FCk_Request_Goap_Planner_RegisterActionCostProvider{InChildClass});
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	Request_RemoveAction(
		FCk_Handle_Goap_Planner& InPlanner,
		TSubclassOf<UCk_GoapAction_EntityScript> InChildActionClass) -> FCk_Handle_Goap_Planner
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in Request_RemoveAction"))
	{ return InPlanner; }

	CK_ENSURE_IF_NOT(ck::IsValid(InChildActionClass),
		TEXT("Invalid ChildActionClass in Request_RemoveAction (Planner [{}])"), InPlanner)
	{ return InPlanner; }

	// Catalog lookup (class → tag → action).
	const auto ActionTag = UCk_GoapAction_EntityScript::Get_ActionTagForClass(InChildActionClass);
	CK_ENSURE_IF_NOT(ActionTag.IsValid(),
		TEXT("Could not derive a valid action tag for class [{}] in Request_RemoveAction (Planner [{}])"),
		InChildActionClass, InPlanner)
	{ return InPlanner; }

	auto ChildAction = Find_Action(InPlanner, ActionTag);
	if (NOT ck::IsValid(ChildAction))
	{
		ck::goap::Warning(
			TEXT("Request_RemoveAction: no Action with tag [{}] (class [{}]) registered on Planner [{}]; nothing to remove."),
			ActionTag, InChildActionClass, InPlanner);
		return InPlanner;
	}

	// Resolve the parent Action whose _ChildActions list contains this child.
	// In the top-level case this is invalid (child has no parent Action — it's
	// a direct child of the Planner, which is not an Action); in the
	// promoted-mid-tier case this is the Planner host (cast to Action). Read
	// it from the child's own Tree fragment — single source of truth populated
	// by AddAction.
	auto ParentAction = FCk_Handle_Goap_Action{};
	{
		const auto& ChildTree = ChildAction.Get<ck::FFragment_Goap_Action_Tree>();
		ParentAction = ChildTree.Get_ParentAction();
	}

	// Remove the catalog entry. RemoveEntry is the public mutator counterpart
	// to AddEntry — friendship on the fragment is class-scoped and doesn't
	// reach namespace-level free functions, but Utils is a friend so we could
	// touch _TagToAction directly. The mutator is preferred for symmetry +
	// future-proofing (e.g. if removal grows side effects).
	{
		auto& Index = InPlanner.Get<ck::FFragment_Goap_Planner_ActionCatalogIndex>();
		Index.RemoveEntry(ActionTag);
	}

	// Detach the child from its parent's _ChildActions list. Utils is a
	// friend of FFragment_Goap_Action_Tree so direct access is allowed.
	if (ck::IsValid(ParentAction))
	{
		auto& ParentTree = ParentAction.Get<ck::FFragment_Goap_Action_Tree>();
		ParentTree._ChildActions.RemoveSingle(ChildAction);

		// If the removed child is currently the parent's Plan[0], the
		// activation cache (_LastActivatedPlan0) still points at it. Clear
		// the cache so the next UpdateActivation tick sees a fresh state
		// transition (mirrors Request_ResetActiveChain's root-clear).
		//
		// PR-B.1b Stage 4: gate with Has<>. The parent may be the implicit-
		// root Action of a top-level Planner — those are no longer stamped
		// with FFragment_Goap_Planner_Activation. Only promoted sub-Planner
		// hosts carry it (via PromoteActionToPlanner). The implicit-root
		// branch never set _LastActivatedPlan0 anyway, so skipping the read
		// preserves behaviour.
		if (ParentAction.Has<ck::FFragment_Goap_Planner_Activation>())
		{
			auto& ParentActivation = ParentAction.Get<ck::FFragment_Goap_Planner_Activation>();
			if (ParentActivation.Get_LastActivatedPlan0() == ChildAction)
			{
				ParentActivation._LastActivatedPlan0 = {};
			}
		}
	}

	// Catalog mutated → re-run setup (cycle detection / dependency rebuild),
	// then enqueue a replan so the next plan reflects the updated operator
	// set. Mirrors AddAction's setup marker.
	InPlanner.AddOrGet<ck::FTag_Goap_Planner_RequiresSetup>();

	// Destroy the Action entity. Standard entity-lifetime path handles record
	// cleanup (owner-cascade-destroy removes the FRecordOfGoapActions entry)
	// and any descendant cascade (a dual-role child with its own _ChildActions
	// gets its sub-tree cascade-destroyed via the standard owner chain — child
	// Actions were created with InPlanner as their lifetime owner, but
	// recursive Actions of a removed mid-tier composite are owned through the
	// Action entity itself in the tree-edge sense; the standard cascade walks
	// owning-entity relationships rather than tree edges, so isolated dual-role
	// catalog entries are addressed by their own catalog-removal calls).
	{
		auto ChildAsGeneric = static_cast<FCk_Handle>(ChildAction);
		UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(ChildAsGeneric);
	}

	// Trigger the replan after destruction is queued. Routes through the
	// existing Request_Plan shim, which enqueues on the implicit-root
	// Action's request queue (the entity running A*).
	Request_Plan(InPlanner);

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

// --------------------------------------------------------------------------------------------------------------------
// PR-B.1b Stage 5: per-Planner signals.
//
// PlanComplete / PlanFailed broadcast on the Planner entity directly
// (FProcessor_Goap_Planner_HandleRequests and _HandleResult).
//
// PlannerActivated / Deactivated broadcast on the Action entity for promoted
// mid-tier Planners (sub-Planners ARE Actions). Top-level Planners don't fire
// Activate/Deactivate (they're always active). The bind utilities resolve
// Planner-cast-to-Action when the entity carries the Action role; otherwise
// they bind on the Planner directly (top-level — never fires but harmless).
// --------------------------------------------------------------------------------------------------------------------

namespace ck::goap::internal_planner
{
	// Resolve a Planner handle to the entity that broadcasts activation
	// signals. Promoted mid-tier Planner-Actions broadcast on themselves;
	// top-level Planners don't fire activation signals — bind on the Planner
	// itself (harmless — never fires).
	static auto DoResolveBroadcastEntity_ForActivation(const FCk_Handle_Goap_Planner& InPlanner) -> FCk_Handle
	{
		if (NOT ck::IsValid(InPlanner)) { return {}; }

		if (InPlanner.Has<ck::FFragment_Goap_Action_Tree>())
		{
			return static_cast<FCk_Handle>(UCk_Utils_Goap_Action_UE::CastChecked(InPlanner));
		}

		return static_cast<FCk_Handle>(InPlanner);
	}
}

auto
	UCk_Utils_Goap_Planner_UE::
	BindTo_OnPlanComplete(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnPlanComplete& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior) -> FCk_Handle_Goap_Planner
{
	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoap_Planner_PlanComplete,
		InPlanner, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	UnbindFrom_OnPlanComplete(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnPlanComplete& InDelegate) -> FCk_Handle_Goap_Planner
{
	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoap_Planner_PlanComplete, InPlanner, InDelegate);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	BindTo_OnPlanFailed(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnPlanFailed& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior) -> FCk_Handle_Goap_Planner
{
	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoap_Planner_PlanFailed,
		InPlanner, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	UnbindFrom_OnPlanFailed(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnPlanFailed& InDelegate) -> FCk_Handle_Goap_Planner
{
	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoap_Planner_PlanFailed, InPlanner, InDelegate);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	BindTo_OnPlannerActivated(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnPlannerActivated& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior) -> FCk_Handle_Goap_Planner
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in BindTo_OnPlannerActivated"))
	{ return InPlanner; }

	auto BroadcastEntity = ck::goap::internal_planner::DoResolveBroadcastEntity_ForActivation(InPlanner);
	CK_ENSURE_IF_NOT(ck::IsValid(BroadcastEntity),
		TEXT("Planner [{}] has no activation-broadcast entity. Bind ignored."),
		InPlanner)
	{ return InPlanner; }

	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoap_Planner_Activated,
		BroadcastEntity, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	UnbindFrom_OnPlannerActivated(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnPlannerActivated& InDelegate) -> FCk_Handle_Goap_Planner
{
	if (NOT ck::IsValid(InPlanner)) { return InPlanner; }

	auto BroadcastEntity = ck::goap::internal_planner::DoResolveBroadcastEntity_ForActivation(InPlanner);
	if (NOT ck::IsValid(BroadcastEntity)) { return InPlanner; }

	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoap_Planner_Activated, BroadcastEntity, InDelegate);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	BindTo_OnPlannerDeactivated(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnPlannerDeactivated& InDelegate,
		ECk_Signal_BindingPolicy InBindingPolicy,
		ECk_Signal_PostFireBehavior InPostFireBehavior) -> FCk_Handle_Goap_Planner
{
	CK_ENSURE_IF_NOT(ck::IsValid(InPlanner),
		TEXT("Invalid Planner handle in BindTo_OnPlannerDeactivated"))
	{ return InPlanner; }

	auto BroadcastEntity = ck::goap::internal_planner::DoResolveBroadcastEntity_ForActivation(InPlanner);
	CK_ENSURE_IF_NOT(ck::IsValid(BroadcastEntity),
		TEXT("Planner [{}] has no activation-broadcast entity. Bind ignored."),
		InPlanner)
	{ return InPlanner; }

	CK_SIGNAL_BIND(ck::UUtils_Signal_OnGoap_Planner_Deactivated,
		BroadcastEntity, InDelegate, InBindingPolicy, InPostFireBehavior);
	return InPlanner;
}

auto
	UCk_Utils_Goap_Planner_UE::
	UnbindFrom_OnPlannerDeactivated(
		FCk_Handle_Goap_Planner& InPlanner,
		const FCk_Delegate_Goap_OnPlannerDeactivated& InDelegate) -> FCk_Handle_Goap_Planner
{
	if (NOT ck::IsValid(InPlanner)) { return InPlanner; }

	auto BroadcastEntity = ck::goap::internal_planner::DoResolveBroadcastEntity_ForActivation(InPlanner);
	if (NOT ck::IsValid(BroadcastEntity)) { return InPlanner; }

	CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnGoap_Planner_Deactivated, BroadcastEntity, InDelegate);
	return InPlanner;
}
