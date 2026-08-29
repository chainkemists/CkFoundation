#pragma once

#include "CkInventory/Inventory/Coordinator/CkInventory_OperationCoordinator_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Inventory_DataOnly_HandleRequests;
    class FProcessor_Inventory_Spatial_HandleRequests;
    class FProcessor_Inventory_MassTransfer_Churn;

    class CKINVENTORY_API FProcessor_Inventory_OperationCoordinator_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Inventory_OperationCoordinator_HandleRequests,
            FCk_Handle,
            TReadWrite<FFragment_Inventory_OperationCoordinator_Current>,
            TReadWrite<FFragment_Inventory_OperationCoordinator_Requests>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay;
        using RunAfter = TDepList<
            FProcessor_Inventory_DataOnly_HandleRequests,
            FProcessor_Inventory_Spatial_HandleRequests,
            FProcessor_Inventory_MassTransfer_Churn>;
        using MarkedDirtyBy = FFragment_Inventory_OperationCoordinator_Requests;
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InCoordinator,
            FFragment_Inventory_OperationCoordinator_Current& InCurrent,
            FFragment_Inventory_OperationCoordinator_Requests& InRequests) -> void;
    };

    class CKINVENTORY_API FProcessor_Inventory_OperationCoordinator_CancelOnEndPlay : public ck_exp::TProcessor<
            FProcessor_Inventory_OperationCoordinator_CancelOnEndPlay,
            FCk_Handle,
            TReadOnly<FFragment_Inventory_OperationCoordinator_Current>,
            TReadOnly<FFragment_Inventory_OperationCoordinator_Requests>,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InCoordinator,
            const FFragment_Inventory_OperationCoordinator_Current& InCurrent,
            const FFragment_Inventory_OperationCoordinator_Requests& InRequests) -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
