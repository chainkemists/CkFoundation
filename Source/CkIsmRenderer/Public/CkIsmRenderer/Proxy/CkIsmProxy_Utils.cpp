#include "CkIsmProxy_Utils.h"

#include "CkIsmRenderer/Proxy/CkIsmProxy_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_IsmProxy_UE, FCk_Handle_IsmProxy, ck::FFragment_IsmProxy_Params)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IsmProxy_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_IsmProxy_ParamsData& InParams)
    -> FCk_Handle_IsmProxy
{
    InHandle.Add<ck::FFragment_IsmProxy_Params>(InParams);
    InHandle.Add<ck::FTag_IsmProxy_NeedsSetup>();

    if (InParams.Get_Mobility() == ECk_Mobility::Movable)
    { InHandle.Add<ck::FTag_IsmProxy_Dynamic>(); }

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------
