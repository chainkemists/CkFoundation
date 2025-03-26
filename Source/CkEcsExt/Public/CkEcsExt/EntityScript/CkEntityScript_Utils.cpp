#include "CkEntityScript_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcsExt/EntityScript/CkEntityScript_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityScript_UE::
    Request_SpawnEntity(
        FCk_Handle& InLifetimeOwner,
        UCk_EntityScript_UE* InEntityScript)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InEntityScript), TEXT("Invalid EntityScript supplied, cannot request to Spawn Entity"))
    { return {}; }

    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InLifetimeOwner);
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InLifetimeOwner);

    const auto Request = FCk_Request_EntityScript_SpawnEntity{InEntityScript->GetClass(), NewEntity}
                            .Set_EntityScriptTemplate(InEntityScript)
                            .Set_Owner(InLifetimeOwner);

    RequestEntity.Add<ck::FFragment_EntityScript_RequestSpawnEntity>(Request);

    return NewEntity;
}

auto
    UCk_Utils_EntityScript_UE::
    DoAdd(
        FCk_Handle& InHandle,
        UCk_EntityScript_UE* InEntityScript)
    -> void
{
    InHandle.Add<ck::FFragment_EntityScript_Current>(InEntityScript);
}

auto
    UCk_Utils_EntityScript_UE::
    Request_MarkEntityScript_AsHasBegunPlay(
        FCk_Handle& InHandle)
    -> void
{
    InHandle.Add<ck::FTag_EntityScript_HasBegunPlay>();
}

// --------------------------------------------------------------------------------------------------------------------
