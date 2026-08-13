#include "CkGoap/Action/CkGoap_Action_Processor.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/CkGoap_Stats.h"
#include "CkGoap/CkGoap_Fragment.h"  // dirty tags FTag_Goap_Dirty_WorldState / _Cost
#include "CkGoap/Action/CkGoap_Action_Utils.h"  // CastChecked for Planner-as-Action
#include "CkGoap/EntityScripts/CkGoapAction_EntityScript.h"
#include "CkGoap/Planner/CkGoap_Planner_Utils.h"  // resolve owning Planner for signal payload

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

#include "Algo/Reverse.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Action_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Planner_AutoReplan);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Planner_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Planner_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Planner_Execute);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Planner_HandleResult);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Planner_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("GoapAction::Setup"), STAT_Goap_Action_Setup, STATGROUP_CkGoap);
DECLARE_CYCLE_STAT(TEXT("GoapPlanner::AutoReplan"), STAT_Goap_Planner_AutoReplan, STATGROUP_CkGoap);
DECLARE_CYCLE_STAT(TEXT("GoapPlanner::HandleRequests"), STAT_Goap_HandleRequestsProc, STATGROUP_CkGoap);
DECLARE_CYCLE_STAT(TEXT("GoapPlanner::HandleResult"), STAT_Goap_Planner_HandleResult, STATGROUP_CkGoap);

DECLARE_CYCLE_STAT(TEXT("Goap::BuildGraphAndSeed"), STAT_Goap_BuildGraphAndSeed, STATGROUP_CkGoap);
DECLARE_DWORD_COUNTER_STAT(TEXT("Goap Replans Requested"), STAT_Goap_ReplansRequested, STATGROUP_CkGoap);
DECLARE_DWORD_COUNTER_STAT(TEXT("Goap Plans Completed"), STAT_Goap_PlansCompleted, STATGROUP_CkGoap);
DECLARE_DWORD_COUNTER_STAT(TEXT("Goap Plans Failed"), STAT_Goap_PlansFailed, STATGROUP_CkGoap);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{

// --------------------------------------------------------------------------------------------------------------------

namespace ck_goap_action_processor
{
	auto ResolveCondition(const goap::FKeyRegistry& InRegistry, const goap::FWorldStateCondition_Raw& InRaw)
		-> goap::FWorldStateCondition
	{
		const auto Key = InRegistry.Find(InRaw.Key);
		if (Key == goap::InvalidGoapKey) { return {}; }
		return goap::FWorldStateCondition{Key, InRaw.Value};
	}

	auto ResolveEffect(const goap::FKeyRegistry& InRegistry, const goap::FWorldStateEffect_Raw& InRaw)
		-> goap::FWorldStateEffect
	{
		const auto Key = InRegistry.Find(InRaw.Key);
		if (Key == goap::InvalidGoapKey) { return {}; }
		return goap::FWorldStateEffect{Key, InRaw.Value};
	}

	auto BuildConstraintSet(const TArray<goap::FWorldStateCondition>& InConditions)
		-> goap::FConstraintSet
	{
		auto Set = goap::FConstraintSet{};
		for (const auto& C : InConditions)
		{
			if (C.IsValid()) { Set.Add(C); }
		}
		return Set;
	}

	// A promoted mid-tier Planner IS the Action that owns the Tree, so it reads
	// its children from there; a top-level Planner reads its ActionCatalogIndex.
	auto Goap_Planner_GetCandidateChildren(const FCk_Handle_Goap_Planner& InPlanner)
		-> TArray<FCk_Handle_Goap_Action>
	{
		if (NOT ck::IsValid(InPlanner)) { return {}; }

		if (InPlanner.template Has<FFragment_Goap_Action_Tree>())
		{
			return InPlanner.template Get<FFragment_Goap_Action_Tree>().Get_ChildActions();
		}

		auto Result = TArray<FCk_Handle_Goap_Action>{};
		if (NOT InPlanner.template Has<FFragment_Goap_Planner_ActionCatalogIndex>()) { return Result; }
		const auto& Index = InPlanner.template Get<FFragment_Goap_Planner_ActionCatalogIndex>();
		Result.Reserve(Index.Get_TagToAction().Num());
		for (const auto& Pair : Index.Get_TagToAction())
		{
			if (ck::IsValid(Pair.Value)) { Result.Add(Pair.Value); }
		}
		return Result;
	}

	// For parent-plan gating: the Planner whose A* search picks us as a candidate.
	// Top-level Planners have no Action role, hence no parent, and are never gated.
	auto Goap_Planner_GetParentPlanner(const FCk_Handle_Goap_Planner& InPlanner)
		-> FCk_Handle_Goap_Planner
	{
		if (NOT ck::IsValid(InPlanner)) { return {}; }
		if (NOT InPlanner.template Has<FFragment_Goap_Action_Tree>()) { return {}; }

		const auto& Tree = InPlanner.template Get<FFragment_Goap_Action_Tree>();
		auto Walker = static_cast<FCk_Handle>(Tree.Get_ParentAction());
		constexpr auto MaxDepth = 64;
		for (auto Depth = 0; Depth < MaxDepth; ++Depth)
		{
			if (NOT ck::IsValid(Walker)) { break; }
			if (UCk_Utils_Goap_Planner_UE::Has(Walker))
			{
				return UCk_Utils_Goap_Planner_UE::CastChecked(Walker);
			}
			Walker = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(Walker);
		}
		return {};
	}
}

// --------------------------------------------------------------------------------------------------------------------

auto
	FProcessor_Goap_Action_Setup::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Action_Params& InParams,
		FFragment_Goap_Action_Definition& InActionDef,
		FFragment_Goap_Planner_WorldStateSource& InWSSource) -> void
{
	SCOPE_CYCLE_COUNTER(STAT_Goap_Action_Setup);

	// Defer to the next frame WITHOUT removing the setup tag: the WS source is
	// resolved at AddAction time (top-level) or at activation time (non-root).
	const auto Source = InWSSource.Get_Resolved();
	if (NOT ck::IsValid(Source))
	{
		return;
	}

	InHandle.Remove<FTag_Goap_Action_RequiresSetup>();

	auto& SourceRegistry = const_cast<FCk_Handle_Goap_WorldState&>(Source)
		.template Get<FFragment_Goap_WorldState_KeyRegistry>().Get_MutableRegistry();

	const auto ActionClass = InParams.Get_ActionClass();
	if (ck::IsValid(ActionClass))
	{
		if (auto* CDO = ActionClass.GetDefaultObject(); ck::IsValid(CDO))
		{
			CDO->DefineAction();

			InActionDef._Preconditions = CDO->_Preconditions;
			InActionDef._Effects       = CDO->_Effects;
			InActionDef._Cost          = CDO->_Cost;

			for (const auto& Pre : InActionDef._Preconditions) { SourceRegistry.FindOrRegister(Pre.Key); }
			for (const auto& Eff : InActionDef._Effects)       { SourceRegistry.FindOrRegister(Eff.Key); }
		}
	}

	// Goal keys are deliberately NOT auto-registered here: a goal key absent from
	// every Action's preconditions/effects is a diagnostic (Get_InvalidGoal), and
	// registering it would let the planner search toward a key it cannot affect.

	if (SourceRegistry.Num() >= goap::WorldState_MaxKeys)
	{
		ck::goap::Warning(
			TEXT("GOAP WorldState [{}] (resolved source of Action [{}]) is at key capacity ({}/{}). Further keys will be silently rejected."),
			Source, InHandle, SourceRegistry.Num(), goap::WorldState_MaxKeys);
	}

	{
		auto& Def = InActionDef._CachedActionDef;
		Def = goap::FActionDef{};
		Def.ActionClass = ActionClass;
		Def.Cost = InActionDef._Cost;

		// Action_Tag identity (debug only — chain matching uses entity handles).
		if (ck::IsValid(ActionClass))
		{
			Def.ActionTag = UCk_GoapAction_EntityScript::Get_ActionTagForClass(ActionClass);
		}

		Def.Preconditions.Reserve(InActionDef._Preconditions.Num());
		for (const auto& Pre : InActionDef._Preconditions)
		{
			const auto Resolved = ck_goap_action_processor::ResolveCondition(SourceRegistry, Pre);
			if (Resolved.IsValid()) { Def.Preconditions.Add(Resolved); }
		}

		Def.Effects.Reserve(InActionDef._Effects.Num());
		for (const auto& Eff : InActionDef._Effects)
		{
			const auto Resolved = ck_goap_action_processor::ResolveEffect(SourceRegistry, Eff);
			if (Resolved.IsValid()) { Def.Effects.Add(Resolved); }
		}
	}

	// Stored in authored (tag) form: ChainUpdate resolves it against the WS
	// registry of the *activating* Action, which may differ from this one's.
	InActionDef._GoalFromEffects.Reset();
	InActionDef._GoalFromEffects.Reserve(InActionDef._Effects.Num());
	for (const auto& Eff : InActionDef._Effects)
	{
		InActionDef._GoalFromEffects.Add(FCk_GoapWS_Condition_Authored{Eff.Key, Eff.Value});
	}

	// Normally empty: the FindOrRegister loops above registered every effect key.
	// It only fills once the registry overflows WorldState_MaxKeys — FindOrRegister
	// then silently rejects, and Find returns InvalidGoapKey.
	InActionDef._InvalidGoal.Reset();
	for (const auto& Eff : InActionDef._Effects)
	{
		if (SourceRegistry.Find(Eff.Key) == goap::InvalidGoapKey)
		{
			InActionDef._InvalidGoal.Add(FCk_GoapWS_Condition_Authored{Eff.Key, Eff.Value});
		}
	}

	if (InActionDef._InvalidGoal.Num() > 0)
	{
		ck::goap::Verbose(
			TEXT("Action [{}] has [{}] effect key(s) not in resolved WS registry."),
			InHandle, InActionDef._InvalidGoal.Num());
	}
}

// --------------------------------------------------------------------------------------------------------------------

auto
	FProcessor_Goap_Planner_AutoReplan::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Planner_Params& InParams,
		const FFragment_Goap_Planner_Current& InCurrent,
		const FFragment_Goap_Planner_WorldStateSource& InWSSource,
		FFragment_Goap_Planner_ReplanThrottle& InThrottle) -> void
{
	SCOPE_CYCLE_COUNTER(STAT_Goap_Planner_AutoReplan);

	// Dirty / initial-plan tags stay set while disabled, so a re-enable resumes
	// from the deferred state.
	if (InCurrent.Get_EnableToggle() == ECk_EnableDisable::Disable) { return; }

	// Planner-side Setup (cycle detection + goal resolution) must land first.
	if (InHandle.Has<FTag_Goap_Planner_RequiresSetup>()) { return; }

	// A promoted mid-tier Planner has no _Resolved until its activation walk runs;
	// without this gate a premature Plan request fires a spurious PlanFailed.
	if (NOT ck::IsValid(InWSSource.Get_Resolved())) { return; }

	InThrottle._SecondsSinceLastReplan += InDeltaT.Get_Seconds();

	const auto IsInitialPlanPending = InHandle.Has<FTag_Goap_Planner_RequiresInitialPlan>();
	const auto WSDirty   = InHandle.Has<FTag_Goap_Dirty_WorldState>();
	const auto CostDirty = InHandle.Has<FTag_Goap_Dirty_Cost>();

	const auto PolicyAllowsReplan = [&]() -> bool
	{
		switch (InParams.Get_ReplanPolicy())
		{
			case ECk_Goap_ReplanPolicy::OnWorldStateDirty: return WSDirty;
			case ECk_Goap_ReplanPolicy::OnCostDirty:       return CostDirty;
			case ECk_Goap_ReplanPolicy::OnEitherDirty:     return WSDirty || CostDirty;
			case ECk_Goap_ReplanPolicy::Explicit:          return false;
		}
		return false;
	}();

	const auto ThrottleElapsed = InThrottle._SecondsSinceLastReplan
		>= InParams.Get_MinReplanIntervalSeconds();

	const auto ShouldFire = IsInitialPlanPending || (PolicyAllowsReplan && ThrottleElapsed);
	if (NOT ShouldFire) { return; }

	// Replan frequency across all Planners — #1 scaling risk (default replan throttle is 0).
	INC_DWORD_STAT(STAT_Goap_ReplansRequested);

	const auto Origin = [&]
	{
		if (IsInitialPlanPending)  { return ECk_Goap_ReplanOrigin::PlanOnStart; }
		if (WSDirty && CostDirty)  { return ECk_Goap_ReplanOrigin::WorldStateAndCostDirty; }
		if (CostDirty)             { return ECk_Goap_ReplanOrigin::CostDirty; }
		return ECk_Goap_ReplanOrigin::WorldStateDirty;
	}();

	auto& Requests = InHandle.AddOrGet<FFragment_Goap_Planner_Requests>();
	Requests._Requests.Add(FCk_Request_Goap_Planner_Plan{}.Set_Origin(Origin));

	InThrottle._SecondsSinceLastReplan = 0.0f;
	InHandle.Try_Remove<FTag_Goap_Dirty_WorldState>();
	InHandle.Try_Remove<FTag_Goap_Dirty_Cost>();
	InHandle.Try_Remove<FTag_Goap_Planner_RequiresInitialPlan>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
	FProcessor_Goap_Planner_HandleRequests::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Planner_Params& InParams,
		const FFragment_Goap_Planner_Current& InCurrent,
		FFragment_AStar_Params& InAStarParams,
		FFragment_Goap_Planner_PlanState& InPlanState,
		FFragment_Goap_Planner_Goal& InGoal,
		FFragment_Goap_Planner_WorldStateSource& InWSSource,
		const FFragment_Goap_Planner_Requests& InRequests,
		FFragment_Goap_Planner_SearchState& InSearchState,
		FFragment_Goap_Planner_Result& InResult,
		FFragment_Goap_Planner_PlanContext& InPlanContext) const -> void
{
	SCOPE_CYCLE_COUNTER(STAT_Goap_HandleRequestsProc);

	(void)InParams;
	if (InCurrent.Get_EnableToggle() == ECk_EnableDisable::Disable) { return; }

	const auto IsParentPlanInFlight = [&]() -> bool
	{
		const auto Parent = ck_goap_action_processor::Goap_Planner_GetParentPlanner(InHandle);
		if (NOT ck::IsValid(Parent)) { return false; }

		if (Parent.template Has<FTag_Goap_Planner_PlanInFlight>()) { return true; }

		const auto& ParentPlanState = Parent.template Get<FFragment_Goap_Planner_PlanState>();
		switch (ParentPlanState.Get_PlanStatus())
		{
			case ECk_GoapPlanStatus::Planning:
				return true;
			case ECk_GoapPlanStatus::Idle:
				// Idle = the parent has not planned YET; only defer while its initial
				// plan is still pending, otherwise it never intends to plan at all.
				return Parent.template Has<FTag_Goap_Planner_RequiresInitialPlan>();
			default:
				return false;
		}
	}();

	InHandle.CopyAndRemove(InRequests, [&](FFragment_Goap_Planner_Requests& InRequestsCopy)
	{
		algo::ForEachRequest(InRequestsCopy._Requests, ck::Visitor([&](const auto& InTypedRequest)
		{
			using T = std::decay_t<decltype(InTypedRequest)>;

			// None of the branches below reject a request — even the "parent plan in flight" defer
			// and the WS-unresolved PlanFailed path are the Planner engaging with and dispatching
			// the request, not refusing it (a game-content Planner must always reach PlanFound/
			// PlanFailed via its fallback Action — see CkGoap CLAUDE.md "Every Planner must always
			// produce a valid plan"). Several branches below return early, so Result is set up front
			// rather than after the if-constexpr chain.
			auto Result = ECk_Request_OperationResult::Succeeded;
			const auto Guard = MakeCompletionGuard(InTypedRequest, InHandle, Result);

			if constexpr (std::is_same_v<T, FCk_Request_Goap_Planner_Plan>)
			{
				if (IsParentPlanInFlight)
				{
					// Re-arm the tag instead of re-queueing the request:
					// MarkedDirtyBy = FFragment_Goap_Planner_Requests, so rewriting the
					// fragment re-runs this processor until the pump limit fires.
					InHandle.AddOrGet<FTag_Goap_Planner_RequiresInitialPlan>();
					return;
				}

				InHandle.Try_Remove<FTag_AStar_SearchActive>();
				InHandle.Try_Remove<FTag_AStar_SearchComplete>();

				++InPlanState._PlanAttemptCount;

				// Replan-cause record — the debugger's coalescing evidence.
				{
					auto& Cause = InHandle.AddOrGet<FFragment_Goap_Planner_ReplanCause>();
					auto NewInfo = FCk_Goap_ReplanCauseInfo{};
					NewInfo._Origin = InTypedRequest.Get_Origin();
					NewInfo._AttemptNumber = InPlanState.Get_PlanAttemptCount();
					NewInfo._FrameNumber = static_cast<int64>(GFrameCounter);

					if (const auto CauseWS = InWSSource.Get_Resolved(); ck::IsValid(CauseWS))
					{
						if (CauseWS.Has<FFragment_Goap_WorldState_ChangeLog>())
						{
							const auto& ChangeLog = CauseWS.Get<FFragment_Goap_WorldState_ChangeLog>();
							for (const auto& Change : ChangeLog.Get_Entries())
							{
								if (Change.Get_FrameNumber() > Cause._LastReplanFrame)
								{ NewInfo._ChangedKeys.Add(Change); }
							}
						}
					}

					Cause._Info = MoveTemp(NewInfo);
					Cause._LastReplanFrame = static_cast<int64>(GFrameCounter);
				}

				const auto Source = InWSSource.Get_Resolved();
				if (NOT ck::IsValid(Source))
				{
					InPlanState._PlanStatus = ECk_GoapPlanStatus::PlanFailed;
					InPlanState._Plan.Reset();
					InPlanState._PlanCost = 0.0f;
					InHandle.Try_Remove<FTag_Goap_Planner_PlanInFlight>();

					CK_ENSURE_IF_NOT(InCurrent.Get_HasUnconditionalFallback() || InParams.Get_AllowPlanFailed(),
						TEXT("Planner [{}] (tag [{}]) reached PlanFailed at HandleRequests because the "
							 "resolved WorldStateSource is invalid, and the catalog has no unconditional "
							 "fallback Action AND _AllowPlanFailed=false. Set "
							 "PlannerParams._WorldStateSource (top-level) or verify the activation walk "
							 "resolved it (promoted), and add a fallback Action so PlanFailed is "
							 "structurally impossible."),
						InHandle, InParams.Get_PlannerTag())
					{ /* still proceed — broadcast and let consumers react */ }

					INC_DWORD_STAT(STAT_Goap_PlansFailed);
					UUtils_Signal_OnGoap_Planner_PlanFailed::Broadcast(
						InHandle, ck::MakePayload(InHandle, FCk_Goap_Payload_OnPlanFailed{}));
					return;
				}

				// Flatten the override stack (bottom-to-top, top wins) into a local
				// snapshot so the A* inner loop never walks the stack.
				auto SourceWorldState = const_cast<FCk_Handle_Goap_WorldState&>(Source)
					.template Get<FFragment_Goap_WorldState_Values>().Get_Values();

				if (const_cast<FCk_Handle_Goap_WorldState&>(Source)
					.template Has<FFragment_Goap_WorldState_OverrideStack>())
				{
					const auto& Registry = const_cast<FCk_Handle_Goap_WorldState&>(Source)
						.template Get<FFragment_Goap_WorldState_KeyRegistry>().Get_Registry();
					const auto& Stack = const_cast<FCk_Handle_Goap_WorldState&>(Source)
						.template Get<FFragment_Goap_WorldState_OverrideStack>();

					for (const auto& Layer : Stack.Get_Layers())
					{
						for (const auto& Kv : Layer.Values)
						{
							const auto FlatKey = Registry.Find(Kv.Key);
							if (FlatKey == goap::InvalidGoapKey) { continue; }
							SourceWorldState.Set(FlatKey, Kv.Value);
						}
					}
				}

				const auto IsGoalSatisfied = [&]() -> bool
				{
					for (const auto& C : InGoal._Goal)
					{
						if (NOT SourceWorldState.Satisfies(C)) { return false; }
					}
					return true;
				}();

				if (InGoal._Goal.IsEmpty() || IsGoalSatisfied)
				{
					InPlanState._PlanStatus = ECk_GoapPlanStatus::PlanFound;
					InPlanState._Plan.Reset();
					InPlanState._PlanCost = 0.0f;
					InHandle.Try_Remove<FTag_Goap_Planner_PlanInFlight>();
					INC_DWORD_STAT(STAT_Goap_PlansCompleted);
					UUtils_Signal_OnGoap_Planner_PlanComplete::Broadcast(
						InHandle, ck::MakePayload(InHandle, FCk_Goap_Payload_OnPlanComplete{
							TArray<TSubclassOf<UCk_GoapAction_EntityScript>>{}, 0.0f}));
					return;
				}

				InPlanState._PlanStatus = ECk_GoapPlanStatus::Planning;
				InPlanState._Plan.Reset();
				InPlanState._PlanCost = 0.0f;

				// Gates child Planners until our terminal status; removed by
				// HandleResult / CancelPlan / the early-out paths above.
				InHandle.AddOrGet<FTag_Goap_Planner_PlanInFlight>();

				SCOPE_CYCLE_COUNTER(STAT_Goap_BuildGraphAndSeed);

				// Each candidate's ActionIndex MUST match its position in Candidates:
				// FGoapGraph stores it in EdgeActions and HandleResult maps path edges
				// back through it. INDEX_NONE silently yields PlanFound + an empty plan.
				const auto ChildHandles = ck_goap_action_processor::Goap_Planner_GetCandidateChildren(InHandle);
				auto Candidates = TArray<goap::FActionDef>{};
				Candidates.Reserve(ChildHandles.Num());
				for (const auto& ChildHandle : ChildHandles)
				{
					if (NOT ck::IsValid(ChildHandle)) { continue; }
					const auto& ChildDef = ChildHandle.template Get<FFragment_Goap_Action_Definition>();
					auto Candidate = ChildDef.AsActionDef();
					Candidate.ActionIndex = Candidates.Num();
					Candidates.Add(MoveTemp(Candidate));
				}

				const auto GoalConditions = ck_goap_action_processor::BuildConstraintSet(InGoal._Goal);
				auto Graph = goap::FGoapGraph{SourceWorldState, Candidates, GoalConditions};
				InPlanContext._Graph = Graph;

				constexpr auto GoalSentinel = TNumericLimits<int32>::Max();
				InSearchState._State = astar::TSearchState<int32, goap::FGoapGraph>{
					MoveTemp(Graph), 0, GoalSentinel};

				InResult._Path.Reset();
				InResult._TotalCost = 0.0f;
				InResult._SearchStatus = ECk_AStarSearchStatus::InProgress;
				InResult._TotalIterations = 0;
				InResult._TotalTimeMicroseconds = 0;

				InHandle.Add<FTag_AStar_SearchActive>();
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Planner_CancelPlan>)
			{
				InHandle.Try_Remove<FTag_AStar_SearchActive>();
				InHandle.Try_Remove<FTag_AStar_SearchComplete>();
				InHandle.Try_Remove<FTag_Goap_Planner_PlanInFlight>();
				InSearchState._State = {};
				InPlanState._PlanStatus = ECk_GoapPlanStatus::Idle;
				InPlanState._Plan.Reset();
				InPlanState._PlanCost = 0.0f;
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Planner_SetGoal>)
			{
				const auto NewAuthored = InTypedRequest.Get_NewGoal();

				InGoal._GoalAuthored = NewAuthored;
				InGoal._Goal.Reset();
				InGoal._InvalidGoal.Reset();

				const auto Source = InWSSource.Get_Resolved();
				if (NOT ck::IsValid(Source))
				{
					for (const auto& Cond : NewAuthored)
					{
						InGoal._InvalidGoal.Add(Cond);
					}
					return;
				}

				// Use Find (read-only) — goal keys that aren't in the WS registry
				// are diagnostics (Get_InvalidGoal) rather than silent additions.
				const auto& Registry = const_cast<FCk_Handle_Goap_WorldState&>(Source)
					.template Get<FFragment_Goap_WorldState_KeyRegistry>().Get_Registry();
				for (const auto& Cond : NewAuthored)
				{
					const auto Key = Registry.Find(Cond.Get_Key());
					if (Key == goap::InvalidGoapKey)
					{
						InGoal._InvalidGoal.Add(Cond);
					}
					else
					{
						InGoal._Goal.Add(goap::FWorldStateCondition{Key, Cond.Get_Value()});
					}
				}

				InHandle.AddOrGet<FTag_Goap_Planner_RequiresInitialPlan>();
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Planner_SetActionCost>)
			{
				// _CachedActionDef.Cost is the mirror the A* search actually reads.
				const auto TargetClass = InTypedRequest.Get_ActionClass();
				const auto NewCost = InTypedRequest.Get_Cost();

				const auto Candidates = ck_goap_action_processor::Goap_Planner_GetCandidateChildren(InHandle);
				for (auto ChildHandle : Candidates)
				{
					if (NOT ck::IsValid(ChildHandle)) { continue; }

					const auto& ChildParams = ChildHandle.template Get<FFragment_Goap_Action_Params>();
					if (ChildParams.Get_ActionClass() != TargetClass) { continue; }

					auto& ChildDef = ChildHandle.template Get<FFragment_Goap_Action_Definition>();
					ChildDef._Cost = NewCost;
					ChildDef._CachedActionDef.Cost = NewCost;
					break;
				}

				InHandle.AddOrGet<FTag_Goap_Dirty_Cost>();
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Planner_RegisterActionCostProvider>)
			{
				const auto TargetClass = InTypedRequest.Get_ActionClass();

				const auto Candidates = ck_goap_action_processor::Goap_Planner_GetCandidateChildren(InHandle);
				for (auto ChildHandle : Candidates)
				{
					if (NOT ck::IsValid(ChildHandle)) { continue; }

					const auto& ChildParams = ChildHandle.template Get<FFragment_Goap_Action_Params>();
					if (ChildParams.Get_ActionClass() != TargetClass) { continue; }

					ChildHandle.template AddOrGet<FTag_Goap_Action_HasCostProvider>();
					break;
				}
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Planner_SetReplanInterval>)
			{
				ck::goap::Warning(
					TEXT("Request_SetReplanInterval not yet wired for runtime tuning — set in PlannerParams at Add time."));
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Planner_SetReplanPolicy>)
			{
				ck::goap::Warning(
					TEXT("Request_SetReplanPolicy not yet wired for runtime tuning — set in PlannerParams at Add time."));
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Planner_SetSearchBudget>)
			{
				InAStarParams.Set_BudgetMicroseconds(InTypedRequest.Get_SearchBudgetMicroseconds());
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_Planner_SetCostThreshold>)
			{
				InAStarParams.Set_CostThreshold(InTypedRequest.Get_CostThreshold());
			}
		}));
	});
}

// --------------------------------------------------------------------------------------------------------------------

auto
	FProcessor_Goap_Planner_CancelPendingRequests::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Planner_Requests& InRequestsComp)
	-> void
{
	request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
}

// --------------------------------------------------------------------------------------------------------------------

auto
	FProcessor_Goap_Planner_HandleResult::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Planner_Params& InParams,
		const FFragment_Goap_Planner_Current& InCurrent,
		const FFragment_Goap_Planner_Result& InResult,
		const FFragment_Goap_Planner_PlanContext& InPlanContext,
		FFragment_Goap_Planner_PlanState& InPlanState) -> void
{
	SCOPE_CYCLE_COUNTER(STAT_Goap_Planner_HandleResult);

	if (InCurrent.Get_EnableToggle() == ECk_EnableDisable::Disable) { return; }

	InHandle.Remove<FTag_AStar_SearchComplete>();

	// Any terminal status releases the parent-plan gate for our children.
	InHandle.Try_Remove<FTag_Goap_Planner_PlanInFlight>();

	switch (InResult._SearchStatus)
	{
		case ECk_AStarSearchStatus::Complete:
		{
			const auto ChildHandles = ck_goap_action_processor::Goap_Planner_GetCandidateChildren(InHandle);
			const auto& Graph = InPlanContext.Get_Graph();
			const auto& Actions = Graph.Get_Actions();
			const auto& Path = InResult._Path;

			InPlanState._Plan.Reset();
			if (Path.Num() >= 2)
			{
				InPlanState._Plan.Reserve(Path.Num() - 1);
			}

			for (auto i = 0; i + 1 < Path.Num(); ++i)
			{
				const auto ActionIdx = Graph.Get_ActionForEdge(Path[i], Path[i + 1]);
				if (NOT Actions.IsValidIndex(ActionIdx)) { continue; }

				const auto& ActionDefOnEdge = Actions[ActionIdx];
				const auto TargetClass = ActionDefOnEdge.ActionClass;

				const auto* MatchingChild = ChildHandles.FindByPredicate(
					[&](const FCk_Handle_Goap_Action& InCandidate)
					{
						if (NOT ck::IsValid(InCandidate)) { return false; }
						const auto& CandidateParams = InCandidate.template Get<FFragment_Goap_Action_Params>();
						return CandidateParams.Get_ActionClass() == TargetClass;
					});

				CK_ENSURE_IF_NOT(MatchingChild != nullptr,
					TEXT("A* path contains ActionDef class [{}] not in parent Action [{}]'s child set."),
					TargetClass, InHandle)
				{ continue; }

				InPlanState._Plan.Add(*MatchingChild);
			}

			// Regressive search emits goal-to-current. Reverse for execution order.
			Algo::Reverse(InPlanState._Plan);

			InPlanState._PlanStatus = ECk_GoapPlanStatus::PlanFound;
			InPlanState._PlanCost = InResult._TotalCost;

			INC_DWORD_STAT(STAT_Goap_PlansCompleted);
			UUtils_Signal_OnGoap_Planner_PlanComplete::Broadcast(
				InHandle, ck::MakePayload(InHandle, FCk_Goap_Payload_OnPlanComplete{
					InPlanState.Get_PlanClasses(), InResult._TotalCost}));
			break;
		}

		case ECk_AStarSearchStatus::Failed:
		{
			InPlanState._PlanStatus = ECk_GoapPlanStatus::PlanFailed;
			InPlanState._Plan.Reset();
			InPlanState._PlanCost = 0.0f;

			CK_ENSURE_IF_NOT(InCurrent.Get_HasUnconditionalFallback() || InParams.Get_AllowPlanFailed(),
				TEXT("Planner [{}] (tag [{}]) reached PlanFailed but has no unconditional fallback "
					 "Action and PlannerParams._AllowPlanFailed=false. Add a fallback Action "
					 "(no preconditions, effect=goal, cost ~999.0) or set _AllowPlanFailed=true."),
				InHandle, InParams.Get_PlannerTag())
			{ /* still proceed — broadcast and let consumers react */ }

			INC_DWORD_STAT(STAT_Goap_PlansFailed);
			UUtils_Signal_OnGoap_Planner_PlanFailed::Broadcast(
				InHandle, ck::MakePayload(InHandle, FCk_Goap_Payload_OnPlanFailed{}));
			break;
		}

		case ECk_AStarSearchStatus::CostThresholdReached:
		{
			InPlanState._PlanStatus = ECk_GoapPlanStatus::CostThresholdReached;
			InPlanState._Plan.Reset();
			InPlanState._PlanCost = 0.0f;

			// Deliberately no runtime ensure: a budget cap the user asked for is
			// not a catalog misconfiguration.

			INC_DWORD_STAT(STAT_Goap_PlansFailed);
			UUtils_Signal_OnGoap_Planner_PlanFailed::Broadcast(
				InHandle, ck::MakePayload(InHandle, FCk_Goap_Payload_OnPlanFailed{}));
			break;
		}

		default:
			break;
	}
}

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck
