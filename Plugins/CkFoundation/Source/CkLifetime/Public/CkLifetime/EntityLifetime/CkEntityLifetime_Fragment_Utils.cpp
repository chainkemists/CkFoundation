#include "CkEntityLifetime_Fragment_Utils.h"

#include "CkLifetime/EntityLifetime/CkEntityLifetime_Fragment.h"

auto UCk_Utils_EntityLifetime_UE::
Request_DestroyEntity(FCk_Handle InHandle) -> void
{
    InHandle.Add<ck::FCk_Tag_TriggerDestroyEntity>();
}

auto UCk_Utils_EntityLifetime_UE::
Request_CreateEntity(FCk_Handle InHandle) -> FCk_Handle
{
    const auto& NewEntity = (*InHandle)->CreateEntity();
    InHandle.Add<ck::FCk_Tag_EntityJustCreated>(NewEntity);

    return HandleType{ NewEntity, **InHandle };
}

auto UCk_Utils_EntityLifetime_UE::
Request_CreateEntity(RegistryType& InRegistry) -> HandleType
{
    const auto& NewEntity = InRegistry.CreateEntity();
    InRegistry.Add<ck::FCk_Tag_EntityJustCreated>(NewEntity);

    return HandleType{ NewEntity, InRegistry };
}

auto UCk_Utils_EntityLifetime_UE::
Request_CreateEntity(RegistryType& InRegistry,
    const EntityIdHint& InEntityHint) -> HandleType
{
    const auto& NewEntity = InRegistry.CreateEntity(InEntityHint.Entity);
    InRegistry.Add<ck::FCk_Tag_EntityJustCreated>(NewEntity);

    return HandleType{ NewEntity, InRegistry };
}

auto UCk_Utils_EntityLifetime_UE::
Get_TransientEntity(RegistryType& InRegistry) -> HandleType
{
    return HandleType{InRegistry._TransientEntity, InRegistry};
}
