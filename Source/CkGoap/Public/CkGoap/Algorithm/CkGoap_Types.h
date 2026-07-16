#pragma once

#include "CoreMinimal.h"
#include "CkGoap_WorldState.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_GoapAction_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::goap
{

// --------------------------------------------------------------------------------------------------------------------
// Lightweight action metadata extracted from an EntityScript CDO.

struct FActionDef
{
	int32 ActionIndex = INDEX_NONE;
	TArray<FWorldStateCondition> Preconditions;
	TArray<FWorldStateEffect>    Effects;
	float Cost = 1.0f;
	TSubclassOf<UCk_GoapAction_EntityScript> ActionClass;

	// Legacy chain-update identity. The unified model reads Plan[0] handles
	// directly rather than tag-matching; retained only for compile compatibility.
	FGameplayTag ActionTag;
};

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck::goap
