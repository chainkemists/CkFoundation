#pragma once

#include "CkInventory/Inventory/CkInventory_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Client-side: reconcile replicated inventory state onto Record and grid cells
    class CKINVENTORY_API FProcessor_Inventory_SyncReplication : public ck_exp::TProcessor<
            FProcessor_Inventory_SyncReplication,
            FCk_Handle_Inventory,
            FFragment_Inventory_Params,
            FFragment_Inventory_SyncReplication,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::ClientOnly;
        using MarkedDirtyBy = FFragment_Inventory_SyncReplication;

    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Inventory_Params& InParams,
            const FFragment_Inventory_SyncReplication& InSync) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Authority: process add/remove/stack/split/transfer requests, manage grid slots, update Record
    class CKINVENTORY_API FProcessor_Inventory_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Inventory_HandleRequests,
            FCk_Handle_Inventory,
            FFragment_Inventory_Params,
            FFragment_Inventory_Requests,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_Inventory_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Inventory_Params& InParams,
            FFragment_Inventory_Requests& InRequests) const -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Inventory_Params& InParams,
            const FFragment_Inventory_Requests::AddItemRequestType& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Inventory_Params& InParams,
            const FFragment_Inventory_Requests::RemoveItemRequestType& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Inventory_Params& InParams,
            const FFragment_Inventory_Requests::StackItemsRequestType& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Inventory_Params& InParams,
            const FFragment_Inventory_Requests::SplitStackRequestType& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Inventory_Params& InParams,
            const FFragment_Inventory_Requests::AddItemByDefinitionRequestType& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Inventory_Params& InParams,
            const FFragment_Inventory_Requests::TransferItemRequestType& InRequest) -> void;

        static auto
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Inventory_Params& InParams,
            const FFragment_Inventory_Requests::SortRequestType& InRequest) -> void;

        // ---- Bind / Unbind helpers ----

        /** Connects item to inventory record, sets parent back-ref, transfers lifetime ownership. */
        static auto
        DoBindItemToInventory(
            HandleType& InInventory,
            FCk_Handle_Item& InItem) -> void;

        /** Disconnects item from inventory record, clears parent back-ref. */
        static auto
        DoUnbindItemFromInventory(
            HandleType& InInventory,
            FCk_Handle_Item& InItem) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Authority: diff current items vs previous snapshot, fire OnItemsChanged signal
    class CKINVENTORY_API FProcessor_Inventory_FireSignals : public ck_exp::TProcessor<
            FProcessor_Inventory_FireSignals,
            FCk_Handle_Inventory,
            FFragment_Inventory_Params,
            FFragment_Inventory_PreviousItems,
            FTag_Inventory_MayHaveChanged,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Inventory_Params& InParams,
            FFragment_Inventory_PreviousItems& InPreviousItems) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Server-side: replicate inventory item list to clients
    class CKINVENTORY_API FProcessor_Inventory_Replicate : public ck_exp::TProcessor<
            FProcessor_Inventory_Replicate,
            FCk_Handle_Inventory,
            FFragment_Inventory_Params,
            FTag_Inventory_MayRequireReplication,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        static constexpr auto NetModeRequirement = ECk_ProcessorNetModeRequirement::AuthorityOnly;
        using MarkedDirtyBy = FTag_Inventory_MayRequireReplication;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        DoTick(
            TimeType InDeltaT) -> void;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Inventory_Params& InParams) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
