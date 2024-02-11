#include "CkRegistry.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Registry::
    FCk_Registry()
{
    _TransientEntity = CreateEntity();
}

auto
    FCk_Registry::
    CreateEntity()
    -> EntityType
{
    const auto EntityFromEntt = _InternalRegistry->create();
    const auto CreatedEntity = EntityType{EntityFromEntt};

    CK_ENSURE_IF_NOT(_InternalRegistry->orphan(CreatedEntity.Get_ID()),
        TEXT("The Newly Created Entity with ID [{}] still HAS components attached. We have likely RUN OUT of Entities"),
        static_cast<int32>(CreatedEntity.Get_ID()))
    { return {}; }

    return CreatedEntity;
}

auto
    FCk_Registry::
    CreateEntity(EntityType InEntityHint)
    -> EntityType
{
    const auto EntityFromEntt = _InternalRegistry->create(InEntityHint.Get_ID());
    const auto CreatedEntity = EntityType{EntityFromEntt};

    CK_ENSURE_IF_NOT(_InternalRegistry->orphan(CreatedEntity.Get_ID()),
        TEXT("The Newly Created Entity with ID [{}] through HINTING still HAS components attached. We have likely RUN OUT of Entities"),
        static_cast<int32>(CreatedEntity.Get_ID()))
    { return {}; }

    return CreatedEntity;
}

auto
    FCk_Registry::
    DestroyEntity(EntityType InEntity)
    -> void
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
        static_cast<int32>(InEntity),
        *this)
    {  return EntityType{}; }

    return Entity;
}

auto
    FCk_Registry::
    IsValid(EntityType InEntity) const
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(_InternalRegistry),
        TEXT("InternalRegistry is Invalid. Cannot determine whether Entity [{}] is valid or not"),
        InEntity)
    { return false; }

    return _InternalRegistry->valid(InEntity.Get_ID());
}

auto
    FCk_Registry::
    Orphan(
        EntityType InEntity) const
    -> bool
{
    return _InternalRegistry->orphan(InEntity.Get_ID());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    GetTypeHash(
        const FCk_Registry& InRegistry)
    -> uint32
{
    return GetTypeHash(InRegistry._InternalRegistry);
}

// --------------------------------------------------------------------------------------------------------------------
