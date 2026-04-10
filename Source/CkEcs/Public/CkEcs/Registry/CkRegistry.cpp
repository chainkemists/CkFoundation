#include "CkRegistry.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Registry::
    FCk_Registry()
    : _DirtyMarkerVersions(MakeShared<TMap<uint32, uint64>>())
{
    _TransientEntity = CreateEntity();
}

FCk_Registry::
    FCk_Registry(const FCk_Registry& InOther)
    : _InternalRegistry(InOther._InternalRegistry)
    , _TransientEntity(InOther._TransientEntity)
    , _DirtyMarkerVersions(InOther._DirtyMarkerVersions)
{
    // _IsInParallelRegion deliberately NOT copied — copies start outside parallel regions
}

FCk_Registry::
    FCk_Registry(FCk_Registry&& InOther) noexcept
    : _InternalRegistry(MoveTemp(InOther._InternalRegistry))
    , _TransientEntity(MoveTemp(InOther._TransientEntity))
    // TSharedRef doesn't support being emptied, so moving is a ref-count copy. The moved-from
    // registry is expected to be discarded so double-sharing the map is harmless.
    , _DirtyMarkerVersions(InOther._DirtyMarkerVersions)
{
}

auto
    FCk_Registry::
    operator=(const FCk_Registry& InOther)
    -> FCk_Registry&
{
    if (this != &InOther)
    {
        _InternalRegistry = InOther._InternalRegistry;
        _TransientEntity = InOther._TransientEntity;
        _DirtyMarkerVersions = InOther._DirtyMarkerVersions;
    }
    return *this;
}

auto
    FCk_Registry::
    operator=(FCk_Registry&& InOther) noexcept
    -> FCk_Registry&
{
    if (this != &InOther)
    {
        _InternalRegistry = MoveTemp(InOther._InternalRegistry);
        _TransientEntity = MoveTemp(InOther._TransientEntity);
        _DirtyMarkerVersions = InOther._DirtyMarkerVersions;
    }
    return *this;
}

auto
    FCk_Registry::
    Get_DirtyMarkerVersion(
        uint32 InFragmentTypeHash) const
    -> uint64
{
    const auto* Found = _DirtyMarkerVersions->Find(InFragmentTypeHash);
    return Found != nullptr ? *Found : uint64{0};
}

auto
    FCk_Registry::
    CreateEntity()
    -> EntityType
{
#if !UE_BUILD_SHIPPING
    AssertNotInParallelRegion(TEXT("Registry::CreateEntity"));
#endif

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
    CreateEntity(
        EntityType InEntityHint)
    -> EntityType
{
#if !UE_BUILD_SHIPPING
    AssertNotInParallelRegion(TEXT("Registry::CreateEntity"));
#endif

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
    DestroyEntity(
        EntityType InEntity)
    -> void
{
#if !UE_BUILD_SHIPPING
    AssertNotInParallelRegion(TEXT("Registry::DestroyEntity"));
#endif

    _InternalRegistry->destroy(InEntity.Get_ID());
}

auto
    FCk_Registry::
    DestroyEntities(
        const TArray<EntityType>& InEntities)
    -> void
{
#if !UE_BUILD_SHIPPING
    AssertNotInParallelRegion(TEXT("Registry::DestroyEntities"));
#endif
    const auto& EntityIDs = ck::algo::Transform<TArray<EntityType::IdType>>(InEntities,
        [](const EntityType& Entity)
    {
        return Entity.Get_ID();
    });
    _InternalRegistry->destroy(EntityIDs.begin(), EntityIDs.end());
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
    IsValid(
        EntityType InEntity) const
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