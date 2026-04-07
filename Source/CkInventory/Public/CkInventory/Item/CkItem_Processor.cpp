#include "CkItem_Processor.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkInventory/Inventory/CkInventory_Fragment.h"
#include "CkInventory/Inventory/CkInventory_Utils.h"
#include "CkInventory/Item/CkItem_Utils.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_InventoryItem_EndPlay);

// ============================================================================
// FProcessor_InventoryItem_EndPlay
// ============================================================================

namespace ck
{
    auto
        FProcessor_InventoryItem_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_InventoryItem&)
        -> void
    {
        if (NOT TUtils_Item_ParentInventory::Has(InHandle))
        { return; }

        auto InventoryHandle = TUtils_Item_ParentInventory::Get_StoredEntity(InHandle);

        if (ck::Is_NOT_Valid(InventoryHandle))
        { return; }

        // ---- Only enqueue if the item is still in the inventory record ----

        if (NOT FInventoryItemRecordUtils::Get_ContainsEntry(InventoryHandle, InHandle))
        { return; }

        // ---- Enqueue remove request so the inventory fires proper callbacks ----

        UCk_Utils_Inventory_UE::Request_RemoveItem(InventoryHandle, FCk_Request_Inventory_RemoveItem{InHandle}, {});
    }
}

// --------------------------------------------------------------------------------------------------------------------
