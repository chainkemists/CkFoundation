#pragma once

// ====================================================================================================================

namespace ck::goap
{

// ====================================================================================================================
// NEIGHBORS — Regressive expansion: find actions whose effects satisfy conditions in this state
// ====================================================================================================================

inline auto
	FGoapGraph::
	Neighbors(int32 InStateIndex) const
	-> TArray<int32>
{
	check(_Shared.IsValid());

	auto Result = TArray<int32>{};
	Result.Reserve(_Actions.Num());

	// NOTE: copy by value — the loop below appends to _Shared->StatePool via
	// StatePool.Add, which can reallocate the underlying buffer and invalidate
	// any reference into it. Capturing by value keeps CurrentConditions valid
	// across all iterations.
	const auto CurrentConditions = _Shared->StatePool[InStateIndex];

	for (const auto& Action : _Actions)
	{
		// Two gates on this action's applicability in the current regressive state:
		//
		// 1) HasRelevantEffect — at least one effect (K, V) matches a required
		//    condition (K, V) in CurrentConditions. Without this, the action
		//    does no useful regressive work.
		//
		// 2) NO conflicting effect — no effect (K, X) clashes with a required
		//    (K, Y) where X != Y. Without this gate, the search accepts an
		//    action on the basis of one matching effect while silently
		//    ignoring that its OTHER effects would violate other required
		//    conditions. That produces predecessor states which are
		//    unreachable in forward execution — e.g. BuildMill claiming
		//    predecessor {HasMill=true, HasWood=true} via effect HasMill=true
		//    while its effect HasWood=false would actually leave HasWood false.
		auto HasRelevantEffect = false;
		auto HasConflictingEffect = false;
		for (const auto& [EffectKey, EffectValue] : Action.Effects.GetValues())
		{
			if (NOT CurrentConditions.Has(EffectKey))
			{
				continue;
			}

			if (CurrentConditions.Get(EffectKey) == EffectValue)
			{
				HasRelevantEffect = true;
			}
			else
			{
				HasConflictingEffect = true;
				break;
			}
		}

		if (HasConflictingEffect || NOT HasRelevantEffect)
		{
			continue;
		}

		// Build new condition set: remove satisfied conditions, add preconditions.
		auto NewConditions = FWorldState{};

		// Keep conditions not satisfied by this action's effects.
		for (const auto& [Key, Value] : CurrentConditions.GetValues())
		{
			const auto* EffectValue = Action.Effects.GetValues().Find(Key);
			const auto IsSatisfiedByEffect = EffectValue != nullptr && *EffectValue == Value;

			if (NOT IsSatisfiedByEffect)
			{
				NewConditions.Set(Key, Value);
			}
		}

		// Add action's preconditions as new requirements.
		for (const auto& [Key, Value] : Action.Preconditions.GetValues())
		{
			if (NOT NewConditions.Has(Key))
			{
				NewConditions.Set(Key, Value);
			}
		}

		// Dedupe by content: if an equivalent FWorldState already lives in the
		// pool, reuse its index instead of allocating a new one. Without this,
		// conjunctive goals (e.g. {A,B,C,D} all needed) explode the search
		// space — every permutation of sub-goal ordering would get its own
		// index even though the world content is identical.
		const auto NewHash = HashWorldState(NewConditions);
		auto NewStateIndex = int32{INDEX_NONE};

		if (auto* Candidates = _Shared->StateLookup.Find(NewHash))
		{
			for (const auto CandidateIdx : *Candidates)
			{
				if (_Shared->StatePool[CandidateIdx] == NewConditions)
				{
					NewStateIndex = CandidateIdx;
					break;
				}
			}
		}

		if (NewStateIndex == INDEX_NONE)
		{
			NewStateIndex = _Shared->StatePool.Num();
			_Shared->StatePool.Add(MoveTemp(NewConditions));
			_Shared->StateLookup.FindOrAdd(NewHash).Add(NewStateIndex);
		}

		// Record which action produced this edge. If the edge is already
		// known via another action, keep whichever is cheaper so Cost()
		// returns the best transition.
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
// HEURISTIC — Count of unsatisfied conditions (admissible for regressive GOAP)
// ====================================================================================================================

inline auto
	FGoapGraph::
	Heuristic(int32 InCurrent, [[maybe_unused]] int32 InGoal) const
	-> float
{
	check(_Shared.IsValid());
	const auto& Conditions = _Shared->StatePool[InCurrent];
	return static_cast<float>(Conditions.CountUnsatisfied(_CurrentWorldState));
}

// ====================================================================================================================
// IS GOAL — All conditions in this state are satisfied by the current world state
// ====================================================================================================================

inline auto
	FGoapGraph::
	IsGoal(int32 InStateIndex) const
	-> bool
{
	check(_Shared.IsValid());
	const auto& Conditions = _Shared->StatePool[InStateIndex];
	return Conditions.IsSatisfiedBy(_CurrentWorldState);
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
