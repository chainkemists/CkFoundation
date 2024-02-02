#include "CkEntityBridge_Utils.h"

#include "CkEntityBridge_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityBridge_UE::
    Request_Spawn(
        FCk_Handle InHandle,
        const FCk_Request_EntityBridge_SpawnEntity& InRequest)
    -> void
{
    // TODO: Same treatment as the SpawnActor
    auto& RequestComp = InHandle.AddOrGet<ck::FFragment_EntityBridge_Requests>();

    RequestComp._Requests.Emplace(InRequest);
}

// --------------------------------------------------------------------------------------------------------------------
