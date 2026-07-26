#pragma once

#include "CkItem_Fragment_Data.h"

#include "CkCore/Enums/CkEnums.h"               // ECk_CardinalRotation

// --------------------------------------------------------------------------------------------------------------------

class UCk_InventoryItem_Definition;
class UCk_Utils_Inventory_Spatial_UE;

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKINVENTORY_API FFragment_InventoryItem
    {
        CK_GENERATED_BODY(FFragment_InventoryItem);

    private:
        TWeakObjectPtr<const UCk_InventoryItem_Definition> _Definition;

    public:
        CK_PROPERTY_GET(_Definition);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_InventoryItem, _Definition);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Written/removed EXCLUSIVELY by UCk_Utils_Inventory_Spatial_UE::Request_PlaceItemOnGrid /
    // Request_RemoveItemFromGrid, alongside the cell ItemRefs and the item Transform's yaw. Those
    // ItemRefs are derived state in the grid's private registry, so this is the snapshotable home.
    struct CKINVENTORY_API FFragment_Item_SpatialPlacement
    {
        CK_GENERATED_BODY(FFragment_Item_SpatialPlacement);

        friend class ::UCk_Utils_Inventory_Spatial_UE;

    private:
        FIntPoint _Anchor = FIntPoint::ZeroValue;
        ECk_CardinalRotation _Rotation = ECk_CardinalRotation::None;

    public:
        CK_PROPERTY_GET(_Anchor);
        CK_PROPERTY_GET(_Rotation);

    public:
        CK_DEFINE_CONSTRUCTORS(FFragment_Item_SpatialPlacement, _Anchor, _Rotation);
    };
}

// --------------------------------------------------------------------------------------------------------------------
