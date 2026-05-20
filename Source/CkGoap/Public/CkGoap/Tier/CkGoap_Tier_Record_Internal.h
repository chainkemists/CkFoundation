#pragma once

// ====================================================================================================================
// PRIVATE: ActionSet's record-of-tiers fragment + utils struct.
//
// Included by CkGoap_Tier_Utils.cpp (AddTier creates record entries).
// Future processors that need to enumerate the tier catalog directly will
// also include this. NOT a public header — keeps CkRecord out of the public
// include surface.
// ====================================================================================================================

#include "CkGoap/Tier/CkGoap_Tier_Fragment_Data.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

namespace ck
{
	CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfGoapTiers, FCk_Handle_Goap_Tier);
}

namespace ck::goap::internal_tier
{
	struct FRecordOfGoapTiers_Utils
		: public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfGoapTiers> {};
}
