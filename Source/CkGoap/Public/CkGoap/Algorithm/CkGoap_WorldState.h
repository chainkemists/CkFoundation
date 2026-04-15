#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

// ====================================================================================================================

namespace ck::goap
{

// ====================================================================================================================
// WORLD STATE — A set of boolean conditions keyed by FGameplayTag
// ====================================================================================================================

// Represents either a full world state (ground truth) or a partial set of conditions
// (for goal requirements and action preconditions/effects).

struct FWorldState
{
public:
	FWorldState() = default;

	// ----------------------------------------------------------------------------------------------------------------
	// ACCESSORS
	// ----------------------------------------------------------------------------------------------------------------

	auto
	Get(FGameplayTag InKey) const -> bool
	{
		const auto* Found = _Values.Find(InKey);
		return Found != nullptr && *Found;
	}

	auto
	Set(FGameplayTag InKey, bool InValue) -> void
	{
		_Values.FindOrAdd(InKey) = InValue;
	}

	auto
	Has(FGameplayTag InKey) const -> bool
	{
		return _Values.Contains(InKey);
	}

	auto
	Remove(FGameplayTag InKey) -> void
	{
		_Values.Remove(InKey);
	}

	auto
	Num() const -> int32
	{
		return _Values.Num();
	}

	auto
	GetValues() const -> const TMap<FGameplayTag, bool>&
	{
		return _Values;
	}

	// ----------------------------------------------------------------------------------------------------------------
	// COMPARISON
	// ----------------------------------------------------------------------------------------------------------------

	auto
	operator==(const FWorldState& InOther) const -> bool
	{
		if (_Values.Num() != InOther._Values.Num())
		{
			return false;
		}

		for (const auto& [Key, Value] : _Values)
		{
			const auto* OtherValue = InOther._Values.Find(Key);
			if (OtherValue == nullptr || *OtherValue != Value)
			{
				return false;
			}
		}

		return true;
	}

	// ----------------------------------------------------------------------------------------------------------------
	// CONDITION CHECKING
	// ----------------------------------------------------------------------------------------------------------------

	// Returns the number of conditions in this state that are NOT satisfied by InGroundTruth.
	auto
	CountUnsatisfied(const FWorldState& InGroundTruth) const -> int32
	{
		auto Count = int32{0};
		for (const auto& [Key, RequiredValue] : _Values)
		{
			const auto* GroundValue = InGroundTruth._Values.Find(Key);
			if (GroundValue == nullptr || *GroundValue != RequiredValue)
			{
				++Count;
			}
		}
		return Count;
	}

	// Returns true if ALL conditions in this state are satisfied by InGroundTruth.
	auto
	IsSatisfiedBy(const FWorldState& InGroundTruth) const -> bool
	{
		return CountUnsatisfied(InGroundTruth) == 0;
	}

private:
	TMap<FGameplayTag, bool> _Values;
};

// ====================================================================================================================

} // namespace ck::goap
