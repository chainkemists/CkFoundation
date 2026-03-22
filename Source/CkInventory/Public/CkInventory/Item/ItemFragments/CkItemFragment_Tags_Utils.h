#pragma once

#include "CkInventory/Item/CkInventoryItem_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkEcs/Signal/CkSignal_Fragment_Data.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include <GameplayTagContainer.h>

#include "CkItemFragment_Tags_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_Payload_Item_OnTagsChanged
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Payload_Item_OnTagsChanged);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Item _Item;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _TagsAdded;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _TagsRemoved;

public:
    CK_PROPERTY_GET(_Item);
    CK_PROPERTY_GET(_TagsAdded);
    CK_PROPERTY_GET(_TagsRemoved);

    CK_DEFINE_CONSTRUCTORS(FCk_Payload_Item_OnTagsChanged, _Item, _TagsAdded, _TagsRemoved);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_ItemTags_OnTagsChanged,
    FCk_Handle_Item, InItem,
    FCk_Payload_Item_OnTagsChanged, InPayload);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKINVENTORY_API,
        ItemTags_OnTagsChanged,
        FCk_Delegate_ItemTags_OnTagsChanged,
        FCk_Handle_Item,
        FCk_Payload_Item_OnTagsChanged);
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Item"))
class CKINVENTORY_API UCk_Utils_ItemFragment_Tags_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ItemFragment_Tags_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InventoryItem|Tags",
              DisplayName = "[Ck][InventoryItem] Get Has Tags Feature")
    static bool
    Get_HasTagsFeature(
        const FCk_Handle_Item& InItem);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InventoryItem|Tags",
              DisplayName = "[Ck][InventoryItem] Get Tags")
    static FGameplayTagContainer
    Get_Tags(
        const FCk_Handle_Item& InItem);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InventoryItem|Tags",
              DisplayName = "[Ck][InventoryItem] Get Has Tag")
    static bool
    HasTag(
        const FCk_Handle_Item& InItem,
        FGameplayTag InTag);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InventoryItem|Tags",
              DisplayName = "[Ck][InventoryItem] Get Has Tag Exact")
    static bool
    HasTagExact(
        const FCk_Handle_Item& InItem,
        FGameplayTag InTag);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InventoryItem|Tags",
              DisplayName = "[Ck][InventoryItem] Get Has Any Tags")
    static bool
    HasAny(
        const FCk_Handle_Item& InItem,
        FGameplayTagContainer InTags);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InventoryItem|Tags",
              DisplayName = "[Ck][InventoryItem] Get Has All Tags")
    static bool
    HasAll(
        const FCk_Handle_Item& InItem,
        FGameplayTagContainer InTags);

    // ---- Requests (Authority Only) ----

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|InventoryItem|Tags",
              DisplayName = "[Ck][InventoryItem] Request Add Tags")
    static void
    Request_AddTags(
        UPARAM(ref) FCk_Handle_Item& InItem,
        const FGameplayTagContainer& InTags);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|InventoryItem|Tags",
              DisplayName = "[Ck][InventoryItem] Request Add Tag")
    static void
    Request_AddTag(
        UPARAM(ref) FCk_Handle_Item& InItem,
        FGameplayTag InTag);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|InventoryItem|Tags",
              DisplayName = "[Ck][InventoryItem] Request Remove Tags")
    static void
    Request_RemoveTags(
        UPARAM(ref) FCk_Handle_Item& InItem,
        const FGameplayTagContainer& InTags);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|InventoryItem|Tags",
              DisplayName = "[Ck][InventoryItem] Request Remove Tag")
    static void
    Request_RemoveTag(
        UPARAM(ref) FCk_Handle_Item& InItem,
        FGameplayTag InTag);

    // ---- Signals ----

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InventoryItem|Tags",
              DisplayName = "[Ck][InventoryItem] Bind To OnTagsChanged")
    static FCk_Handle_Item
    BindTo_OnTagsChanged(
        UPARAM(ref) FCk_Handle_Item& InItem,
        const FCk_Delegate_ItemTags_OnTagsChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InventoryItem|Tags",
              DisplayName = "[Ck][InventoryItem] Unbind From OnTagsChanged")
    static FCk_Handle_Item
    UnbindFrom_OnTagsChanged(
        UPARAM(ref) FCk_Handle_Item& InItem,
        const FCk_Delegate_ItemTags_OnTagsChanged& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
