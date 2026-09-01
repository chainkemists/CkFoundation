#pragma once

#include "CkInventory/Inventory/CkInventory_Fragment.h"
#include "CkInventory/Item/CkItem_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Delegates/CkDelegates.h"
#include "CkEcs/Request/CkRequest_Completion.h"

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
    /** PreferStacking lets soft "no room" failures pass when existing items can absorb the item's
     *  full stack via stacking. Hard failures (invalid, already-in-inventory, custom rejection,
     *  missing dimensions trait) cannot.
     *  NOTE: the incoming unit count is read from the item's committed stack count — for an item
     *  whose stack was written this frame (attribute modifiers fold next pump pass) this reads the
     *  pre-write value. Internal handlers that know the intended count use the explicit-count
     *  overload below. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Get Can Accept Item")
    static ECk_Inventory_OperationResult_Add
    Get_CanAcceptItem(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem,
        ECk_Inventory_AddPolicy InPolicy = ECk_Inventory_AddPolicy::ForceNewItem);

    /** Explicit-count variant for callers that know the unit count the item will carry once
     *  pending stack writes settle. InIncomingUnits <= 0 derives the count from the item. */
    static auto
    Get_CanAcceptItem_WithCount(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem,
        ECk_Inventory_AddPolicy InPolicy,
        int32 InIncomingUnits) -> ECk_Inventory_OperationResult_Add;

    /** Runs only the custom acceptance logic (native delegate, dynamic delegate,
     *  FMemberReference). Skips structural checks (validity, containment).
     *  Returns true if no custom logic rejects. */
    static bool
    Get_PassesCustomAcceptValidation(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem);

    /** Resolves the custom quantitative quota (native delegate, dynamic delegate,
     *  FMemberReference) — "how many MORE units of this definition can this inventory absorb under
     *  custom rules (weight, ...)". Returns MAX_int32 when no quota logic is bound. Multiple bound
     *  hooks compose by min(). Never negative. InItem may be invalid (definition-level queries). */
    static auto
    Get_CustomAbsorbableUnits(
        const FCk_Handle_Inventory& InInventory,
        const UCk_InventoryItem_Definition* InDefinition,
        const FCk_Handle_Item& InItem) -> int32;

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

public:
    /** Every item entity in InAnyEntityInWorld's world that no inventory lists, but that something is
     *  still holding onto. This is the leak detector for the removal contract: a Remove takes the item
     *  out of the RECORD, and the record is what every query answers from -- so an item nobody destroyed
     *  and nobody re-added stays a live entity that is invisible to Get_Items, Get_NumItems and
     *  Get_CanAcceptItem, keeps being written into every snapshot, and dies with whatever still owns it.
     *
     *  Two shapes count, and they are not interchangeable:
     *
     *  1. STRANDED UNDER AN INVENTORY (_StrandedUnder valid) -- the item's LIFETIME OWNER is an
     *     inventory whose record does not contain it. This is what a removal left behind before Remove
     *     released the claim Add took, and it is the only shape that survives a save/load: it lives in
     *     the lifetime graph, which the save records.
     *  2. DETACHED IN THIS SESSION (_StrandedUnder invalid) -- the item carries a ParentInventory holder
     *     that points at nothing. Only Add ever installs that holder, so this positively means "was in
     *     an inventory, is in none now"; a world item that was never in one has no holder at all and is
     *     correctly ignored. It does NOT survive a load (the holder is rebuilt from the record), so it
     *     is the shape that catches a caller leaking TODAY rather than one that leaked historically.
     *
     *  Pending-kill items are skipped -- something already took responsibility for those.
     *
     *  Shape 2 is a HEURISTIC, and the one way it can cry wolf is a caller that deliberately removes
     *  with KeepItem and holds the item alive outside any inventory for a while. No such caller exists
     *  today (every consuming site declares DestroyItem, transfer re-adds within the same handler
     *  call, and the item-EndPlay removal is already pending-destroy), but if one is ever written it
     *  will be reported here for as long as it holds the item. That is why shape 2 names no container
     *  and why the shape a save can carry is kept distinguishable from it: a destructive consumer must
     *  be able to act on shape 1 alone.
     *
     *  Read-only and allocation-light, but it walks every item entity in the world: call it at a save,
     *  at a load, or from a debug verb -- not per tick. AUTHORITY ONLY as a basis for acting: on a
     *  client the ParentInventory holder is cleared by the replication sync ahead of the server's
     *  destroy replicating, so shape 2 is a normal transient state there. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Get Orphaned Items")
    static TArray<FCk_Inventory_OrphanedItem>
    Get_OrphanedItems(
        const FCk_Handle& InAnyEntityInWorld);

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

    /** Enumerate every inventory composed on InOwnerEntity (its RecordOfInventories).
     *  Mirrors ForEach_AnimPlan: call with an unbound delegate to simply get the array of
     *  all inventory handles; bind InDelegate to run it per-inventory instead. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] For Each Inventory",
              meta = (AutoCreateRefTerm = "InOptionalPayload, InDelegate"))
    static TArray<FCk_Handle_Inventory>
    ForEach_Inventories(
        UPARAM(ref) FCk_Handle& InOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate);

    static auto
    ForEach_Inventories(
        FCk_Handle& InOwnerEntity,
        const TFunction<void(FCk_Handle_Inventory&)>& InFunc) -> void;

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

    /** Sum of stack counts across all items (a non-stackable item counts as 1 unit). */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Get Total Units")
    static int32
    Get_TotalUnits(
        const FCk_Handle_Inventory& InInventory);

    /** How many units of InDefinition this inventory can absorb in total right now:
     *  min over (room in existing same-definition stacks, new-entry room x effective max stack,
     *  the bound metric's remaining capacity, the custom absorbable-units quota).
     *  Computed from committed state — queued-but-undrained requests are invisible (plan-time
     *  number; handlers re-check at execution time). Spatial new-entry room divides free cells by
     *  the definition's footprint area, which ignores fragmentation — treat as an upper bound. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Get Absorbable Units")
    static int32
    Get_AbsorbableUnits(
        const FCk_Handle_Inventory& InInventory,
        const UCk_InventoryItem_Definition* InDefinition);

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
              meta = (AutoCreateRefTerm = "InDelegate, InCompletionDelegate"))
    static FCk_Handle_Inventory
    Request_AddItem(
        UPARAM(ref) FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_AddItem& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Add& InDelegate,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Request Remove Item",
              meta = (AutoCreateRefTerm = "InDelegate, InCompletionDelegate"))
    static FCk_Handle_Inventory
    Request_RemoveItem(
        UPARAM(ref) FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_RemoveItem& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Remove& InDelegate,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Request Stack Items",
              meta = (AutoCreateRefTerm = "InDelegate, InCompletionDelegate"))
    static FCk_Handle_Inventory
    Request_StackItems(
        UPARAM(ref) FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_StackItems& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Stack& InDelegate,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Request Split Stack",
              meta = (AutoCreateRefTerm = "InDelegate, InCompletionDelegate"))
    static FCk_Handle_Inventory
    Request_SplitStack(
        UPARAM(ref) FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_SplitStack& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Split& InDelegate,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Request Add Item (By Definition)",
              meta = (AutoCreateRefTerm = "InDelegate, InCompletionDelegate"))
    static FCk_Handle_Inventory
    Request_AddItemByDefinition(
        UPARAM(ref) FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_AddItemByDefinition& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_AddByDefinition& InDelegate,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Request Sort",
              meta = (AutoCreateRefTerm = "InDelegate, InCompletionDelegate"))
    static FCk_Handle_Inventory
    Request_Sort(
        UPARAM(ref) FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_Sort& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Sort& InDelegate,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Request Transfer Item (To Spatial)",
              meta = (AutoCreateRefTerm = "InDelegate, InCompletionDelegate"))
    static FCk_Handle_Inventory
    Request_TransferItem_ToSpatial(
        UPARAM(ref) FCk_Handle_Inventory& InSourceInventory,
        const FCk_Request_Inventory_TransferItem_ToSpatial& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Transfer& InDelegate,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Request Transfer Item (To DataOnly)",
              meta = (AutoCreateRefTerm = "InDelegate, InCompletionDelegate"))
    static FCk_Handle_Inventory
    Request_TransferItem_ToDataOnly(
        UPARAM(ref) FCk_Handle_Inventory& InSourceInventory,
        const FCk_Request_Inventory_TransferItem_ToDataOnly& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Transfer& InDelegate,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate);

    /** Bulk-moves every (filtered) item out of InRequest.SourceInventories into the best-fitting
     *  candidate (InRequest.TargetResolution), paced over multiple pump passes so deferred stack-count
     *  writes fold between transfers. Standalone + self-owned: spawns a transient-owned op entity (a
     *  plain FCk_Handle, discriminated by FFragment_Inventory_MassTransfer_InFlight) from
     *  InAnyHandle's world/registry and returns it — NOT scoped to any one inventory. InAnyHandle is any
     *  live handle, used only to resolve context + authority. InDelegate fires once on completion (or
     *  synchronously with Failed_NotEnqueued on a boundary reject). */
    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|Inventory",
              DisplayName = "[Ck][Inventory] Request Mass Transfer",
              meta = (AutoCreateRefTerm = "InDelegate, InCompletionDelegate"))
    static FCk_Handle
    Request_MassTransfer(
        const FCk_Handle& InAnyHandle,
        const FCk_Request_Inventory_MassTransfer& InRequest,
        const FCk_Delegate_Inventory_MassTransfer_OnComplete& InDelegate,
        const FCk_Delegate_Request_OnCompleted& InCompletionDelegate);

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

    /** Resolves a FMemberReference and invokes it with the GetAbsorbableUnits signature.
     *  Returns empty TOptional if the reference is unbound. */
    static auto
    Resolve_GetAbsorbableUnits(
        const FMemberReference& InRef,
        FCk_Handle_Inventory InInventory,
        const UCk_InventoryItem_Definition* InDefinition,
        FCk_Handle_Item InItem) -> TOptional<int32>;

    // FMemberReference function-picker prototypes — signature references only; not meant to be called.
#if WITH_EDITOR
private:
    UFUNCTION(meta = (BlueprintInternalUseOnly = "true"))
    static bool
    Prototype_CanAcceptItem(
        FCk_Handle_Inventory InInventory,
        FCk_Handle_Item InItem)
    { return false; }

    UFUNCTION(meta = (BlueprintInternalUseOnly = "true"))
    static int32
    Prototype_GetAbsorbableUnits(
        FCk_Handle_Inventory InInventory,
        const UCk_InventoryItem_Definition* InDefinition,
        FCk_Handle_Item InItem)
    { return 0; }
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
