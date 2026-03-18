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

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InventorySlot",
              DisplayName = "[Ck][InventorySlot] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_InventorySlot
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InventorySlot",
              DisplayName = "[Ck][InventorySlot] Handle -> Slot Handle",
              meta = (CompactNodeTitle = "<AsInventorySlot>", BlueprintAutocast))
    static FCk_Handle_InventorySlot
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Invalid InventorySlot Handle",
              Category = "Ck|Utils|InventorySlot",
              meta = (CompactNodeTitle = "INVALID_InventorySlotHandle", Keywords = "make"))
    static FCk_Handle_InventorySlot
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InventorySlot",
              DisplayName = "[Ck][InventorySlot] Get Item")
    static FCk_Handle_Item
    Get_Item(
        const FCk_Handle_InventorySlot& InSlot);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InventorySlot",
              DisplayName = "[Ck][InventorySlot] Get Is Occupied")
    static bool
    Get_IsOccupied(
        const FCk_Handle_InventorySlot& InSlot);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InventorySlot",
              DisplayName = "[Ck][InventorySlot] Get Coordinate")
    static FIntPoint
    Get_Coordinate(
        const FCk_Handle_InventorySlot& InSlot);
};

// --------------------------------------------------------------------------------------------------------------------
