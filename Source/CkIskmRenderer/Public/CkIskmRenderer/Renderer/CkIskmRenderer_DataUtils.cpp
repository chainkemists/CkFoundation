#include "CkIskmRenderer_DataUtils.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkIskmRenderer/AnimCollection/CkIskmAnimCollection_Fragment_Data.h"

auto
    UCk_Utils_IskmRendererData_UE::
    Find_SubmeshIndex_ByName(const UCk_IskmRenderer_Data* InAsset, FName InName)
    -> int32
{
    if (ck::Is_NOT_Valid(InAsset))
    { return INDEX_NONE; }
    return InAsset->Find_SubmeshIndex_ByName(InName);
}

auto
    UCk_Utils_IskmRendererData_UE::
    Get_AnimCollection(const UCk_IskmRenderer_Data* InAsset)
    -> UCk_IskmAnimCollection_Data*
{
    if (ck::Is_NOT_Valid(InAsset))
    { return nullptr; }
    return InAsset->Get_AnimCollection();
}

auto
    UCk_Utils_IskmRendererData_UE::
    Get_MaxSubmeshPerInstance(const UCk_IskmRenderer_Data* InAsset)
    -> int32
{
    if (ck::Is_NOT_Valid(InAsset))
    { return 0; }
    return InAsset->Get_MaxSubmeshPerInstance();
}
