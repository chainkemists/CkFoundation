#include "CkInventory_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Payload/CkPayload.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkInventory/Inventory/CkInventory_RequestHandlers.h"
#include "CkInventory/Inventory/CkInventory_Utils.h"
// Full defs for the churn's RunAfter (forward-declared in the header) — registration ordering needs them.
#include "CkInventory/Inventory/Spatial/CkInventory_Spatial_Processor.h"
#include "CkInventory/Inventory/DataOnly/CkInventory_DataOnly_Processor.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_inventory_processor
{
    template <typename... TFunctions>
    struct TOverloaded : TFunctions...
    {
        using TFunctions::operator()...;
    };

    template <typename... TFunctions>
    TOverloaded(TFunctions...) -> TOverloaded<TFunctions...>;
}

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_FireSignals);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_MassTransfer_Churn);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_MassTransfer_CancelOnEndPlay);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_DataOnly_OperationQueue_Churn);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_Spatial_OperationQueue_Churn);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_DataOnly_OperationQueue_CancelOnEndPlay);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_Spatial_OperationQueue_CancelOnEndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Inventory_FireSignals::
        ForEachEntity(
            TimeType /*InDeltaT*/,
            HandleType InHandle,
            const FFragment_Inventory_Params& /*InParams*/,
            FFragment_Inventory_PreviousItems& InPreviousItems) -> void
    {
        const auto CurrentItems = UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils::Get_ValidEntries(InHandle);
        const auto& PreviousItems = InPreviousItems.Get_Items();

        const auto ItemsAdded   = algo::Except(CurrentItems, PreviousItems);
        const auto ItemsRemoved = algo::Except(PreviousItems, CurrentItems);

        if (NOT ItemsAdded.IsEmpty() || NOT ItemsRemoved.IsEmpty())
        {
            UCk_Utils_Inventory_UE::Request_TryReplicateInventory(InHandle);

            UUtils_Signal_Inventory_OnItemsChanged::Broadcast(
                InHandle,
                MakePayload(InHandle, ItemsAdded, ItemsRemoved));
        }

        InPreviousItems._Items = CurrentItems;
        InHandle.Remove<FTag_Inventory_MayHaveChanged>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Inventory_MassTransfer_Churn::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Inventory_MassTransfer_InFlight& InFlight)
        -> void
    {
        FCk_Handle Base  = InHandle;
        auto&      Pacer = Base.AddOrGet<ck::FFragment_PacedWork>(
            InFlight.Get_StepsPerPass(), InFlight.Get_MaxStepsPerFrame());

        // Copied before the drain: RunPacedSteps removes the fragment on Done, and the completion
        // delegate is fired after that removal.
        const auto CompletionDelegate = InFlight.Get_CompletionDelegate();

        auto Finish = FFinishSnapshot{};

        // The in-flight fragment IS the dirty marker; RunPacedSteps removes it on Done. The completing
        // step snapshots the result into Finish BEFORE that removal — never read InFlight past Done.
        const auto Done = ck::RunPacedSteps<FFragment_Inventory_MassTransfer_InFlight>(
            Base, Pacer, InDeltaT, [&]() -> ck::EPacedStepResult { return DoOneStep(InHandle, InFlight, Finish); });

        if (NOT Done)
        { return; }

        Base.Try_Remove<ck::FFragment_PacedWork>();

        UUtils_Signal_Inventory_MassTransfer_OnComplete::Broadcast(
            InHandle, MakePayload(InHandle, Finish.Result, Finish.Units, Finish.Fully, Finish.Failed));

        CompletionDelegate.ExecuteIfBound(InHandle,
            inventory_handlers::ToCompletionResult(Finish.Result));

        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Inventory_MassTransfer_Churn::
        DoOneStep(
            FCk_Handle InOperation,
            FFragment_Inventory_MassTransfer_InFlight& InFlight,
            FFinishSnapshot& OutFinish)
        -> ck::EPacedStepResult
    {
        if (InOperation.Has<FTag_Inventory_MassTransferStepRouted>())
        { return ck::EPacedStepResult::Continue; }

        // Items can be destroyed mid-op by another system — skip the dead cursor entries.
        while (InFlight._Cursor < InFlight._Pending.Num()
            && ck::Is_NOT_Valid(InFlight._Pending[InFlight._Cursor]))
        { ++InFlight._Cursor; }

        if (InFlight._Cursor >= InFlight._Pending.Num())
        {
            OutFinish = ComputeFinish(InFlight);
            return ck::EPacedStepResult::Done;
        }

        auto Item   = InFlight._Pending[InFlight._Cursor];
        auto Source = TUtils_Item_ParentInventory::Has(Item)
            ? TUtils_Item_ParentInventory::Get_StoredEntity(Item)
            : FCk_Handle_Inventory{};

        // Get_CanAcceptItem rejects the item's own current inventory, so a candidate that is also a
        // source never resolves as the target for an item it already holds.
        auto Target = UCk_Utils_ItemResolution_UE::ResolveBestTransferTarget(Item, InFlight._TargetResolution);

        if (ck::Is_NOT_Valid(Target))
        {
            ++InFlight._ItemsFailed;
            ++InFlight._Cursor;
            return ck::EPacedStepResult::Continue;
        }

        if (ck::IsValid(Source) && Source.Has<FTag_Inventory_OperationRouted>())
        { return ck::EPacedStepResult::Continue; }

        if (ck::IsValid(Source))
        { Source.Add<FTag_Inventory_OperationRouted>(); }
        InOperation.Add<FTag_Inventory_MassTransferStepRouted>();

        Target.AddOrGet<FFragment_Inventory_OperationQueue>().Get_OperationsMutable().Emplace(
            FQueuedInventoryOperation_MassTransferStep{InOperation, Item, Source, Target});

        return ck::EPacedStepResult::Continue;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Inventory_MassTransfer_Churn::
        ExecuteQueuedStep(
            FFragment_Inventory_MassTransfer_InFlight& InFlight,
            FCk_Handle_Item InItem,
            FCk_Handle_Inventory InSource,
            FCk_Handle_Inventory InTarget) -> void
    {
        auto Item   = InItem;
        auto Target = InTarget;

        const auto Units = inventory_handlers::ExecuteTransferNow(
            Item, Target, InFlight._TargetResolution.Get_AddPolicy());

        if (Units > 0)
        {
            InFlight._UnitsMoved  += Units;
            InFlight._AnyUnitMoved = true;

            const auto ItemLeftSource = ck::Is_NOT_Valid(Item)
                || ck::Is_NOT_Valid(InSource)
                || NOT UCk_Utils_Inventory_UE::Get_ContainsItem(InSource, Item);
            if (ItemLeftSource)
            { ++InFlight._ItemsFullyMoved; }
        }
        else
        {
            ++InFlight._ItemsFailed;
        }

        ++InFlight._Cursor;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Inventory_MassTransfer_Churn::
        ComputeFinish(
            const FFragment_Inventory_MassTransfer_InFlight& InFlight)
        -> FFinishSnapshot
    {
        auto Out = FFinishSnapshot{};
        Out.Units  = InFlight._UnitsMoved;
        Out.Fully  = InFlight._ItemsFullyMoved;
        Out.Failed = InFlight._ItemsFailed;

        if (InFlight._Pending.IsEmpty())
        { Out.Result = ECk_Inventory_MassTransfer_Result::Failed_NothingToTransfer; }
        else if (NOT InFlight._AnyUnitMoved)
        { Out.Result = ECk_Inventory_MassTransfer_Result::Failed_NoCandidateAccepts; }
        else if (InFlight._ItemsFailed == 0)
        { Out.Result = ECk_Inventory_MassTransfer_Result::Success; }
        else
        { Out.Result = ECk_Inventory_MassTransfer_Result::Success_Partial; }

        return Out;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Inventory_MassTransfer_CancelOnEndPlay::
        ForEachEntity(
            TimeType,
            HandleType InHandle,
            const FFragment_Inventory_MassTransfer_InFlight& InFlight)
        -> void
    {
        UUtils_Signal_Inventory_MassTransfer_OnComplete::Broadcast(
            InHandle, MakePayload(InHandle,
                ECk_Inventory_MassTransfer_Result::Failed_OperationCancelled,
                InFlight.Get_UnitsMoved(), InFlight.Get_ItemsFullyMoved(), InFlight.Get_ItemsFailed()));

        InFlight.Get_CompletionDelegate().ExecuteIfBound(InHandle,
            ECk_Request_OperationResult::Failed_Cancelled);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        inventory_operation_queue::
        Execute(
            const FQueuedInventoryOperation& InOperation) -> void
    {
        std::visit(ck_inventory_processor::TOverloaded{
            [](const FQueuedInventoryOperation_DataOnlySource& InQueued)
            {
                auto Source = InQueued.Source;
                using Traits = TInventoryRequestTraits<FCk_Handle_Inventory_DataOnly>;

                std::visit([&](const auto& InEntry)
                {
                    if (ck::IsValid(Source) && Source.Has<FFragment_Inventory_Params>())
                    {
                        inventory_handlers::DispatchToHandler<Traits>(
                            Source, Source.Get<FFragment_Inventory_Params>(), InEntry);
                    }
                    else
                    { inventory_handlers::DispatchCancel<Traits>(Source, InEntry); }
                }, InQueued.Request);

                if (ck::IsValid(Source))
                { UCk_Utils_Inventory_UE::Request_MarkInventory_AsMayHaveChanged(Source); }
                ReleaseSource(Source);
            },
            [](const FQueuedInventoryOperation_SpatialSource& InQueued)
            {
                auto Source = InQueued.Source;
                using Traits = TInventoryRequestTraits<FCk_Handle_Inventory_Spatial>;

                std::visit([&](const auto& InEntry)
                {
                    if (ck::IsValid(Source) && Source.Has<FFragment_Inventory_Params>())
                    {
                        inventory_handlers::DispatchToHandler<Traits>(
                            Source, Source.Get<FFragment_Inventory_Params>(), InEntry);
                    }
                    else
                    { inventory_handlers::DispatchCancel<Traits>(Source, InEntry); }
                }, InQueued.Request);

                if (ck::IsValid(Source))
                { UCk_Utils_Inventory_UE::Request_MarkInventory_AsMayHaveChanged(Source); }
                ReleaseSource(Source);
            },
            [](const FQueuedInventoryOperation_MassTransferStep& InQueued)
            {
                auto Operation = InQueued.Operation;
                if (ck::IsValid(Operation)
                    && Operation.Has<FFragment_Inventory_MassTransfer_InFlight>())
                {
                    auto& InFlight = Operation.Get<FFragment_Inventory_MassTransfer_InFlight>();
                    FProcessor_Inventory_MassTransfer_Churn::ExecuteQueuedStep(
                        InFlight, InQueued.Item, InQueued.Source, InQueued.Target);

                    Operation.Try_Remove<FTag_Inventory_MassTransferStepRouted>();
                    Operation.AddOrGet<FFragment_Inventory_MassTransfer_InFlight>();
                }

                ReleaseSource(InQueued.Source);
            }}, InOperation);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        inventory_operation_queue::
        ReleaseSource(
            FCk_Handle_Inventory InSource) -> void
    {
        if (ck::Is_NOT_Valid(InSource))
        { return; }

        InSource.Try_Remove<FTag_Inventory_OperationRouted>();

        if (InSource.Has<FFragment_Inventory_DataOnly_Requests>())
        { InSource.AddOrGet<FFragment_Inventory_DataOnly_Requests>(); }
        else if (InSource.Has<FFragment_Inventory_Spatial_Requests>())
        { InSource.AddOrGet<FFragment_Inventory_Spatial_Requests>(); }
        else
        { InSource.Try_Remove<FFragment_PacedWork>(); }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        inventory_operation_queue::
        CancelAll(
            const FFragment_Inventory_OperationQueue& InQueue) -> void
    {
        // Cancellation delegates are user code and may re-enter this inventory. Iterate a snapshot so
        // callbacks cannot invalidate the container being traversed.
        const auto Operations = InQueue.Get_Operations();
        for (const auto& Operation : Operations)
        {
            std::visit(ck_inventory_processor::TOverloaded{
                [](const FQueuedInventoryOperation_DataOnlySource& InQueued)
                {
                    auto Source = InQueued.Source;
                    using Traits = TInventoryRequestTraits<FCk_Handle_Inventory_DataOnly>;
                    std::visit([&](const auto& InEntry)
                    { inventory_handlers::DispatchCancel<Traits>(Source, InEntry); }, InQueued.Request);
                    ReleaseSource(Source);
                },
                [](const FQueuedInventoryOperation_SpatialSource& InQueued)
                {
                    auto Source = InQueued.Source;
                    using Traits = TInventoryRequestTraits<FCk_Handle_Inventory_Spatial>;
                    std::visit([&](const auto& InEntry)
                    { inventory_handlers::DispatchCancel<Traits>(Source, InEntry); }, InQueued.Request);
                    ReleaseSource(Source);
                },
                [](const FQueuedInventoryOperation_MassTransferStep& InQueued)
                {
                    auto Operation = InQueued.Operation;
                    if (ck::IsValid(Operation)
                        && Operation.Has<FFragment_Inventory_MassTransfer_InFlight>())
                    {
                        Operation.Try_Remove<FTag_Inventory_MassTransferStepRouted>();
                        Operation.AddOrGet<FFragment_Inventory_MassTransfer_InFlight>();
                    }
                    ReleaseSource(InQueued.Source);
                }}, Operation);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
