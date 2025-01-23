#include "CkEcsTemplate_Utils.h"

#include "CkEcsTemplate/CkEcsTemplate_Fragment.h"
#include "CkEcsTemplate/CkEcsTemplate_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EcsTemplate_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_EcsTemplate_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_EcsTemplate
{
    InHandle.Add<ck::FFragment_EcsTemplate_Params>(InParams);
    InHandle.Add<ck::FFragment_EcsTemplate_Current>();

    InHandle.Add<ck::FTag_EcsTemplate_RequiresSetup>();

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::ecs_template::VeryVerbose
        (
            TEXT("Skipping creation of EcsTemplate Rep Fragment on Entity [{}] because it's set to [{}]"),
            InHandle,
            InReplicates
        );

        return Cast(InHandle);
    }

    TryAddReplicatedFragment<UCk_Fragment_EcsTemplate_Rep>(InHandle);
    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_EcsTemplate_UE, FCk_Handle_EcsTemplate,
    ck::FFragment_EcsTemplate_Params, ck::FFragment_EcsTemplate_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EcsTemplate_UE::
    Request_ExampleRequest(
        FCk_Handle_EcsTemplate& InEcsTemplate,
        const FCk_Request_EcsTemplate_ExampleRequest& InRequest)
    -> FCk_Handle_EcsTemplate
{
    InEcsTemplate.AddOrGet<ck::FFragment_EcsTemplate_Requests>()._Requests.Emplace(InRequest);
    return InEcsTemplate;
}

// --------------------------------------------------------------------------------------------------------------------