#include "CkShapeCylinder_Utils.h"

#include "CkShapes/CkShapes_Log.h"
#include "CkShapes/CkShapes_Utils.h"
#include "CkShapes/Cylinder/CkShapeCylinder_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeCylinder_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_ShapeCylinder_ParamsData& InParams)
    -> FCk_Handle_ShapeCylinder
{
    CK_ENSURE_IF_NOT(NOT UCk_Utils_Shapes_UE::Has_Any(InHandle),
        TEXT("Trying to Add a Cylinder Shape to [{}] but it already has an existing Shape feature!"), InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_ShapeCylinder_Params>(InParams);
    InHandle.Add<ck::FFragment_ShapeCylinder_Current>(InParams.Get_InitialDimensions());

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_ShapeCylinder_UE, FCk_Handle_ShapeCylinder,
    ck::FFragment_ShapeCylinder_Params, ck::FFragment_ShapeCylinder_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeCylinder_UE::
    Request_UpdateDimensions(
        FCk_Handle_ShapeCylinder& InShapeCylinder,
        const FCk_Request_ShapeCylinder_UpdateDimensions& InRequest)
    -> FCk_Handle_ShapeCylinder
{
    InShapeCylinder.AddOrGet<ck::FFragment_ShapeCylinder_Requests>()._Requests.Emplace(InRequest);
    return InShapeCylinder;
}

auto
    UCk_Utils_ShapeCylinder_UE::
    Get_Dimensions(
        const FCk_Handle_ShapeCylinder& InShapeCylinder)
        -> FCk_ShapeCylinder_Dimensions
{
    return InShapeCylinder.Get<ck::FFragment_ShapeCylinder_Current>().Get_Dimensions();
}

// --------------------------------------------------------------------------------------------------------------------
