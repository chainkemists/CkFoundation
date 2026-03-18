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

    // 2. Composite: iterates inventories, processes slot requests + fires slot signals
    InWorld.Add<ck::FProcessor_Inventory_ProcessSlots>(InWorld.Get_Registry());
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
