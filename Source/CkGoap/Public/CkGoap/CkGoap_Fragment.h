#pragma once

#include "CkGoap_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

// ====================================================================================================================
//
// Bundle/Tier era CkGoap module-level fragments.
//
// Per-tier ECS fragments + per-tier signals → see Tier/CkGoap_Tier_Fragment.h
// Per-ActionSet ECS fragments + ActionSet signals → see ActionSet/CkGoap_ActionSet_Fragment.h
//
// This header carries only what lives on the GOAP ROOT entity (the
// FCk_Handle_Goap container) plus the dirty-tracking tags that are shared
// across tier subscribers.
//
// FFragment_RecordOfGoapActionSets (the root's record of ActionSet entities) is
// intentionally NOT declared here. Defining it would force this public header
// to include CkRecord_Fragment.h, which transitively pulls
// FCk_Handle_EntityExtension into every consumer of CkGoap. That trips link
// errors in dependents that don't list CkEntityExtension in their Build.cs.
// The record fragment + its nested utils struct will live in CkGoap_Utils.cpp
// (Phase 3); only Utils.cpp ever touches them.
//
// ====================================================================================================================

namespace ck
{

// ====================================================================================================================
// TAGS — shared dirty tracking (consumed per-tier)
// ====================================================================================================================

	// Added by WS subscriber plumbing when a registered WS key changes value
	// (not on every Set call). Consumed + removed by per-tier AutoReplan once
	// the tier's replan policy + throttle permit firing a Plan request.
	CK_DEFINE_ECS_TAG(FTag_Goap_Dirty_WorldState);
	CK_DEFINE_ECS_TAG(FTag_Goap_Dirty_Cost);

// ====================================================================================================================

} // namespace ck
