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
    static FCk_Fragment_Inventory_DataOnly_ParamsData
    Make_Params(
        UPARAM(meta = (Categories = "Inventory")) FGameplayTag InName,
        const FCk_Delegate_Inventory_CustomCanAcceptItem_Dynamic& InCustomCanAcceptItem,
        const FCk_Delegate_Inventory_CustomCanStackItems_Dynamic& InCustomCanStackItems);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|DataOnly",
              DisplayName = "[Ck][Inventory][DataOnly] Make Params (Bounded)",
              meta = (NativeMakeFunc, AutoCreateRefTerm = "InCustomCanAcceptItem, InCustomCanStackItems"))
    static FCk_Fragment_Inventory_DataOnly_ParamsData
    Make_Params_Bounded(
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
        const FCk_Fragment_Inventory_DataOnly_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates,
        const UObject* InWorldContextObject = nullptr);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Inventory|DataOnly",
              DisplayName = "[Ck][Inventory][DataOnly] Add Multiple New Inventories",
              meta = (DefaultToSelf = "InWorldContextObject", HidePin = "InWorldContextObject"))
    static TArray<FCk_Handle_Inventory_DataOnly>
    AddMultiple(
        UPARAM(ref) FCk_Handle& InOwnerEntity,
        const FCk_Fragment_MultipleInventory_DataOnly_ParamsData& InParams,
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

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|DataOnly",
              DisplayName = "[Ck][Inventory][DataOnly] Get Remaining Slots")
    static int32
    Get_RemainingSlots(
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
