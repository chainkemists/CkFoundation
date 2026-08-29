#pragma once

#include "CkInventory/Inventory/DataOnly/CkInventory_DataOnly_Fragment.h"
#include "CkInventory/Inventory/Spatial/CkInventory_Spatial_Fragment.h"

#include "CkEcs/Pacing/CkPacedWork.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FQueuedInventoryOperation_DataOnlySource
    {
        FCk_Handle_Inventory_DataOnly Source;
        TInventoryRequestTraits<FCk_Handle_Inventory_DataOnly>::Variant Request;
    };

    struct FQueuedInventoryOperation_SpatialSource
    {
        FCk_Handle_Inventory_Spatial Source;
        TInventoryRequestTraits<FCk_Handle_Inventory_Spatial>::Variant Request;
    };

    struct FQueuedInventoryOperation_MassTransferStep
    {
        FCk_Handle           Operation;
        FCk_Handle_Item      Item;
        FCk_Handle_Inventory Source;
        FCk_Handle_Inventory Target;
    };

    using FQueuedInventoryOperation = std::variant<
        FQueuedInventoryOperation_DataOnlySource,
        FQueuedInventoryOperation_SpatialSource,
        FQueuedInventoryOperation_MassTransferStep>;

    struct CKINVENTORY_API FFragment_Inventory_OperationQueue
    {
    public:
        CK_GENERATED_BODY(FFragment_Inventory_OperationQueue);

    private:
        TArray<FQueuedInventoryOperation> _Operations;
        FFragment_PacedWork _Pacer{1, 1};

    public:
        CK_PROPERTY_GET(_Operations);
        auto Get_OperationsMutable() -> TArray<FQueuedInventoryOperation>& { return _Operations; }
        auto Get_PacerMutable() -> FFragment_PacedWork& { return _Pacer; }
    };
}

// --------------------------------------------------------------------------------------------------------------------
