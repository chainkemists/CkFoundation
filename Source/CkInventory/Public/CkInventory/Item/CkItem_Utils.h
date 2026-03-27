#pragma once

#include "CkInventory/Item/CkItem_Fragment.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkInventory/Inventory/CkInventory_Fragment_Data.h"

#include "CkItem_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_InventoryItem_Definition;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Item"))
class CKINVENTORY_API UCk_Utils_Item_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Item_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Item);

public:
    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|Item",
              DisplayName = "[Ck][Item] Create New Item")
    static FCk_Handle_Item
    Create(
        UPARAM(ref) FCk_Handle& InOwnerEntity,
        const UCk_InventoryItem_Definition* InDefinition);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Item",
              DisplayName = "[Ck][Item] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Item
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Item",
              DisplayName = "[Ck][Item] Handle -> Item Handle",
              meta = (CompactNodeTitle = "<AsItem>", BlueprintAutocast))
    static FCk_Handle_Item
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Invalid Item Handle",
              Category = "Ck|Utils|Item",
              meta = (CompactNodeTitle = "INVALID_ItemHandle", Keywords = "make"))
    static FCk_Handle_Item
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Item",
              DisplayName = "[Ck][Item] Get Definition")
    static const UCk_InventoryItem_Definition*
    Get_Definition(
        const FCk_Handle_Item& InItem);

    /** Returns the inventory this item belongs to, or invalid if not in any inventory. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Item",
              DisplayName = "[Ck][Item] Get Parent Inventory")
    static FCk_Handle_Inventory
    Get_ParentInventory(
        const FCk_Handle_Item& InItem);
};

// --------------------------------------------------------------------------------------------------------------------
