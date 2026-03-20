#pragma once

#include "CkInventory/InventorySlot/CkInventorySlot_Fragment.h"
#include "CkInventory/Item/CkInventoryItem_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Utils.h"

#include "CkInventorySlot_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_InventorySlot"))
class CKINVENTORY_API UCk_Utils_InventorySlot_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_InventorySlot_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_InventorySlot);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

    static FCk_Handle_Item
    Get_Item(
        const FCk_Handle_InventorySlot& InSlot);

    static bool
    Get_IsOccupied(
        const FCk_Handle_InventorySlot& InSlot);

    static FIntPoint
    Get_Coordinate(
        const FCk_Handle_InventorySlot& InSlot);
};

// --------------------------------------------------------------------------------------------------------------------
