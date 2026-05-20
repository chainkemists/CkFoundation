#pragma once

#include "CoreMinimal.h"
#include "CkGoap_WorldState.h"

// ====================================================================================================================

class UCk_GoapAction_EntityScript;

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

	// Bundle/Tier era chain-update identity (legacy). In the unified
	// ActionSet/Action model, chain extension reads Plan[0] handles directly
	// rather than tag matching. Field retained for U1 compile-clean; U3 will
	// remove it when the legacy resolver is fully replaced.
	FGameplayTag ActionTag;
};

// ====================================================================================================================

} // namespace ck::goap
