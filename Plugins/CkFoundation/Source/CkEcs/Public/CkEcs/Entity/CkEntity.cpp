#include "CkEntity.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Entity FCk_Entity::_Tombstone = FCk_Entity::TombstoneType{};

// --------------------------------------------------------------------------------------------------------------------

FCk_Entity::
FCk_Entity(IdType InID)
    : _ID(InID)
#if WITH_EDITORONLY_DATA
    , _EntityID(static_cast<int32>(Get_ID()))
    , _EntityNumber(Get_EntityNumber())
    , _EntityVersion(Get_VersionNumber())
#endif
{
}

FCk_Entity::FCk_Entity(TombstoneType)
    : FCk_Entity(IdType{ entt::tombstone_t{} })
{
}

auto FCk_Entity::
operator=(IdType InOther) noexcept -> ThisType&
{
    *this = FCk_Entity{InOther};
    return *this;
}

auto FCk_Entity::
operator=(TombstoneType InOther) noexcept
    -> ThisType&
{
    *this = FCk_Entity{InOther};
    return *this;
}

auto FCk_Entity::
operator==(ThisType InOther) const -> bool
{
    return Get_ID() == InOther.Get_ID();
}

auto FCk_Entity::
operator<(ThisType InOther) const -> bool
{
    return Get_ID() < InOther.Get_ID();
}

auto FCk_Entity::
Get_IsTombstone() const -> bool
{
    return operator==(Tombstone());
}

auto FCk_Entity::
Get_EntityNumber() const -> EntityNumberType
{
    return entt::entt_traits<IdType>::to_entity(_ID);
}

auto FCk_Entity::
Get_VersionNumber() const -> VersionNumberType
{
    return entt::entt_traits<IdType>::to_version(_ID);
}

auto FCk_Entity::
Tombstone() -> FCk_Entity
{
    return _Tombstone;
}

auto
    GetTypeHash(
        FCk_Entity InEntity)
    -> uint32
{
    return GetTypeHash(InEntity.Get_ID());
}

// --------------------------------------------------------------------------------------------------------------------
