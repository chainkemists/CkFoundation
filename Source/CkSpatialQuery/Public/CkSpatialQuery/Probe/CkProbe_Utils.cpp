#include "CkProbe_Utils.h"

#include "CkSpatialQuery/CkSpatialQuery_Log.h"
#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Probe_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Probe_ParamsData& InParams)
    -> FCk_Handle_Probe
{
    InHandle.Add<ck::FFragment_Probe_Params>(InParams);
    InHandle.Add<ck::FFragment_Probe_Current>();

    InHandle.Add<ck::FTag_Probe_RequiresSetup>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Probe_UE, FCk_Handle_Probe,
    ck::FFragment_Probe_Params, ck::FFragment_Probe_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Probe_UE::
    Request_ExampleRequest(
        FCk_Handle_Probe& InProbe,
        const FCk_Request_Probe_ExampleRequest& InRequest)
    -> FCk_Handle_Probe
{
    InProbe.AddOrGet<ck::FFragment_Probe_Requests>()._Requests.Emplace(InRequest);
    return InProbe;
}

// --------------------------------------------------------------------------------------------------------------------