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

CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_FireSignals);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_MassTransfer_Churn);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_MassTransfer_CancelOnEndPlay);

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

        auto Finish = FFinishSnapshot{};

        // The in-flight fragment IS the dirty marker; RunPacedSteps removes it on Done. The completing
        // step snapshots the result into Finish BEFORE that removal — never read InFlight past Done.
        const auto Done = ck::RunPacedSteps<FFragment_Inventory_MassTransfer_InFlight>(
            Base, Pacer, InDeltaT, [&]() -> ck::EPacedStepResult { return DoOneStep(InFlight, Finish); });

        if (NOT Done)
        { return; }

        Base.Try_Remove<ck::FFragment_PacedWork>();

        UUtils_Signal_Inventory_MassTransfer_OnComplete::Broadcast(
            InHandle, MakePayload(InHandle, Finish.Result, Finish.Units, Finish.Fully, Finish.Failed));

        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Inventory_MassTransfer_Churn::
        DoOneStep(
            FFragment_Inventory_MassTransfer_InFlight& InFlight,
            FFinishSnapshot& OutFinish)
        -> ck::EPacedStepResult
    {
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

        const auto Units = inventory_handlers::ExecuteTransferNow(
            Item, Target, InFlight._TargetResolution.Get_AddPolicy());

        if (Units > 0)
        {
            InFlight._UnitsMoved  += Units;
            InFlight._AnyUnitMoved = true;

            const auto ItemLeftSource = ck::Is_NOT_Valid(Item)
                || ck::Is_NOT_Valid(Source)
                || NOT UCk_Utils_Inventory_UE::Get_ContainsItem(Source, Item);
            if (ItemLeftSource)
            { ++InFlight._ItemsFullyMoved; }
        }
        else
        {
            ++InFlight._ItemsFailed;
        }

        ++InFlight._Cursor;
        return ck::EPacedStepResult::Continue;
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
    }
}

// --------------------------------------------------------------------------------------------------------------------