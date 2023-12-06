#include "CkEntityBridge_Utils.h"

#include "CkEntityBridge_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityBridge_UE::
    Request_Spawn(
        FCk_Handle InHandle,
        const FCk_Request_EntityBridge_SpawnEntity& InRequest) -> void
{
    // TODO: Same treatment as the SpawnActor
    auto& RequestComp = InHandle.AddOrGet<ck::FFragment_EntityBridge_Requests>();

    RequestComp._Requests.Emplace(InRequest);
}

auto
    UCk_Utils_EntityBridge_UE::
    BuildEntity(
        FCk_Handle InHandle,
        const UCk_EntityBridge_Config_Base_PDA* InEntityConfig)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InEntityConfig),
        TEXT("InEntityConfig is INVALID. Cannot build Entity for Handle [{}]"), InHandle)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InHandle),
        TEXT("Handle is INVALID. Unable to build entity for [{}]"), InEntityConfig)
    { return {}; }

    const auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);
    InEntityConfig->Build(NewEntity);

    return NewEntity;
}

// --------------------------------------------------------------------------------------------------------------------
