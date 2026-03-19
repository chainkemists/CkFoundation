#pragma once

#include "CkInventory/Item/CkInventoryItem_Fragment.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkInventoryItem_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_InventoryItem_Definition;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Item"))
class CKINVENTORY_API UCk_Utils_InventoryItem_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_InventoryItem_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Item);

public:
    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|InventoryItem",
              DisplayName = "[Ck][InventoryItem] Create New Item")
    static FCk_Handle_Item
    Create(
        UPARAM(ref) FCk_Handle& InOwnerEntity,
        UCk_InventoryItem_Definition* InDefinition);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InventoryItem",
              DisplayName = "[Ck][InventoryItem] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Item
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InventoryItem",
              DisplayName = "[Ck][InventoryItem] Handle -> Item Handle",
              meta = (CompactNodeTitle = "<AsItem>", BlueprintAutocast))
    static FCk_Handle_Item
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Invalid Item Handle",
              Category = "Ck|Utils|InventoryItem",
              meta = (CompactNodeTitle = "INVALID_ItemHandle", Keywords = "make"))
    static FCk_Handle_Item
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InventoryItem",
              DisplayName = "[Ck][InventoryItem] Get Definition")
    static TSoftObjectPtr<UCk_InventoryItem_Definition>
    Get_Definition(
        const FCk_Handle_Item& InItem);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InventoryItem",
              DisplayName = "[Ck][InventoryItem] Get Dimensions")
    static FIntPoint
    Get_Dimensions(
        const FCk_Handle_Item& InItem);
};

// --------------------------------------------------------------------------------------------------------------------
