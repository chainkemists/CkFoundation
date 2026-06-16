#pragma once

#include "CkInventory/Inventory/CkInventory_Fragment.h"
#include "CkInventory/Inventory/CkInventory_RequestHandlers.h"
#include "CkInventory/Inventory/CkInventory_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkCore/Payload/CkPayload.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Pacing/CkPacedWork.h"
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
        // Drain one request per pump pass (not the whole queue at once) so a request's deferred,
        // attribute-backed writes fold before the next request reads them — same-pass requests would
        // otherwise read each other's un-settled state. The Requests fragment is the dirty marker
        // (every enqueue path touches it); FFragment_PacedWork bounds the per-frame count internally.
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Inventory_Params& InParams,
            TRequestsFragment& InRequestsComp) const -> void
        {
            FCk_Handle Base  = InHandle;
            auto&      Pacer = Base.AddOrGet<FFragment_PacedWork>(1, 8);

            // RunPacedSteps removes TRequestsFragment (the marker) once the step reports Done, so
            // InRequestsComp must not be read after this returns true — drive teardown off Done.
            const auto Done = ck::RunPacedSteps<TRequestsFragment>(Base, Pacer, InDeltaT, [&]() -> ck::EPacedStepResult
            {
                auto& Requests = InRequestsComp.Get_RequestsMutable();
                if (Requests.IsEmpty())
                { return ck::EPacedStepResult::Done; }

                const auto Entry = Requests[0];
                Requests.RemoveAt(0);

                std::visit([&](const auto& InEntry)
                {
                    inventory_handlers::DispatchToHandler<Traits>(InHandle, InParams, InEntry);
                }, Entry);

                return Requests.IsEmpty() ? ck::EPacedStepResult::Done : ck::EPacedStepResult::Continue;
            });

            if (Done)
            { Base.Try_Remove<FFragment_PacedWork>(); }

            UCk_Utils_Inventory_UE::Request_MarkInventory_AsMayHaveChanged(InHandle);
        }
    };

    // --------------------------------------------------------------------------------------------------------------------

    // HandleRequests carries CK_IGNORE_PENDING_KILL, so a destroyed inventory's still-queued requests
    // are never drained. This EndPlay processor fires each with Failed_OperationCancelled via
    // DispatchCancel so callers awaiting completion don't hang.
    template <typename T_Derived, typename TInventoryHandle, typename TRequestsFragment, typename TInventoryTypeTag>
    class TProcessor_Inventory_CancelOnEndPlay_Base : public ck_exp::TProcessor<
            T_Derived,
            TInventoryHandle,
            TReadOnly<TRequestsFragment>,
            TInventoryTypeTag,
            CK_IF_END_PLAY>
    {
    public:
        using ParentProcessor = ck_exp::TProcessor<
            T_Derived,
            TInventoryHandle,
            TReadOnly<TRequestsFragment>,
            TInventoryTypeTag,
            CK_IF_END_PLAY>;
        using HandleType = TInventoryHandle;
        using TimeType   = typename ParentProcessor::TimeType;
        using Traits     = TInventoryRequestTraits<TInventoryHandle>;
        using ParentProcessor::ParentProcessor;

    public:
        auto
        ForEachEntity(
            TimeType,
            HandleType InHandle,
            const TRequestsFragment& InRequestsComp) const -> void
        {
            ck::algo::ForEachRequest(InRequestsComp.Get_Requests(),
                ck::Visitor([&](const auto& InEntry)
                {
                    inventory_handlers::DispatchCancel<Traits>(InHandle, InEntry);
                }),
                ck::policy::DontResetContainer{});
        }
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Forward decls — the churn's RunAfter references both shapes' HandleRequests, which are defined in
    // the shape processor headers (those include THIS header). TDepList is a pure type list, so forward
    // decls suffice; CkInventory_Processor.cpp includes the shape headers for registration.
    class FProcessor_Inventory_Spatial_HandleRequests;
    class FProcessor_Inventory_DataOnly_HandleRequests;

    // Drains a standalone mass-transfer op ONE item per pump pass (paced) so each transfer's deferred,
    // attribute-backed stack writes fold before the next item resolves its target — coherent capacity
    // reads, same hazard the paced HandleRequests fix addressed. Runs AFTER both shapes' HandleRequests
    // so a step reads the prior step's folded writes. The op is a plain FCk_Handle discriminated by
    // FFragment_Inventory_MassTransfer_InFlight (NOT a typesafe handle, NOT scoped to an inventory).
    class CKINVENTORY_API FProcessor_Inventory_MassTransfer_Churn : public ck_exp::TProcessor<
            FProcessor_Inventory_MassTransfer_Churn,
            FCk_Handle,
            TReadWrite<FFragment_Inventory_MassTransfer_InFlight>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group         = FGroup_Gameplay;
        using RunAfter      = TDepList<FProcessor_Inventory_Spatial_HandleRequests, FProcessor_Inventory_DataOnly_HandleRequests>;
        using MarkedDirtyBy = FFragment_Inventory_MassTransfer_InFlight;
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Inventory_MassTransfer_InFlight& InFlight) -> void;

    private:
        // Captured INSIDE the completing step (before RunPacedSteps removes the in-flight marker), so
        // the finish broadcast never reads the freed fragment.
        struct FFinishSnapshot
        {
            ECk_Inventory_MassTransfer_Result Result = ECk_Inventory_MassTransfer_Result::Failed_NothingToTransfer;
            int32 Units  = 0;
            int32 Fully  = 0;
            int32 Failed = 0;
        };

        static auto
        DoOneStep(
            FFragment_Inventory_MassTransfer_InFlight& InFlight,
            FFinishSnapshot& OutFinish) -> ck::EPacedStepResult;

        static auto
        ComputeFinish(
            const FFragment_Inventory_MassTransfer_InFlight& InFlight) -> FFinishSnapshot;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The churn carries CK_IGNORE_PENDING_KILL, so an op destroyed mid-flight (world teardown) never
    // finishes its drain. This fires the completion signal with Failed_OperationCancelled so a caller
    // awaiting OnComplete doesn't hang. Normal completion removes the in-flight fragment first, so a
    // completed op no longer matches this view — no double-fire.
    class CKINVENTORY_API FProcessor_Inventory_MassTransfer_CancelOnEndPlay : public ck_exp::TProcessor<
            FProcessor_Inventory_MassTransfer_CancelOnEndPlay,
            FCk_Handle,
            TReadOnly<FFragment_Inventory_MassTransfer_InFlight>,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Inventory_MassTransfer_InFlight& InFlight) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------