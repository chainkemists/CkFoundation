#include "CkEntityScript_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/EntityScript/CkEntityScript_Fragment_Data.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityScript_UE::
    DoRequest_SpawnEntity(
        FCk_Handle& InLifetimeOwner,
        UCk_EntityScript_UE* InEntityScript,
        FInstancedStruct InSpawnParams)
    -> FCk_Handle
{
    return Request_SpawnEntity(InLifetimeOwner, InEntityScript->GetClass(), InSpawnParams, InEntityScript);
}

auto
    UCk_Utils_EntityScript_UE::
    Request_SpawnEntity(
        const FCk_Handle& InLifetimeOwner,
        const TSubclassOf<UCk_EntityScript_UE>& InEntityScript,
        const FInstancedStruct& InSpawnParams,
        UCk_EntityScript_UE* InEntityScriptTemplate)
    -> FCk_Handle
{
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InLifetimeOwner);

    return Add(NewEntity, InEntityScript, InSpawnParams, InEntityScriptTemplate);
}

auto
    UCk_Utils_EntityScript_UE::
    Add(
        FCk_Handle& InScriptEntity,
        const TSubclassOf<UCk_EntityScript_UE>& InEntityScript,
        const FInstancedStruct& InSpawnParams,
        UCk_EntityScript_UE* InEntityScriptTemplate,
        FCk_EntityScript_PostConstruction_Func InOptionalFunc)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InEntityScript), TEXT("Invalid EntityScript supplied, cannot request to Spawn Entity"))
    { return {}; }

    UCk_Utils_Handle_UE::Set_DebugName(InScriptEntity, *ck::Format_UE(TEXT("{}"), InEntityScript));

    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InScriptEntity);

    const auto Request = FCk_Request_EntityScript_SpawnEntity{
                             InScriptEntity,
                             UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InScriptEntity),
                             InEntityScript}
                        .Set_EntityScriptTemplate(InEntityScriptTemplate)
                        .Set_SpawnParams(InSpawnParams)
                        .Set_PostConstruction_Func(InOptionalFunc);

    RequestEntity.Add<ck::FFragment_EntityScript_RequestSpawnEntity>(Request);

    return InScriptEntity;
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
