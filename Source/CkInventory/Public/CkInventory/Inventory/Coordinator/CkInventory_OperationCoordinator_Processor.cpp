#include "CkInventory_OperationCoordinator_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkInventory/Inventory/CkInventory_Processor.h"
#include "CkInventory/Inventory/CkInventory_RequestHandlers.h"
#include "CkInventory/Inventory/CkInventory_Utils.h"
#include "CkInventory/Inventory/DataOnly/CkInventory_DataOnly_Processor.h"
#include "CkInventory/Inventory/Spatial/CkInventory_Spatial_Processor.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_inventory_operation_coordinator
{
    template <typename... TFunctions>
    struct TOverloaded : TFunctions...
    {
        using TFunctions::operator()...;
    };

    template <typename... TFunctions>
    TOverloaded(TFunctions...) -> TOverloaded<TFunctions...>;

    auto
        AreParticipantsValid(
            const ck::FInventoryOperation_Submission& InSubmission)
        -> bool
    {
        return ck::algo::AllOf(InSubmission.Participants, [](const FCk_Handle_Inventory& InParticipant)
        { return ck::IsValid(InParticipant); });
    }

    auto
        Execute(
            const ck::FInventoryOperation_Submission& InSubmission)
        -> void
    {
        const auto ParticipantsAreValid = AreParticipantsValid(InSubmission);

        std::visit(TOverloaded{
            [&](const ck::FInventoryOperation_DataOnlySource& InOperation)
            {
                auto Source = InOperation.Source;
                using Traits = ck::TInventoryRequestTraits<FCk_Handle_Inventory_DataOnly>;

                std::visit([&](const auto& InEntry)
                {
                    if (ParticipantsAreValid && ck::IsValid(Source)
                        && Source.Has<ck::FFragment_Inventory_Params>())
                    {
                        ck::inventory_handlers::DispatchToHandler<Traits>(
                            Source, Source.Get<ck::FFragment_Inventory_Params>(), InEntry);
                    }
                    else
                    { ck::inventory_handlers::DispatchCancel<Traits>(Source, InEntry); }
                }, InOperation.Request);

                if (ck::IsValid(Source))
                { UCk_Utils_Inventory_UE::Request_MarkInventory_AsMayHaveChanged(Source); }
            },
            [&](const ck::FInventoryOperation_SpatialSource& InOperation)
            {
                auto Source = InOperation.Source;
                using Traits = ck::TInventoryRequestTraits<FCk_Handle_Inventory_Spatial>;

                std::visit([&](const auto& InEntry)
                {
                    if (ParticipantsAreValid && ck::IsValid(Source)
                        && Source.Has<ck::FFragment_Inventory_Params>())
                    {
                        ck::inventory_handlers::DispatchToHandler<Traits>(
                            Source, Source.Get<ck::FFragment_Inventory_Params>(), InEntry);
                    }
                    else
                    { ck::inventory_handlers::DispatchCancel<Traits>(Source, InEntry); }
                }, InOperation.Request);

                if (ck::IsValid(Source))
                { UCk_Utils_Inventory_UE::Request_MarkInventory_AsMayHaveChanged(Source); }
            },
            [&](const ck::FInventoryOperation_MassTransferStep& InOperation)
            {
                auto Operation = InOperation.Operation;
                if (ck::Is_NOT_Valid(Operation)
                    || NOT Operation.Has<ck::FFragment_Inventory_MassTransfer_InFlight>())
                { return; }

                auto& InFlight = Operation.Get<ck::FFragment_Inventory_MassTransfer_InFlight>();
                InFlight.Set_StepSubmitted(false);

                if (ParticipantsAreValid)
                {
                    ck::FProcessor_Inventory_MassTransfer_Churn::ExecuteQueuedStep(
                        InFlight, InOperation.Item, InOperation.Source, InOperation.Target);
                }

                Operation.AddOrGet<ck::FFragment_Inventory_MassTransfer_InFlight>();
            }}, InSubmission.Operation);
    }

    auto
        CancelOrdinary(
            const ck::FInventoryOperation_Submission& InSubmission)
        -> void
    {
        std::visit(TOverloaded{
            [](const ck::FInventoryOperation_DataOnlySource& InOperation)
            {
                auto Source = InOperation.Source;
                using Traits = ck::TInventoryRequestTraits<FCk_Handle_Inventory_DataOnly>;
                std::visit([&](const auto& InEntry)
                { ck::inventory_handlers::DispatchCancel<Traits>(Source, InEntry); }, InOperation.Request);
            },
            [](const ck::FInventoryOperation_SpatialSource& InOperation)
            {
                auto Source = InOperation.Source;
                using Traits = ck::TInventoryRequestTraits<FCk_Handle_Inventory_Spatial>;
                std::visit([&](const auto& InEntry)
                { ck::inventory_handlers::DispatchCancel<Traits>(Source, InEntry); }, InOperation.Request);
            },
            [](const ck::FInventoryOperation_MassTransferStep&) {}}, InSubmission.Operation);
    }
}

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_OperationCoordinator_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_OperationCoordinator_CancelOnEndPlay);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
    FProcessor_Inventory_OperationCoordinator_HandleRequests::
    ForEachEntity(
        TimeType,
        HandleType InCoordinator,
        FFragment_Inventory_OperationCoordinator_Current& InCurrent,
        FFragment_Inventory_OperationCoordinator_Requests& InRequests) -> void
    {
        const auto RequestsCopy = InRequests.Get_Requests();
        InRequests.Get_RequestsMutable().Reset();

        auto& Pending = InCurrent.Get_PendingMutable();
        Pending.Append(RequestsCopy);
        Pending.StableSort([](const FInventoryOperation_Submission& InA, const FInventoryOperation_Submission& InB)
        { return InA.Ordinal < InB.Ordinal; });

        auto SeenSources = TArray<FCk_Handle_Inventory>{};
        auto ReservedParticipants = TArray<FCk_Handle_Inventory>{};
        auto AdmittedOrdinals = TArray<uint64>{};

        for (const auto& Submission : Pending)
        {
            if (SeenSources.Contains(Submission.Source))
            { continue; }
            SeenSources.Add(Submission.Source);

            const auto HasReservedParticipant = ck::algo::AnyOf(
                Submission.Participants,
                [&](const FCk_Handle_Inventory& InParticipant)
                { return ReservedParticipants.Contains(InParticipant); });
            if (HasReservedParticipant)
            { continue; }

            ReservedParticipants.Append(Submission.Participants);
            AdmittedOrdinals.Add(Submission.Ordinal);
        }

        auto Batch = TArray<FInventoryOperation_Submission>{};
        auto Remaining = TArray<FInventoryOperation_Submission>{};
        Batch.Reserve(AdmittedOrdinals.Num());
        Remaining.Reserve(Pending.Num() - AdmittedOrdinals.Num());

        for (auto& Submission : Pending)
        {
            if (AdmittedOrdinals.Contains(Submission.Ordinal))
            { Batch.Emplace(MoveTemp(Submission)); }
            else
            { Remaining.Emplace(MoveTemp(Submission)); }
        }
        Pending = MoveTemp(Remaining);

        if (Pending.IsEmpty())
        { InCoordinator.Remove<MarkedDirtyBy>(); }
        else
        { InCoordinator.AddOrGet<MarkedDirtyBy>(); }

        // Completion callbacks may enqueue requests or destroy entities. Everything needed for this
        // pass is local now; do not read coordinator fragments after dispatch begins.
        for (const auto& Submission : Batch)
        { ck_inventory_operation_coordinator::Execute(Submission); }
    }

    auto
    FProcessor_Inventory_OperationCoordinator_CancelOnEndPlay::
    ForEachEntity(
        TimeType,
        HandleType,
        const FFragment_Inventory_OperationCoordinator_Current& InCurrent,
        const FFragment_Inventory_OperationCoordinator_Requests& InRequests) -> void
    {
        const auto Pending = InCurrent.Get_Pending();
        for (const auto& Submission : Pending)
        { ck_inventory_operation_coordinator::CancelOrdinary(Submission); }

        const auto LiveRequests = InRequests.Get_Requests();
        for (const auto& Submission : LiveRequests)
        { ck_inventory_operation_coordinator::CancelOrdinary(Submission); }
    }
}

// --------------------------------------------------------------------------------------------------------------------
