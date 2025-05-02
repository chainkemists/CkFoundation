#include "CkShapeCapsule_Utils.h"

#include "CkShapes/CkShapes_Log.h"
#include "CkShapes/CkShapes_Utils.h"
#include "CkShapes/Capsule/CkShapeCapsule_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeCapsule_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_ShapeCapsule_ParamsData& InParams)
    -> FCk_Handle_ShapeCapsule
{
    CK_ENSURE_IF_NOT(NOT UCk_Utils_Shapes_UE::Has_Any(InHandle),
        TEXT("Trying to Add a Capsule Shape to [{}] but it already has an existing Shape feature!"), InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_ShapeCapsule_Params>(InParams);
    InHandle.Add<ck::FFragment_ShapeCapsule_Current>(InParams.Get_InitialDimensions());

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_ShapeCapsule_UE, FCk_Handle_ShapeCapsule,
    ck::FFragment_ShapeCapsule_Params, ck::FFragment_ShapeCapsule_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeCapsule_UE::
    Request_UpdateDimensions(
        FCk_Handle_ShapeCapsule& InShapeCapsule,
        const FCk_Request_ShapeCapsule_UpdateDimensions& InRequest)
    -> FCk_Handle_ShapeCapsule
{
    InShapeCapsule.AddOrGet<ck::FFragment_ShapeCapsule_Requests>()._Requests.Emplace(InRequest);
    return InShapeCapsule;
}

auto
    UCk_Utils_ShapeCapsule_UE::
    Get_Dimensions(
        const FCk_Handle_ShapeCapsule& InShapeCapsule)
        -> FCk_ShapeCapsule_Dimensions
{
    return InShapeCapsule.Get<ck::FFragment_ShapeCapsule_Current>().Get_Dimensions();
}

// --------------------------------------------------------------------------------------------------------------------
