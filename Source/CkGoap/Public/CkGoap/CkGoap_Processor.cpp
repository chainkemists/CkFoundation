#include "CkGoap_Processor.h"

#include "CkGoap/CkGoap_Log.h"
#include "CkGoap/EntityScripts/CkGoapAction_EntityScript.h"
#include "CkGoap/EntityScripts/CkGoapGoal_EntityScript.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/TypeTraits/CkTypeTraits.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

// ====================================================================================================================
// DIAGNOSTIC HELPERS — static graph analysis (internal)
// ====================================================================================================================

namespace ck::goap_diagnostics
{
	using FCondition = TPair<FGameplayTag, bool>;

	// Build closure of (tag, value) pairs achievable by chaining actions
	// starting from InSeed. Used at setup (seed = empty) and at plan time
	// (seed = current world state).
	static auto
	ComputeReachableClosure(
		const TSet<FCondition>& InSeed,
		const TArray<goap::FActionDef>& InActions)
		-> TSet<FCondition>
	{
		auto Reachable = InSeed;
		auto Changed = true;
		while (Changed)
		{
			Changed = false;
			for (const auto& Action : InActions)
			{
				auto AllPreMet = true;
				for (const auto& [K, V] : Action.Preconditions.GetValues())
				{
					if (NOT Reachable.Contains({K, V})) { AllPreMet = false; break; }
				}
				if (NOT AllPreMet) { continue; }

				for (const auto& [K, V] : Action.Effects.GetValues())
				{
					const auto Pair = FCondition{K, V};
					if (NOT Reachable.Contains(Pair))
					{
						Reachable.Add(Pair);
						Changed = true;
					}
				}
			}
		}
		return Reachable;
	}

	// Tarjan's SCC over the condition-dependency graph. Edge u→v means
	// "condition u is a precondition of some action that produces v" —
	// achieving v requires u first. An SCC with >1 node (or a self-loop) is a
	// cycle: every condition in it requires something in the same SCC to
	// already hold, so none is producible from empty state.
	struct FSccContext
	{
		TMap<FCondition, int32> Index;
		TMap<FCondition, int32> LowLink;
		TMap<FCondition, bool>  OnStack;
		TArray<FCondition>      Stack;
		int32 NextIndex = 0;
		TArray<TArray<FCondition>> Sccs;
	};

	static auto
	StrongConnect(
		const FCondition& InNode,
		const TMap<FCondition, TArray<FCondition>>& InAdjacency,
		FSccContext& InCtx)
		-> void
	{
		InCtx.Index.Add(InNode, InCtx.NextIndex);
		InCtx.LowLink.Add(InNode, InCtx.NextIndex);
		++InCtx.NextIndex;
		InCtx.Stack.Push(InNode);
		InCtx.OnStack.Add(InNode, true);

		if (const auto* Adj = InAdjacency.Find(InNode))
		{
			for (const auto& Next : *Adj)
			{
				if (NOT InCtx.Index.Contains(Next))
				{
					StrongConnect(Next, InAdjacency, InCtx);
					InCtx.LowLink[InNode] = FMath::Min(InCtx.LowLink[InNode], InCtx.LowLink[Next]);
				}
				else if (InCtx.OnStack.FindRef(Next))
				{
					InCtx.LowLink[InNode] = FMath::Min(InCtx.LowLink[InNode], InCtx.Index[Next]);
				}
			}
		}

		if (InCtx.LowLink[InNode] == InCtx.Index[InNode])
		{
			auto Scc = TArray<FCondition>{};
			while (true)
			{
				const auto Top = InCtx.Stack.Pop();
				InCtx.OnStack[Top] = false;
				Scc.Add(Top);
				if (Top == InNode) { break; }
			}
			InCtx.Sccs.Add(MoveTemp(Scc));
		}
	}

	static auto
	DetectDependencyCycles(
		const TArray<goap::FActionDef>& InActions)
		-> TArray<FCk_GoapDiagnostic_DependencyCycle>
	{
		auto Adjacency = TMap<FCondition, TArray<FCondition>>{};
		for (const auto& Action : InActions)
		{
			for (const auto& [PreK, PreV] : Action.Preconditions.GetValues())
			{
				for (const auto& [EffK, EffV] : Action.Effects.GetValues())
				{
					Adjacency.FindOrAdd({PreK, PreV}).AddUnique({EffK, EffV});
				}
			}
		}

		auto AllNodes = TSet<FCondition>{};
		for (const auto& Action : InActions)
		{
			for (const auto& [K, V] : Action.Preconditions.GetValues()) { AllNodes.Add({K, V}); }
			for (const auto& [K, V] : Action.Effects.GetValues())        { AllNodes.Add({K, V}); }
		}

		auto Ctx = FSccContext{};
		for (const auto& Node : AllNodes)
		{
			if (NOT Ctx.Index.Contains(Node))
			{
				StrongConnect(Node, Adjacency, Ctx);
			}
		}

		// Build the implicit initial seed used to decide SCC escapability.
		// We cannot see the entity's actual initial world state at setup time,
		// but we can infer a safe approximation:
		//
		//   a) Externally-set conditions — any precondition (K, V) that is
		//      NOT written as an effect by any action. Such a condition can
		//      only enter the world state via initial seeding, so by the
		//      fact that it appears as a precondition we assume the designer
		//      intends it to be seeded true (e.g. HasVillager=true).
		//
		//   b) Default-false booleans — for every tag K that appears anywhere,
		//      seed (K, false). Boolean world-state keys conventionally
		//      default to false; this lets actions with (K=false) preconditions
		//      (e.g. GatherWood requiring !HasWood) fire from the initial
		//      state even when some OTHER action also writes (K, false) as a
		//      consumption effect.
		//
		// Once the seed is built, compute the closure over all actions. Any
		// SCC whose members intersect that closure has a real escape path
		// from the initial state — it's not a genuine dead-end cycle.
		auto AllTags = TSet<FGameplayTag>{};
		auto WrittenEffects = TSet<FCondition>{};
		for (const auto& Action : InActions)
		{
			for (const auto& [K, V] : Action.Preconditions.GetValues()) { AllTags.Add(K); }
			for (const auto& [K, V] : Action.Effects.GetValues())
			{
				AllTags.Add(K);
				WrittenEffects.Add({K, V});
			}
		}

		auto Seed = TSet<FCondition>{};
		for (const auto& Action : InActions)
		{
			for (const auto& [K, V] : Action.Preconditions.GetValues())
			{
				if (NOT WrittenEffects.Contains({K, V}))
				{
					Seed.Add({K, V});
				}
			}
		}
		for (const auto& Tag : AllTags)
		{
			Seed.Add({Tag, false});
		}

		const auto ReachableFromInitial = ComputeReachableClosure(Seed, InActions);

		// An SCC is escapable when some member condition is reachable from
		// the inferred initial state — i.e. the cycle can be "broken into"
		// without requiring another cycle member to already hold.
		const auto IsSccEscapable = [&](const TSet<FCondition>& InSccSet) -> bool
		{
			for (const auto& Member : InSccSet)
			{
				if (ReachableFromInitial.Contains(Member)) { return true; }
			}
			return false;
		};

		auto Result = TArray<FCk_GoapDiagnostic_DependencyCycle>{};
		for (const auto& Scc : Ctx.Sccs)
		{
			const auto IsSelfLoop = Scc.Num() == 1
				&& Adjacency.Contains(Scc[0])
				&& Adjacency[Scc[0]].Contains(Scc[0]);
			if (Scc.Num() <= 1 && NOT IsSelfLoop) { continue; }

			auto SccSet = TSet<FCondition>{Scc};

			// Skip escapable cycles — they're fine in practice because some
			// action outside the cycle seeds them.
			if (IsSccEscapable(SccSet)) { continue; }

			auto ActionsInCycle = TArray<TSubclassOf<UCk_GoapAction_EntityScript>>{};
			auto Tags = TArray<FGameplayTag>{};

			for (const auto& Node : Scc) { Tags.AddUnique(Node.Key); }

			for (const auto& Action : InActions)
			{
				auto TouchesScc = false;
				for (const auto& [K, V] : Action.Preconditions.GetValues())
				{
					if (SccSet.Contains({K, V})) { TouchesScc = true; break; }
				}
				if (NOT TouchesScc)
				{
					for (const auto& [K, V] : Action.Effects.GetValues())
					{
						if (SccSet.Contains({K, V})) { TouchesScc = true; break; }
					}
				}
				if (TouchesScc && ck::IsValid(Action.ActionClass))
				{
					ActionsInCycle.AddUnique(Action.ActionClass);
				}
			}

			Result.Add(FCk_GoapDiagnostic_DependencyCycle{ActionsInCycle, Tags});
		}
		return Result;
	}
}

// ====================================================================================================================

CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_Execute);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_HandleResult);
CK_REGISTER_PROCESSOR(ck::FProcessor_Goap_EndPlay);

// ====================================================================================================================

namespace ck
{

// ====================================================================================================================
// SETUP — Extract CDO metadata into lightweight fragments
// ====================================================================================================================

auto
	FProcessor_Goap_Setup::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_ActionClasses& InActionClasses,
		const FFragment_Goap_GoalClasses& InGoalClasses,
		FFragment_Goap_Actions& InActions,
		FFragment_Goap_Goals& InGoals,
		FFragment_Goap_Diagnostics& InDiagnostics)
	-> void
{
	InHandle.Remove<FTag_Goap_RequiresSetup>();

	// Extract action metadata from CDOs
	InActions._ActionDefs.Reset();
	InActions._ActionDefs.Reserve(InActionClasses._Classes.Num());

	for (auto Index = int32{0}; Index < InActionClasses._Classes.Num(); ++Index)
	{
		const auto ActionClass = InActionClasses._Classes[Index];
		if (NOT ck::IsValid(ActionClass))
		{
			continue;
		}

		auto* CDO = ActionClass.GetDefaultObject();
		if (NOT ck::IsValid(CDO))
		{
			continue;
		}

		CDO->DefineAction();

		auto ActionDef = goap::FActionDef{};
		ActionDef.ActionIndex = Index;
		ActionDef.Preconditions = CDO->_Preconditions;
		ActionDef.Effects = CDO->_Effects;
		ActionDef.Cost = CDO->_Cost;
		ActionDef.ActionClass = ActionClass;

		InActions._ActionDefs.Add(MoveTemp(ActionDef));
	}

	// Extract goal metadata from CDOs
	InGoals._GoalDefs.Reset();
	InGoals._GoalDefs.Reserve(InGoalClasses._Classes.Num());

	for (auto Index = int32{0}; Index < InGoalClasses._Classes.Num(); ++Index)
	{
		const auto GoalClass = InGoalClasses._Classes[Index];
		if (NOT ck::IsValid(GoalClass))
		{
			continue;
		}

		auto* CDO = GoalClass.GetDefaultObject();
		if (NOT ck::IsValid(CDO))
		{
			continue;
		}

		CDO->DefineGoal();

		auto GoalDef = goap::FGoalDef{};
		GoalDef.GoalIndex = Index;
		GoalDef.Conditions = CDO->_Conditions;
		GoalDef.Priority = CDO->_Priority;
		GoalDef.GoalClass = GoalClass;

		InGoals._GoalDefs.Add(MoveTemp(GoalDef));
	}

	// Static graph analysis: detect dependency cycles in the action graph.
	// A cycle means some condition can only be produced via actions that
	// transitively depend on that same condition — unreachable from empty
	// state. Emits a warning for each detected cycle; stored on the entity
	// so the debugger can surface them.
	InDiagnostics._DependencyCycles = goap_diagnostics::DetectDependencyCycles(InActions._ActionDefs);
	InDiagnostics._LastUnreachableGoalConditions.Reset();
	InDiagnostics._LastFailedGoalClass = nullptr;

	for (const auto& Cycle : InDiagnostics._DependencyCycles)
	{
		auto ActionNames = FString{};
		for (const auto& Cls : Cycle.Get_ActionsInCycle())
		{
			if (ActionNames.Len() > 0) { ActionNames += TEXT(", "); }
			ActionNames += ck::IsValid(Cls) ? Cls->GetName() : TEXT("<invalid>");
		}
		auto TagNames = FString{};
		for (const auto& T : Cycle.Get_CycleConditions())
		{
			if (TagNames.Len() > 0) { TagNames += TEXT(", "); }
			TagNames += T.ToString();
		}
		ck::goap::Warning(TEXT("GOAP dependency cycle detected on [{}]. Actions: [{}]. Conditions: [{}]. These conditions cannot be achieved unless seeded by the initial world state."),
			InHandle, ActionNames, TagNames);
	}
}

// ====================================================================================================================
// HANDLE REQUESTS
// ====================================================================================================================

auto
	FProcessor_Goap_HandleRequests::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Actions& InActions,
		const FFragment_Goap_Goals& InGoals,
		FFragment_Goap_WorldState& InWorldState,
		FFragment_Goap_Current& InCurrent,
		const FFragment_Goap_Requests& InRequests,
		FFragment_Goap_SearchState& InSearchState,
		FFragment_Goap_Result& InResult,
		FFragment_Goap_PlanContext& InPlanContext,
		FFragment_Goap_Diagnostics& InDiagnostics) const
	-> void
{
	InHandle.CopyAndRemove(InRequests, [&](FFragment_Goap_Requests& InRequestsCopy)
	{
		algo::ForEachRequest(InRequestsCopy._Requests, ck::Visitor([&](const auto& InTypedRequest)
		{
			using T = std::decay_t<decltype(InTypedRequest)>;

			if constexpr (std::is_same_v<T, FCk_Request_Goap_Plan>)
			{
				DoHandleRequest(InHandle, InActions, InGoals, InWorldState, InCurrent,
					InSearchState, InResult, InPlanContext, InDiagnostics, InTypedRequest);
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_SetWorldState>)
			{
				DoHandleRequest(InHandle, InWorldState, InTypedRequest);
			}
			else if constexpr (std::is_same_v<T, FCk_Request_Goap_CancelPlan>)
			{
				DoHandleRequest(InHandle, InCurrent, InSearchState, InTypedRequest);
			}
		}));
	});
}

// ====================================================================================================================
// PLAN REQUEST — Select goal, build graph, start A* search
// ====================================================================================================================

auto
	FProcessor_Goap_HandleRequests::
	DoHandleRequest(
		HandleType InHandle,
		const FFragment_Goap_Actions& InActions,
		const FFragment_Goap_Goals& InGoals,
		FFragment_Goap_WorldState& InWorldState,
		FFragment_Goap_Current& InCurrent,
		FFragment_Goap_SearchState& InSearchState,
		FFragment_Goap_Result& InResult,
		FFragment_Goap_PlanContext& InPlanContext,
		FFragment_Goap_Diagnostics& InDiagnostics,
		const FCk_Request_Goap_Plan& InRequest)
	-> void
{
	// Cancel any in-progress search
	InHandle.Try_Remove<FTag_AStar_SearchActive>();
	InHandle.Try_Remove<FTag_AStar_SearchComplete>();

	// Reset plan-time diagnostics. Setup-time cycles persist.
	InDiagnostics._LastUnreachableGoalConditions.Reset();
	InDiagnostics._LastFailedGoalClass = nullptr;

	// Tick the plan counter — every Request_Plan produces exactly one
	// "attempt", regardless of how quickly it resolves. Debuggers tail this.
	++InCurrent._PlanAttemptCount;

	// Select goal
	const goap::FGoalDef* SelectedGoal = nullptr;

	if (ck::IsValid(InRequest.Get_SpecificGoalClass()))
	{
		// Specific goal requested
		for (const auto& Goal : InGoals._GoalDefs)
		{
			if (Goal.GoalClass == InRequest.Get_SpecificGoalClass())
			{
				SelectedGoal = &Goal;
				break;
			}
		}
	}
	else
	{
		// Auto-select highest priority goal whose conditions are not already satisfied
		for (const auto& Goal : InGoals._GoalDefs)
		{
			if (Goal.Conditions.IsSatisfiedBy(InWorldState._WorldState))
			{
				continue;
			}

			if (SelectedGoal == nullptr || Goal.Priority > SelectedGoal->Priority)
			{
				SelectedGoal = &Goal;
			}
		}
	}

	if (SelectedGoal == nullptr)
	{
		InCurrent._PlanStatus = ECk_GoapPlanStatus::PlanFailed;
		InCurrent._Plan.Reset();
		InCurrent._PlanCost = 0.0f;
		InCurrent._ActiveGoalClass = nullptr;

		UUtils_Signal_OnGoapPlanFailed::Broadcast(InHandle,
			MakePayload(InHandle, FCk_Goap_Payload_OnPlanFailed{}));
		return;
	}

	InCurrent._ActiveGoalClass = SelectedGoal->GoalClass;
	InCurrent._PlanStatus = ECk_GoapPlanStatus::Planning;
	InCurrent._Plan.Reset();
	InCurrent._PlanCost = 0.0f;

	// Plan-time reachability: compute which (tag, value) pairs are achievable
	// starting from current world state + action closure. If any goal condition
	// is not reachable, short-circuit to PlanFailed with a diagnostic so the
	// A* doesn't waste a budget pass chasing impossible goals.
	{
		auto Seed = TSet<goap_diagnostics::FCondition>{};
		for (const auto& [K, V] : InWorldState._WorldState.GetValues())
		{
			Seed.Add({K, V});
		}
		const auto Reachable = goap_diagnostics::ComputeReachableClosure(Seed, InActions._ActionDefs);

		auto Unreachable = TArray<FCk_GoapDiagnostic_ConditionPair>{};
		for (const auto& [K, V] : SelectedGoal->Conditions.GetValues())
		{
			if (NOT Reachable.Contains({K, V}))
			{
				Unreachable.Add(FCk_GoapDiagnostic_ConditionPair{K, V});
			}
		}

		if (Unreachable.Num() > 0)
		{
			InDiagnostics._LastUnreachableGoalConditions = MoveTemp(Unreachable);
			InDiagnostics._LastFailedGoalClass = SelectedGoal->GoalClass;

			auto TagNames = FString{};
			for (const auto& C : InDiagnostics._LastUnreachableGoalConditions)
			{
				if (TagNames.Len() > 0) { TagNames += TEXT(", "); }
				TagNames += FString::Printf(TEXT("%s=%s"),
					*C.Get_Key().ToString(), C.Get_Value() ? TEXT("true") : TEXT("false"));
			}
			const auto GoalName = ck::IsValid(SelectedGoal->GoalClass)
				? SelectedGoal->GoalClass->GetName() : TEXT("<invalid>");
			ck::goap::Warning(TEXT("GOAP [{}] cannot plan goal [{}]: unreachable conditions [{}]. Check action graph for cycles or missing producers."),
				InHandle, GoalName, TagNames);

			InCurrent._PlanStatus = ECk_GoapPlanStatus::PlanFailed;
			InCurrent._Plan.Reset();
			InCurrent._PlanCost = 0.0f;

			UUtils_Signal_OnGoapPlanFailed::Broadcast(InHandle,
				MakePayload(InHandle, FCk_Goap_Payload_OnPlanFailed{}));
			return;
		}
	}

	// Build the GOAP graph adapter
	auto Graph = goap::FGoapGraph{
		InWorldState._WorldState,
		InActions._ActionDefs,
		SelectedGoal->Conditions};

	// Store graph in PlanContext (shares _Shared data with the copy inside TSearchState)
	InPlanContext._Graph = Graph;

	// Construct search state: start = goal conditions (index 0), goal = sentinel
	constexpr auto GoalSentinel = TNumericLimits<int32>::Max();
	InSearchState._State = astar::TSearchState<int32, goap::FGoapGraph>{
		MoveTemp(Graph),
		0,
		GoalSentinel};

	// Reset result
	InResult._Path.Reset();
	InResult._TotalCost = 0.0f;
	InResult._SearchStatus = ECk_AStarSearchStatus::InProgress;
	InResult._TotalIterations = 0;
	InResult._TotalTimeMicroseconds = 0;

	// Activate A* search
	InHandle.Add<FTag_AStar_SearchActive>();
}

// ====================================================================================================================
// SET WORLD STATE REQUEST
// ====================================================================================================================

auto
	FProcessor_Goap_HandleRequests::
	DoHandleRequest(
		HandleType InHandle,
		FFragment_Goap_WorldState& InWorldState,
		const FCk_Request_Goap_SetWorldState& InRequest)
	-> void
{
	InWorldState._WorldState.Set(InRequest.Get_Key(), InRequest.Get_Value());
}

// ====================================================================================================================
// CANCEL PLAN REQUEST
// ====================================================================================================================

auto
	FProcessor_Goap_HandleRequests::
	DoHandleRequest(
		HandleType InHandle,
		FFragment_Goap_Current& InCurrent,
		FFragment_Goap_SearchState& InSearchState,
		const FCk_Request_Goap_CancelPlan& InRequest)
	-> void
{
	InHandle.Try_Remove<FTag_AStar_SearchActive>();
	InHandle.Try_Remove<FTag_AStar_SearchComplete>();

	InSearchState._State = {};
	InCurrent._PlanStatus = ECk_GoapPlanStatus::Idle;
	InCurrent._Plan.Reset();
	InCurrent._PlanCost = 0.0f;
	InCurrent._ActiveGoalClass = nullptr;
}

// ====================================================================================================================
// HANDLE RESULT — Convert A* path to action sequence, fire signals
// ====================================================================================================================

auto
	FProcessor_Goap_HandleResult::
	ForEachEntity(
		TimeType InDeltaT,
		HandleType InHandle,
		const FFragment_Goap_Result& InResult,
		const FFragment_Goap_PlanContext& InPlanContext,
		FFragment_Goap_Current& InCurrent)
	-> void
{
	InHandle.Remove<FTag_AStar_SearchComplete>();

	const auto& Graph = InPlanContext._Graph;

	switch (InResult._SearchStatus)
	{
	case ECk_AStarSearchStatus::Complete:
	{
		// Convert path (state indices) to action sequence.
		// Path goes: [goalConditions(0), ..., satisfiedByCurrentState(N)]
		// Each edge represents an action applied in reverse.
		// Execution order = edges in REVERSE.

		const auto& Path = InResult._Path;
		auto ActionClasses = TArray<TSubclassOf<UCk_GoapAction_EntityScript>>{};

		if (Path.Num() > 1)
		{
			ActionClasses.Reserve(Path.Num() - 1);

			for (auto Index = int32{0}; Index < Path.Num() - 1; ++Index)
			{
				const auto ActionIndex = Graph.Get_ActionForEdge(Path[Index], Path[Index + 1]);
				if (ActionIndex != INDEX_NONE)
				{
					const auto& Actions = Graph.Get_Actions();
					if (Actions.IsValidIndex(ActionIndex))
					{
						ActionClasses.Add(Actions[ActionIndex].ActionClass);
					}
				}
			}

			// Reverse for forward execution order
			Algo::Reverse(ActionClasses);
		}

		InCurrent._PlanStatus = ECk_GoapPlanStatus::PlanFound;
		InCurrent._Plan = ActionClasses;
		InCurrent._PlanCost = InResult._TotalCost;

		UUtils_Signal_OnGoapPlanComplete::Broadcast(InHandle,
			MakePayload(InHandle, FCk_Goap_Payload_OnPlanComplete{
				MoveTemp(ActionClasses), InResult._TotalCost}));
		break;
	}

	case ECk_AStarSearchStatus::Failed:
	{
		InCurrent._PlanStatus = ECk_GoapPlanStatus::PlanFailed;
		InCurrent._Plan.Reset();
		InCurrent._PlanCost = 0.0f;

		UUtils_Signal_OnGoapPlanFailed::Broadcast(InHandle,
			MakePayload(InHandle, FCk_Goap_Payload_OnPlanFailed{}));
		break;
	}

	case ECk_AStarSearchStatus::CostThresholdReached:
	{
		InCurrent._PlanStatus = ECk_GoapPlanStatus::CostThresholdReached;
		InCurrent._Plan.Reset();
		InCurrent._PlanCost = 0.0f;

		UUtils_Signal_OnGoapPlanFailed::Broadcast(InHandle,
			MakePayload(InHandle, FCk_Goap_Payload_OnPlanFailed{}));
		break;
	}

	default:
		break;
	}
}

// ====================================================================================================================

} // namespace ck
