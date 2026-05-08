#pragma once

#include "CkInventory/Inventory/CkInventory_Fragment.h"
#include "CkInventory/Inventory/CkInventory_RequestHandlers.h"
#include "CkInventory/Inventory/CkInventory_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkCore/Payload/CkPayload.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_NetModePolicy.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKINVENTORY_API FProcessor_Inventory_FireSignals : public ck_exp::TProcessor<
            FProcessor_Inventory_FireSignals,
            FCk_Handle_Inventory,
            TReadOnly<FFragment_Inventory_Params>,
            TReadWrite<FFragment_Inventory_PreviousItems>,
            FTag_Inventory_MayHaveChanged,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
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

    // Templated SyncReplication processor base. Diffs the incoming replicated entries against
    // the previous snapshot and applies the per-shape OnEntryAdded / OnEntryRemoved hooks.
    // Defaults are no-op (DataOnly); per-shape concrete derived classes override (CRTP) to
    // perform shape-specific work — Spatial places/removes from grid.
    //
    // Per-entry order is load-bearing for replication correctness: removes happen first
    // (per-shape cleanup → record disconnect → parent ref clear), then adds (record connect
    // → parent ref → per-shape setup).
    template <typename T_Derived, typename TInventoryHandle, typename TSyncFragment, typename TInventoryTypeTag, typename TEntry>
    class TProcessor_Inventory_SyncReplication_Base : public ck_exp::TProcessor<
            T_Derived,
            TInventoryHandle,
            TReadOnly<FFragment_Inventory_Params>,
            TReadOnly<TSyncFragment>,
            TInventoryTypeTag,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using ParentProcessor = ck_exp::TProcessor<
            T_Derived,
            TInventoryHandle,
            TReadOnly<FFragment_Inventory_Params>,
            TReadOnly<TSyncFragment>,
            TInventoryTypeTag,
            CK_IGNORE_PENDING_KILL>;
        using HandleType = TInventoryHandle;
        using TimeType   = typename ParentProcessor::TimeType;
        using ParentProcessor::ParentProcessor;

    public:
        // Per-shape hooks. Default no-op; CRTP derived class shadows with its own static
        // method to override.
        static auto OnEntryAdded(TInventoryHandle&, const TEntry&, FCk_Handle_Item&) -> void {}
        static auto OnEntryRemoved(TInventoryHandle&, FCk_Handle_Item&) -> void {}

        static auto
        ForEachEntity(
            TimeType,
            HandleType InHandle,
            const FFragment_Inventory_Params&,
            const TSyncFragment& InSync) -> void
        {
            const auto& InCurrent  = InSync.Get_ItemsToReplicate();
            const auto& InPrevious = InSync.Get_ItemsToReplicate_Previous();

            if (InCurrent == InPrevious)
            {
                InHandle.template Remove<TSyncFragment>();
                return;
            }

            const auto ByItemHandle = &TEntry::Get_ItemHandle;
            const auto EntriesAdded   = ck::algo::Except(InCurrent, InPrevious, ByItemHandle);
            const auto EntriesRemoved = ck::algo::Except(InPrevious, InCurrent, ByItemHandle);

            auto ItemsAdded   = TArray<FCk_Handle_Item>{};
            auto ItemsRemoved = TArray<FCk_Handle_Item>{};

            FCk_Handle_Inventory BaseHandle = InHandle;

            for (const auto& Entry : EntriesRemoved)
            {
                auto ItemHandle = Entry.Get_ItemHandle();

                T_Derived::OnEntryRemoved(InHandle, ItemHandle);

                UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils::Request_Disconnect(BaseHandle, ItemHandle);

                // Order-safe for transfers: only clear parent if it still points to this inventory.
                if (ck::IsValid(ItemHandle) && TUtils_Item_ParentInventory::Has(ItemHandle))
                {
                    if (TUtils_Item_ParentInventory::Get_StoredEntity(ItemHandle) == BaseHandle)
                    {
                        TUtils_Item_ParentInventory::AddOrReplace(ItemHandle, FCk_Handle_Inventory());
                    }
                }

                ItemsRemoved.Add(ItemHandle);
            }

            for (const auto& Entry : EntriesAdded)
            {
                auto ItemHandle = Entry.Get_ItemHandle();

                UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils::Request_Connect(
                    BaseHandle, ItemHandle, ECk_Record_LabelRequirementPolicy::Optional);
                TUtils_Item_ParentInventory::AddOrReplace(ItemHandle, BaseHandle);

                T_Derived::OnEntryAdded(InHandle, Entry, ItemHandle);

                ItemsAdded.Add(ItemHandle);
            }

            if (NOT ItemsAdded.IsEmpty() || NOT ItemsRemoved.IsEmpty())
            {
                UUtils_Signal_Inventory_OnItemsChanged::Broadcast(
                    BaseHandle, MakePayload(BaseHandle, ItemsAdded, ItemsRemoved));
            }

            InHandle.template Remove<TSyncFragment>();
        }
    };

    // Templated request-handling processor base. Each typed inventory's TInventoryRequestTraits<>
    // specialization supplies the per-operation handler bundle; the visitor dispatches via
    // inventory_handlers::DispatchToHandler.
    template <typename T_Derived, typename TInventoryHandle, typename TRequestsFragment, typename TInventoryTypeTag>
    class TProcessor_Inventory_HandleRequests_Base : public ck_exp::TProcessor<
            T_Derived,
            TInventoryHandle,
            TReadOnly<FFragment_Inventory_Params>,
            TReadWrite<TRequestsFragment>,
            TInventoryTypeTag,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using ParentProcessor = ck_exp::TProcessor<
            T_Derived,
            TInventoryHandle,
            TReadOnly<FFragment_Inventory_Params>,
            TReadWrite<TRequestsFragment>,
            TInventoryTypeTag,
            CK_IGNORE_PENDING_KILL>;
        using HandleType = TInventoryHandle;
        using TimeType   = typename ParentProcessor::TimeType;
        using Traits     = TInventoryRequestTraits<TInventoryHandle>;
        using ParentProcessor::ParentProcessor;

    public:
        auto
        ForEachEntity(
            TimeType,
            HandleType InHandle,
            const FFragment_Inventory_Params& InParams,
            TRequestsFragment& InRequestsComp) const -> void
        {
            InHandle.CopyAndRemove(InRequestsComp, [&](const TRequestsFragment& InRequests)
            {
                ck::algo::ForEachRequest(InRequests.Get_Requests(),
                    ck::Visitor([&](const auto& InEntry)
                    {
                        inventory_handlers::DispatchToHandler<Traits>(InHandle, InParams, InEntry);
                    }),
                    ck::policy::DontResetContainer{});
            });

            UCk_Utils_Inventory_UE::Request_MarkInventory_AsMayHaveChanged(InHandle);
        }
    };
}

// --------------------------------------------------------------------------------------------------------------------