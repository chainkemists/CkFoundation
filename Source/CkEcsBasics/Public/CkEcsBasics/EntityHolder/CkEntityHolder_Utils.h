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
            HandleType& InHandle,
            const EntityType& InEntityToStore) -> void;

        static auto Has(
            const HandleType& InHandle) -> bool;

        static auto Ensure(
            const HandleType& InHandle) -> bool;

        static auto Get_StoredEntity(
            const HandleType& InHandle) -> HandleType;
    };
}

// ------------------------------------------------------------Owning--------------------------------------------------------
// Definitions

namespace ck
{
    template <typename T_DerivedCompType>
    auto
        TUtils_EntityHolder<T_DerivedCompType>::
        Add(
            HandleType& InHandle,
            const EntityType& InEntityToStore)
        -> void
    {
        InHandle.Add<CompType>(InEntityToStore);
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
        -> HandleType
    {
        if (NOT Ensure(InHandle))
        { return {}; }

        const auto& EntityHolderComp = InHandle.Get<CompType>();
        const auto& StoredEntity     = EntityHolderComp.Get_Entity();

        return StoredEntity;
    }
}

// --------------------------------------------------------------------------------------------------------------------
