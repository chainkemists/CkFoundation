#pragma once

#include "CkInventory/Inventory/CkInventory_RequestHandlers.h"
#include "CkInventory/Inventory/DataOnly/CkInventory_DataOnly_Fragment_Data.h"

namespace ck
{
    // Relocate is intentionally omitted — DataOnly has no cell placement to relocate, and the
    // dispatcher SFINAE-skips the branch for it (known gap).
    template <>
    struct CKINVENTORY_API TInventoryRequestTraits<FCk_Handle_Inventory_DataOnly>
    {
        using HandleType          = FCk_Handle_Inventory_DataOnly;
        using ReplicationFragment = FCk_RepData_Inventory_DataOnly_Items;

        using AddItem             = inventory_handlers::TAddItem<FCk_Handle_Inventory_DataOnly>;
        using RemoveItem          = inventory_handlers::TRemoveItem<FCk_Handle_Inventory_DataOnly>;
        using StackItems          = inventory_handlers::TStackItems<FCk_Handle_Inventory_DataOnly>;
        using SplitStack          = inventory_handlers::TSplitStack<FCk_Handle_Inventory_DataOnly>;
        using AddByDefinition     = inventory_handlers::TAddByDefinition<FCk_Handle_Inventory_DataOnly>;
        using Sort                = inventory_handlers::TSort<FCk_Handle_Inventory_DataOnly>;
        using Transfer_ToSpatial  = inventory_handlers::TTransfer<FCk_Handle_Inventory_DataOnly, FCk_Handle_Inventory_Spatial>;
        using Transfer_ToDataOnly = inventory_handlers::TTransfer<FCk_Handle_Inventory_DataOnly, FCk_Handle_Inventory_DataOnly>;

        using Variant = std::variant<
            typename AddItem::Entry,
            typename RemoveItem::Entry,
            typename StackItems::Entry,
            typename SplitStack::Entry,
            typename AddByDefinition::Entry,
            typename Sort::Entry,
            typename Transfer_ToSpatial::Entry,
            typename Transfer_ToDataOnly::Entry>;

        static auto Setup_PerShape(FCk_Handle_Inventory&, const FCk_Fragment_Inventory_ParamsData&, ECk_Replication) -> void;
    };
}
