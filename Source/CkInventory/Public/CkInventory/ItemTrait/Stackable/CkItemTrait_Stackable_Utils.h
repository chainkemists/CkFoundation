#pragma once

#include "CkInventory/Item/CkItem_Fragment_Data.h"
#include "CkInventory/Inventory/CkInventory_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkEcs/Signal/CkSignal_Fragment_Data.h"
#include "CkEcs/Signal/CkSignal_Macros.h"

#include "CkItemTrait_Stackable_Utils.generated.h"

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

// Return payload for Request_FillExistingStacks — bundles the total count that was moved into
// existing stacks with a handle to the last stack that received quantity. Bundled into a struct
// rather than using a pointer out-param so the function matches the codebase's returned-by-value
// result convention (see FCk_SpatialPlacementResult).
USTRUCT(BlueprintType)
struct CKINVENTORY_API FCk_FillExistingStacksResult
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_FillExistingStacksResult);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _FilledCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Item _LastFilledItem;

public:
    CK_PROPERTY_GET(_FilledCount);
    CK_PROPERTY_GET(_LastFilledItem);

    CK_DEFINE_CONSTRUCTORS(FCk_FillExistingStacksResult, _FilledCount, _LastFilledItem);
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

namespace ck
{
    class FProcessor_Inventory_Spatial_HandleRequests;
    class FProcessor_Inventory_DataOnly_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Item"))
class CKINVENTORY_API UCk_Utils_ItemTrait_Stackable_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

    friend class ck::FProcessor_Inventory_Spatial_HandleRequests;
    friend class ck::FProcessor_Inventory_DataOnly_HandleRequests;

public:
    CK_GENERATED_BODY(UCk_Utils_ItemTrait_Stackable_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Item|Stackable",
              DisplayName = "[Ck][Item] Get Is Stackable")
    static bool
    Get_IsStackable(
        const FCk_Handle_Item& InItem);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Item|Stackable",
              DisplayName = "[Ck][Item] Get Stack Count")
    static int32
    Get_StackCount(
        const FCk_Handle_Item& InItem);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Item|Stackable",
              DisplayName = "[Ck][Item] Get Max Stack Size")
    static int32
    Get_MaxStackSize(
        const FCk_Handle_Item& InItem);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Item|Stackable",
              DisplayName = "[Ck][Item] Get Has Max Stack Size")
    static bool
    Get_HasMaxStackSize(
        const FCk_Handle_Item& InItem);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Item|Stackable",
              DisplayName = "[Ck][Item] Get Is Stack Full")
    static bool
    Get_IsStackFull(
        const FCk_Handle_Item& InItem);

    /** Returns how many more units the target item can accept (definition-level cap only).
     *  Returns MAX_int32 if no max stack size is defined. Returns 0 if full or not stackable.
     *  For capacity within a specific inventory (which may clamp stacks tighter), use
     *  Get_RemainingStackCapacity_InInventory. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Item|Stackable",
              DisplayName = "[Ck][Item] Get Remaining Stack Capacity")
    static int32
    Get_RemainingStackCapacity(
        const FCk_Handle_Item& InItem);

    /** The max stack size that applies to InItem while it sits in InInventory:
     *  min(item definition max, the inventory's StackingPolicy clamp). NoStacking yields 1.
     *  MAX_int32 when neither caps the stack. Non-stackable items yield 1.
     *  An invalid inventory yields the definition-level value. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Item|Stackable",
              DisplayName = "[Ck][Item] Get Effective Max Stack Size")
    static int32
    Get_EffectiveMaxStackSize(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem);

    /** Definition-level variant of Get_EffectiveMaxStackSize for planning queries that have no
     *  item instance yet. */
    static auto
    Get_EffectiveMaxStackSize_ByDefinition(
        const FCk_Handle_Inventory& InInventory,
        const UCk_InventoryItem_Definition* InDefinition) -> int32;

    /** Get_RemainingStackCapacity additionally clamped by the inventory's StackingPolicy. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Item|Stackable",
              DisplayName = "[Ck][Item] Get Remaining Stack Capacity (In Inventory)")
    static int32
    Get_RemainingStackCapacity_InInventory(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem);

    /** Full validation for merging two specific items. Both items must be valid. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Item|Stackable",
              DisplayName = "[Ck][Item] Get Can Stack Items")
    static ECk_Inventory_OperationResult_Stack
    Get_CanStackItems(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InSourceItem,
        const FCk_Handle_Item& InTargetItem);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Item|Stackable",
              DisplayName = "[Ck][Item] Bind To OnStackCountChanged")
    static FCk_Handle_Item
    BindTo_OnStackCountChanged(
        UPARAM(ref) FCk_Handle_Item& InItem,
        const FCk_Delegate_Stackable_OnStackCountChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Item|Stackable",
              DisplayName = "[Ck][Item] Unbind From OnStackCountChanged")
    static FCk_Handle_Item
    UnbindFrom_OnStackCountChanged(
        UPARAM(ref) FCk_Handle_Item& InItem,
        const FCk_Delegate_Stackable_OnStackCountChanged& InDelegate);

public:
    // Internal helpers — plain C++ statics (not UFUNCTIONs); not exposed to BP/AS.
    // Used by typed inventory processors and inventory_helpers namespace.

    /** Sets the stack count to an absolute value via the underlying integer attribute's Override modifier.
     *  WARNING: Override modifiers coalesce as last-writer-wins within a single frame. Safe ONLY for
     *  freshly-created items whose stack count has not yet been mutated this frame. For existing items
     *  whose count is being adjusted, use Request_AdjustStackCount instead — Add/Subtract modifiers
     *  compose (deltas sum) and survive multiple same-frame calls correctly. */
    static void
    Request_OverrideStackCount(
        const FCk_Handle_Item& InItem,
        int32 InNewCount);

    /** Adjusts the stack count by InDelta via the underlying integer attribute's Add/Subtract modifier.
     *  Delta-based mutations compose across same-frame calls (two adjusts of +5 yield +10), unlike
     *  Override which would last-writer-wins. Use this for any mutation that's relative to current count. */
    static void
    Request_AdjustStackCount(
        const FCk_Handle_Item& InItem,
        int32 InDelta);

    static auto
    Resolve_CanStackItems(
        const FMemberReference& InRef,
        FCk_Handle_Inventory InInventory,
        FCk_Handle_Item InSourceItem,
        FCk_Handle_Item InTargetItem) -> TOptional<bool>;

    static bool
    Get_PassesCustomStackValidation(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InSourceItem,
        const FCk_Handle_Item& InTargetItem);

    /** Distributes up to InCount units of InDefinition into existing compatible stacks in
     *  InInventory. Per-stack room respects the inventory's StackingPolicy clamp, and each
     *  candidate stack must pass the inventory's custom stack validation. InSourceItem is handed
     *  to that validation when the units come from a real item (transfers); pass an invalid handle
     *  when filling from a bare definition (AddByDefinition) — custom validators must tolerate an
     *  invalid source item for that path. */
    static FCk_FillExistingStacksResult
    Request_FillExistingStacks(
        const FCk_Handle_Inventory& InInventory,
        const UCk_InventoryItem_Definition* InDefinition,
        int32 InCount,
        const FCk_Handle_Item& InSourceItem = FCk_Handle_Item{});

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
