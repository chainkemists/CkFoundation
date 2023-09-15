#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcsBasics/EntityHolder/CkEntityHolder_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedCompType = FFragment_EntityHolder>
    class TUtils_EntityHolder
    {
    public:
        using CompType = T_DerivedCompType;
        using EntityType = typename CompType::EntityType;
        using HandleType = FCk_Handle;

    public:
        static auto Add(
            HandleType InHandle,
            EntityType InEntityToStore) -> void;

        static auto Has(
            HandleType InHandle) -> bool;

        static auto Ensure(
            HandleType InHandle) -> bool;

        static auto Get_StoredEntity(
            HandleType InHandle) -> HandleType;
    };
}

// --------------------------------------------------------------------------------------------------------------------
// Definitions

namespace ck
{
    template <typename T_DerivedCompType>
    auto
        TUtils_EntityHolder<T_DerivedCompType>::
        Add(
            HandleType InHandle,
            EntityType InEntityToStore)
        -> void
    {
        InHandle.Add<CompType>(InEntityToStore);
    }

    template <typename T_DerivedCompType>
    auto
        TUtils_EntityHolder<T_DerivedCompType>::
        Has(
            HandleType InHandle)
        -> bool
    {
        return InHandle.Has<CompType>();
    }

    template <typename T_DerivedCompType>
    auto
        TUtils_EntityHolder<T_DerivedCompType>::
        Ensure(
            HandleType InHandle)
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
            HandleType InHandle)
        -> HandleType
    {
        if (NOT Ensure(InHandle))
        { return {}; }

        const auto& entityHolderComp = InHandle.Get<CompType>();
        const auto& storedEntity     = entityHolderComp.Get_Entity();

        return storedEntity;
    }

    // --------------------------------------------------------------------------------------------------------------------

    struct UCk_Utils_ParentEntity : public TUtils_EntityHolder<FFragment_ParentEntity> {};

    // --------------------------------------------------------------------------------------------------------------------

    struct UCk_Utils_TargetEntity : public TUtils_EntityHolder<FFragment_TargetEntity> {};

    // --------------------------------------------------------------------------------------------------------------------

    struct UCk_Utils_OwningEntity : public TUtils_EntityHolder<FFragment_OwningEntity> {};
}

// --------------------------------------------------------------------------------------------------------------------
