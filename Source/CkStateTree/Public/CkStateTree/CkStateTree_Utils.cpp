#include "CkStateTree_Utils.h"

#include "CkStateTree/CkStateTree_Fragment.h"
#include "CkStateTree/CkStateTree_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateTree_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_StateTree_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_StateTree
{
    InHandle.Add<ck::FFragment_StateTree_Params>(InParams);
    InHandle.Add<ck::FFragment_StateTree_Current>();

    InHandle.Add<ck::FTag_StateTree_RequiresSetup>();

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::statetree::VeryVerbose
        (
            TEXT("Skipping creation of StateTree Rep Fragment on Entity [{}] because it's set to [{}]"),
            InHandle,
            InReplicates
        );

        return Cast(InHandle);
    }

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_StateTree_UE, FCk_Handle_StateTree,
    ck::FFragment_StateTree_Params, ck::FFragment_StateTree_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_StateTree_UE::
    Request_ExampleRequest(
        FCk_Handle_StateTree& InStateTree,
        const FCk_Request_StateTree_ExampleRequest& InRequest)
    -> FCk_Handle_StateTree
{
    InStateTree.AddOrGet<ck::FFragment_StateTree_Requests>()._Requests.Emplace(InRequest);
    return InStateTree;
}

// --------------------------------------------------------------------------------------------------------------------