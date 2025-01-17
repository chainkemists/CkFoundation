#include "CkShapeCapsule_Utils.h"

#include "CkShapes/CkShapes_Log.h"
#include "CkShapes/Capsule/CkShapeCapsule_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeCapsule_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_ShapeCapsule_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_ShapeCapsule
{
    InHandle.Add<ck::FFragment_ShapeCapsule_Params>(InParams);
    InHandle.Add<ck::FFragment_ShapeCapsule_Current>();

    InHandle.Add<ck::FTag_ShapeCapsule_RequiresSetup>();

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::shapes::VeryVerbose
        (
            TEXT("Skipping creation of ShapeCapsule Rep Fragment on Entity [{}] because it's set to [{}]"),
            InHandle,
            InReplicates
        );

        return Cast(InHandle);
    }

    TryAddReplicatedFragment<UCk_Fragment_ShapeCapsule_Rep>(InHandle);
    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_ShapeCapsule_UE, FCk_Handle_ShapeCapsule,
    ck::FFragment_ShapeCapsule_Params, ck::FFragment_ShapeCapsule_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeCapsule_UE::
    Request_ExampleRequest(
        FCk_Handle_ShapeCapsule& InShapeCapsule,
        const FCk_Request_ShapeCapsule_ExampleRequest& InRequest)
    -> FCk_Handle_ShapeCapsule
{
    InShapeCapsule.AddOrGet<ck::FFragment_ShapeCapsule_Requests>()._Requests.Emplace(InRequest);
    return InShapeCapsule;
}

// --------------------------------------------------------------------------------------------------------------------