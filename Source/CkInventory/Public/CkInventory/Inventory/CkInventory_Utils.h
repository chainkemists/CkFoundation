#pragma once

#include "CkInventory/Inventory/CkInventory_Fragment.h"
#include "CkInventory/Item/CkItem_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

// Typed Utils included before generated.h so UHT's "generated.h must be last" invariant holds.
#include "CkInventory/Inventory/Spatial/CkInventory_Spatial_Utils.h"
#include "CkInventory/Inventory/DataOnly/CkInventory_DataOnly_Utils.h"

#include "CkInventory_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_Inventory_FireSignals;

    // Templated inventory factory — instantiated by each typed Utils' Add() forwarder with that
    // shape's TInventoryRequestTraits specialization. Performs shape-neutral entity creation +
    // delegates to Traits::Setup_PerShape for the shape-specific bits + finalizes the connection.
    template <typename TraitsType>
    auto CreateInventory(
        FCk_Handle& InOwnerEntity,
        const FCk_Fragment_Inventory_ParamsData& InParams,
        ECk_Replication InReplicates = ECk_Replication::Replicates,
        const UObject* InWorldContextObject = nullptr) -> FCk_Handle_Inventory;
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
    friend class ck::FProcessor_Inventory_FireSignals;
    friend class UCk_Utils_ItemTrait_Stackable_UE;

public:
    /** PreferStacking lets soft "no room" failures pass when an existing item can absorb via stacking.
     *  Hard failures (invalid, already-in-inventory, custom rejection, missing dimensions trait) cannot. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Get Can Accept Item")
    static ECk_Inventory_OperationResult_Add
    Get_CanAcceptItem(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem,
        ECk_Inventory_AddPolicy InPolicy = ECk_Inventory_AddPolicy::ForceNewItem);

    /** Runs only the custom acceptance logic (native delegate, dynamic delegate,
     *  FMemberReference). Skips structural checks (validity, containment).
     *  Returns true if no custom logic rejects. */
    static bool
    Get_PassesCustomAcceptValidation(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem);

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

    /** Sums Get_RemainingStackCapacity across every existing item in InInventory that can stack with InItem.
     *  Useful for "how many more units of this item can merge into existing stacks here?".
     *  Returns 0 if InItem isn't stackable. Returns MAX_int32 if any matching stack has no max stack size. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Get Stack Room For")
    static int32
    Get_StackRoomFor(
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
              DisplayName = "[Ck][Inventory] Request Sort",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Inventory
    Request_Sort(
        UPARAM(ref) FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_Sort& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Sort& InDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Request Transfer Item (To Spatial)",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Inventory
    Request_TransferItem_ToSpatial(
        UPARAM(ref) FCk_Handle_Inventory& InSourceInventory,
        const FCk_Request_Inventory_TransferItem_ToSpatial& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Transfer& InDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Request Transfer Item (To DataOnly)",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Inventory
    Request_TransferItem_ToDataOnly(
        UPARAM(ref) FCk_Handle_Inventory& InSourceInventory,
        const FCk_Request_Inventory_TransferItem_ToDataOnly& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Transfer& InDelegate);

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

public:
    /** Resolves a FMemberReference and invokes it with the CanAcceptItem signature.
     *  Uses the class stored inside the FMemberReference (MemberParent) for resolution.
     *  Returns empty TOptional if the reference is unbound. */
    static auto
    Resolve_CanAcceptItem(
        const FMemberReference& InRef,
        FCk_Handle_Inventory InInventory,
        FCk_Handle_Item InItem) -> TOptional<bool>;

    // FMemberReference function-picker prototype — signature reference only; not meant to be called.
#if WITH_EDITOR
private:
    UFUNCTION(meta = (BlueprintInternalUseOnly = "true"))
    static bool
    Prototype_CanAcceptItem(
        FCk_Handle_Inventory InInventory,
        FCk_Handle_Item InItem)
    { return false; }
#endif

public:
    static auto
    Request_TryReplicateInventory(
        FCk_Handle_Inventory& InInventory) -> void;

    static auto
    Request_MarkInventory_AsMayHaveChanged(
        FCk_Handle_Inventory& InInventory) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

using FInventoryItemRecordUtils = UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils;

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Item"))
class CKINVENTORY_API UCk_Utils_ItemResolution_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_ItemResolution_UE);

public:
    /** Picks the best inventory among InRequest.Candidates to receive InItem.
     *  Filters out invalid candidates and any that fail Get_CanAcceptItem.
     *  Honors the request's StackingPreference (Prefer / Require / Ignore) for both filtering and ranking.
     *  When a custom sort comparator is bound on the request, it replaces the built-in scorer.
     *  Returns invalid handle if no candidate qualifies. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|ItemResolution",
              DisplayName = "[Ck][ItemResolution] Resolve Best Transfer Target")
    static FCk_Handle_Inventory
    ResolveBestTransferTarget(
        UPARAM(ref) FCk_Handle_Item& InItem,
        const FCk_BestTransferTargetParams& InRequest);
};

// --------------------------------------------------------------------------------------------------------------------
