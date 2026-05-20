#pragma once

// ====================================================================================================================
// PRIVATE: ActionSet's record-of-actions fragment + utils struct.
//
// Included by CkGoap_Action_Utils.cpp (AddAction_ToActionSet creates record
// entries). Future processors that need to enumerate the action catalog
// directly will also include this. NOT a public header — keeps CkRecord out
// of the public include surface.
// ====================================================================================================================

#include "CkGoap/Action/CkGoap_Action_Fragment_Data.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

namespace ck
{
	CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfGoapActions, FCk_Handle_Goap_Action);
}

namespace ck::goap::internal_action
{
	struct FRecordOfGoapActions_Utils
		: public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfGoapActions> {};
}
