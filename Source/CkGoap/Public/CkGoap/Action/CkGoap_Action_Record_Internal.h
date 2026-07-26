#pragma once

// --------------------------------------------------------------------------------------------------------------------
// PRIVATE: not a public header — keeps CkRecord out of CkGoap's public include
// surface. Included only by CkGoap_Action_Utils.cpp.

#include "CkGoap/Action/CkGoap_Action_Fragment_Data.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

namespace ck
{
	CK_DEFINE_RECORD_OF_ENTITIES_TRANSIENT(FFragment_RecordOfGoapActions, FCk_Handle_Goap_Action);
}

namespace ck::goap::internal_action
{
	struct FRecordOfGoapActions_Utils
		: public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfGoapActions> {};
}
