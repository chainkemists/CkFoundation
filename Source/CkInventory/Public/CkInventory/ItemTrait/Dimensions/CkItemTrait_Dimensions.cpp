#include "CkItemTrait_Dimensions.h"

#include "CkInventory/Item/CkItem_Utils.h"

#include "CkCore/Format/CkFormat.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ItemTrait_Dimensions::
    DoConstruct_Implementation(
        FCk_Handle& InHandle) const
    -> void
{
    auto TransformHandle = UCk_Utils_Transform_UE::Add(InHandle, FTransform::Identity);

    const auto GridParams = FCk_Fragment_2dGridSystem_ParamsData(
        _Dimensions,
        FVector2D(1.0, 1.0));

    UCk_Utils_2dGridSystem_UE::Add(TransformHandle, GridParams);
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR

auto
    UCk_ItemTrait_Dimensions::
    DoValidate_Implementation(
        const UCk_InventoryItem_Definition* InDefinition,
        TArray<FText>& OutErrors) const
    -> EDataValidationResult
{
    auto Result = EDataValidationResult::Valid;

    if (_Dimensions.X <= 0 || _Dimensions.Y <= 0)
    {
        OutErrors.Add(FText::FromString(ck::Format_UE(
            TEXT("Dimensions: Both X ({}) and Y ({}) must be greater than 0."),
            _Dimensions.X, _Dimensions.Y)));

        Result = EDataValidationResult::Invalid;
    }

    return Result;
}

#endif

// --------------------------------------------------------------------------------------------------------------------
