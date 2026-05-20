#pragma once

// ====================================================================================================================
// PRIVATE: record-of-actionsets fragment + utils struct.
//
// Included by CkGoap_Utils.cpp (the Goap-root creation entry) and by
// CkGoap_ActionSet_Utils.cpp (which adds entries to the record at AddActionSet time).
//
// Lives in a header — not declared inline in either .cpp — so the two
// translation units see the SAME fragment type (one ODR), but it's NOT in
// any public header so consumers don't transitively depend on CkRecord.
//
// Do not include from anywhere except those two .cpp files.
// ====================================================================================================================

#include "CkGoap/ActionSet/CkGoap_ActionSet_Fragment_Data.h"

#include "CkRecord/Record/CkRecord_Fragment.h"
#include "CkRecord/Record/CkRecord_Utils.h"

namespace ck
{
	CK_DEFINE_RECORD_OF_ENTITIES(FFragment_RecordOfGoapActionSets, FCk_Handle_Goap_ActionSet);
}

namespace ck::goap::internal_root
{
	struct FRecordOfGoapActionSets_Utils
		: public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfGoapActionSets> {};
}
