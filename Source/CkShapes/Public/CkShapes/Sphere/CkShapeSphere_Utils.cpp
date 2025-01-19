#include "CkShapeSphere_Utils.h"

#include "CkShapes/Sphere/CkShapeSphere_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeSphere_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_ShapeSphere_ParamsData& InParams)
    -> FCk_Handle_ShapeSphere
{
    InHandle.Add<ck::FFragment_ShapeSphere_Params>(InParams);
    InHandle.Add<ck::FFragment_ShapeSphere_Current>();

    InHandle.Add<ck::FTag_ShapeSphere_RequiresSetup>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_ShapeSphere_UE, FCk_Handle_ShapeSphere,
    ck::FFragment_ShapeSphere_Params, ck::FFragment_ShapeSphere_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeSphere_UE::
    Request_UpdateShape(
        FCk_Handle_ShapeSphere& InShapeSphere,
        const FCk_Request_ShapeSphere_UpdateShape& InRequest)
    -> FCk_Handle_ShapeSphere
{
    InShapeSphere.AddOrGet<ck::FFragment_ShapeSphere_Requests>()._Requests.Emplace(InRequest);
    return InShapeSphere;
}

auto
    UCk_Utils_ShapeSphere_UE::
    Get_ShapeData(
        const FCk_Handle_ShapeSphere& InShapeSphere)
        -> FCk_Fragment_ShapeSphere_ShapeData
{
    if (InShapeSphere.Has<ck::FTag_ShapeSphere_RequiresSetup>())
    {
        return InShapeSphere.Get<ck::FFragment_ShapeSphere_Params>().Get_Params().Get_Shape();
    }

    return InShapeSphere.Get<ck::FFragment_ShapeSphere_Current>().Get_CurrentShape();
}

// --------------------------------------------------------------------------------------------------------------------
