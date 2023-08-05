#include "CkRegistry.h"

// --------------------------------------------------------------------------------------------------------------------

void foo()
{
    FCk_Registry r;
    FCk_Entity e;
    r.Add<int>(e);
}

// --------------------------------------------------------------------------------------------------------------------

auto FCk_Registry::CreateEntity() -> EntityType
{
    return _InternalRegistry->create();
}

auto FCk_Registry::CreateEntity(EntityType InEntityHint) -> EntityType
{
    return _InternalRegistry->create(InEntityHint.Get_ID());
}

auto FCk_Registry::DestroyEntity(EntityType InEntity) -> void
{
    _InternalRegistry->destroy(InEntity.Get_ID());
}

auto FCk_Registry::IsValid(EntityType InEntity) -> bool
{
    return _InternalRegistry->valid(InEntity.Get_ID());
}

// --------------------------------------------------------------------------------------------------------------------
