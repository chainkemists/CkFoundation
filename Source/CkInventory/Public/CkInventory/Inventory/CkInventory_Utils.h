#pragma once

#include "CkInventory/Inventory/CkInventory_Fragment.h"
#include "CkInventory/Item/CkInventoryItem_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Fragment_Data.h"

#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkInventory_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Inventory_ProcessSlots;
    class FProcessor_Inventory_Replicate;
    class FProcessor_Inventory_SyncReplication;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Inventory"))
class CKINVENTORY_API UCk_Utils_Inventory_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Inventory_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Inventory);

public:
    struct RecordOfInventories_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfInventories> {};
    struct RecordOfInventoryItems_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfInventoryItems> {};

public:
    friend class ck::FProcessor_Inventory_ProcessSlots;
    friend class ck::FProcessor_Inventory_Replicate;
    friend class ck::FProcessor_Inventory_SyncReplication;

    // ---- Make Params ----

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Make Inventory Params (Spatial)",
              meta = (NativeMakeFunc))
    static FCk_Fragment_Inventory_ParamsData
    Make_InventoryParams_Spatial(
        UPARAM(meta = (Categories = "Inventory")) FGameplayTag InName,
        FIntPoint InDimensions);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Make Inventory Params (DataOnly)",
              meta = (NativeMakeFunc))
    static FCk_Fragment_Inventory_ParamsData
    Make_InventoryParams_DataOnly(
        UPARAM(meta = (Categories = "Inventory")) FGameplayTag InName);

    // ---- Creation ----

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Add New Inventory")
    static FCk_Handle_Inventory
    Add(
        UPARAM(ref) FCk_Handle& InOwnerEntity,
        const FCk_Fragment_Inventory_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    // ---- Queries ----

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Has Any Inventory")
    static bool
    Has_Any(
        const FCk_Handle& InOwnerEntity);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Inventory
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Handle -> Inventory Handle",
              meta = (CompactNodeTitle = "<AsInventory>", BlueprintAutocast))
    static FCk_Handle_Inventory
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Invalid Inventory Handle",
              Category = "Ck|Utils|Inventory",
              meta = (CompactNodeTitle = "INVALID_InventoryHandle", Keywords = "make"))
    static FCk_Handle_Inventory
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Try Get Inventory")
    static FCk_Handle_Inventory
    TryGet_Inventory(
        const FCk_Handle& InOwnerEntity,
        UPARAM(meta = (Categories = "Inventory")) FGameplayTag InInventoryName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Get Items")
    static TArray<FCk_Handle_Item>
    Get_Items(
        const FCk_Handle_Inventory& InInventory);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Get Num Items")
    static int32
    Get_NumItems(
        const FCk_Handle_Inventory& InInventory);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Get Contains Item")
    static bool
    Get_ContainsItem(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Get Inventory Type")
    static ECk_InventoryType
    Get_InventoryType(
        const FCk_Handle_Inventory& InInventory);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Get Is Spatial")
    static bool
    Get_IsSpatial(
        const FCk_Handle_Inventory& InInventory);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Get Is DataOnly")
    static bool
    Get_IsDataOnly(
        const FCk_Handle_Inventory& InInventory);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Get Grid")
    static FCk_Handle_2dGridSystem
    Get_Grid(
        const FCk_Handle_Inventory& InInventory);

    // ---- Requests (Authority Only) ----

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Request Add Item",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Inventory
    Request_AddItem(
        UPARAM(ref) FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_AddItem& InRequest,
        const FCk_Delegate_Inventory_OnItemAddedOrNot& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Request Remove Item",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Inventory
    Request_RemoveItem(
        UPARAM(ref) FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_RemoveItem& InRequest,
        const FCk_Delegate_Inventory_OnItemRemovedOrNot& InDelegate);

    // ---- Signals ----

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Inventory] Bind to OnItemsChanged")
    static FCk_Handle_Inventory
    BindTo_OnItemsChanged(
        UPARAM(ref) FCk_Handle_Inventory& InInventory,
        const FCk_Delegate_Inventory_OnItemsChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Inventory] Unbind From OnItemsChanged")
    static FCk_Handle_Inventory
    UnbindFrom_OnItemsChanged(
        UPARAM(ref) FCk_Handle_Inventory& InInventory,
        const FCk_Delegate_Inventory_OnItemsChanged& InDelegate);

    // ---- Internal ----

private:
    static auto
    Request_ItemsUpdated(
        FCk_Handle_Inventory& InInventory) -> void;

    static auto
    Request_TryReplicateInventory(
        FCk_Handle_Inventory& InInventory) -> void;

    // ---- Spatial helpers (C++ only) ----

    static auto
    Get_CanPlaceItemAt(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem,
        const FIntPoint& InCoordinate) -> bool;

    static auto
    Get_FindFirstAvailablePlacement(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem) -> FIntPoint;

    static auto
    DoPlaceItemOnGrid(
        FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem,
        const FIntPoint& InCoordinate) -> void;

    static auto
    DoRemoveItemFromGrid(
        FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem) -> void;

};

// --------------------------------------------------------------------------------------------------------------------
