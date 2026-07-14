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

    // The item's Spatial placement DECISION RECORD: where this item was placed on its inventory's grid.
    // Written/removed exclusively by UCk_Utils_Inventory_Spatial_UE::Request_PlaceItemOnGrid /
    // Request_RemoveItemFromGrid — the same two functions that stamp the cell ItemRefs (DERIVED state,
    // living in the grid's PRIVATE cell registry, invisible to snapshots) and the item Transform's yaw.
    // This is the snapshotable home for the coordinate (and the O(1) read path for placement queries —
    // the cell scan remains the fallback for pre-fragment data).
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
