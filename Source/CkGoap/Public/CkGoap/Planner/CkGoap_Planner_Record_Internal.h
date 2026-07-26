#pragma once

// --------------------------------------------------------------------------------------------------------------------
// PRIVATE despite living under Public/: a header so both including .cpp files see the SAME
// fragment type (one ODR) while consumers avoid a transitive CkRecord dependency.
// Include only from CkGoap_Utils.cpp and CkGoap_Planner_Utils.cpp.

#include "CkGoap/Planner/CkGoap_Planner_Fragment_Data.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

namespace ck
{
	CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfGoapPlanners, FCk_Handle_Goap_Planner);
}

namespace ck::goap::internal_planner_record
{
	struct FRecordOfGoapPlanners_Utils
		: public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfGoapPlanners> {};
}
