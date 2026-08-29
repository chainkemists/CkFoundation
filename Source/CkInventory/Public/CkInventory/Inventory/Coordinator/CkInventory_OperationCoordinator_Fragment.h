#pragma once

#include "CkInventory/Inventory/DataOnly/CkInventory_DataOnly_Fragment.h"
#include "CkInventory/Inventory/Spatial/CkInventory_Spatial_Fragment.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include <variant>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FInventoryOperation_DataOnlySource
    {
        FCk_Handle_Inventory_DataOnly Source;
        TInventoryRequestTraits<FCk_Handle_Inventory_DataOnly>::Variant Request;
    };

    struct FInventoryOperation_SpatialSource
    {
        FCk_Handle_Inventory_Spatial Source;
        TInventoryRequestTraits<FCk_Handle_Inventory_Spatial>::Variant Request;
    };

    struct FInventoryOperation_MassTransferStep
    {
        FCk_Handle           Operation;
        FCk_Handle_Item      Item;
        FCk_Handle_Inventory Source;
        FCk_Handle_Inventory Target;
    };

    using FInventoryOperation = std::variant<
        FInventoryOperation_DataOnlySource,
        FInventoryOperation_SpatialSource,
        FInventoryOperation_MassTransferStep>;

    struct FInventoryOperation_Submission
    {
        uint64                       Ordinal = 0;
        FCk_Handle_Inventory         Source;
        TArray<FCk_Handle_Inventory> Participants;
        FInventoryOperation          Operation;
    };

    struct CKINVENTORY_API FFragment_Inventory_OperationCoordinator_Requests
    {
    public:
        CK_GENERATED_BODY(FFragment_Inventory_OperationCoordinator_Requests);

    private:
        TArray<FInventoryOperation_Submission> _Requests;

    public:
        CK_PROPERTY_GET(_Requests);
        auto
        Get_RequestsMutable()
            -> TArray<FInventoryOperation_Submission>&
        { return _Requests; }
    };

    CK_ECS_DEFINE_CALLSTACK_FRAGMENT_FOR(FFragment_Inventory_OperationCoordinator_Requests);

    struct CKINVENTORY_API FFragment_Inventory_OperationCoordinator_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_Inventory_OperationCoordinator_Current);

    private:
        TArray<FInventoryOperation_Submission> _Pending;
        uint64 _NextSubmissionOrdinal = 1;

    public:
        CK_PROPERTY_GET(_Pending);
        auto
        Get_PendingMutable()
            -> TArray<FInventoryOperation_Submission>&
        { return _Pending; }

        auto
        ReserveSubmissionOrdinal()
            -> uint64
        { return _NextSubmissionOrdinal++; }
    };
}

// --------------------------------------------------------------------------------------------------------------------
