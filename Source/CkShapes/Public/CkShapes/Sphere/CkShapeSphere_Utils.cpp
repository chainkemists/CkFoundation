#include "CkShapeSphere_Utils.h"

#include "CkShapes/CkShapes_Log.h"
#include "CkShapes/CkShapes_Utils.h"
#include "CkShapes/Sphere/CkShapeSphere_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeSphere_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_ShapeSphere_ParamsData& InParams)
    -> FCk_Handle_ShapeSphere
{
    CK_ENSURE_IF_NOT(NOT UCk_Utils_Shapes_UE::Has_Any(InHandle),
        TEXT("Trying to Add a Sphere Shape to [{}] but it already has an existing Shape feature!"))
    { return {}; }

    InHandle.Add<ck::FFragment_ShapeSphere_Params>(InParams);
    InHandle.Add<ck::FFragment_ShapeSphere_Current>(InParams.Get_InitialDimensions());

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_ShapeSphere_UE, FCk_Handle_ShapeSphere,
    ck::FFragment_ShapeSphere_Params, ck::FFragment_ShapeSphere_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeSphere_UE::
    Request_UpdateDimensions(
        FCk_Handle_ShapeSphere& InShapeSphere,
        const FCk_Request_ShapeSphere_UpdateDimensions& InRequest)
    -> FCk_Handle_ShapeSphere
{
    InShapeSphere.AddOrGet<ck::FFragment_ShapeSphere_Requests>()._Requests.Emplace(InRequest);
    return InShapeSphere;
}

auto
    UCk_Utils_ShapeSphere_UE::
    Get_Dimensions(
        const FCk_Handle_ShapeSphere& InShapeSphere)
        -> FCk_ShapeSphere_Dimensions
{
    return InShapeSphere.Get<ck::FFragment_ShapeSphere_Current>().Get_Dimensions();
}

// --------------------------------------------------------------------------------------------------------------------
