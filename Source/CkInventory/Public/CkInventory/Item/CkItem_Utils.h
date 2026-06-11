#pragma once

#include "CkInventory/Item/CkItem_Definition.h"
#include "CkInventory/Item/CkItem_Fragment.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkInventory/Inventory/CkInventory_Fragment_Data.h"

#include "CkItem_Utils.generated.h"

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
              Category = "Ck|Utils|Item",
              DisplayName = "[Ck][Item] Add Feature")
    static FCk_Handle_Item
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const UCk_InventoryItem_Definition* InDefinition);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|Item",
              DisplayName = "[Ck][Item] Create New Item")
    static FCk_Handle_Item
    Create(
        UPARAM(ref) FCk_Handle& InOwnerEntity,
        const UCk_InventoryItem_Definition* InDefinition);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Item",
              DisplayName = "[Ck][Item] Get Or Create Transient Item Definition")
    static UCk_InventoryItem_Definition*
    GetOrCreate_TransientItemDefinition(
        UObject* InOuter,
        FName InName,
        const FCk_InventoryItem_CoreInfo& InCoreInfo,
        const TArray<UCk_ItemTrait*>& InTraits);

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

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Item",
              DisplayName = "[Ck][Item] Has Item Trait",
              meta = (DeterminesOutputType = "InTraitClass"))
    static bool
    Has_ItemTrait(
        const FCk_Handle_Item& InItem,
        TSubclassOf<UCk_ItemTrait> InTraitClass);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Item",
              DisplayName = "[Ck][Item] Get Item Trait",
              meta = (DeterminesOutputType = "InTraitClass"))
    static const UCk_ItemTrait*
    Get_ItemTrait(
        const FCk_Handle_Item& InItem,
        TSubclassOf<UCk_ItemTrait> InTraitClass);

public:
    template<typename T> requires std::is_base_of_v<UCk_ItemTrait, T>
    static auto Has_ItemTrait(const FCk_Handle_Item& InItem) -> bool;

    template<typename T> requires std::is_base_of_v<UCk_ItemTrait, T>
    static auto Get_ItemTrait(const FCk_Handle_Item& InItem) -> const T*;
};

// --------------------------------------------------------------------------------------------------------------------

template<typename T> requires std::is_base_of_v<UCk_ItemTrait, T>
auto
    UCk_Utils_Item_UE::
    Has_ItemTrait(
        const FCk_Handle_Item& InItem)
    -> bool
{
    const auto* Definition = Get_Definition(InItem);

    if (ck::Is_NOT_Valid(Definition))
    { return false; }

    return Definition->Has_ItemTrait<T>();
}

// --------------------------------------------------------------------------------------------------------------------

template<typename T> requires std::is_base_of_v<UCk_ItemTrait, T>
auto
    UCk_Utils_Item_UE::
    Get_ItemTrait(
        const FCk_Handle_Item& InItem)
    -> const T*
{
    const auto* Definition = Get_Definition(InItem);

    if (ck::Is_NOT_Valid(Definition))
    { return nullptr; }

    return Definition->Get_ItemTrait<T>();
}

// --------------------------------------------------------------------------------------------------------------------
