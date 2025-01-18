#include "CkShapeBox_Utils.h"

#include "CkShapes/CkShapes_Log.h"
#include "CkShapes/Box/CkShapeBox_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeBox_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_ShapeBox_ParamsData& InParams)
    -> FCk_Handle_ShapeBox
{
    InHandle.Add<ck::FFragment_ShapeBox_Params>(InParams);
    InHandle.Add<ck::FFragment_ShapeBox_Current>();

    InHandle.Add<ck::FTag_ShapeBox_RequiresSetup>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_ShapeBox_UE, FCk_Handle_ShapeBox,
    ck::FFragment_ShapeBox_Params, ck::FFragment_ShapeBox_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeBox_UE::
    Request_UpdateShape(
        FCk_Handle_ShapeBox& InShapeBox,
        const FCk_Request_ShapeBox_UpdateShape& InRequest)
    -> FCk_Handle_ShapeBox
{
    InShapeBox.AddOrGet<ck::FFragment_ShapeBox_Requests>()._Requests.Emplace(InRequest);
    return InShapeBox;
}

auto
    UCk_Utils_ShapeBox_UE::
    Get_ShapeData(
        const FCk_Handle_ShapeBox& InShapeBox)
        -> FCk_Fragment_ShapeBox_ShapeData
{
    if (InShapeBox.Has<ck::FTag_ShapeBox_RequiresSetup>())
    {
        return InShapeBox.Get<ck::FFragment_ShapeBox_Params>().Get_Params().Get_Shape();
    }

    return InShapeBox.Get<ck::FFragment_ShapeBox_Current>().Get_CurrentShape();
}

// --------------------------------------------------------------------------------------------------------------------
