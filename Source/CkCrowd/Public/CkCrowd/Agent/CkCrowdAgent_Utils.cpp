#include "CkCrowdAgent_Utils.h"

#include "CkCrowd_Log.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_CrowdAgent_UE::
    Add(
        FCk_Handle& InOwner,
        const FCk_Fragment_CrowdAgent_ParamsData& InParams)
    -> FCk_Handle_CrowdAgent
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOwner),
        TEXT("Invalid owner handle [{}].{}"), InOwner, ck::Context(this))
    { return {}; }

    auto NewAgentEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_CrowdAgent>(InOwner);

    NewAgentEntity.Add<ck::FFragment_CrowdAgent_Params>(InParams);
    NewAgentEntity.Add<ck::FTag_CrowdAgent_NeedsSetup>();

    ck::crowd::Verbose(TEXT("CrowdAgent added to [{}] -> [{}] (radius={}, height={})"),
        InOwner, NewAgentEntity, InParams.Get_Radius(), InParams.Get_Height());

    return NewAgentEntity;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_CrowdAgent_UE, FCk_Handle_CrowdAgent, ck::FFragment_CrowdAgent_Params);

// --------------------------------------------------------------------------------------------------------------------
