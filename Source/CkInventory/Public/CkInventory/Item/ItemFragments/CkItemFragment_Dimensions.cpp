#include "CkItemFragment_Dimensions.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_ItemFragment_Dimensions::
    OnApplied(
        FCk_Handle_Item& InItem) const
    -> void
{
    auto TransformHandle = UCk_Utils_Transform_UE::Add(InItem, FTransform::Identity);

    const auto GridParams = FCk_Fragment_2dGridSystem_ParamsData(
        _Dimensions,
        FVector2D(1.0, 1.0));

    UCk_Utils_2dGridSystem_UE::Add(TransformHandle, GridParams);
}

// --------------------------------------------------------------------------------------------------------------------
