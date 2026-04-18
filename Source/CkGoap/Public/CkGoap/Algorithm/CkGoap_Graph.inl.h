#pragma once

// ====================================================================================================================

namespace ck::goap
{

// ====================================================================================================================
// HELPERS — effect / constraint interaction for boolean-only GOAP
// ====================================================================================================================

namespace detail
{
	// Does this effect satisfy this constraint in a regressive sense?
	// For bool-only GOAP this is simply: same key AND effect value matches
	// required value.
	static auto
	EffectHelpsConstraint(const FWorldStateEffect& InEffect, const FWorldStateCondition& InC) -> bool
	{
		return InEffect.Key == InC.Key && InEffect.Value == InC.Value;
	}

	// Does this effect conflict with this constraint? Only meaningful on a
	// shared key: if the effect forces the opposite value of what the
	// constraint requires, it's a conflict.
	static auto
	EffectConflictsWithConstraint(const FWorldStateEffect& InEffect, const FWorldStateCondition& InC) -> bool
	{
		return InEffect.Key == InC.Key && InEffect.Value != InC.Value;
	}
}

// ====================================================================================================================
// NEIGHBORS — Regressive expansion
// ====================================================================================================================

inline auto
	FGoapGraph::
	Neighbors(int32 InStateIndex) const
	-> TArray<int32>
{
	check(_Shared.IsValid());

	auto Result = TArray<int32>{};
	Result.Reserve(_Actions.Num());

	// Copy by value — the pool may reallocate as we append below.
	const auto Current = _Shared->StatePool[InStateIndex];
	if (Current.IsConflicted()) { return Result; }

	for (const auto& Action : _Actions)
	{
		// Classify this action against the current constraint set.
		auto Helps    = false;  // at least one effect satisfies a constraint
		auto Conflict = false;  // at least one effect violates a constraint

		for (const auto& Effect : Action.Effects)
		{
			const auto C = Current.Get(Effect.Key);
			if (C == EConstraint::None) { continue; }

			const auto Required = (C == EConstraint::MustBeTrue);
			if (Effect.Value == Required) { Helps = true; }
			else                          { Conflict = true; break; }
		}
		if (Conflict) { continue; }
		if (NOT Helps) { continue; }

		// Build the predecessor constraint set.
		auto New = Current;

		// Apply every effect's regressive transform to any matching constraint.
		for (const auto& Effect : Action.Effects)
		{
			New.RegressThroughEffect(Effect);
		}

		// Add the action's preconditions as new constraints on the predecessor.
		for (const auto& Pre : Action.Preconditions)
		{
			New.Add(Pre);
		}

		if (New.IsConflicted()) { continue; }

		// Dedupe by content against StatePool.
		const auto NewHash = New.GetHash();
		auto NewStateIndex = int32{INDEX_NONE};

		if (auto* Candidates = _Shared->StateLookup.Find(NewHash))
		{
			for (const auto CandidateIdx : *Candidates)
			{
				if (_Shared->StatePool[CandidateIdx] == New)
				{
					NewStateIndex = CandidateIdx;
					break;
				}
			}
		}

		if (NewStateIndex == INDEX_NONE)
		{
			NewStateIndex = _Shared->StatePool.Num();
			_Shared->StatePool.Add(MoveTemp(New));
			_Shared->StateLookup.FindOrAdd(NewHash).Add(NewStateIndex);
		}

		// Record which action produced this edge — cheaper transition wins.
		const auto EdgeKey = PackEdgeKey(InStateIndex, NewStateIndex);
		const auto* ExistingActionIdx = _Shared->EdgeActions.Find(EdgeKey);
		if (ExistingActionIdx == nullptr)
		{
			_Shared->EdgeActions.Add(EdgeKey, Action.ActionIndex);
		}
		else if (Action.Cost < _Actions[*ExistingActionIdx].Cost)
		{
			_Shared->EdgeActions[EdgeKey] = Action.ActionIndex;
		}

		Result.Add(NewStateIndex);
	}

	return Result;
}

// ====================================================================================================================
// COST — Lookup the action cost for an edge
// ====================================================================================================================

inline auto
	FGoapGraph::
	Cost(int32 InFrom, int32 InTo) const
	-> float
{
	check(_Shared.IsValid());

	const auto EdgeKey = PackEdgeKey(InFrom, InTo);
	const auto* ActionIndex = _Shared->EdgeActions.Find(EdgeKey);

	if (ActionIndex == nullptr || *ActionIndex < 0 || *ActionIndex >= _Actions.Num())
	{
		return TNumericLimits<float>::Max();
	}

	return _Actions[*ActionIndex].Cost;
}

// ====================================================================================================================
// HEURISTIC — Sum of min-cost actions that could satisfy each unsatisfied constraint
// ====================================================================================================================
//
// For each unsatisfied constraint, the cheapest action whose Set effect
// produces the required value is a lower bound on the cost to satisfy it.
// Summing across independent constraints (h^add) is admissible for most
// GOAP domains and tightly informs A* so it moves directly toward the goal
// rather than sprawling in uniform-cost mode. Falls back to CountUnsatisfied
// if no action can produce a given key (unreachable — the search will just
// never find a goal state, which IsGoal correctly signals).

inline auto
	FGoapGraph::
	Heuristic(int32 InCurrent, [[maybe_unused]] int32 InGoal) const
	-> float
{
	check(_Shared.IsValid());

	const auto& State = _Shared->StatePool[InCurrent];
	if (State.IsConflicted()) { return TNumericLimits<float>::Max() / 2.0f; }

	auto Total = 0.0f;
	for (auto Key = 0; Key < WorldState_MaxKeys; ++Key)
	{
		const auto C = State.Get(Key);
		if (C == EConstraint::None) { continue; }

		const auto Required = (C == EConstraint::MustBeTrue);
		if (_CurrentWorldState.Get(Key) == Required) { continue; }  // already satisfied

		// Cheapest action whose Set effect matches the required value.
		auto Best = TNumericLimits<float>::Max();
		for (const auto& Action : _Actions)
		{
			for (const auto& Effect : Action.Effects)
			{
				if (Effect.Key == Key && Effect.Value == Required)
				{
					Best = FMath::Min(Best, Action.Cost);
					break;
				}
			}
		}
		Total += (Best == TNumericLimits<float>::Max()) ? 1.0e6f : Best;
	}
	return Total;
}

// ====================================================================================================================
// IS GOAL — All constraints in this state are satisfied by the current world state
// ====================================================================================================================

inline auto
	FGoapGraph::
	IsGoal(int32 InStateIndex) const
	-> bool
{
	check(_Shared.IsValid());
	return _Shared->StatePool[InStateIndex].IsSatisfiedBy(_CurrentWorldState);
}

// ====================================================================================================================
// GET ACTION FOR EDGE
// ====================================================================================================================

inline auto
	FGoapGraph::
	Get_ActionForEdge(int32 InFrom, int32 InTo) const
	-> int32
{
	check(_Shared.IsValid());

	const auto EdgeKey = PackEdgeKey(InFrom, InTo);
	const auto* ActionIndex = _Shared->EdgeActions.Find(EdgeKey);
	return ActionIndex != nullptr ? *ActionIndex : INDEX_NONE;
}

// ====================================================================================================================

} // namespace ck::goap
