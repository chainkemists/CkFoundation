#pragma once

#include "CkInventory/Inventory/CkInventory_Fragment.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"

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

    // Authority: process add/remove item requests, manage grid slots, update Record
    class CKINVENTORY_API FProcessor_Inventory_ProcessSlots : public ck_exp::TProcessor<
            FProcessor_Inventory_ProcessSlots,
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
            const FFragment_Inventory_Requests::AddItemRequestType& InRequest,
            TArray<FCk_Handle>& OutItemsAdded,
            TArray<FCk_Handle>& OutItemsRemoved) -> void;

        static auto
        DoHandleRequest(
            HandleType& InHandle,
            const FFragment_Inventory_Params& InParams,
            const FFragment_Inventory_Requests::RemoveItemRequestType& InRequest,
            TArray<FCk_Handle>& OutItemsAdded,
            TArray<FCk_Handle>& OutItemsRemoved) -> void;
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
