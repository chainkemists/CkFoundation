#include "CkIsmProxy_Fragment_Data.h"

#include <Misc/DataValidation.h>

// --------------------------------------------------------------------------------------------------------------------

//#if WITH_EDITOR
//auto
//    UCk_Ism_Definition_PDA::
//    IsDataValid(
//        FDataValidationContext& Context) const
//    -> EDataValidationResult
//{
//    auto Result = Super::IsDataValid(Context);
//
//    if (IsTemplate())
//    { return Result; }
//
//    if (ck::Is_NOT_Valid(Get_MeshParams().Get_RendererName()))
//    {
//        Context.AddError(FText::FromString(ck::Format_UE(TEXT("Ism Definition [{}] is missing a valid Renderer Name"), this)));
//
//        Result = CombineDataValidationResults(Result, EDataValidationResult::Invalid);
//    }
//
//    if (ck::Is_NOT_Valid(Get_MeshParams().Get_Mesh()))
//    {
//        Context.AddError(FText::FromString(ck::Format_UE(TEXT("Ism Definition [{}] is missing a valid Mesh"), this)));
//
//        Result = CombineDataValidationResults(Result, EDataValidationResult::Invalid);
//    }
//
//    return Result;
//}
//#endif
//
//auto
//    UCk_Ism_Definition_PDA::
//    GetPrimaryAssetId() const
//    -> FPrimaryAssetId
//{
//    return FPrimaryAssetId(_AssetRegistryCategory, GetFName());
//}

// --------------------------------------------------------------------------------------------------------------------
