#pragma once

#include "CkInventory/Inventory/CkInventory_RequestHandlers.h"
#include "CkInventory/Inventory/Spatial/CkInventory_Spatial_Fragment_Data.h"

namespace ck
{
    // Spatial inventory's trait bundle — pure type aliases. All per-shape Handle specializations
    // and divergence-helper specializations (e.g. TStackItems<Spatial>::OnSourceFullyConsumed)
    // live in CkInventory_Spatial_RequestTraits.cpp, alongside the explicit instantiations of
    // every Spatial-rooted TXxx<...>.
    template <>
    struct CKINVENTORY_API TInventoryRequestTraits<FCk_Handle_Inventory_Spatial>
    {
        using HandleType          = FCk_Handle_Inventory_Spatial;
        using ReplicationFragment = FCk_RepData_Inventory_Spatial_Items;

        using AddItem             = inventory_handlers::TAddItem<FCk_Handle_Inventory_Spatial, FCk_SpatialPlacement>;
        using RemoveItem          = inventory_handlers::TRemoveItem<FCk_Handle_Inventory_Spatial>;
        using StackItems          = inventory_handlers::TStackItems<FCk_Handle_Inventory_Spatial>;
        using SplitStack          = inventory_handlers::TSplitStack<FCk_Handle_Inventory_Spatial, FCk_SpatialPlacement>;
        using AddByDefinition     = inventory_handlers::TAddByDefinition<FCk_Handle_Inventory_Spatial>;
        using Sort                = inventory_handlers::TSort<FCk_Handle_Inventory_Spatial>;
        using Transfer_ToSpatial  = inventory_handlers::TTransfer<FCk_Handle_Inventory_Spatial, FCk_Handle_Inventory_Spatial>;
        using Transfer_ToDataOnly = inventory_handlers::TTransfer<FCk_Handle_Inventory_Spatial, FCk_Handle_Inventory_DataOnly>;
        using Relocate            = inventory_handlers::TRelocate<FCk_Handle_Inventory_Spatial>;

        using Variant = std::variant<
            typename AddItem::Entry,
            typename RemoveItem::Entry,
            typename StackItems::Entry,
            typename SplitStack::Entry,
            typename AddByDefinition::Entry,
            typename Sort::Entry,
            typename Transfer_ToSpatial::Entry,
            typename Transfer_ToDataOnly::Entry,
            typename Relocate::Entry>;

        // Inventory creation hook — per-shape setup (GridSystem, type tag).
        static auto Setup_PerShape(FCk_Handle_Inventory&, const FCk_Fragment_Inventory_ParamsData&, ECk_Replication) -> void;
    };
}

// Forward declarations of Spatial-specific TXxx::Handle / divergence-helper specializations.
// Definitions live in CkInventory_Spatial_RequestTraits.cpp. These declarations are required so
// that the explicit-instantiation site (CkInventory_RequestHandlers.cpp, where the default
// bodies live) routes through Spatial's overrides instead of falling back to defaults.
namespace ck::inventory_handlers
{
    template <>
    auto TAddItem<FCk_Handle_Inventory_Spatial, FCk_SpatialPlacement>::Handle(
        FCk_Handle_Inventory_Spatial&,
        const ck::FFragment_Inventory_Params&,
        const Entry&) -> Result;

    template <>
    auto TRemoveItem<FCk_Handle_Inventory_Spatial>::Handle(
        FCk_Handle_Inventory_Spatial&,
        const ck::FFragment_Inventory_Params&,
        const Entry&) -> Result;

    template <>
    auto TStackItems<FCk_Handle_Inventory_Spatial>::OnSourceFullyConsumed(
        FCk_Handle_Inventory_Spatial&, FCk_Handle_Item&) -> void;

    template <>
    auto TSplitStack<FCk_Handle_Inventory_Spatial, FCk_SpatialPlacement>::Handle(
        FCk_Handle_Inventory_Spatial&,
        const ck::FFragment_Inventory_Params&,
        const Entry&) -> Result;

    template <>
    auto TAddByDefinition<FCk_Handle_Inventory_Spatial>::TryPlace(
        FCk_Handle_Inventory_Spatial&, FCk_Handle_Item&) -> bool;

    template <>
    auto TSort<FCk_Handle_Inventory_Spatial>::Handle(
        FCk_Handle_Inventory_Spatial&,
        const ck::FFragment_Inventory_Params&,
        const Entry&) -> Result;

    template <>
    auto TRelocate<FCk_Handle_Inventory_Spatial>::Handle(
        FCk_Handle_Inventory_Spatial&,
        const ck::FFragment_Inventory_Params&,
        const Entry&) -> Result;
}
