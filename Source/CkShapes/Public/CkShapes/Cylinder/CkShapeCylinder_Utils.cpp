#include "CkShapeCylinder_Utils.h"

#include "CkShapes/Cylinder/CkShapeCylinder_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeCylinder_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_ShapeCylinder_ParamsData& InParams)
    -> FCk_Handle_ShapeCylinder
{
    InHandle.Add<ck::FFragment_ShapeCylinder_Params>(InParams);
    InHandle.Add<ck::FFragment_ShapeCylinder_Current>();

    InHandle.Add<ck::FTag_ShapeCylinder_RequiresSetup>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_ShapeCylinder_UE, FCk_Handle_ShapeCylinder,
    ck::FFragment_ShapeCylinder_Params, ck::FFragment_ShapeCylinder_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeCylinder_UE::
    Request_UpdateShape(
        FCk_Handle_ShapeCylinder& InShapeCylinder,
        const FCk_Request_ShapeCylinder_UpdateShape& InRequest)
    -> FCk_Handle_ShapeCylinder
{
    InShapeCylinder.AddOrGet<ck::FFragment_ShapeCylinder_Requests>()._Requests.Emplace(InRequest);
    return InShapeCylinder;
}

auto
    UCk_Utils_ShapeCylinder_UE::
    Get_ShapeData(
        const FCk_Handle_ShapeCylinder& InShapeCylinder)
        -> FCk_Fragment_ShapeCylinder_ShapeData
{
    if (InShapeCylinder.Has<ck::FTag_ShapeCylinder_RequiresSetup>())
    {
        return InShapeCylinder.Get<ck::FFragment_ShapeCylinder_Params>().Get_Params().Get_Shape();
    }

    return InShapeCylinder.Get<ck::FFragment_ShapeCylinder_Current>().Get_CurrentShape();
}

// --------------------------------------------------------------------------------------------------------------------
