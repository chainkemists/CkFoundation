#pragma once

#include "CkInventory/Inventory/CkInventory_Fragment.h"
#include "CkInventory/Inventory/CkInventory_Fragment_Data.h"
#include "CkInventory/Inventory/Spatial/CkInventory_Spatial_Fragment_Data.h"
#include "CkInventory/Inventory/DataOnly/CkInventory_DataOnly_Fragment_Data.h"

#include <variant>

namespace ck::inventory_handlers
{
    // Each TXxx<H, A> is the complete handler for that (Handle, Addon) combination.
    // Default Handle body lives in CkInventory_RequestHandlers.cpp and works for the addon-less,
    // shape-divergence-free case (typically DataOnly).
    //
    // When a shape needs different behavior:
    //   - If the addon type differs (e.g. TAddItem<H, FCk_SpatialPlacement>), specialize the
    //     entire ::Handle for that (H, A) pair.
    //   - If the body is mostly shared but has a small divergent step, the TXxx exposes a
    //     static helper (e.g. OnSourceFullyConsumed, TryPlace) with a default no-op/generic
    //     body. Per-shape folders specialize JUST that helper.
    //
    // All specializations live in the typed inventory's folder
    // (Spatial/CkInventory_Spatial_RequestTraits.cpp and DataOnly equivalent).

    template <typename TInventoryHandle, typename TAddon = FCk_EmptyAddon>
    struct CKINVENTORY_API TAddItem
    {
        using Entry  = TInventory_RequestEntry<FCk_Request_Inventory_AddItem, TAddon>;
        using Result = ECk_Inventory_OperationResult_Add;
        static auto Handle(TInventoryHandle&, const FFragment_Inventory_Params&, const Entry&) -> Result;
    };

    template <typename TInventoryHandle, typename TAddon = FCk_EmptyAddon>
    struct CKINVENTORY_API TRemoveItem
    {
        using Entry  = TInventory_RequestEntry<FCk_Request_Inventory_RemoveItem, TAddon>;
        using Result = ECk_Inventory_OperationResult_Remove;
        static auto Handle(TInventoryHandle&, const FFragment_Inventory_Params&, const Entry&) -> Result;
    };

    template <typename TInventoryHandle, typename TAddon = FCk_EmptyAddon>
    struct CKINVENTORY_API TStackItems
    {
        using Entry  = TInventory_RequestEntry<FCk_Request_Inventory_StackItems, TAddon>;
        using Result = ECk_Inventory_OperationResult_Stack;
        // Divergence hook: runs when the source stack is fully consumed by the merge,
        // *before* unbind+destroy. Default no-op (DataOnly); Spatial specializes to remove from grid.
        static auto OnSourceFullyConsumed(TInventoryHandle&, FCk_Handle_Item&) -> void;
        static auto Handle(TInventoryHandle&, const FFragment_Inventory_Params&, const Entry&) -> Result;
    };

    template <typename TInventoryHandle, typename TAddon = FCk_EmptyAddon>
    struct CKINVENTORY_API TSplitStack
    {
        using Entry  = TInventory_RequestEntry<FCk_Request_Inventory_SplitStack, TAddon>;
        using Result = ECk_Inventory_OperationResult_Split;
        static auto Handle(TInventoryHandle&, const FFragment_Inventory_Params&, const Entry&) -> Result;
    };

    template <typename TInventoryHandle, typename TAddon = FCk_EmptyAddon>
    struct CKINVENTORY_API TAddByDefinition
    {
        using Entry  = TInventory_RequestEntry<FCk_Request_Inventory_AddItemByDefinition, TAddon>;
        using Result = ECk_Inventory_OperationResult_AddByDefinition;
        // Divergence hook: per-iteration placement. Default returns true (DataOnly);
        // Spatial specializes to find a placement + place on grid (false on no-space).
        static auto TryPlace(TInventoryHandle&, FCk_Handle_Item&) -> bool;
        static auto Handle(TInventoryHandle&, const FFragment_Inventory_Params&, const Entry&) -> Result;
    };

    template <typename TInventoryHandle, typename TAddon = FCk_EmptyAddon>
    struct CKINVENTORY_API TSort
    {
        using Entry  = TInventory_RequestEntry<FCk_Request_Inventory_Sort, TAddon>;
        using Result = ECk_Inventory_OperationResult_Sort;
        static auto Handle(TInventoryHandle&, const FFragment_Inventory_Params&, const Entry&) -> Result;
    };

    template <typename TSourceHandle, typename TTargetHandle, typename TAddon = FCk_EmptyAddon>
    struct CKINVENTORY_API TTransfer;

    template <typename TSourceHandle, typename TAddon>
    struct CKINVENTORY_API TTransfer<TSourceHandle, FCk_Handle_Inventory_Spatial, TAddon>
    {
        using Entry  = TInventory_RequestEntry<FCk_Request_Inventory_TransferItem_ToSpatial, TAddon>;
        using Result = ECk_Inventory_OperationResult_Transfer;
        static auto Handle(TSourceHandle&, const FFragment_Inventory_Params&, const Entry&) -> Result;
    };

    template <typename TSourceHandle, typename TAddon>
    struct CKINVENTORY_API TTransfer<TSourceHandle, FCk_Handle_Inventory_DataOnly, TAddon>
    {
        using Entry  = TInventory_RequestEntry<FCk_Request_Inventory_TransferItem_ToDataOnly, TAddon>;
        using Result = ECk_Inventory_OperationResult_Transfer;
        static auto Handle(TSourceHandle&, const FFragment_Inventory_Params&, const Entry&) -> Result;
    };

    template <typename TInventoryHandle, typename TAddon = FCk_EmptyAddon>
    struct CKINVENTORY_API TRelocate
    {
        using Entry  = TInventory_RequestEntry<FCk_Request_Inventory_Spatial_RelocateItem, TAddon>;
        using Result = ECk_Inventory_OperationResult_Relocate;
        static auto Handle(TInventoryHandle&, const FFragment_Inventory_Params&, const Entry&) -> Result;
    };
}

namespace ck
{
    // Each typed inventory's folder must specialize this trait, listing every operation it supplies
    // and exposing a Variant typedef built from the operations' Entry types. Missing specialization
    // = compile error at the point of use (templated processor instantiation).
    template <typename TInventoryHandle>
    struct TInventoryRequestTraits;
}

namespace ck::inventory_handlers
{
    namespace detail
    {
        template <typename, typename = void> struct HasRelocate : std::false_type {};
        template <typename T> struct HasRelocate<T, std::void_t<typename T::Relocate>> : std::true_type {};
    }

    // Centralized dispatcher — given an entry type, calls the matching handler from the Traits bundle.
    template <typename Traits, typename TInventoryHandle, typename TEntry>
    auto DispatchToHandler(
        TInventoryHandle& InHandle,
        const FFragment_Inventory_Params& InParams,
        const TEntry& InEntry) -> void
    {
        if constexpr (std::is_same_v<TEntry, typename Traits::AddItem::Entry>)
            std::ignore = Traits::AddItem::Handle(InHandle, InParams, InEntry);
        else if constexpr (std::is_same_v<TEntry, typename Traits::RemoveItem::Entry>)
            std::ignore = Traits::RemoveItem::Handle(InHandle, InParams, InEntry);
        else if constexpr (std::is_same_v<TEntry, typename Traits::StackItems::Entry>)
            std::ignore = Traits::StackItems::Handle(InHandle, InParams, InEntry);
        else if constexpr (std::is_same_v<TEntry, typename Traits::SplitStack::Entry>)
            std::ignore = Traits::SplitStack::Handle(InHandle, InParams, InEntry);
        else if constexpr (std::is_same_v<TEntry, typename Traits::AddByDefinition::Entry>)
            std::ignore = Traits::AddByDefinition::Handle(InHandle, InParams, InEntry);
        else if constexpr (std::is_same_v<TEntry, typename Traits::Sort::Entry>)
            std::ignore = Traits::Sort::Handle(InHandle, InParams, InEntry);
        else if constexpr (std::is_same_v<TEntry, typename Traits::Transfer_ToSpatial::Entry>)
            std::ignore = Traits::Transfer_ToSpatial::Handle(InHandle, InParams, InEntry);
        else if constexpr (std::is_same_v<TEntry, typename Traits::Transfer_ToDataOnly::Entry>)
            std::ignore = Traits::Transfer_ToDataOnly::Handle(InHandle, InParams, InEntry);
        else if constexpr (detail::HasRelocate<Traits>::value)
        {
            if constexpr (std::is_same_v<TEntry, typename Traits::Relocate::Entry>)
                std::ignore = Traits::Relocate::Handle(InHandle, InParams, InEntry);
            else
                static_assert(sizeof(TEntry) == 0,
                    "DispatchToHandler: entry type does not match any operation in TInventoryRequestTraits.");
        }
        else
        {
            static_assert(sizeof(TEntry) == 0,
                "DispatchToHandler: entry type does not match any operation in TInventoryRequestTraits.");
        }
    }
}
