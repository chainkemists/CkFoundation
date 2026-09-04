#include "CkInventory_OperationCoordinator_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::inventory_operation_coordinator
{
    auto
        ReserveSubmissionOrdinal(
            const FCk_Handle& InAnyHandle)
        -> uint64
    {
        const auto IsHandleValid = ck::IsValid(InAnyHandle);
        CK_ENSURE_IF_NOT(IsHandleValid,
            TEXT("Cannot reserve an inventory operation ordinal from invalid handle [{}]"), InAnyHandle)
        { return 0; }

        auto Coordinator = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InAnyHandle);
        const auto IsCoordinatorValid = ck::IsValid(Coordinator);
        CK_ENSURE_IF_NOT(IsCoordinatorValid,
            TEXT("Cannot resolve the inventory operation coordinator from handle [{}]"), InAnyHandle)
        { return 0; }

        return Coordinator.AddOrGet<FFragment_Inventory_OperationCoordinator_Current>()
            .ReserveSubmissionOrdinal();
    }

    auto
        Submit(
            const FCk_Handle& InAnyHandle,
            FInventoryOperation_Submission InSubmission)
        -> bool
    {
        const auto IsHandleValid = ck::IsValid(InAnyHandle);
        CK_ENSURE_IF_NOT(IsHandleValid,
            TEXT("Cannot submit an inventory operation from invalid handle [{}]"), InAnyHandle)
        { return false; }

        auto Coordinator = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InAnyHandle);
        const auto IsCoordinatorValid = ck::IsValid(Coordinator);
        CK_ENSURE_IF_NOT(IsCoordinatorValid,
            TEXT("Cannot resolve the inventory operation coordinator from handle [{}]"), InAnyHandle)
        { return false; }

        if (InSubmission.Ordinal == 0)
        {
            InSubmission.Ordinal = Coordinator.AddOrGet<FFragment_Inventory_OperationCoordinator_Current>()
                .ReserveSubmissionOrdinal();
        }

        if (ck::IsValid(InSubmission.Source))
        { InSubmission.Participants.AddUnique(InSubmission.Source); }
        InSubmission.Participants.Sort([](const FCk_Handle_Inventory& InA, const FCk_Handle_Inventory& InB)
        { return InA < InB; });

        CK_CALLSTACK_RECORD(FFragment_Inventory_OperationCoordinator_Requests, Coordinator);
        Coordinator.AddOrGet<FFragment_Inventory_OperationCoordinator_Requests>()
            .Get_RequestsMutable().Emplace(MoveTemp(InSubmission));
        return true;
    }
}

// --------------------------------------------------------------------------------------------------------------------
