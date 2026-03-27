#pragma once

#include "CkInventory/Inventory/CkInventory_Fragment_Data.h"
#include "CkInventory/Item/CkItem_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include <UObject/Object.h>

#include "CkInventoryUI_ListViewObject.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Lightweight UObject wrapper for inventory item data.
 * Required because UListView operates on UObject* items.
 * Blueprint widgets implementing IUserObjectListEntry receive this
 * in OnListItemObjectSet and query display data via the handles.
 */
UCLASS(BlueprintType)
class CKINVENTORY_API UCk_InventoryUI_ListViewObject : public UObject
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_InventoryUI_ListViewObject);

private:
    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Item _ItemHandle;

    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Inventory _InventoryHandle;

public:
    CK_PROPERTY_GET(_ItemHandle);
    CK_PROPERTY_GET(_InventoryHandle);
    CK_PROPERTY_SET(_ItemHandle);
    CK_PROPERTY_SET(_InventoryHandle);
};

// --------------------------------------------------------------------------------------------------------------------
