#include "CkRegistry.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Registry::
    FCk_Registry()
{
    CreateEntity();
}

auto
    FCk_Registry::
    CreateEntity() -> EntityType
{
    return _InternalRegistry->create();
}

auto
    FCk_Registry::
    CreateEntity(EntityType InEntityHint)
    -> EntityType
{
    return _InternalRegistry->create(InEntityHint.Get_ID());
}

auto FCk_Registry::DestroyEntity(EntityType InEntity) -> void
{
    _InternalRegistry->destroy(InEntity.Get_ID());
}

auto
    FCk_Registry::
    Get_ValidEntity(
        EntityType::IdType InEntity) const
    -> EntityType
{
    const auto Entity = EntityType{InEntity};
    CK_ENSURE_IF_NOT(IsValid(Entity), TEXT("Entity ID [{}] is NOT a valid ID for Registry [{}]"),
        static_cast<int32>(InEntity), *this)
    {  return EntityType{}; }

    return Entity;
}

auto
    FCk_Registry::
    IsValid(EntityType InEntity) const
    -> bool
{
    return _InternalRegistry->valid(InEntity.Get_ID());
}

// --------------------------------------------------------------------------------------------------------------------

auto
GetTypeHash(const FCk_Registry& InRegistry) -> uint32
{
    return GetTypeHash(InRegistry._InternalRegistry);
}

// --------------------------------------------------------------------------------------------------------------------
