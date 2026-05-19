#pragma once

#include "CoreMinimal.h"
#include "CkGoap_WorldState.h"

// ====================================================================================================================

class UCk_GoapAction_EntityScript;
class UCk_GoapGoal_EntityScript;

// ====================================================================================================================

namespace ck::goap
{

// ====================================================================================================================
// ACTION DEF — Lightweight action metadata extracted from EntityScript CDO
// ====================================================================================================================

struct FActionDef
{
	int32 ActionIndex = INDEX_NONE;
	TArray<FWorldStateCondition> Preconditions;
	TArray<FWorldStateEffect>    Effects;
	float Cost = 1.0f;
	TSubclassOf<UCk_GoapAction_EntityScript> ActionClass;

	// Bundle/Tier chain-update identity. If Plan[0]'s ActionTag matches a
	// Tier's _TierTag in the same bundle, that tier auto-activates as a
	// sub-tier. Leave invalid (default) for "leaf" actions.
	FGameplayTag ActionTag;
};

// ====================================================================================================================
// GOAL DEF — Lightweight goal metadata extracted from EntityScript CDO
// ====================================================================================================================

struct FGoalDef
{
	int32 GoalIndex = INDEX_NONE;
	TArray<FWorldStateCondition> Conditions;
	int32 Priority = 0;
	TSubclassOf<UCk_GoapGoal_EntityScript> GoalClass;
};

// ====================================================================================================================

} // namespace ck::goap
