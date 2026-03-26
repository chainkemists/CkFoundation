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
    class FProcessor_Inventory_HandleRequests;
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
    friend class ck::FProcessor_Inventory_HandleRequests;
    friend class ck::FProcessor_Inventory_Replicate;
    friend class ck::FProcessor_Inventory_SyncReplication;
    friend class ck::FProcessor_Inventory_FireSignals;
    friend class UCk_Utils_ItemFragment_Stackable_UE;

    // ---- Make Params ----

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Make Inventory Params (Spatial)",
              meta = (NativeMakeFunc))
    static FCk_Fragment_Inventory_ParamsData
    Make_InventoryParams_Spatial(
        UPARAM(meta = (Categories = "Inventory")) FGameplayTag InName,
        FIntPoint InDimensions,
        FCk_Delegate_Inventory_CustomCanAcceptItem_Dynamic InCustomCanAcceptItem,
        FCk_Delegate_Inventory_CustomCanStackItems_Dynamic InCustomCanStackItems);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Make Inventory Params (DataOnly)",
              meta = (NativeMakeFunc))
    static FCk_Fragment_Inventory_ParamsData
    Make_InventoryParams_DataOnly(
        UPARAM(meta = (Categories = "Inventory")) FGameplayTag InName,
        FCk_Delegate_Inventory_CustomCanAcceptItem_Dynamic InCustomCanAcceptItem,
        FCk_Delegate_Inventory_CustomCanStackItems_Dynamic InCustomCanStackItems);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Make Inventory Params (DataOnly | Bounded)",
              meta = (NativeMakeFunc))
    static FCk_Fragment_Inventory_ParamsData
    Make_InventoryParams_DataOnly_Bounded(
        UPARAM(meta = (Categories = "Inventory")) FGameplayTag InName,
        int32 InBoundLimit,
        FCk_Delegate_Inventory_CustomCanAcceptItem_Dynamic InCustomCanAcceptItem,
        FCk_Delegate_Inventory_CustomCanStackItems_Dynamic InCustomCanStackItems);

    // ---- Creation ----

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Add New Inventory",
              meta = (DefaultToSelf = "InWorldContextObject", HidePin = "InWorldContextObject"))
    static FCk_Handle_Inventory
    Add(
        UPARAM(ref) FCk_Handle& InOwnerEntity,
        const FCk_Fragment_Inventory_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates,
        UObject* InWorldContextObject = nullptr);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Add Multiple New Inventories",
              meta = (DefaultToSelf = "InWorldContextObject", HidePin = "InWorldContextObject"))
    static TArray<FCk_Handle_Inventory>
    AddMultiple(
        UPARAM(ref) FCk_Handle& InOwnerEntity,
        const FCk_Fragment_MultipleInventory_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates,
        UObject* InWorldContextObject = nullptr);

    // ---- Validation ----

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Get Can Accept Item")
    static ECk_Inventory_OperationResult_Add
    Get_CanAcceptItem(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem);

    /** Runs only the custom acceptance logic (native delegate, dynamic delegate,
     *  FMemberReference). Skips structural checks (validity, containment).
     *  Returns true if no custom logic rejects. */
    static bool
    Get_PassesCustomAcceptValidation(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem);

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

    // ---- Requests (Authority Only) ----

public:
    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Request Add Item",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Inventory
    Request_AddItem(
        UPARAM(ref) FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_AddItem& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Add& InDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Request Remove Item",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Inventory
    Request_RemoveItem(
        UPARAM(ref) FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_RemoveItem& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Remove& InDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Request Stack Items",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Inventory
    Request_StackItems(
        UPARAM(ref) FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_StackItems& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Stack& InDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Request Split Stack",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Inventory
    Request_SplitStack(
        UPARAM(ref) FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_SplitStack& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Split& InDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Request Add Item (By Definition)",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Inventory
    Request_AddItemByDefinition(
        UPARAM(ref) FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_AddItemByDefinition& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_AddByDefinition& InDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Request Transfer Item",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Inventory
    Request_TransferItem(
        UPARAM(ref) FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_TransferItem& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Transfer& InDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Request Sort",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Inventory
    Request_Sort(
        UPARAM(ref) FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_Sort& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Sort& InDelegate);

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

    // ---- FMemberReference Resolution ----

public:
    /** Resolves a FMemberReference and invokes it with the CanAcceptItem signature.
     *  Uses the class stored inside the FMemberReference (MemberParent) for resolution.
     *  Returns empty TOptional if the reference is unbound. */
    static auto
    Resolve_CanAcceptItem(
        const FMemberReference& InRef,
        FCk_Handle_Inventory InInventory,
        FCk_Handle_Item InItem) -> TOptional<bool>;

    // ---- FMemberReference Prototypes (signature references for function picker, not meant to be called) ----

#if WITH_EDITOR
private:
    UFUNCTION(meta = (BlueprintInternalUseOnly = "true"))
    static bool
    Prototype_CanAcceptItem(
        FCk_Handle_Inventory InInventory,
        FCk_Handle_Item InItem)
    { return false; }
#endif

    // ---- Internal ----

private:
    static auto
    Request_TryReplicateInventory(
        FCk_Handle_Inventory& InInventory) -> void;

    static auto
    Request_MarkInventory_AsMayHaveChanged(
        FCk_Handle_Inventory& InInventory) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

using FInventoryItemRecordUtils = UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils;

// ============================================================================
// Spatial Inventory Utils
// ============================================================================

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Inventory_Spatial"))
class CKINVENTORY_API UCk_Utils_Inventory_Spatial_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Inventory_Spatial_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Inventory_Spatial);

    friend class ck::FProcessor_Inventory_HandleRequests;
    friend class ck::FProcessor_Inventory_Replicate;
    friend class ck::FProcessor_Inventory_SyncReplication;

    // ---- Queries ----

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Inventory|Spatial",
              DisplayName = "[Ck][Inventory][Spatial] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Inventory_Spatial
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|Spatial",
              DisplayName = "[Ck][Inventory][Spatial] Handle -> Inventory Handle (Spatial)",
              meta = (CompactNodeTitle = "<AsInventory_Spatial>", BlueprintAutocast))
    static FCk_Handle_Inventory_Spatial
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Invalid Spatial Inventory Handle",
              Category = "Ck|Utils|Inventory|Spatial",
              meta = (CompactNodeTitle = "INVALID_InventoryHandle_Spatial", Keywords = "make"))
    static FCk_Handle_Inventory_Spatial
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|Spatial",
              DisplayName = "[Ck][Inventory][Spatial] Get Dimensions")
    static FIntPoint
    Get_Dimensions(
        const FCk_Handle_Inventory_Spatial& InInventory);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|Spatial",
              DisplayName = "[Ck][Inventory][Spatial] Get Can Place Item At")
    static bool
    Get_CanPlaceItemAt(
        const FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Handle_Item& InItem,
        const FIntPoint& InCoordinate);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|Spatial",
              DisplayName = "[Ck][Inventory][Spatial] Get First Available Placement")
    static FIntPoint
    Get_FirstAvailablePlacement(
        const FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Handle_Item& InItem);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|Spatial",
              DisplayName = "[Ck][Inventory][Spatial] Get Grid")
    static FCk_Handle_2dGridSystem
    Get_Grid(
        const FCk_Handle_Inventory_Spatial& InInventory);

    // ---- Internal spatial helpers (used by processors) ----

private:
    static auto
    Request_PlaceItemOnGrid(
        FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Handle_Item& InItem,
        const FIntPoint& InCoordinate) -> void;

    static auto
    Request_RemoveItemFromGrid(
        FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Handle_Item& InItem) -> void;
};

// ============================================================================
// DataOnly Inventory Utils
// ============================================================================

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Inventory_DataOnly"))
class CKINVENTORY_API UCk_Utils_Inventory_DataOnly_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Inventory_DataOnly_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Inventory_DataOnly);

    // ---- Queries ----

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
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
              DisplayName = "[Ck] Get Invalid DataOnly Inventory Handle",
              Category = "Ck|Utils|Inventory|DataOnly",
              meta = (CompactNodeTitle = "INVALID_InventoryHandle_DataOnly", Keywords = "make"))
    static FCk_Handle_Inventory_DataOnly
    Get_InvalidHandle() { return {}; };

    // ---- Bounds ----

private:
    /** Blueprint version: returns the bound max and whether the inventory is bounded. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|DataOnly",
              DisplayName = "[Ck][Inventory][DataOnly] Get Bound Max")
    static int32
    Get_BoundMax_BP(
        const FCk_Handle_Inventory_DataOnly& InInventory,
        bool& OutIsBounded);

public:
    /** Returns the bound max as an optional. Empty if unbounded. */
    static TOptional<int32>
    Get_BoundMax(
        const FCk_Handle_Inventory_DataOnly& InInventory);

    // ---- Requests (Authority Only) ----

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

// --------------------------------------------------------------------------------------------------------------------
