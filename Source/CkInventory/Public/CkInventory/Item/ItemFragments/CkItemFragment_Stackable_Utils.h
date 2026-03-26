#pragma once

#include "CkInventory/Item/CkInventoryItem_Fragment_Data.h"
#include "CkInventory/Inventory/CkInventory_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkEcs/Signal/CkSignal_Fragment_Data.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkItemFragment_Stackable_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_Payload_Item_OnStackCountChanged
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Payload_Item_OnStackCountChanged);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Item _Item;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _NewCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _PreviousCount = 0;

public:
    CK_PROPERTY_GET(_Item);
    CK_PROPERTY_GET(_NewCount);
    CK_PROPERTY_GET(_PreviousCount);

    CK_DEFINE_CONSTRUCTORS(FCk_Payload_Item_OnStackCountChanged, _Item, _NewCount, _PreviousCount);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Stackable_OnStackCountChanged,
    FCk_Handle_Item, InItem,
    FCk_Payload_Item_OnStackCountChanged, InPayload);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE(
        CKINVENTORY_API,
        Stackable_OnStackCountChanged,
        FCk_Delegate_Stackable_OnStackCountChanged,
        FCk_Handle_Item,
        FCk_Payload_Item_OnStackCountChanged);
}

// --------------------------------------------------------------------------------------------------------------------

struct FCk_Handle_Inventory;
class UCk_InventoryItem_Definition;

namespace ck { class FProcessor_Inventory_HandleRequests; }

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Item"))
class CKINVENTORY_API UCk_Utils_ItemFragment_Stackable_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

    friend class ck::FProcessor_Inventory_HandleRequests;

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

    /** Returns how many more units the target item can accept.
     *  Returns MAX_int32 if no max stack size is defined. Returns 0 if full or not stackable. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InventoryItem|Stackable",
              DisplayName = "[Ck][InventoryItem] Get Remaining Stack Capacity")
    static int32
    Get_RemainingStackCapacity(
        const FCk_Handle_Item& InItem);

    /** Full validation for merging two specific items. Both items must be valid. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|InventoryItem|Stackable",
              DisplayName = "[Ck][InventoryItem] Get Can Stack Items")
    static ECk_Inventory_OperationResult_Stack
    Get_CanStackItems(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InSourceItem,
        const FCk_Handle_Item& InTargetItem);

    // ---- Signals ----

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InventoryItem|Stackable",
              DisplayName = "[Ck][InventoryItem] Bind To OnStackCountChanged")
    static FCk_Handle_Item
    BindTo_OnStackCountChanged(
        UPARAM(ref) FCk_Handle_Item& InItem,
        const FCk_Delegate_Stackable_OnStackCountChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|InventoryItem|Stackable",
              DisplayName = "[Ck][InventoryItem] Unbind From OnStackCountChanged")
    static FCk_Handle_Item
    UnbindFrom_OnStackCountChanged(
        UPARAM(ref) FCk_Handle_Item& InItem,
        const FCk_Delegate_Stackable_OnStackCountChanged& InDelegate);

private:
    static void
    Request_OverrideStackCount(
        const FCk_Handle_Item& InItem,
        int32 InNewCount);

    /** Resolves a FMemberReference and invokes it with the CanStackItems signature.
     *  Uses the class stored inside the FMemberReference (MemberParent) for resolution.
     *  Returns empty TOptional if the reference is unbound. */
    static auto
    Resolve_CanStackItems(
        const FMemberReference& InRef,
        FCk_Handle_Inventory InInventory,
        FCk_Handle_Item InSourceItem,
        FCk_Handle_Item InTargetItem) -> TOptional<bool>;

    /**
     * Runs only the custom validation logic (native delegate, dynamic delegate,
     * FMemberReference) for stacking. Skips structural checks (containment,
     * definition match, capacity). Both items must be valid.
     * Returns true if no custom logic rejects.
     */
    static bool
    Get_PassesCustomStackValidation(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InSourceItem,
        const FCk_Handle_Item& InTargetItem);

    /**
     * Distributes InCount units into existing compatible stacks within the inventory.
     * Only checks definition compatibility and remaining capacity — does NOT run
     * custom stack validation (no source item to validate against).
     * Returns the number of units actually added to existing stacks.
     */
    static int32
    Request_FillExistingStacks(
        const FCk_Handle_Inventory& InInventory,
        const UCk_InventoryItem_Definition* InDefinition,
        int32 InCount);

    // ---- FMemberReference Prototypes (signature references for function picker, not meant to be called) ----

#if WITH_EDITOR
    UFUNCTION(meta = (BlueprintInternalUseOnly = "true"))
    static bool
    Prototype_CanStackItems(
        FCk_Handle_Inventory InInventory,
        FCk_Handle_Item InSourceItem,
        FCk_Handle_Item InTargetItem)
    { return false; }
#endif
};

// --------------------------------------------------------------------------------------------------------------------
