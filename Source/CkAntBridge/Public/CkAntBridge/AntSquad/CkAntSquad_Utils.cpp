#include "CkAntSquad_Utils.h"

#include "CkAntSquad_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_AntSquad_UE, FCk_Handle_AntSquad, ck::FFragment_AntSquad_Current, ck::FFragment_AntSquad_Params)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AntSquad_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_AntSquad_ParamsData& InParams)
    -> FCk_Handle_AntSquad
{
    InHandle.Add<ck::FFragment_AntSquad_Params>(InParams);
    InHandle.Add<ck::FFragment_AntSquad_Current>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AntSquad_UE::
    Request_AddAgent(
        FCk_Handle_AntSquad& InAntSquadHandle,
        const FCk_Request_AntSquad_AddAgent& InRequest)
    -> FCk_Handle_AntSquad
{
    InAntSquadHandle.AddOrGet<ck::FFragment_AntSquad_Requests>()._Requests.Emplace(InRequest);
    return InAntSquadHandle;
}

// --------------------------------------------------------------------------------------------------------------------
