#include "CkRegistry.h"

// --------------------------------------------------------------------------------------------------------------------

void foo()
{
    // TODO: transfer to spec file and flesh out the tests
    FCk_Registry r;
    FCk_Entity e;
    r.Add<int>(e);

    struct EmptyStruct {};
    struct Struct {int32 i;};

    using FViewType = decltype(r.View<Struct , EmptyStruct, ck::TExclude<Struct>>());
    using FComponentsAndTags = FViewType::FragmentsAndTags;
    using FOnlyComponents = FViewType::OnlyFragments;
    using ExcludesOnly = FViewType::FragmentsOnly<Struct, EmptyStruct>;

    r.View<int32, float>().ForEach([&](FCk_Entity, int32, float)
    {
    });
}

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
