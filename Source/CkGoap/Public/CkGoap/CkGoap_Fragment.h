#pragma once

#include "CkGoap_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Macros.h"

// --------------------------------------------------------------------------------------------------------------------
// FFragment_RecordOfGoapPlanners is deliberately NOT declared here: it would force
// CkRecord_Fragment.h into this public header, pulling FCk_Handle_EntityExtension
// into every CkGoap consumer and breaking links for dependents that don't list
// CkEntityExtension in their Build.cs. It lives in a .cpp instead.

namespace ck
{

// --------------------------------------------------------------------------------------------------------------------

	// Added by WS subscriber plumbing only when a registered key's value CHANGES
	// (not on every Set); consumed + removed by AutoReplan.
	CK_DEFINE_ECS_TAG(FTag_Goap_Dirty_WorldState);
	CK_DEFINE_ECS_TAG(FTag_Goap_Dirty_Cost);

// --------------------------------------------------------------------------------------------------------------------

} // namespace ck
