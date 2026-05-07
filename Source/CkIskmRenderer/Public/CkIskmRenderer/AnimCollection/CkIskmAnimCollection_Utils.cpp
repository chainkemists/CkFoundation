#include "CkIskmAnimCollection_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

auto
    UCk_Utils_IskmAnimCollection_UE::
    Find_SequenceIndex_ByAsset(
        const UCk_IskmAnimCollection_Data* InAsset,
        const UAnimSequenceBase* InSequence)
    -> int32
{
    if (ck::Is_NOT_Valid(InAsset)) { return INDEX_NONE; }
    return InAsset->Find_SequenceIndex_ByAsset(InSequence);
}

auto
    UCk_Utils_IskmAnimCollection_UE::
    Get_Skeleton(const UCk_IskmAnimCollection_Data* InAsset)
    -> USkeleton*
{
    if (ck::Is_NOT_Valid(InAsset)) { return nullptr; }
    return InAsset->Get_Skeleton();
}
