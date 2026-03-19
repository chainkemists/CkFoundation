#include "CkInventoryItem_Definition.h"

#include "CkInventory/Item/CkInventoryItem_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InventoryItem_Definition::
    DoConstruct_Implementation(
        FCk_Handle& InHandle) const
    -> void
{
    InHandle.Add<ck::FFragment_InventoryItem_Params>(
        FCk_Fragment_InventoryItem_ParamsData(
            TSoftObjectPtr<UCk_InventoryItem_Definition>(const_cast<UCk_InventoryItem_Definition*>(this))));
}

// --------------------------------------------------------------------------------------------------------------------
