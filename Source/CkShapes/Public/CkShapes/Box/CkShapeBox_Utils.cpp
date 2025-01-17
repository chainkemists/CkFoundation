#include "CkShapeBox_Utils.h"

#include "CkShapes/CkShapes_Log.h"
#include "CkShapes/Box/CkShapeBox_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeBox_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_ShapeBox_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_ShapeBox
{
    InHandle.Add<ck::FFragment_ShapeBox_Params>(InParams);
    InHandle.Add<ck::FFragment_ShapeBox_Current>();

    InHandle.Add<ck::FTag_ShapeBox_RequiresSetup>();

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::shapes::VeryVerbose
        (
            TEXT("Skipping creation of ShapeBox Rep Fragment on Entity [{}] because it's set to [{}]"),
            InHandle,
            InReplicates
        );

        return Cast(InHandle);
    }

    TryAddReplicatedFragment<UCk_Fragment_ShapeBox_Rep>(InHandle);
    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_ShapeBox_UE, FCk_Handle_ShapeBox,
    ck::FFragment_ShapeBox_Params, ck::FFragment_ShapeBox_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeBox_UE::
    Request_ExampleRequest(
        FCk_Handle_ShapeBox& InShapeBox,
        const FCk_Request_ShapeBox_ExampleRequest& InRequest)
    -> FCk_Handle_ShapeBox
{
    InShapeBox.AddOrGet<ck::FFragment_ShapeBox_Requests>()._Requests.Emplace(InRequest);
    return InShapeBox;
}

// --------------------------------------------------------------------------------------------------------------------