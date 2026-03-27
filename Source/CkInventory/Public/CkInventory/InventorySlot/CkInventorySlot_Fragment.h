#pragma once

#include "CkInventorySlot_Fragment_Data.h"

#include "CkEcsExt/EntityHolder/CkEntityHolder_Fragment.h"
#include "CkEcsExt/EntityHolder/CkEntityHolder_Utils.h"

#include "CkInventory/Item/CkItem_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_ENTITY_HOLDER_AND_UTILS(TUtils_InventorySlot_ItemRef, FFragment_InventorySlot_ItemRef, FCk_Handle_Item);
}

// --------------------------------------------------------------------------------------------------------------------
