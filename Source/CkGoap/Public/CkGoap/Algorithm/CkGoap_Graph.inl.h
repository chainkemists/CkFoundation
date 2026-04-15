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

	const auto& CurrentConditions = _Shared->StatePool[InStateIndex];

	for (const auto& Action : _Actions)
	{
		// Check if this action's effects satisfy at least one unsatisfied condition.
		auto HasRelevantEffect = false;
		for (const auto& [EffectKey, EffectValue] : Action.Effects.GetValues())
		{
			if (NOT CurrentConditions.Has(EffectKey))
			{
				continue;
			}

			if (CurrentConditions.Get(EffectKey) == EffectValue)
			{
				HasRelevantEffect = true;
				break;
			}
		}

		if (NOT HasRelevantEffect)
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

		// Add new state to pool.
		const auto NewStateIndex = _Shared->StatePool.Num();
		_Shared->StatePool.Add(MoveTemp(NewConditions));

		// Record which action produced this edge.
		const auto EdgeKey = PackEdgeKey(InStateIndex, NewStateIndex);
		_Shared->EdgeActions.Add(EdgeKey, Action.ActionIndex);

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
