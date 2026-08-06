#pragma once

#include "CkInventory/Inventory/DataOnly/CkInventory_DataOnly_Fragment.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkInventory_DataOnly_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Inventory_DataOnly_HandleRequests;
    class FProcessor_Inventory_DataOnly_Replicate;
    class FProcessor_Inventory_DataOnly_SyncReplication;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Inventory_DataOnly"))
class CKINVENTORY_API UCk_Utils_Inventory_DataOnly_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Inventory_DataOnly_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Inventory_DataOnly);

    friend class ck::FProcessor_Inventory_DataOnly_HandleRequests;
    friend class ck::FProcessor_Inventory_DataOnly_Replicate;
    friend class ck::FProcessor_Inventory_DataOnly_SyncReplication;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|DataOnly",
              DisplayName = "[Ck][Inventory][DataOnly] Make Params",
              meta = (NativeMakeFunc, AutoCreateRefTerm = "InCustomCanAcceptItem, InCustomCanStackItems"))
    static FCk_Inventory_DataOnly_Spec
    Make_Params(
        UPARAM(meta = (Categories = "Inventory")) FGameplayTag InName,
        const FCk_Delegate_Inventory_CustomCanAcceptItem_Dynamic& InCustomCanAcceptItem,
        const FCk_Delegate_Inventory_CustomCanStackItems_Dynamic& InCustomCanStackItems);

    /** Bounded by UNIQUE ENTRIES — the limit counts item entries; stack counts are invisible to it. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|DataOnly",
              DisplayName = "[Ck][Inventory][DataOnly] Make Params (Bounded By Unique Entries)",
              meta = (NativeMakeFunc, AutoCreateRefTerm = "InCustomCanAcceptItem, InCustomCanStackItems"))
    static FCk_Inventory_DataOnly_Spec
    Make_Params_Bounded(
        UPARAM(meta = (Categories = "Inventory")) FGameplayTag InName,
        int32 InBoundLimit,
        const FCk_Delegate_Inventory_CustomCanAcceptItem_Dynamic& InCustomCanAcceptItem,
        const FCk_Delegate_Inventory_CustomCanStackItems_Dynamic& InCustomCanStackItems);

    /** Bounded by TOTAL UNITS — the limit counts the sum of stack counts (a non-stackable item is
     *  1 unit). Entry count is unconstrained. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|DataOnly",
              DisplayName = "[Ck][Inventory][DataOnly] Make Params (Bounded By Total Units)",
              meta = (NativeMakeFunc, AutoCreateRefTerm = "InCustomCanAcceptItem, InCustomCanStackItems"))
    static FCk_Inventory_DataOnly_Spec
    Make_Params_BoundedByTotalUnits(
        UPARAM(meta = (Categories = "Inventory")) FGameplayTag InName,
        int32 InBoundLimit,
        const FCk_Delegate_Inventory_CustomCanAcceptItem_Dynamic& InCustomCanAcceptItem,
        const FCk_Delegate_Inventory_CustomCanStackItems_Dynamic& InCustomCanStackItems);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Inventory|DataOnly",
              DisplayName = "[Ck][Inventory][DataOnly] Add New Inventory",
              meta = (DefaultToSelf = "InWorldContextObject", HidePin = "InWorldContextObject"))
    static FCk_Handle_Inventory_DataOnly
    Add(
        UPARAM(ref) FCk_Handle& InOwnerEntity,
        const FCk_Inventory_DataOnly_Spec& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates,
        const UObject* InWorldContextObject = nullptr);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Inventory|DataOnly",
              DisplayName = "[Ck][Inventory][DataOnly] Add Multiple New Inventories",
              meta = (DefaultToSelf = "InWorldContextObject", HidePin = "InWorldContextObject"))
    static TArray<FCk_Handle_Inventory_DataOnly>
    AddMultiple(
        UPARAM(ref) FCk_Handle& InOwnerEntity,
        const FCk_MultipleInventory_DataOnly_Spec& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates,
        const UObject* InWorldContextObject = nullptr);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Inventory|DataOnly",
              DisplayName = "[Ck][Inventory][DataOnly] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Inventory_DataOnly
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|DataOnly",
              DisplayName = "[Ck][Inventory][DataOnly] Handle -> Inventory Handle (DataOnly)",
              meta = (CompactNodeTitle = "<AsInventory_DataOnly>", BlueprintAutocast))
    static FCk_Handle_Inventory_DataOnly
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Invalid Inventory Handle (DataOnly)",
              Category = "Ck|Utils|Inventory|DataOnly",
              meta = (CompactNodeTitle = "INVALID_InventoryHandle_DataOnly", Keywords = "make"))
    static FCk_Handle_Inventory_DataOnly
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|DataOnly",
              DisplayName = "[Ck][Inventory][DataOnly] Get Bounds Info")
    static FCk_Inventory_DataOnly_BoundsInfo
    Get_BoundsInfo(
        const FCk_Handle_Inventory_DataOnly& InInventory);

    static TOptional<int32>
    Get_BoundMax(
        const FCk_Handle_Inventory_DataOnly& InInventory);

    /** The bound mode that is actually in effect right now. Differs from the params' declared
     *  mode when Request_OverrideBounds changed boundedness at runtime: an attribute value of
     *  UnboundedBoundLimit reports Unbounded; a bounded value on a declared-Unbounded inventory
     *  reports BoundedByUniqueEntries (the legacy interpretation). */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|DataOnly",
              DisplayName = "[Ck][Inventory][DataOnly] Get Effective Bound Mode")
    static ECk_Inventory_DataOnly_BoundMode
    Get_EffectiveBoundMode(
        const FCk_Handle_Inventory_DataOnly& InInventory);

    /** Remaining ENTRY room. Only the BoundedByUniqueEntries metric constrains entries —
     *  Unbounded and BoundedByTotalUnits inventories return MAX_int32 here.
     *  For the metric-aware room (units under BoundedByTotalUnits), use Get_RemainingCapacity. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|DataOnly",
              DisplayName = "[Ck][Inventory][DataOnly] Get Remaining Slots")
    static int32
    Get_RemainingSlots(
        const FCk_Handle_Inventory_DataOnly& InInventory);

    /** Metric-aware remaining room: entries left under BoundedByUniqueEntries, units left under
     *  BoundedByTotalUnits, MAX_int32 when Unbounded. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|DataOnly",
              DisplayName = "[Ck][Inventory][DataOnly] Get Remaining Capacity")
    static int32
    Get_RemainingCapacity(
        const FCk_Handle_Inventory_DataOnly& InInventory);

public:
    /** Override the bound max through the underlying integer attribute system.
     *  Use ck::Inventory::UnboundedBoundLimit (-1) to make the inventory unbounded. */
    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|Inventory|DataOnly",
              DisplayName = "[Ck][Inventory][DataOnly] Request Override Bounds")
    static FCk_Handle_Inventory_DataOnly
    Request_OverrideBounds(
        UPARAM(ref) FCk_Handle_Inventory_DataOnly& InInventory,
        int32 InNewBoundMax);
};
