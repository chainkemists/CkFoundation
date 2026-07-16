#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

// --------------------------------------------------------------------------------------------------------------------
//
// Classical GOAP (F.E.A.R.-style) operates on a flat boolean state vector.
// Each gameplay tag maps to a small integer key via a per-entity registry,
// and the world state + every regressive-search ConstraintSet live in fixed
// TStaticArray<uint8> buffers. That gives us:
//
//   - O(1) memcmp equality (for A* closed-set dedup)
//   - O(1) FCrc::MemCrc32 hashing (for content-addressed state lookup)
//   - Zero heap allocations per search state (whole state is ~64 bytes)
//
// Values are bools only. Gameplay code can compute whatever it wants in the
// wider world (numeric economies, distances, enum machines) and write the
// derived boolean truth into a shared GOAP WorldState entity via utility
// calls — e.g.
// `utils_goap_world_state::Set_Value(WS, Tag_HasEnoughFood, ActualFood >= Threshold)`.
// The planner only ever sees and regresses over bools, which is what makes
// GOAP tractable.
//
// WorldState encoding:  uint8{0 = false, 1 = true} — initial all-zero.
// ConstraintSet encoding: uint8{0 = unconstrained, 1 = must-be-false,
//                               2 = must-be-true}.
//
// The "must-be-false/true" distinction is separate from "key was set vs
// unset" — WorldState treats unset as false (matches classical GOAP), but
// a constraint must be able to say "I don't care about this key."

namespace ck::goap
{

// --------------------------------------------------------------------------------------------------------------------

constexpr int32 WorldState_MaxKeys = 64;

using FCk_GoapKey = int32;
constexpr FCk_GoapKey InvalidGoapKey = INDEX_NONE;

// --------------------------------------------------------------------------------------------------------------------
// CONDITION (precondition / goal condition)

struct FWorldStateCondition
{
	FCk_GoapKey Key = InvalidGoapKey;
	bool Value = false;

	FWorldStateCondition() = default;
	FWorldStateCondition(FCk_GoapKey InKey, bool InValue)
		: Key(InKey), Value(InValue) {}

	auto IsValid() const -> bool { return Key != InvalidGoapKey; }

	auto operator==(const FWorldStateCondition& InOther) const -> bool
	{
		return Key == InOther.Key && Value == InOther.Value;
	}
};

// --------------------------------------------------------------------------------------------------------------------
// Always a Set, on boolean state

struct FWorldStateEffect
{
	FCk_GoapKey Key = InvalidGoapKey;
	bool Value = false;

	FWorldStateEffect() = default;
	FWorldStateEffect(FCk_GoapKey InKey, bool InValue)
		: Key(InKey), Value(InValue) {}

	auto IsValid() const -> bool { return Key != InvalidGoapKey; }

	auto operator==(const FWorldStateEffect& InOther) const -> bool
	{
		return Key == InOther.Key && Value == InOther.Value;
	}
};

// --------------------------------------------------------------------------------------------------------------------
// RAW (TAG-KEYED) CONDITION / EFFECT
//
// Action / Goal CDO builder APIs work in tag-space because the per-entity
// FCk_GoapKey registry doesn't exist yet at CDO-definition time. The Setup
// processor scans every action's + goal's raw entries, builds the registry,
// then converts these to FCk_GoapKey-indexed form for planning.

struct FWorldStateCondition_Raw
{
	FGameplayTag Key;
	bool Value = false;
};

struct FWorldStateEffect_Raw
{
	FGameplayTag Key;
	bool Value = false;
};

// --------------------------------------------------------------------------------------------------------------------
// Indexed by FCk_GoapKey, flat bool array

struct FWorldState
{
public:
	FWorldState()
	{
		for (auto& V : _Values) { V = 0; }
	}

	// ----------------------------------------------------------------------------------------------------------------
	// ----------------------------------------------------------------------------------------------------------------

	auto Get(FCk_GoapKey InKey) const -> bool
	{
		return IsKeyInRange(InKey) && _Values[InKey] != 0;
	}

	// ----------------------------------------------------------------------------------------------------------------
	// ----------------------------------------------------------------------------------------------------------------

	auto Set(FCk_GoapKey InKey, bool InValue) -> void
	{
		if (NOT IsKeyInRange(InKey)) { return; }
		_Values[InKey] = InValue ? uint8{1} : uint8{0};
	}

	auto ApplyEffect(const FWorldStateEffect& InEffect) -> void
	{
		Set(InEffect.Key, InEffect.Value);
	}

	// ----------------------------------------------------------------------------------------------------------------
	// ----------------------------------------------------------------------------------------------------------------

	// Evaluate a single constraint against this state. A constraint is
	// satisfied iff the stored boolean matches the required value.
	auto Satisfies(const FWorldStateCondition& InCondition) const -> bool
	{
		if (NOT IsKeyInRange(InCondition.Key)) { return false; }
		return (_Values[InCondition.Key] != 0) == InCondition.Value;
	}

	// ----------------------------------------------------------------------------------------------------------------
	// COMPARISON / HASH — operate on the whole fixed-size buffer
	// ----------------------------------------------------------------------------------------------------------------

	auto operator==(const FWorldState& InOther) const -> bool
	{
		return FMemory::Memcmp(_Values.GetData(), InOther._Values.GetData(),
			sizeof(uint8) * WorldState_MaxKeys) == 0;
	}

	auto GetHash() const -> uint32
	{
		return FCrc::MemCrc32(_Values.GetData(), sizeof(uint8) * WorldState_MaxKeys);
	}

	// ----------------------------------------------------------------------------------------------------------------
	// ----------------------------------------------------------------------------------------------------------------

	auto Raw() const -> const TStaticArray<uint8, WorldState_MaxKeys>& { return _Values; }
	auto Raw()       ->       TStaticArray<uint8, WorldState_MaxKeys>& { return _Values; }

	static constexpr auto MaxKeys() -> int32 { return WorldState_MaxKeys; }

private:
	TStaticArray<uint8, WorldState_MaxKeys> _Values;

	static auto IsKeyInRange(FCk_GoapKey InKey) -> bool
	{
		return InKey >= 0 && InKey < WorldState_MaxKeys;
	}
};

inline auto GetTypeHash(const FWorldState& InState) -> uint32
{
	return InState.GetHash();
}

// --------------------------------------------------------------------------------------------------------------------
// The regressive-search state type
//
// One slot per key (no multi-slot needed now that constraints are purely
// "must be true" / "must be false"). Encoded as uint8:
//   0 = no constraint on this key
//   1 = key must be false
//   2 = key must be true

enum class EConstraint : uint8
{
	None       = 0,
	MustBeFalse = 1,
	MustBeTrue  = 2,
};

struct FConstraintSet
{
public:
	FConstraintSet()
	{
		for (auto& C : _Constraints) { C = static_cast<uint8>(EConstraint::None); }
	}

	// ----------------------------------------------------------------------------------------------------------------
	// ----------------------------------------------------------------------------------------------------------------

	// Add a constraint. If one already exists and is compatible it's a no-op;
	// if it's incompatible the set is marked Conflict (A* will prune).
	auto Add(const FWorldStateCondition& InCondition) -> void
	{
		if (NOT IsKeyInRange(InCondition.Key))
		{
			_Conflict = true;
			return;
		}

		const auto Wanted = InCondition.Value
			? static_cast<uint8>(EConstraint::MustBeTrue)
			: static_cast<uint8>(EConstraint::MustBeFalse);

		auto& Existing = _Constraints[InCondition.Key];
		if (Existing == static_cast<uint8>(EConstraint::None))
		{
			Existing = Wanted;
			return;
		}
		if (Existing != Wanted)
		{
			_Conflict = true;
		}
	}

	auto RemoveAllForKey(FCk_GoapKey InKey) -> void
	{
		if (NOT IsKeyInRange(InKey)) { return; }
		_Constraints[InKey] = static_cast<uint8>(EConstraint::None);
	}

	// Regress through an effect: if the effect sets the key to the required
	// value, the constraint is satisfied by whatever produced it, and we drop
	// the constraint from this predecessor state. If the effect sets the key
	// to the opposite value, it's a conflict (already filtered in Neighbors,
	// but we keep the check as a safety net).
	auto RegressThroughEffect(const FWorldStateEffect& InEffect) -> void
	{
		if (NOT IsKeyInRange(InEffect.Key)) { return; }
		auto& Existing = _Constraints[InEffect.Key];
		if (Existing == static_cast<uint8>(EConstraint::None)) { return; }

		const auto Required = Existing == static_cast<uint8>(EConstraint::MustBeTrue);
		if (Required == InEffect.Value)
		{
			// Effect produces exactly what's required — constraint is satisfied
			// at post-state, so drop it from pre-state.
			Existing = static_cast<uint8>(EConstraint::None);
		}
		else
		{
			_Conflict = true;
		}
	}

	// ----------------------------------------------------------------------------------------------------------------
	// ----------------------------------------------------------------------------------------------------------------

	auto IsEmpty() const -> bool
	{
		for (const auto& C : _Constraints)
		{
			if (C != static_cast<uint8>(EConstraint::None)) { return false; }
		}
		return true;
	}

	auto IsConflicted() const -> bool { return _Conflict; }

	auto IsSatisfiedBy(const FWorldState& InGround) const -> bool
	{
		if (_Conflict) { return false; }
		for (auto Key = 0; Key < WorldState_MaxKeys; ++Key)
		{
			const auto C = _Constraints[Key];
			if (C == static_cast<uint8>(EConstraint::None)) { continue; }

			const auto Required = (C == static_cast<uint8>(EConstraint::MustBeTrue));
			if (InGround.Get(Key) != Required) { return false; }
		}
		return true;
	}

	auto CountUnsatisfied(const FWorldState& InGround) const -> int32
	{
		auto Count = int32{0};
		for (auto Key = 0; Key < WorldState_MaxKeys; ++Key)
		{
			const auto C = _Constraints[Key];
			if (C == static_cast<uint8>(EConstraint::None)) { continue; }

			const auto Required = (C == static_cast<uint8>(EConstraint::MustBeTrue));
			if (InGround.Get(Key) != Required) { ++Count; }
		}
		return Count + (_Conflict ? 1 : 0);
	}

	auto HasAnyConstraintForKey(FCk_GoapKey InKey) const -> bool
	{
		if (NOT IsKeyInRange(InKey)) { return false; }
		return _Constraints[InKey] != static_cast<uint8>(EConstraint::None);
	}

	// Public accessor for the typed-constraint at a key — used by neighbor
	// expansion + the debugger. Returns EConstraint::None if unconstrained.
	auto Get(FCk_GoapKey InKey) const -> EConstraint
	{
		if (NOT IsKeyInRange(InKey)) { return EConstraint::None; }
		return static_cast<EConstraint>(_Constraints[InKey]);
	}

	// ----------------------------------------------------------------------------------------------------------------
	// EQUALITY / HASH — whole-buffer comparison is trivial now
	// ----------------------------------------------------------------------------------------------------------------

	auto operator==(const FConstraintSet& InOther) const -> bool
	{
		if (_Conflict != InOther._Conflict) { return false; }
		return FMemory::Memcmp(_Constraints.GetData(), InOther._Constraints.GetData(),
			sizeof(uint8) * WorldState_MaxKeys) == 0;
	}

	auto GetHash() const -> uint32
	{
		const auto BufferHash = FCrc::MemCrc32(_Constraints.GetData(),
			sizeof(uint8) * WorldState_MaxKeys);
		return _Conflict ? HashCombine(BufferHash, 0xDEADBEEFu) : BufferHash;
	}

	auto Raw() const -> const TStaticArray<uint8, WorldState_MaxKeys>& { return _Constraints; }

	static constexpr auto Capacity() -> int32 { return WorldState_MaxKeys; }

private:
	TStaticArray<uint8, WorldState_MaxKeys> _Constraints;
	bool _Conflict = false;

	static auto IsKeyInRange(FCk_GoapKey InKey) -> bool
	{
		return InKey >= 0 && InKey < WorldState_MaxKeys;
	}
};

inline auto GetTypeHash(const FConstraintSet& InSet) -> uint32
{
	return InSet.GetHash();
}

// --------------------------------------------------------------------------------------------------------------------
// Maps FGameplayTag ↔ FCk_GoapKey for a single planner entity

struct FKeyRegistry
{
public:
	auto FindOrRegister(FGameplayTag InTag) -> FCk_GoapKey
	{
		if (NOT InTag.IsValid()) { return InvalidGoapKey; }
		if (const auto* Found = _IndexByTag.Find(InTag)) { return *Found; }
		if (_TagByIndex.Num() >= WorldState_MaxKeys) { return InvalidGoapKey; }
		const auto NewIndex = _TagByIndex.Num();
		_TagByIndex.Add(InTag);
		_IndexByTag.Add(InTag, NewIndex);
		return NewIndex;
	}

	auto Find(FGameplayTag InTag) const -> FCk_GoapKey
	{
		if (const auto* Found = _IndexByTag.Find(InTag)) { return *Found; }
		return InvalidGoapKey;
	}

	auto GetTag(FCk_GoapKey InKey) const -> FGameplayTag
	{
		return _TagByIndex.IsValidIndex(InKey) ? _TagByIndex[InKey] : FGameplayTag{};
	}

	auto Num() const -> int32 { return _TagByIndex.Num(); }

	auto GetAllTags() const -> const TArray<FGameplayTag>& { return _TagByIndex; }

	auto Reset() -> void
	{
		_TagByIndex.Reset();
		_IndexByTag.Reset();
	}

private:
	TArray<FGameplayTag> _TagByIndex;
	TMap<FGameplayTag, FCk_GoapKey> _IndexByTag;
};

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck::goap
