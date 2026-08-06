#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"
#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Fragment_Data.h"
#include "CkInventory/Inventory/Spatial/CkInventory_Spatial_Fragment.h"
#include "CkInventory/Item/CkItem_Fragment_Data.h"
#include "CkInventory_Spatial_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Inventory_Spatial_HandleRequests;
    class FProcessor_Inventory_Spatial_Replicate;
    class FProcessor_Inventory_Spatial_SyncReplication;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Inventory_Spatial"))
class CKINVENTORY_API UCk_Utils_Inventory_Spatial_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Inventory_Spatial_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Inventory_Spatial);

    friend class ck::FProcessor_Inventory_Spatial_HandleRequests;
    friend class ck::FProcessor_Inventory_Spatial_Replicate;
    friend class ck::FProcessor_Inventory_Spatial_SyncReplication;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|Spatial",
              DisplayName = "[Ck][Inventory][Spatial] Make Params",
              meta = (NativeMakeFunc, AutoCreateRefTerm = "InCustomCanAcceptItem, InCustomCanStackItems"))
    static FCk_Inventory_Spatial_Spec
    Make_Params(
        UPARAM(meta = (Categories = "Inventory")) FGameplayTag InName,
        FIntPoint InDimensions,
        const FCk_Delegate_Inventory_CustomCanAcceptItem_Dynamic& InCustomCanAcceptItem,
        const FCk_Delegate_Inventory_CustomCanStackItems_Dynamic& InCustomCanStackItems);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Inventory|Spatial",
              DisplayName = "[Ck][Inventory][Spatial] Add New Inventory",
              meta = (DefaultToSelf = "InWorldContextObject", HidePin = "InWorldContextObject"))
    static FCk_Handle_Inventory_Spatial
    Add(
        UPARAM(ref) FCk_Handle& InOwnerEntity,
        const FCk_Inventory_Spatial_Spec& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates,
        const UObject* InWorldContextObject = nullptr);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Inventory|Spatial",
              DisplayName = "[Ck][Inventory][Spatial] Add Multiple New Inventories",
              meta = (DefaultToSelf = "InWorldContextObject", HidePin = "InWorldContextObject"))
    static TArray<FCk_Handle_Inventory_Spatial>
    AddMultiple(
        UPARAM(ref) FCk_Handle& InOwnerEntity,
        const FCk_MultipleInventory_Spatial_Spec& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates,
        const UObject* InWorldContextObject = nullptr);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

public:
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
              DisplayName = "[Ck] Get Invalid Inventory Handle (Spatial)",
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

    /** Counts active grid cells (excluding disabled cells) that are not currently occupied by any item.
     *  O(W*H) — walks every cell. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|Spatial",
              DisplayName = "[Ck][Inventory][Spatial] Get Num Free Cells")
    static int32
    Get_NumFreeCells(
        const FCk_Handle_Inventory_Spatial& InInventory);

    /** Returns the item's placement rotation derived from its Transform yaw. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|Spatial",
              DisplayName = "[Ck][Inventory][Spatial] Get Item Placement Rotation")
    static ECk_CardinalRotation
    Get_ItemPlacementRotation(
        const FCk_Handle_Item& InItem);

    /** Returns the item's active cells rotated by the given rotation. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|Spatial",
              DisplayName = "[Ck][Inventory][Spatial] Get Item Active Cells (Rotated)")
    static TArray<FIntPoint>
    Get_ItemActiveCells_Rotated(
        const FCk_Handle_Item& InItem,
        ECk_CardinalRotation InRotation);

    /** Checks if an item can be placed at a specific coordinate. Tries all 4 rotations and returns the first valid one. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|Spatial",
              DisplayName = "[Ck][Inventory][Spatial] Get Can Place Item At")
    static FCk_SpatialPlacementResult
    Get_CanPlaceItemAt(
        const FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Handle_Item& InItem,
        const FIntPoint& InCoordinate);

    /** Scans the grid for the first position where the item fits. Tries all 4 rotations per position. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|Spatial",
              DisplayName = "[Ck][Inventory][Spatial] Get First Available Placement")
    static FCk_SpatialPlacementResult
    Get_FirstAvailablePlacement(
        const FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Handle_Item& InItem);

    /** Returns the coordinate of an item's top-left cell in the spatial inventory grid. Returns (-1,-1) if not found. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|Spatial",
              DisplayName = "[Ck][Inventory][Spatial] Get Item Placement Coordinate")
    static FIntPoint
    Get_ItemPlacementCoordinate(
        const FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Handle_Item& InItem);

    /** Returns the item at a specific coordinate in the spatial inventory, or invalid if the cell is empty. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|Spatial",
              DisplayName = "[Ck][Inventory][Spatial] Get Item At Coordinate")
    static FCk_Handle_Item
    Get_ItemAtCoordinate(
        const FCk_Handle_Inventory_Spatial& InInventory,
        const FIntPoint& InCoordinate);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory|Spatial",
              DisplayName = "[Ck][Inventory][Spatial] Get Grid")
    static FCk_Handle_2dGridSystem
    Get_Grid(
        const FCk_Handle_Inventory_Spatial& InInventory);

public:
    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|Inventory|Spatial",
              DisplayName = "[Ck][Inventory][Spatial] Request Add Item (With Placement)",
              meta = (AutoCreateRefTerm = "InPlacement, InDelegate, InCompletionDelegate"))
    static FCk_Handle_Inventory_Spatial
    Request_AddItem(
        UPARAM(ref) FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Request_Inventory_AddItem& InRequest,
        const FCk_SpatialPlacement& InPlacementAddon,
        const FCk_Delegate_Inventory_OnOperationResult_Add& InDelegate,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|Inventory|Spatial",
              DisplayName = "[Ck][Inventory][Spatial] Request Split Stack (With Placement)",
              meta = (AutoCreateRefTerm = "InPlacement, InDelegate, InCompletionDelegate"))
    static FCk_Handle_Inventory_Spatial
    Request_SplitStack(
        UPARAM(ref) FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Request_Inventory_SplitStack& InRequest,
        const FCk_SpatialPlacement& InPlacementAddon,
        const FCk_Delegate_Inventory_OnOperationResult_Split& InDelegate,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|Inventory|Spatial",
              DisplayName = "[Ck][Inventory][Spatial] Request Relocate Item",
              meta = (AutoCreateRefTerm = "InDelegate, InCompletionDelegate"))
    static FCk_Handle_Inventory_Spatial
    Request_RelocateItem(
        UPARAM(ref) FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Request_Inventory_Spatial_RelocateItem& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Relocate& InDelegate,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate);

public:
    /** Checks if an item fits at a specific coordinate with a specific rotation (no rotation search). */
    static auto
    DoCanPlaceItemAt(
        const FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Handle_Item& InItem,
        const FIntPoint& InCoordinate,
        ECk_CardinalRotation InRotation) -> bool;

    static auto
    Request_PlaceItemOnGrid(
        FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Handle_Item& InItem,
        const FIntPoint& InCoordinate,
        ECk_CardinalRotation InRotation = ECk_CardinalRotation::None) -> void;

    static auto
    Request_RemoveItemFromGrid(
        FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Handle_Item& InItem) -> void;
};
