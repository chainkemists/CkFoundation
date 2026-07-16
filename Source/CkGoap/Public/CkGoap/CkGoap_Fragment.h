#pragma once

#include "CkGoap_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------
//
// CkGoap module-level fragments (shared dirty-tracking tags).
//
// Per-Action ECS fragments + per-Action signals → see Action/CkGoap_Action_Fragment.h
// Per-Planner ECS fragments + Planner signals → see Planner/CkGoap_Planner_Fragment.h
//
// FFragment_RecordOfGoapPlanners is intentionally NOT declared here. Defining
// it would force this public header to include CkRecord_Fragment.h, which
// transitively pulls FCk_Handle_EntityExtension into every consumer of CkGoap.
// That trips link errors in dependents that don't list CkEntityExtension in
// their Build.cs. The record fragment + its nested utils struct live in a .cpp;
// only that .cpp ever touches them.
//

namespace ck
{

// --------------------------------------------------------------------------------------------------------------------

	// Added by WS subscriber plumbing when a registered WS key changes value
	// (not on every Set call). Consumed + removed by per-Action AutoReplan
	// once the Action's replan policy + throttle permit firing a Plan request.
	CK_DEFINE_ECS_TAG(FTag_Goap_Dirty_WorldState);
	CK_DEFINE_ECS_TAG(FTag_Goap_Dirty_Cost);

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck
