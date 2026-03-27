#pragma once

#include "CkInventory/Inventory/CkInventory_Fragment_Data.h"
#include "CkInventory/Item/CkItem_Fragment_Data.h"

#include "CkCore/Macros/CkMacros.h"

#include <Blueprint/DragDropOperation.h>

#include "CkInventoryUI_DragDropOperation.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_InventoryUI_DropOperation : uint8
{
    Cancel,
    MoveItem,
    StackItems,
    SplitStack,
    SwapItems
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_InventoryUI_DropOperation);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Drag-drop operation carrying inventory item context.
 * Created by UCk_InventoryUI_ItemSlotEntry when a drag is detected.
 * Drop targets inspect _SourceItem and _SourceInventory to determine
 * valid operations and enqueue the appropriate inventory request.
 */
UCLASS(BlueprintType)
class CKINVENTORY_API UCk_InventoryUI_DragDropOperation : public UDragDropOperation
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_InventoryUI_DragDropOperation);

private:
    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Item _SourceItem;

    UPROPERTY(BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Inventory _SourceInventory;

    UPROPERTY(BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_InventoryUI_DropOperation _DropOperation = ECk_InventoryUI_DropOperation::Cancel;

public:
    CK_PROPERTY(_SourceItem);
    CK_PROPERTY(_SourceInventory);
    CK_PROPERTY(_DropOperation);
};

// --------------------------------------------------------------------------------------------------------------------
