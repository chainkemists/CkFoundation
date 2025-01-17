#include "CkShapeSphere_Utils.h"

#include "CkShapes/CkShapes_Log.h"
#include "CkShapes/Sphere/CkShapeSphere_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeSphere_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_ShapeSphere_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_ShapeSphere
{
    InHandle.Add<ck::FFragment_ShapeSphere_Params>(InParams);
    InHandle.Add<ck::FFragment_ShapeSphere_Current>();

    InHandle.Add<ck::FTag_ShapeSphere_RequiresSetup>();

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::shapes::VeryVerbose
        (
            TEXT("Skipping creation of ShapeSphere Rep Fragment on Entity [{}] because it's set to [{}]"),
            InHandle,
            InReplicates
        );

        return Cast(InHandle);
    }

    TryAddReplicatedFragment<UCk_Fragment_ShapeSphere_Rep>(InHandle);
    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_ShapeSphere_UE, FCk_Handle_ShapeSphere,
    ck::FFragment_ShapeSphere_Params, ck::FFragment_ShapeSphere_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeSphere_UE::
    Request_ExampleRequest(
        FCk_Handle_ShapeSphere& InShapeSphere,
        const FCk_Request_ShapeSphere_ExampleRequest& InRequest)
    -> FCk_Handle_ShapeSphere
{
    InShapeSphere.AddOrGet<ck::FFragment_ShapeSphere_Requests>()._Requests.Emplace(InRequest);
    return InShapeSphere;
}

// --------------------------------------------------------------------------------------------------------------------