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
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InLifetimeOwner);

    return Add(NewEntity, InEntityScript, InEntityScriptTemplate);
}

auto
    UCk_Utils_EntityScript_UE::
    Add(
        const FCk_Handle& InHandle,
        const TSubclassOf<UCk_EntityScript_UE>& InEntityScript,
        UCk_EntityScript_UE* InEntityScriptTemplate,
        FCk_EntityScript_PostConstruction_Func InOptionalFunc)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InEntityScript), TEXT("Invalid EntityScript supplied, cannot request to Spawn Entity"))
    { return {}; }

    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle);

    const auto Request = FCk_Request_EntityScript_SpawnEntity{
                             InHandle,
                             UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle),
                             InEntityScript
                         }
                         .Set_EntityScriptTemplate(InEntityScriptTemplate).Set_PostConstruction_Func(InOptionalFunc);

    RequestEntity.Add<ck::FFragment_EntityScript_RequestSpawnEntity>(Request);

    return InHandle;
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
