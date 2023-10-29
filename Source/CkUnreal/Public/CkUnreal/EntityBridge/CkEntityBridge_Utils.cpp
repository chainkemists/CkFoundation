#include "CkEntityBridge_Utils.h"

#include "CkEntityBridge_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityBridge_UE::
    Request_Spawn(
        FCk_Handle InHandle,
        const FCk_Request_EntityBridge_SpawnEntity& InRequest) -> void
{
    auto& RequestComp = InHandle.Add<ck::FFragment_EntityBridge_Requests>();

    RequestComp._Requests.Emplace(InRequest);
}

// --------------------------------------------------------------------------------------------------------------------
