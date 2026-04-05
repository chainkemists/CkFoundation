#include "CkShapeBox_Utils.h"

#include "CkShapes/CkShapes_Log.h"
#include "CkShapes/CkShapes_Utils.h"
#include "CkShapes/Box/CkShapeBox_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeBox_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_ShapeBox_ParamsData& InParams)
        -> FCk_Handle_ShapeBox
{
    CK_ENSURE_IF_NOT(NOT UCk_Utils_Shapes_UE::Has(InHandle),
        TEXT("Trying to Add a Box Shape to [{}] but it already has an existing Shape feature!"), InHandle)
    {
        return {};
    }

    InHandle.Add<ck::FFragment_ShapeBox_Params>(InParams);
    InHandle.Add<ck::FFragment_ShapeBox_Current>(InParams.Get_InitialDimensions());

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_ShapeBox_UE, FCk_Handle_ShapeBox,
    ck::FFragment_ShapeBox_Params, ck::FFragment_ShapeBox_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeBox_UE::
    Request_UpdateDimensions(
        FCk_Handle_ShapeBox& InShapeBox,
        const FCk_Request_ShapeBox_UpdateDimensions& InRequest)
    -> FCk_Handle_ShapeBox
{
    InShapeBox.AddOrGet<ck::FFragment_ShapeBox_Requests>()._Requests.Emplace(InRequest);
    return InShapeBox;
}

auto
    UCk_Utils_ShapeBox_UE::
    Get_Dimensions(
        const FCk_Handle_ShapeBox& InShapeBox)
        -> FCk_ShapeBox_Dimensions
{
    return InShapeBox.Get<ck::FFragment_ShapeBox_Current>().Get_Dimensions();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeBox_UE::
    BindTo_OnDimensionsChanged(
        FCk_Handle_ShapeBox& InShapeBox,
        const FCk_Delegate_ShapeBox_OnDimensionsChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_ShapeBox
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_OnShapeBoxDimensionsChanged, InShapeBox, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InShapeBox;
}

auto
    UCk_Utils_ShapeBox_UE::
    UnbindFrom_OnDimensionsChanged(
        FCk_Handle_ShapeBox& InShapeBox,
        const FCk_Delegate_ShapeBox_OnDimensionsChanged& InDelegate)
    -> FCk_Handle_ShapeBox
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnShapeBoxDimensionsChanged, InShapeBox, InDelegate);
    return InShapeBox;
}

// --------------------------------------------------------------------------------------------------------------------