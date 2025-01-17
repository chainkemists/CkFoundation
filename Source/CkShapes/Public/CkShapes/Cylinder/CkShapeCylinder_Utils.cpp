#include "CkShapeCylinder_Utils.h"

#include "CkShapes/CkShapes_Log.h"
#include "CkShapes/Cylinder/CkShapeCylinder_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ShapeCylinder_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_ShapeCylinder_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_ShapeCylinder
{
    InHandle.Add<ck::FFragment_ShapeCylinder_Params>(InParams);
    InHandle.Add<ck::FFragment_ShapeCylinder_Current>();

	InHandle.Add<ck::FTag_ShapeCylinder_RequiresSetup>();

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::shapes::VeryVerbose
        (
            TEXT("Skipping creation of ShapeCylinder Rep Fragment on Entity [{}] because it's set to [{}]"),
            InHandle,
            InReplicates
        );

        return Cast(InHandle);
    }

    TryAddReplicatedFragment<UCk_Fragment_ShapeCylinder_Rep>(InHandle);
	return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_ShapeCylinder_UE, FCk_Handle_ShapeCylinder,
    ck::FFragment_ShapeCylinder_Params, ck::FFragment_ShapeCylinder_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_ShapeCylinder_UE::
	Request_ExampleRequest(
		FCk_Handle_ShapeCylinder& InShapeCylinder,
		const FCk_Request_ShapeCylinder_ExampleRequest& InRequest)
	-> FCk_Handle_ShapeCylinder
{
	InShapeCylinder.AddOrGet<ck::FFragment_ShapeCylinder_Requests>()._Requests.Emplace(InRequest);
    return InShapeCylinder;
}

// --------------------------------------------------------------------------------------------------------------------