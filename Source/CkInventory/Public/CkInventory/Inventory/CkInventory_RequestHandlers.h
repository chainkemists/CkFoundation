#pragma once

#include "CkInventory/Inventory/CkInventory_Fragment.h"
#include "CkInventory/Inventory/CkInventory_Fragment_Data.h"
#include "CkInventory/Inventory/Spatial/CkInventory_Spatial_Fragment_Data.h"
#include "CkInventory/Inventory/DataOnly/CkInventory_DataOnly_Fragment_Data.h"

#include "CkCore/Payload/CkPayload.h"

#include "CkEcs/Request/CkRequest_Completion.h"

#include <variant>

namespace ck::inventory_handlers
{
    /** Coarse Succeeded/Failed view of a bespoke per-operation result, for the generic completion
     *  delegate. Both are derived from the SAME result value, so they can never disagree. */
    constexpr auto ToCompletionResult(ECk_Inventory_OperationResult_Add InResult) -> ECk_Request_OperationResult
    {
        return InResult == ECk_Inventory_OperationResult_Add::Success
            ? ECk_Request_OperationResult::Succeeded : ECk_Request_OperationResult::Failed;
    }

    constexpr auto ToCompletionResult(ECk_Inventory_OperationResult_Remove InResult) -> ECk_Request_OperationResult
    {
        return InResult == ECk_Inventory_OperationResult_Remove::Success
            ? ECk_Request_OperationResult::Succeeded : ECk_Request_OperationResult::Failed;
    }

    constexpr auto ToCompletionResult(ECk_Inventory_OperationResult_Stack InResult) -> ECk_Request_OperationResult
    {
        return InResult == ECk_Inventory_OperationResult_Stack::Success
            ? ECk_Request_OperationResult::Succeeded : ECk_Request_OperationResult::Failed;
    }

    constexpr auto ToCompletionResult(ECk_Inventory_OperationResult_Split InResult) -> ECk_Request_OperationResult
    {
        return InResult == ECk_Inventory_OperationResult_Split::Success
            ? ECk_Request_OperationResult::Succeeded : ECk_Request_OperationResult::Failed;
    }

    constexpr auto ToCompletionResult(ECk_Inventory_OperationResult_AddByDefinition InResult) -> ECk_Request_OperationResult
    {
        return InResult == ECk_Inventory_OperationResult_AddByDefinition::Success_AllAdded
            || InResult == ECk_Inventory_OperationResult_AddByDefinition::Success_PartiallyAdded
            ? ECk_Request_OperationResult::Succeeded : ECk_Request_OperationResult::Failed;
    }

    constexpr auto ToCompletionResult(ECk_Inventory_OperationResult_Sort InResult) -> ECk_Request_OperationResult
    {
        return InResult == ECk_Inventory_OperationResult_Sort::Success
            ? ECk_Request_OperationResult::Succeeded : ECk_Request_OperationResult::Failed;
    }

    constexpr auto ToCompletionResult(ECk_Inventory_OperationResult_Transfer InResult) -> ECk_Request_OperationResult
    {
        return InResult == ECk_Inventory_OperationResult_Transfer::Success
            || InResult == ECk_Inventory_OperationResult_Transfer::Success_Partial
            ? ECk_Request_OperationResult::Succeeded : ECk_Request_OperationResult::Failed;
    }

    constexpr auto ToCompletionResult(ECk_Inventory_OperationResult_Relocate InResult) -> ECk_Request_OperationResult
    {
        return InResult == ECk_Inventory_OperationResult_Relocate::Success
            ? ECk_Request_OperationResult::Succeeded : ECk_Request_OperationResult::Failed;
    }

    constexpr auto ToCompletionResult(ECk_Inventory_MassTransfer_Result InResult) -> ECk_Request_OperationResult
    {
        return InResult == ECk_Inventory_MassTransfer_Result::Success
            || InResult == ECk_Inventory_MassTransfer_Result::Success_Partial
            ? ECk_Request_OperationResult::Succeeded : ECk_Request_OperationResult::Failed;
    }

    // --------------------------------------------------------------------------------------------------------------------

    // Default Handle bodies live in CkInventory_RequestHandlers.cpp; per-shape overrides (whole
    // Handle or just a divergence hook) live in the typed inventory's folder's RequestTraits.cpp.

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
        // Runs when the merge fully consumes the source stack, BEFORE unbind + destroy.
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
        // Per-iteration placement hook; false means no space and aborts the iteration.
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

    // Synchronous (non-deferred) transfer of InItem's FULL source stack, internal to the churn.
    // Returns the units moved — 0 on invalid input / same-inventory / no room.
    CKINVENTORY_API auto ExecuteTransferNow(
        FCk_Handle_Item& InItem,
        FCk_Handle_Inventory& InTarget,
        ECk_Inventory_AddPolicy InPolicy) -> int32;
}

namespace ck
{
    // Each typed inventory's folder must specialize this; a missing specialization is a compile error
    // at the point of use (templated processor instantiation).
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

    // The generic completion delegate reports the coarse view of the very same result the bespoke
    // per-operation signal carries, so the two can never disagree.
    template <typename Traits, typename TInventoryHandle, typename TEntry>
    auto DispatchToHandler(
        TInventoryHandle& InHandle,
        const FFragment_Inventory_Params& InParams,
        const TEntry& InEntry) -> void
    {
        const auto Complete = [&](auto InResult) -> void
        {
            InEntry.Common.TryFireCompletion(InHandle, ToCompletionResult(InResult));
        };

        if constexpr (std::is_same_v<TEntry, typename Traits::AddItem::Entry>)
            Complete(Traits::AddItem::Handle(InHandle, InParams, InEntry));
        else if constexpr (std::is_same_v<TEntry, typename Traits::RemoveItem::Entry>)
            Complete(Traits::RemoveItem::Handle(InHandle, InParams, InEntry));
        else if constexpr (std::is_same_v<TEntry, typename Traits::StackItems::Entry>)
            Complete(Traits::StackItems::Handle(InHandle, InParams, InEntry));
        else if constexpr (std::is_same_v<TEntry, typename Traits::SplitStack::Entry>)
            Complete(Traits::SplitStack::Handle(InHandle, InParams, InEntry));
        else if constexpr (std::is_same_v<TEntry, typename Traits::AddByDefinition::Entry>)
            Complete(Traits::AddByDefinition::Handle(InHandle, InParams, InEntry));
        else if constexpr (std::is_same_v<TEntry, typename Traits::Sort::Entry>)
            Complete(Traits::Sort::Handle(InHandle, InParams, InEntry));
        else if constexpr (std::is_same_v<TEntry, typename Traits::Transfer_ToSpatial::Entry>)
            Complete(Traits::Transfer_ToSpatial::Handle(InHandle, InParams, InEntry));
        else if constexpr (std::is_same_v<TEntry, typename Traits::Transfer_ToDataOnly::Entry>)
            Complete(Traits::Transfer_ToDataOnly::Handle(InHandle, InParams, InEntry));
        else if constexpr (detail::HasRelocate<Traits>::value)
        {
            if constexpr (std::is_same_v<TEntry, typename Traits::Relocate::Entry>)
                Complete(Traits::Relocate::Handle(InHandle, InParams, InEntry));
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

    // The bespoke signal fires only if the request still owns a valid handle, and destroys that handle
    // on fire. The generic completion delegate fires unconditionally — TryFireCompletion unbinds after
    // executing, so that unbind is itself the exactly-once gate.
    template <typename Traits, typename TInventoryHandle, typename TEntry>
    auto DispatchCancel(
        TInventoryHandle& InHandle,
        const TEntry& InEntry) -> void
    {
        const auto& Req = InEntry.Common;

        Req.TryFireCompletion(InHandle, ECk_Request_OperationResult::Failed_Cancelled);

        if (NOT Req.Get_IsRequestHandleValid())
        { return; }

        FCk_Handle_Inventory Base = InHandle;

        if constexpr (std::is_same_v<TEntry, typename Traits::AddItem::Entry>)
            UUtils_Signal_Inventory_OnOperationResult_Add::Broadcast(Req.GetAndDestroyRequestHandle(),
                MakePayload(Base, Req.Get_ItemToAdd(),
                    ECk_Inventory_OperationResult_Add::Failed_OperationCancelled));
        else if constexpr (std::is_same_v<TEntry, typename Traits::RemoveItem::Entry>)
            UUtils_Signal_Inventory_OnOperationResult_Remove::Broadcast(Req.GetAndDestroyRequestHandle(),
                MakePayload(Base, Req.Get_ItemToRemove(),
                    ECk_Inventory_OperationResult_Remove::Failed_OperationCancelled));
        else if constexpr (std::is_same_v<TEntry, typename Traits::StackItems::Entry>)
            UUtils_Signal_Inventory_OnOperationResult_Stack::Broadcast(Req.GetAndDestroyRequestHandle(),
                MakePayload(Base, Req.Get_SourceItem(), Req.Get_TargetItem(),
                    ECk_Inventory_OperationResult_Stack::Failed_OperationCancelled));
        else if constexpr (std::is_same_v<TEntry, typename Traits::SplitStack::Entry>)
            UUtils_Signal_Inventory_OnOperationResult_Split::Broadcast(Req.GetAndDestroyRequestHandle(),
                MakePayload(Base, Req.Get_SourceItem(), FCk_Handle_Item{},
                    ECk_Inventory_OperationResult_Split::Failed_OperationCancelled));
        else if constexpr (std::is_same_v<TEntry, typename Traits::AddByDefinition::Entry>)
            UUtils_Signal_Inventory_OnOperationResult_AddByDefinition::Broadcast(Req.GetAndDestroyRequestHandle(),
                MakePayload(Base, ECk_Inventory_OperationResult_AddByDefinition::Failed_OperationCancelled,
                    0, TArray<FCk_Handle_Item>{}));
        else if constexpr (std::is_same_v<TEntry, typename Traits::Sort::Entry>)
            UUtils_Signal_Inventory_OnOperationResult_Sort::Broadcast(Req.GetAndDestroyRequestHandle(),
                MakePayload(Base, ECk_Inventory_OperationResult_Sort::Failed_OperationCancelled));
        else if constexpr (std::is_same_v<TEntry, typename Traits::Transfer_ToSpatial::Entry>)
        {
            FCk_Handle_Inventory TargetBase = Req.Get_TargetInventory();
            UUtils_Signal_Inventory_OnOperationResult_Transfer::Broadcast(Req.GetAndDestroyRequestHandle(),
                MakePayload(Base, Req.Get_SourceItem(), TargetBase, 0, FCk_Handle_Item{},
                    ECk_Inventory_OperationResult_Transfer::Failed_OperationCancelled));
        }
        else if constexpr (std::is_same_v<TEntry, typename Traits::Transfer_ToDataOnly::Entry>)
        {
            FCk_Handle_Inventory TargetBase = Req.Get_TargetInventory();
            UUtils_Signal_Inventory_OnOperationResult_Transfer::Broadcast(Req.GetAndDestroyRequestHandle(),
                MakePayload(Base, Req.Get_SourceItem(), TargetBase, 0, FCk_Handle_Item{},
                    ECk_Inventory_OperationResult_Transfer::Failed_OperationCancelled));
        }
        else if constexpr (detail::HasRelocate<Traits>::value)
        {
            if constexpr (std::is_same_v<TEntry, typename Traits::Relocate::Entry>)
                UUtils_Signal_Inventory_OnOperationResult_Relocate::Broadcast(Req.GetAndDestroyRequestHandle(),
                    MakePayload(Base, Req.Get_Item(), FIntPoint{},
                        ECk_Inventory_OperationResult_Relocate::Failed_OperationCancelled));
            else
                static_assert(sizeof(TEntry) == 0,
                    "DispatchCancel: entry type does not match any operation in TInventoryRequestTraits.");
        }
        else
        {
            static_assert(sizeof(TEntry) == 0,
                "DispatchCancel: entry type does not match any operation in TInventoryRequestTraits.");
        }
    }
}
