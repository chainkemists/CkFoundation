#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedCompType>
    class TUtils_EntityHolder
    {
    public:
        using CompType = T_DerivedCompType;
        using EntityType = typename CompType::EntityType;
        using HandleType = FCk_Handle;

    public:
        static auto
        AddOrReplace(
            HandleType& InHandle,
            const EntityType& InEntityToStore) -> void;

        static auto
        Clear(HandleType& InHandle) -> void;

        static auto
        Has(
            const HandleType& InHandle) -> bool;

        static auto
        Ensure(
            const HandleType& InHandle) -> bool;

        static auto
        Get_StoredEntity(
            const HandleType& InHandle) -> EntityType;

        template <typename T_TypeSafeHandle>
        static auto
        Get_StoredEntity_AsTypeSafe(
            const HandleType& InHandle) -> T_TypeSafeHandle;
    };
}

#define CK_DEFINE_ENTITY_HOLDER_UTILS(_UtilsName_, _NameOfEntityHolder_)\
    using _UtilsName_ = ck::TUtils_EntityHolder<_NameOfEntityHolder_>

// Policy-blind define is removed — use the explicit-policy variant. Hard error if used.
#define CK_DEFINE_ENTITY_HOLDER_AND_UTILS(_UtilsName_, _NameOfEntityHolder_, _HandleType_)                 \
    static_assert(false, "CK_DEFINE_ENTITY_HOLDER_AND_UTILS is removed: choose "                           \
        "CK_DEFINE_ENTITY_HOLDER_AND_UTILS_ROUNDTRIP (authoritative — also add CK_REGISTER_SNAPSHOTABLE) " \
        "or CK_DEFINE_ENTITY_HOLDER_AND_UTILS_TRANSIENT (reconstructed on restore).")

#define CK_DEFINE_ENTITY_HOLDER_AND_UTILS_ROUNDTRIP(_UtilsName_, _NameOfEntityHolder_, _HandleType_)\
    CK_DEFINE_ENTITY_HOLDER_ROUNDTRIP(_NameOfEntityHolder_, _HandleType_);                          \
    CK_DEFINE_ENTITY_HOLDER_UTILS(_UtilsName_, _NameOfEntityHolder_)

#define CK_DEFINE_ENTITY_HOLDER_AND_UTILS_TRANSIENT(_UtilsName_, _NameOfEntityHolder_, _HandleType_)\
    CK_DEFINE_ENTITY_HOLDER_TRANSIENT(_NameOfEntityHolder_, _HandleType_);                          \
    CK_DEFINE_ENTITY_HOLDER_UTILS(_UtilsName_, _NameOfEntityHolder_)

// --------------------------------------------------------------------------------------------------------------------
// Definitions

namespace ck
{
    template <typename T_DerivedCompType>
    auto
        TUtils_EntityHolder<T_DerivedCompType>::
        AddOrReplace(
            HandleType& InHandle,
            const EntityType& InEntityToStore)
        -> void
    {
        auto& Fragment = InHandle.AddOrGet<CompType>();
        Fragment._Entity = InEntityToStore;
    }

    template <typename T_DerivedCompType>
    auto 
        TUtils_EntityHolder<T_DerivedCompType>::
        Clear(
            HandleType& InHandle) 
        -> void
    {
        auto& Fragment = InHandle.AddOrGet<CompType>();
        Fragment._Entity = EntityType{};
    }

    template <typename T_DerivedCompType>
    auto
        TUtils_EntityHolder<T_DerivedCompType>::
        Has(
            const HandleType& InHandle)
        -> bool
    {
        return InHandle.Has<CompType>();
    }

    template <typename T_DerivedCompType>
    auto
        TUtils_EntityHolder<T_DerivedCompType>::
        Ensure(
            const HandleType& InHandle)
        -> bool
    {
        CK_ENSURE_IF_NOT(Has(InHandle), TEXT("Handle [{}] does NOT have an EntityHolder"), InHandle)
        { return false; }

        return true;
    }

    template <typename T_DerivedCompType>
    auto
        TUtils_EntityHolder<T_DerivedCompType>::
        Get_StoredEntity(
            const HandleType& InHandle)
        -> EntityType
    {
        if (NOT Ensure(InHandle))
        { return {}; }

        const auto& EntityHolderComp = InHandle.Get<CompType>();
        const auto& StoredEntity     = EntityHolderComp.Get_Entity();

        return StoredEntity;
    }

    template <typename T_DerivedCompType>
    template <typename T_TypeSafeHandle>
    auto
        TUtils_EntityHolder<T_DerivedCompType>::
        Get_StoredEntity_AsTypeSafe(
            const HandleType& InHandle)
        -> T_TypeSafeHandle
    {
        auto StoredEntity = Get_StoredEntity(InHandle);

        return ck::StaticCast<T_TypeSafeHandle>(StoredEntity);
    }
}

// --------------------------------------------------------------------------------------------------------------------
