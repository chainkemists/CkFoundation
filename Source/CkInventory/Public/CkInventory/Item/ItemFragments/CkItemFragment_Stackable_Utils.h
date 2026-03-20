#pragma once

#include "CkInventory/Item/CkInventoryItem_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkItemFragment_Stackable_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Item"))
class CKINVENTORY_API UCk_Utils_ItemFragment_Stackable_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ItemFragment_Stackable_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InventoryItem|Stackable",
              DisplayName = "[Ck][InventoryItem] Get Is Stackable")
    static bool
    Get_IsStackable(
        const FCk_Handle_Item& InItem);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InventoryItem|Stackable",
              DisplayName = "[Ck][InventoryItem] Get Stack Count")
    static int32
    Get_StackCount(
        const FCk_Handle_Item& InItem);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InventoryItem|Stackable",
              DisplayName = "[Ck][InventoryItem] Get Max Stack Size")
    static int32
    Get_MaxStackSize(
        const FCk_Handle_Item& InItem);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InventoryItem|Stackable",
              DisplayName = "[Ck][InventoryItem] Get Has Max Stack Size")
    static bool
    Get_HasMaxStackSize(
        const FCk_Handle_Item& InItem);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InventoryItem|Stackable",
              DisplayName = "[Ck][InventoryItem] Get Is Stack Full")
    static bool
    Get_IsStackFull(
        const FCk_Handle_Item& InItem);
};

// --------------------------------------------------------------------------------------------------------------------
