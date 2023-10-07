#include "CkEntityLifetime_Fragment_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EntityLifetime_UE::
    Request_DestroyEntity(
        FCk_Handle InHandle)
    -> void
{
    // TODO: Fix for singleplayer
    if (UCk_Utils_ReplicatedObjects_UE::Get_NetRole(InHandle) != ROLE_Authority)
    { return; }

    InHandle.Add<ck::FTag_TriggerDestroyEntity>();
}

auto
    UCk_Utils_EntityLifetime_UE::
    Request_CreateEntity(
        FCk_Handle InHandle)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Cannot create Entity with Invalid Handle"))
    { return {}; }

    auto NewEntity = Request_CreateEntity(**InHandle);
    NewEntity.Add<ck::FFragment_LifetimeOwner>(InHandle);
    return NewEntity;
}

auto
    UCk_Utils_EntityLifetime_UE::
    Get_LifetimeOwner(
        FCk_Handle InHandle)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(InHandle.Has<ck::FFragment_LifetimeOwner>(),
        TEXT("The Entity [{}] does NOT have a LifetimeOwner. Was this Entity created by Request_CreateEntity(RegistryType)?"),
        InHandle)
    { return {}; }

    return InHandle.Get<ck::FFragment_LifetimeOwner>().Get_Entity();
}

auto
    UCk_Utils_EntityLifetime_UE::
    Get_IsPendingDestroy(FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has_Any<ck::FTag_TriggerDestroyEntity, ck::FTag_PendingDestroyEntity>();
}

auto
    UCk_Utils_EntityLifetime_UE::
    Get_TransientEntity( FCk_Handle InHandle)
    -> FCk_Handle
{
    return Get_TransientEntity(**InHandle);
}

auto
    UCk_Utils_EntityLifetime_UE::
    Request_CreateEntity(
        RegistryType& InRegistry)
    -> HandleType
{
    const auto& NewEntity = InRegistry.CreateEntity();
    InRegistry.Add<ck::FTag_EntityJustCreated>(NewEntity);

    return HandleType{ NewEntity, InRegistry };
}

auto
    UCk_Utils_EntityLifetime_UE::
    Request_CreateEntity(
        RegistryType& InRegistry,
        const EntityIdHint& InEntityHint)
    -> HandleType
{
    const auto& NewEntity = InRegistry.CreateEntity(InEntityHint.Get_Entity());
    InRegistry.Add<ck::FTag_EntityJustCreated>(NewEntity);

    return HandleType{ NewEntity, InRegistry };
}

auto
    UCk_Utils_EntityLifetime_UE::
    Get_TransientEntity(
        const RegistryType& InRegistry)
    -> HandleType
{
    return HandleType{InRegistry.Get_TransientEntity(), InRegistry};
}

// --------------------------------------------------------------------------------------------------------------------
