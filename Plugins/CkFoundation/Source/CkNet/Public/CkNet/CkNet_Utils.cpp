#include "CkNet_Utils.h"

#include "CkNet_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Net_UE::
    Get_IsEntityNetMode_Server(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has_Any<ck::FCk_Tag_NetMode_DedicatedServer>();
}

auto
    UCk_Utils_Net_UE::
    Get_IsEntityNetMode_Client(
        FCk_Handle InHandle)
    -> bool
{
    return NOT InHandle.Has_Any<ck::FCk_Tag_NetMode_DedicatedServer>();
}

auto
    UCk_Utils_Net_UE::
    Request_MarkEntityAs_DedicatedServer(
        FCk_Handle InHandle)
    -> void
{
    InHandle.Add<ck::FCk_Tag_NetMode_DedicatedServer>();
}

// --------------------------------------------------------------------------------------------------------------------

