#include "CkShapeCapsule_Utils.h"

#include "CkShapes/Capsule/CkShapeCapsule_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeCapsule_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_ShapeCapsule_ParamsData& InParams)
    -> FCk_Handle_ShapeCapsule
{
    InHandle.Add<ck::FFragment_ShapeCapsule_Params>(InParams);
    InHandle.Add<ck::FFragment_ShapeCapsule_Current>();

    InHandle.Add<ck::FTag_ShapeCapsule_RequiresSetup>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_ShapeCapsule_UE, FCk_Handle_ShapeCapsule,
    ck::FFragment_ShapeCapsule_Params, ck::FFragment_ShapeCapsule_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeCapsule_UE::
    Request_UpdateShape(
        FCk_Handle_ShapeCapsule& InShapeCapsule,
        const FCk_Request_ShapeCapsule_UpdateShape& InRequest)
    -> FCk_Handle_ShapeCapsule
{
    InShapeCapsule.AddOrGet<ck::FFragment_ShapeCapsule_Requests>()._Requests.Emplace(InRequest);
    return InShapeCapsule;
}

auto
    UCk_Utils_ShapeCapsule_UE::
    Get_ShapeData(
        const FCk_Handle_ShapeCapsule& InShapeCapsule)
        -> FCk_Fragment_ShapeCapsule_ShapeData
{
    if (InShapeCapsule.Has<ck::FTag_ShapeCapsule_RequiresSetup>())
    {
        return InShapeCapsule.Get<ck::FFragment_ShapeCapsule_Params>().Get_Params().Get_Shape();
    }

    return InShapeCapsule.Get<ck::FFragment_ShapeCapsule_Current>().Get_CurrentShape();
}

// --------------------------------------------------------------------------------------------------------------------
