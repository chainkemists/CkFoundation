#pragma once

#include "CkInventory/Inventory/Coordinator/CkInventory_OperationCoordinator_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::inventory_operation_coordinator
{
    CKINVENTORY_API auto
    ReserveSubmissionOrdinal(
        const FCk_Handle& InAnyHandle)
        -> uint64;

    CKINVENTORY_API auto
    Submit(
        const FCk_Handle& InAnyHandle,
        FInventoryOperation_Submission InSubmission)
        -> bool;
}

// --------------------------------------------------------------------------------------------------------------------
