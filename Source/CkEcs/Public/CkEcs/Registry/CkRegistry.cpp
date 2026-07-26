#include "CkRegistry.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

// --------------------------------------------------------------------------------------------------------------------
// Convention for a null Resolve() (unset or stale slot-table handle): state-mutating methods fire
// CK_ENSURE_IF_NOT; read-only methods return defaults silently.
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Registry::
    Get_DirtyMarkerVersion(
        uint32 InFragmentTypeHash) const
    -> uint64
{
    return ck::registry_table::Get_DirtyMarkerVersion(_RegistryHandle, InFragmentTypeHash);
}

auto
    FCk_Registry::
    BumpDirtyMarkerVersion(
        uint32 InFragmentTypeHash)
    -> void
{
    ck::registry_table::BumpDirtyMarkerVersion(_RegistryHandle, InFragmentTypeHash);
}

auto
    FCk_Registry::
    CreateEntity()
    -> EntityType
{
#if !UE_BUILD_SHIPPING
    ck::registry_table::AssertNotInParallelRegion(_RegistryHandle, TEXT("Registry::CreateEntity"));
#endif

    auto* Reg = Resolve();
    CK_ENSURE_IF_NOT(Reg != nullptr,
        TEXT("FCk_Registry::CreateEntity: registry handle is stale or unset"))
    { return EntityType{}; }

    const auto EntityFromEntt = Reg->create();
    const auto CreatedEntity = EntityType{EntityFromEntt};

    CK_ENSURE_IF_NOT(Reg->orphan(CreatedEntity.Get_ID()),
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
    ck::registry_table::AssertNotInParallelRegion(_RegistryHandle, TEXT("Registry::CreateEntity"));
#endif

    auto* Reg = Resolve();
    CK_ENSURE_IF_NOT(Reg != nullptr,
        TEXT("FCk_Registry::CreateEntity(hint): registry handle is stale or unset"))
    { return EntityType{}; }

    const auto EntityFromEntt = Reg->create(InEntityHint.Get_ID());
    const auto CreatedEntity = EntityType{EntityFromEntt};

    CK_ENSURE_IF_NOT(Reg->orphan(CreatedEntity.Get_ID()),
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
    ck::registry_table::AssertNotInParallelRegion(_RegistryHandle, TEXT("Registry::DestroyEntity"));
#endif

    auto* Reg = Resolve();
    CK_ENSURE_IF_NOT(Reg != nullptr,
        TEXT("FCk_Registry::DestroyEntity: registry handle is stale or unset"))
    { return; }

    Reg->destroy(InEntity.Get_ID());
}

auto
    FCk_Registry::
    DestroyEntities(
        const TArray<EntityType>& InEntities)
    -> void
{
#if !UE_BUILD_SHIPPING
    ck::registry_table::AssertNotInParallelRegion(_RegistryHandle, TEXT("Registry::DestroyEntities"));
#endif

    auto* Reg = Resolve();
    CK_ENSURE_IF_NOT(Reg != nullptr,
        TEXT("FCk_Registry::DestroyEntities: registry handle is stale or unset"))
    { return; }

    const auto& EntityIDs = ck::algo::Transform<TArray<EntityType::IdType>>(InEntities,
        [](const EntityType& Entity)
    {
        return Entity.Get_ID();
    });
    Reg->destroy(EntityIDs.begin(), EntityIDs.end());
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
    const auto* Reg = Resolve();
    CK_ENSURE_IF_NOT(Reg != nullptr,
        TEXT("InternalRegistry is Invalid. Cannot determine whether Entity [{}] is valid or not"),
        InEntity)
    { return false; }

    return Reg->valid(InEntity.Get_ID());
}

auto
    FCk_Registry::
    Orphan(
        EntityType InEntity) const
    -> bool
{
    const auto* Reg = Resolve();
    if (Reg == nullptr) { return true; }
    return Reg->orphan(InEntity.Get_ID());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    GetTypeHash(
        const FCk_Registry& InRegistry)
    -> uint32
{
    const auto& Handle = InRegistry.Get_RegistryHandle();
    return HashCombine(::GetTypeHash(Handle.SlotIndex), ::GetTypeHash(Handle.Generation));
}

// --------------------------------------------------------------------------------------------------------------------
