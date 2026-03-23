#include "CkInventory_ProcessorInjector.h"

#include "CkInventory/Inventory/CkInventory_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Inventory_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    // 1. Client-side: reconcile replicated state onto grid cells
    InWorld.Add<ck::FProcessor_Inventory_SyncReplication>(InWorld.Get_Registry());

    // 2. Authority: process inventory requests (add, remove, stack, split, transfer)
    InWorld.Add<ck::FProcessor_Inventory_HandleRequests>(InWorld.Get_Registry());

    // 3. Authority: diff current items vs previous snapshot, fire OnItemsChanged + trigger replication
    InWorld.Add<ck::FProcessor_Inventory_FireSignals>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Inventory_ProcessorInjector_Replicate_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    // 3. Server-side: collect grid cell/item state for replication
    InWorld.Add<ck::FProcessor_Inventory_Replicate>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
