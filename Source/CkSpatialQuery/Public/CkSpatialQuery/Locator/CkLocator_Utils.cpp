#include "CkLocator_Utils.h"

#include "CkSpatialQuery/CkSpatialQuery_Log.h"
#include "CkSpatialQuery/Locator/CkLocator_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Locator_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Locator_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_Locator
{
    InHandle.Add<ck::FFragment_Locator_Params>(InParams);
    InHandle.Add<ck::FFragment_Locator_Current>();

	InHandle.Add<ck::FTag_Locator_RequiresSetup>();

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::spatialquery::VeryVerbose
        (
            TEXT("Skipping creation of Locator Rep Fragment on Entity [{}] because it's set to [{}]"),
            InHandle,
            InReplicates
        );

        return Cast(InHandle);
    }

    TryAddReplicatedFragment<UCk_Fragment_Locator_Rep>(InHandle);
	return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Locator_UE, FCk_Handle_Locator,
    ck::FFragment_Locator_Params, ck::FFragment_Locator_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Utils_Locator_UE::
	Request_ExampleRequest(
		FCk_Handle_Locator& InLocator,
		const FCk_Request_Locator_ExampleRequest& InRequest)
	-> FCk_Handle_Locator
{
	InLocator.AddOrGet<ck::FFragment_Locator_Requests>()._Requests.Emplace(InRequest);
    return InLocator;
}

// --------------------------------------------------------------------------------------------------------------------