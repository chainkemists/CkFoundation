#include "CkInventory_DataOnly_Processor.h"

#include "CkInventory/CkInventory_Log.h"
#include "CkInventory/Inventory/CkInventory_Utils.h"
#include "CkInventory/Item/CkItem_Definition.h"
#include "CkInventory/Item/CkItem_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkRecord/Record/CkRecord_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_DataOnly_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_DataOnly_CancelOnEndPlay);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_DataOnly_Replicate);
CK_REGISTER_PROCESSOR(ck::FProcessor_Inventory_DataOnly_SyncReplication);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    using FInventoryItemRecord = UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils;

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Inventory_DataOnly_Replicate::
        ForEachEntity(
            TimeType,
            HandleType& InHandle,
            const FFragment_Inventory_Params&) -> void
    {
        auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

        // One projection: the registered Produce (keyed on this inventory entity) rebuilds the identical item
        // array via the same record walk. Full-replace the owner (lifetime-owner) container with it, as today.
        // KNOWN PRE-EXISTING GAP (unchanged here): two inventories sharing one owner clobber this container entry.
        const auto Produced = UCk_Utils_Net_UE::TryProduce<FCk_RepData_Inventory_DataOnly_Items>(InHandle);
        if (Produced.IsSet())
        { UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_Inventory_DataOnly_Items>(LifetimeOwner, *Produced); }

        InHandle.Remove<MarkedDirtyBy>();
    }
}

// --------------------------------------------------------------------------------------------------------------------
