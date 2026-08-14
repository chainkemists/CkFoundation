#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

// --------------------------------------------------------------------------------------------------------------------
// Classical (F.E.A.R.-style) boolean GOAP: each gameplay tag maps to a small
// integer key and every state lives in a fixed TStaticArray<uint8> — O(1) memcmp
// equality, O(1) MemCrc32 hashing, zero heap per search state. Note that
// WorldState treats an unset key as false, whereas a constraint can be absent
// ("don't care"). Rationale and authoring guidance: CkGoap CLAUDE.md.

namespace ck::goap
{

// --------------------------------------------------------------------------------------------------------------------

constexpr int32 WorldState_MaxKeys = 64;

using FCk_GoapKey = int32;
constexpr FCk_GoapKey InvalidGoapKey = INDEX_NONE;

// --------------------------------------------------------------------------------------------------------------------

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
// Tag-keyed form: CDO builders run before the per-entity FCk_GoapKey registry
// exists; Setup builds the registry and converts these to indexed form.

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
		if (NOT IsKeyInRange(InKey))
		{ return; }
		_Values[InKey] = InValue ? uint8{1} : uint8{0};
	}

	auto ApplyEffect(const FWorldStateEffect& InEffect) -> void
	{
		Set(InEffect.Key, InEffect.Value);
	}

	// ----------------------------------------------------------------------------------------------------------------
	// ----------------------------------------------------------------------------------------------------------------

	auto Satisfies(const FWorldStateCondition& InCondition) const -> bool
	{
		if (NOT IsKeyInRange(InCondition.Key))
		{ return false; }
		return (_Values[InCondition.Key] != 0) == InCondition.Value;
	}

	// ----------------------------------------------------------------------------------------------------------------
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
// The regressive-search state type — one constraint slot per key.

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
		if (NOT IsKeyInRange(InKey))
		{ return; }
		_Constraints[InKey] = static_cast<uint8>(EConstraint::None);
	}

	// If the effect produces the required value the constraint is satisfied at the
	// post-state, so drop it from the pre-state. The opposite value is a conflict —
	// already filtered in Neighbors; kept here as a safety net.
	auto RegressThroughEffect(const FWorldStateEffect& InEffect) -> void
	{
		if (NOT IsKeyInRange(InEffect.Key))
		{ return; }
		auto& Existing = _Constraints[InEffect.Key];
		if (Existing == static_cast<uint8>(EConstraint::None))
		{ return; }

		const auto Required = Existing == static_cast<uint8>(EConstraint::MustBeTrue);
		if (Required == InEffect.Value)
		{
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
			if (C != static_cast<uint8>(EConstraint::None))
			{ return false; }
		}
		return true;
	}

	auto IsConflicted() const -> bool { return _Conflict; }

	auto IsSatisfiedBy(const FWorldState& InGround) const -> bool
	{
		if (_Conflict)
		{ return false; }
		for (auto Key = 0; Key < WorldState_MaxKeys; ++Key)
		{
			const auto C = _Constraints[Key];
			if (C == static_cast<uint8>(EConstraint::None))
			{ continue; }

			const auto Required = (C == static_cast<uint8>(EConstraint::MustBeTrue));
			if (InGround.Get(Key) != Required)
			{ return false; }
		}
		return true;
	}

	auto CountUnsatisfied(const FWorldState& InGround) const -> int32
	{
		auto Count = int32{0};
		for (auto Key = 0; Key < WorldState_MaxKeys; ++Key)
		{
			const auto C = _Constraints[Key];
			if (C == static_cast<uint8>(EConstraint::None))
			{ continue; }

			const auto Required = (C == static_cast<uint8>(EConstraint::MustBeTrue));
			if (InGround.Get(Key) != Required)
			{ ++Count; }
		}
		return Count + (_Conflict ? 1 : 0);
	}

	auto HasAnyConstraintForKey(FCk_GoapKey InKey) const -> bool
	{
		if (NOT IsKeyInRange(InKey))
		{ return false; }
		return _Constraints[InKey] != static_cast<uint8>(EConstraint::None);
	}

	// Returns EConstraint::None when the key is unconstrained or out of range.
	auto Get(FCk_GoapKey InKey) const -> EConstraint
	{
		if (NOT IsKeyInRange(InKey))
		{ return EConstraint::None; }
		return static_cast<EConstraint>(_Constraints[InKey]);
	}

	// ----------------------------------------------------------------------------------------------------------------
	// ----------------------------------------------------------------------------------------------------------------

	auto operator==(const FConstraintSet& InOther) const -> bool
	{
		if (_Conflict != InOther._Conflict)
		{ return false; }
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
		if (NOT InTag.IsValid())
		{ return InvalidGoapKey; }
		if (const auto* Found = _IndexByTag.Find(InTag))
		{ return *Found; }
		if (_TagByIndex.Num() >= WorldState_MaxKeys)
		{ return InvalidGoapKey; }
		const auto NewIndex = _TagByIndex.Num();
		_TagByIndex.Add(InTag);
		_IndexByTag.Add(InTag, NewIndex);
		return NewIndex;
	}

	auto Find(FGameplayTag InTag) const -> FCk_GoapKey
	{
		if (const auto* Found = _IndexByTag.Find(InTag))
		{ return *Found; }
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
