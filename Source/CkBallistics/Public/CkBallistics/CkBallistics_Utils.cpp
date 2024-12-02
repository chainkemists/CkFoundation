#include "CkBallistics_Utils.h"

#include "CkBallistics/CkBallistics_Fragment.h"
#include "CkBallistics/CkBallistics_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Ballistics_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_Ballistics_ParamsData& InParams)
    -> FCk_Handle_Ballistics
{
    CK_ENSURE_IF_NOT(UCk_Utils_OwningActor_UE::Has(InHandle),
        TEXT("Handle [{}] MUST be an Actor Handle"), InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_Ballistics_Params>(InParams);
    InHandle.Add<ck::FFragment_Ballistics_Current>();

    InHandle.Add<ck::FTag_Ballistics_RequiresSetup>();

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Ballistics_UE, FCk_Handle_Ballistics,
    ck::FFragment_Ballistics_Params, ck::FFragment_Ballistics_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Ballistics_UE::
    Request_ExampleRequest(
        FCk_Handle_Ballistics& InBallistics,
        const FCk_Request_Ballistics_ExampleRequest& InRequest)
    -> FCk_Handle_Ballistics
{
    InBallistics.AddOrGet<ck::FFragment_Ballistics_Requests>()._Requests.Emplace(InRequest);
    return InBallistics;
}

// --------------------------------------------------------------------------------------------------------------------