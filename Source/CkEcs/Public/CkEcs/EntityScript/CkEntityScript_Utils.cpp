#include "CkEntityScript_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityScript_UE::
    DoRequest_SpawnEntity(
        FCk_Handle& InLifetimeOwner,
        UCk_EntityScript_UE* InEntityScript)
    -> FCk_Handle
{
    return Request_SpawnEntity(InLifetimeOwner, InEntityScript->GetClass(), InEntityScript);
}

auto
    UCk_Utils_EntityScript_UE::
    Request_SpawnEntity(
        const FCk_Handle& InLifetimeOwner,
        const TSubclassOf<UCk_EntityScript_UE>& InEntityScript,
        UCk_EntityScript_UE* InEntityScriptTemplate)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InEntityScript), TEXT("Invalid EntityScript supplied, cannot request to Spawn Entity"))
    { return {}; }

    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InLifetimeOwner);
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InLifetimeOwner);

    const auto Request = FCk_Request_EntityScript_SpawnEntity{NewEntity, InLifetimeOwner, InEntityScript}
                            .Set_EntityScriptTemplate(InEntityScriptTemplate);

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
