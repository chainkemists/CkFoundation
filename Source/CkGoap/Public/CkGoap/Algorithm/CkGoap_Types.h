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

// Plain data struct used during planning. No UObject access during search.

struct FActionDef
{
	int32 ActionIndex = INDEX_NONE;
	FWorldState Preconditions;
	FWorldState Effects;
	float Cost = 1.0f;
	TSubclassOf<UCk_GoapAction_EntityScript> ActionClass;
};

// ====================================================================================================================
// GOAL DEF — Lightweight goal metadata extracted from EntityScript CDO
// ====================================================================================================================

struct FGoalDef
{
	int32 GoalIndex = INDEX_NONE;
	FWorldState Conditions;
	int32 Priority = 0;
	TSubclassOf<UCk_GoapGoal_EntityScript> GoalClass;
};

// ====================================================================================================================

} // namespace ck::goap
