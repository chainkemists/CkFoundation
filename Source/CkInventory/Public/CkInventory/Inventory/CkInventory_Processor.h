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

    // Per-entry order is load-bearing for replication correctness: removes happen first (per-shape
    // cleanup → record disconnect → parent ref clear), then adds (connect → parent ref → setup).
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
        // Default no-op; the CRTP derived class shadows these with its own statics to override.
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
        // One request per pump pass so a request's deferred, attribute-backed writes fold before the
        // next request reads them — same-pass requests would otherwise read un-settled state.
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Inventory_Params& InParams,
            TRequestsFragment& InRequestsComp) const -> void
        {
            FCk_Handle Base  = InHandle;
            auto&      Pacer = Base.AddOrGet<FFragment_PacedWork>(1, 8);

            // RunPacedSteps removes TRequestsFragment once the step reports Done, so InRequestsComp
            // must not be read after this returns true — drive teardown off Done.
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
    // are never drained — fire each with Failed_OperationCancelled so callers don't hang.
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

    // The shape processor headers include THIS header, and TDepList is a pure type list, so forward
    // decls suffice here; CkInventory_Processor.cpp includes them for registration.
    class FProcessor_Inventory_Spatial_HandleRequests;
    class FProcessor_Inventory_DataOnly_HandleRequests;

    // One item per pump pass so each transfer's deferred stack writes fold before the next item
    // resolves its target; RunAfter both shapes' HandleRequests for the same reason.
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

    // The churn carries CK_IGNORE_PENDING_KILL, so an op destroyed mid-flight never finishes its drain.
    // Normal completion removes the in-flight fragment first, so a completed op cannot double-fire here.
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