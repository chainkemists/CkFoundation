#include "CkInventory_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkInventory/CkInventory_Log.h"
#include "CkInventory/Inventory/CkInventory_Fragment.h"
#include "CkInventory/Inventory/Spatial/CkInventory_Spatial_Utils.h"
#include "CkInventory/Inventory/Spatial/CkInventory_Spatial_RequestTraits.h"
#include "CkInventory/Inventory/DataOnly/CkInventory_DataOnly_Utils.h"
#include "CkInventory/Inventory/DataOnly/CkInventory_DataOnly_RequestTraits.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"

#include "CkInventory/Item/CkItem_Definition.h"
#include "CkInventory/Item/CkItem_Utils.h"
#include "CkInventory/ItemTrait/Dimensions/CkItemTrait_Dimensions.h"
#include "CkInventory/ItemTrait/Stackable/CkItemTrait_Stackable.h"
#include "CkInventory/ItemTrait/Stackable/CkItemTrait_Stackable_Utils.h"
#include "CkInventory/ItemTrait/Tags/CkItemTrait_Tags_Utils.h"

namespace ck_inventory_utils
{
    enum class EDispatchValidity
    {
        Valid,
        NoAuthority,    // caller lacks authority over the inventory entity
        UnknownShape    // inventory has neither the Spatial nor DataOnly shape tag (composition bug)
    };

    auto Get_DispatchValidity(const FCk_Handle_Inventory& InInventory) -> EDispatchValidity
    {
        if (NOT UCk_Utils_Net_UE::Get_HasAuthority(InInventory))
        { return EDispatchValidity::NoAuthority; }

        if (UCk_Utils_Inventory_Spatial_UE::Has(InInventory) || UCk_Utils_Inventory_DataOnly_UE::Has(InInventory))
        { return EDispatchValidity::Valid; }

        return EDispatchValidity::UnknownShape;
    }

    // NoAuthority is a legitimate client-side outcome (Display, so AutoTests don't escalate it);
    // UnknownShape is a composition bug (ensure).
    auto LogDispatchRejection(
        EDispatchValidity InValidity,
        const TCHAR* InContext,
        const FCk_Handle_Inventory& InInventory) -> void
    {
        switch (InValidity)
        {
            case EDispatchValidity::NoAuthority:
                ck::inventory::Display(TEXT("{}: No authority over inventory [{}] — request rejected (not enqueued)."),
                    InContext, InInventory);
                break;
            case EDispatchValidity::UnknownShape:
                CK_TRIGGER_ENSURE(TEXT("{}: Inventory [{}] has no recognized shape tag (neither Spatial nor DataOnly) — request rejected."),
                    InContext, InInventory);
                break;
            default:
                break;
        }
    }

    // Returns true when the request may be bound + enqueued. On failure (reused request, no authority,
    // or unknown shape) it invokes InReject once — which fires the completion delegate with
    // Failed_NotEnqueued — and returns false without populating or binding.
    template <typename TRequest, typename TReject>
    auto ValidateRequestForDispatch(
        const FCk_Handle_Inventory& InInventory,
        const TRequest& InRequest,
        const TCHAR* InContext,
        TReject InReject) -> bool
    {
        CK_ENSURE_IF_NOT(NOT InRequest.Get_IsRequestHandleValid(),
            TEXT("{}: request struct reused (already populated) for inventory [{}]. Construct a fresh request per submission."),
            InContext, InInventory)
        {
            InReject();
            return false;
        }

        if (const auto Validity = Get_DispatchValidity(InInventory);
            Validity != EDispatchValidity::Valid)
        {
            LogDispatchRejection(Validity, InContext, InInventory);
            InReject();
            return false;
        }

        return true;
    }

    // Shape-branches once (the only such branch at the public Utils boundary) and hands the typed
    // handle to the enqueue lambda. Precondition: Get_DispatchValidity(InInventory) == Valid.
    template <typename TEnqueueFn>
    auto DispatchEnqueue_Checked(
        FCk_Handle_Inventory& InInventory,
        TEnqueueFn InEnqueueFn) -> FCk_Handle_Inventory
    {
        if (UCk_Utils_Inventory_Spatial_UE::Has(InInventory))
        {
            auto Typed = UCk_Utils_Inventory_Spatial_UE::CastChecked(InInventory);
            InEnqueueFn(Typed);
        }
        else if (UCk_Utils_Inventory_DataOnly_UE::Has(InInventory))
        {
            auto Typed = UCk_Utils_Inventory_DataOnly_UE::CastChecked(InInventory);
            InEnqueueFn(Typed);
        }
        return InInventory;
    }
}

namespace ck
{
    template <typename TraitsType>
    auto CreateInventory(
        FCk_Handle& InOwnerEntity,
        const FCk_Fragment_Inventory_ParamsData& InParams,
        ECk_Replication InReplicates,
        const UObject* InWorldContextObject) -> FCk_Handle_Inventory
    {
        auto NewInventoryEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_Inventory>(InOwnerEntity);

        auto FixedParams = InParams;
        if (ck::IsValid(InWorldContextObject))
        {
            auto* const ContextClass = InWorldContextObject->GetClass();
            if (auto& AcceptRef = FixedParams.Get_CanAcceptItemRef(); AcceptRef.IsSelfContext())
            { AcceptRef.SetExternalMember(AcceptRef.GetMemberName(), ContextClass); }
            if (auto& StackRef = FixedParams.Get_CanStackItemsRef(); StackRef.IsSelfContext())
            { StackRef.SetExternalMember(StackRef.GetMemberName(), ContextClass); }
            if (auto& AbsorbRef = FixedParams.Get_GetAbsorbableUnitsRef(); AbsorbRef.IsSelfContext())
            { AbsorbRef.SetExternalMember(AbsorbRef.GetMemberName(), ContextClass); }
        }

        NewInventoryEntity.Add<ck::FFragment_Inventory_Params>(FixedParams);
        UCk_Utils_GameplayLabel_UE::Add(NewInventoryEntity, InParams.Get_Name());

        // Per-shape setup (Spatial: GridSystem; DataOnly: IntegerAttribute for bound max).
        TraitsType::Setup_PerShape(NewInventoryEntity, FixedParams, InReplicates);

        UCk_Utils_Inventory_UE::RecordOfInventoryItems_Utils::AddIfMissing(NewInventoryEntity);
        NewInventoryEntity.Add<ck::FFragment_Inventory_PreviousItems>();

        if (InReplicates == ECk_Replication::Replicates)
        {
            UCk_Utils_Net_UE::TryAddContainerFragment<typename TraitsType::ReplicationFragment>(InOwnerEntity);
        }

        UCk_Utils_Inventory_UE::RecordOfInventories_Utils::AddIfMissing(InOwnerEntity, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
        UCk_Utils_Inventory_UE::RecordOfInventories_Utils::Request_Connect(InOwnerEntity, NewInventoryEntity);

        return NewInventoryEntity;
    }

    template CKINVENTORY_API auto CreateInventory<TInventoryRequestTraits<FCk_Handle_Inventory_Spatial>>(
        FCk_Handle&, const FCk_Fragment_Inventory_ParamsData&, ECk_Replication, const UObject*) -> FCk_Handle_Inventory;
    template CKINVENTORY_API auto CreateInventory<TInventoryRequestTraits<FCk_Handle_Inventory_DataOnly>>(
        FCk_Handle&, const FCk_Fragment_Inventory_ParamsData&, ECk_Replication, const UObject*) -> FCk_Handle_Inventory;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(
    UCk_Utils_Inventory_UE,
    FCk_Handle_Inventory,
    ck::FFragment_Inventory_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Request_AddItem(
        FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_AddItem& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Add& InDelegate)
    -> FCk_Handle_Inventory
{
    if (NOT ck_inventory_utils::ValidateRequestForDispatch(InInventory, InRequest, TEXT("Request_AddItem"),
        [&]{ InDelegate.ExecuteIfBound(InInventory, InRequest.Get_ItemToAdd(),
                ECk_Inventory_OperationResult_Add::Failed_NotEnqueued); }))
    { return InInventory; }

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_Add,
        InRequest.PopulateRequestHandle(InInventory), InDelegate);

    return ck_inventory_utils::DispatchEnqueue_Checked(
        InInventory,
        [&InRequest](auto& Typed)
        {
            using TShape = std::remove_reference_t<decltype(Typed)>;
            using TFragment = ck::TFragment_Inventory_Requests<TShape>;
            Typed.template AddOrGet<TFragment>()._Requests.Emplace(
                typename TFragment::AddItemEntry{InRequest});
        });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Request_RemoveItem(
        FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_RemoveItem& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Remove& InDelegate)
    -> FCk_Handle_Inventory
{
    if (NOT ck_inventory_utils::ValidateRequestForDispatch(InInventory, InRequest, TEXT("Request_RemoveItem"),
        [&]{ InDelegate.ExecuteIfBound(InInventory, InRequest.Get_ItemToRemove(),
                ECk_Inventory_OperationResult_Remove::Failed_NotEnqueued); }))
    { return InInventory; }

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_Remove,
        InRequest.PopulateRequestHandle(InInventory), InDelegate);

    return ck_inventory_utils::DispatchEnqueue_Checked(
        InInventory,
        [&InRequest](auto& Typed)
        {
            using TShape = std::remove_reference_t<decltype(Typed)>;
            using TFragment = ck::TFragment_Inventory_Requests<TShape>;
            Typed.template AddOrGet<TFragment>()._Requests.Emplace(
                typename TFragment::RemoveItemEntry{InRequest});
        });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Request_StackItems(
        FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_StackItems& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Stack& InDelegate)
    -> FCk_Handle_Inventory
{
    if (NOT ck_inventory_utils::ValidateRequestForDispatch(InInventory, InRequest, TEXT("Request_StackItems"),
        [&]{ InDelegate.ExecuteIfBound(InInventory, InRequest.Get_SourceItem(), InRequest.Get_TargetItem(),
                ECk_Inventory_OperationResult_Stack::Failed_NotEnqueued); }))
    { return InInventory; }

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_Stack,
        InRequest.PopulateRequestHandle(InInventory), InDelegate);

    return ck_inventory_utils::DispatchEnqueue_Checked(
        InInventory,
        [&InRequest](auto& Typed)
        {
            using TShape = std::remove_reference_t<decltype(Typed)>;
            using TFragment = ck::TFragment_Inventory_Requests<TShape>;
            Typed.template AddOrGet<TFragment>()._Requests.Emplace(
                typename TFragment::StackItemsEntry{InRequest});
        });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Request_SplitStack(
        FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_SplitStack& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Split& InDelegate)
    -> FCk_Handle_Inventory
{
    if (NOT ck_inventory_utils::ValidateRequestForDispatch(InInventory, InRequest, TEXT("Request_SplitStack"),
        [&]{ InDelegate.ExecuteIfBound(InInventory, InRequest.Get_SourceItem(), FCk_Handle_Item{},
                ECk_Inventory_OperationResult_Split::Failed_NotEnqueued); }))
    { return InInventory; }

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_Split,
        InRequest.PopulateRequestHandle(InInventory), InDelegate);

    return ck_inventory_utils::DispatchEnqueue_Checked(
        InInventory,
        [&InRequest](auto& Typed)
        {
            using TShape = std::remove_reference_t<decltype(Typed)>;
            using TFragment = ck::TFragment_Inventory_Requests<TShape>;
            Typed.template AddOrGet<TFragment>()._Requests.Emplace(
                typename TFragment::SplitStackEntry{InRequest});
        });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Request_AddItemByDefinition(
        FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_AddItemByDefinition& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_AddByDefinition& InDelegate)
    -> FCk_Handle_Inventory
{
    if (NOT ck_inventory_utils::ValidateRequestForDispatch(InInventory, InRequest, TEXT("Request_AddItemByDefinition"),
        [&]{ InDelegate.ExecuteIfBound(InInventory,
                ECk_Inventory_OperationResult_AddByDefinition::Failed_NotEnqueued, 0, TArray<FCk_Handle_Item>{}); }))
    { return InInventory; }

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_AddByDefinition,
        InRequest.PopulateRequestHandle(InInventory), InDelegate);

    return ck_inventory_utils::DispatchEnqueue_Checked(
        InInventory,
        [&InRequest](auto& Typed)
        {
            using TShape = std::remove_reference_t<decltype(Typed)>;
            using TFragment = ck::TFragment_Inventory_Requests<TShape>;
            Typed.template AddOrGet<TFragment>()._Requests.Emplace(
                typename TFragment::AddItemByDefinitionEntry{InRequest});
        });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Request_Sort(
        FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_Sort& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Sort& InDelegate)
    -> FCk_Handle_Inventory
{
    if (NOT ck_inventory_utils::ValidateRequestForDispatch(InInventory, InRequest, TEXT("Request_Sort"),
        [&]{ InDelegate.ExecuteIfBound(InInventory,
                ECk_Inventory_OperationResult_Sort::Failed_NotEnqueued); }))
    { return InInventory; }

    auto Request = InRequest;  // Sort carries native delegates that need a mutable copy for PopulateRequestHandle.
    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_Sort,
        Request.PopulateRequestHandle(InInventory), InDelegate);

    return ck_inventory_utils::DispatchEnqueue_Checked(
        InInventory,
        [&Request](auto& Typed)
        {
            using TShape = std::remove_reference_t<decltype(Typed)>;
            using TFragment = ck::TFragment_Inventory_Requests<TShape>;
            Typed.template AddOrGet<TFragment>()._Requests.Emplace(
                typename TFragment::SortEntry{Request});
        });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Request_TransferItem_ToSpatial(
        FCk_Handle_Inventory& InSourceInventory,
        const FCk_Request_Inventory_TransferItem_ToSpatial& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Transfer& InDelegate)
    -> FCk_Handle_Inventory
{
    if (NOT ck_inventory_utils::ValidateRequestForDispatch(InSourceInventory, InRequest, TEXT("Request_TransferItem_ToSpatial"),
        [&]{ InDelegate.ExecuteIfBound(InSourceInventory, InRequest.Get_SourceItem(),
                InRequest.Get_TargetInventory(), 0, FCk_Handle_Item{},
                ECk_Inventory_OperationResult_Transfer::Failed_NotEnqueued); }))
    { return InSourceInventory; }

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_Transfer,
        InRequest.PopulateRequestHandle(InSourceInventory), InDelegate);

    return ck_inventory_utils::DispatchEnqueue_Checked(
        InSourceInventory,
        [&InRequest](auto& Typed)
        {
            using TShape = std::remove_reference_t<decltype(Typed)>;
            using TFragment = ck::TFragment_Inventory_Requests<TShape>;
            Typed.template AddOrGet<TFragment>()._Requests.Emplace(
                typename TFragment::TransferItemToSpatialEntry{InRequest});
        });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Request_TransferItem_ToDataOnly(
        FCk_Handle_Inventory& InSourceInventory,
        const FCk_Request_Inventory_TransferItem_ToDataOnly& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Transfer& InDelegate)
    -> FCk_Handle_Inventory
{
    if (NOT ck_inventory_utils::ValidateRequestForDispatch(InSourceInventory, InRequest, TEXT("Request_TransferItem_ToDataOnly"),
        [&]{ InDelegate.ExecuteIfBound(InSourceInventory, InRequest.Get_SourceItem(),
                InRequest.Get_TargetInventory(), 0, FCk_Handle_Item{},
                ECk_Inventory_OperationResult_Transfer::Failed_NotEnqueued); }))
    { return InSourceInventory; }

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_Transfer,
        InRequest.PopulateRequestHandle(InSourceInventory), InDelegate);

    return ck_inventory_utils::DispatchEnqueue_Checked(
        InSourceInventory,
        [&InRequest](auto& Typed)
        {
            using TShape = std::remove_reference_t<decltype(Typed)>;
            using TFragment = ck::TFragment_Inventory_Requests<TShape>;
            Typed.template AddOrGet<TFragment>()._Requests.Emplace(
                typename TFragment::TransferItemToDataOnlyEntry{InRequest});
        });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Request_MassTransfer(
        const FCk_Handle& InAnyHandle,
        const FCk_Request_Inventory_MassTransfer& InRequest,
        const FCk_Delegate_Inventory_MassTransfer_OnComplete& InDelegate)
    -> FCk_Handle
{
    // Boundary reject — fire the delegate once with Failed_NotEnqueued, no op entity.
    const auto RejectNotEnqueued = [&]() -> FCk_Handle
    {
        InDelegate.ExecuteIfBound(FCk_Handle{},
            ECk_Inventory_MassTransfer_Result::Failed_NotEnqueued, 0, 0, 0);
        return {};
    };

    if (NOT UCk_Utils_Net_UE::Get_HasAuthority(InAnyHandle))
    {
        ck::inventory::Display(TEXT("Request_MassTransfer: no authority over [{}] — rejected (not enqueued)."),
            InAnyHandle);
        return RejectNotEnqueued();
    }

    // Need at least one valid candidate AND one valid source. An EMPTY-but-valid source is NOT a
    // sync reject — that resolves to Failed_NothingToTransfer asynchronously on the same channel.
    const auto HasValidCandidate = ck::algo::AnyOf(
        InRequest.Get_TargetResolution().Get_Candidates(), ck::algo::IsValidEntityHandle{});
    const auto HasValidSource = ck::algo::AnyOf(
        InRequest.Get_SourceInventories(), ck::algo::IsValidEntityHandle{});

    if (NOT HasValidCandidate || NOT HasValidSource)
    {
        ck::inventory::Display(
            TEXT("Request_MassTransfer: rejected (not enqueued) — validCandidate=[{}], validSource=[{}]."),
            HasValidCandidate, HasValidSource);
        return RejectNotEnqueued();
    }

    // Gather the work list once (committed read), deduped. An item lives in exactly one inventory, so
    // item-level dups only arise from a source listed twice — dedup the source list, then the items.
    const auto& Filter        = InRequest.Get_ItemFilter();
    const auto  FilterIsEmpty = Filter.IsEmpty();

    auto SeenSources = TArray<FCk_Handle_Inventory>{};
    for (const auto& Source : InRequest.Get_SourceInventories())
    {
        if (ck::IsValid(Source))
        { SeenSources.AddUnique(Source); }
    }

    auto Pending = TArray<FCk_Handle_Item>{};
    for (const auto& Source : SeenSources)
    {
        // §4.3 caveat: into >=2 candidate inventories that share one lifetime-owner AND shape, the
        // multi-same-owner replicated-container clobber (deferred to Adam's pass) still applies on
        // clients. MVP is correct for distinct-owner targets (the common case).
        for (const auto& Item : UCk_Utils_Inventory_UE::Get_Items(Source))
        {
            if (ck::Is_NOT_Valid(Item))
            { continue; }

            if (NOT FilterIsEmpty && NOT Filter.Matches(UCk_Utils_ItemTrait_Tags_UE::Get_Tags(Item)))
            { continue; }

            Pending.AddUnique(Item);
        }
    }

    // Self-owned op entity from the transient owner — survives independent of any inventory. A plain
    // FCk_Handle (NOT a typesafe handle); FFragment_Inventory_MassTransfer_InFlight discriminates it.
    auto Transient = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InAnyHandle);
    auto Op = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(Transient);

    // The Add marks the churn's MarkedDirtyBy fragment dirty -> pump-eligible.
    auto& InFlight = Op.AddOrGet<ck::FFragment_Inventory_MassTransfer_InFlight>();
    InFlight._Pending          = MoveTemp(Pending);
    InFlight._TargetResolution = InRequest.Get_TargetResolution();
    InFlight._StepsPerPass     = FMath::Max(1, InRequest.Get_StepsPerPass());
    InFlight._MaxStepsPerFrame = FMath::Max(1, InRequest.Get_MaxStepsPerFrame());

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_MassTransfer_OnComplete, Op, InDelegate);

    return Op;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Has_Any(
        const FCk_Handle& InOwnerEntity)
    -> bool
{
    return RecordOfInventories_Utils::Has(InOwnerEntity);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    TryGet_Inventory(
        const FCk_Handle& InOwnerEntity,
        FGameplayTag InInventoryName)
    -> FCk_Handle_Inventory
{
    return RecordOfInventories_Utils::Get_ValidEntry_ByTag(InOwnerEntity, InInventoryName);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    ForEach_Inventories(
        FCk_Handle& InOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Inventory>
{
    auto Inventories = TArray<FCk_Handle_Inventory>{};

    ForEach_Inventories(InOwnerEntity, [&](FCk_Handle_Inventory& InInventory)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InInventory, InOptionalPayload); }
        else
        { Inventories.Emplace(InInventory); }
    });

    return Inventories;
}

auto
    UCk_Utils_Inventory_UE::
    ForEach_Inventories(
        FCk_Handle& InOwnerEntity,
        const TFunction<void(FCk_Handle_Inventory&)>& InFunc)
    -> void
{
    RecordOfInventories_Utils::ForEach_ValidEntry
    (
        InOwnerEntity,
        [&](FCk_Handle_Inventory InInventory)
        {
            InFunc(InInventory);
        }
    );
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Get_Items(
        const FCk_Handle_Inventory& InInventory)
    -> TArray<FCk_Handle_Item>
{
    return RecordOfInventoryItems_Utils::Get_ValidEntries(InInventory);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Get_NumItems(
        const FCk_Handle_Inventory& InInventory)
    -> int32
{
    return RecordOfInventoryItems_Utils::Get_ValidEntriesCount(InInventory);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Get_TotalUnits(
        const FCk_Handle_Inventory& InInventory)
    -> int32
{
    auto TotalUnits = 0;

    for (const auto& Item : Get_Items(InInventory))
    {
        TotalUnits += UCk_Utils_ItemTrait_Stackable_UE::Get_IsStackable(Item)
            ? UCk_Utils_ItemTrait_Stackable_UE::Get_StackCount(Item)
            : 1;
    }

    return TotalUnits;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Get_ContainsItem(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem)
    -> bool
{
    return RecordOfInventoryItems_Utils::Get_ContainsEntry(InInventory, InItem);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Get_StackRoomFor(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem)
    -> int32
{
    if (NOT UCk_Utils_ItemTrait_Stackable_UE::Get_IsStackable(InItem))
    { return 0; }

    auto StackRoom = 0;

    for (const auto& ExistingItem : Get_Items(InInventory))
    {
        // Pass invalid inventory to bypass the "both items must be in same inventory" containment
        // check inside Get_CanStackItems — InItem is the source we're querying for, not necessarily
        // a member of InInventory. Definition / fragment / custom-stack-validation checks still run.
        const auto NoContainmentCheck = FCk_Handle_Inventory{};

        if (UCk_Utils_ItemTrait_Stackable_UE::Get_CanStackItems(NoContainmentCheck, InItem, ExistingItem)
            != ECk_Inventory_OperationResult_Stack::Success)
        { continue; }

        const auto Remaining = UCk_Utils_ItemTrait_Stackable_UE::Get_RemainingStackCapacity_InInventory(
            InInventory, ExistingItem);

        // Remaining capacity is MAX_int32 when neither the definition nor the inventory caps the
        // stack. Any single such match means the inventory has effectively unbounded stack room.
        if (Remaining == TNumericLimits<int32>::Max())
        { return TNumericLimits<int32>::Max(); }

        // Saturating add to avoid overflow if many large stacks accumulate.
        StackRoom = (Remaining > TNumericLimits<int32>::Max() - StackRoom)
            ? TNumericLimits<int32>::Max()
            : StackRoom + Remaining;
    }

    return StackRoom;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Get_InventoryType(
        const FCk_Handle_Inventory& InInventory)
    -> ECk_InventoryType
{
    const auto& Params = InInventory.Get<ck::FFragment_Inventory_Params>();
    return Params.Get_InventoryType();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Get_IsSpatial(
        const FCk_Handle_Inventory& InInventory)
    -> bool
{
    return InInventory.Has<ck::FTag_Inventory_Spatial>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Get_IsDataOnly(
        const FCk_Handle_Inventory& InInventory)
    -> bool
{
    return InInventory.Has<ck::FTag_Inventory_DataOnly>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Get_CanAcceptItem(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem,
        ECk_Inventory_AddPolicy InPolicy)
    -> ECk_Inventory_OperationResult_Add
{
    constexpr auto DeriveUnitsFromItem = -1;
    return Get_CanAcceptItem_WithCount(InInventory, InItem, InPolicy, DeriveUnitsFromItem);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Get_CanAcceptItem_WithCount(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem,
        ECk_Inventory_AddPolicy InPolicy,
        int32 InIncomingUnits)
    -> ECk_Inventory_OperationResult_Add
{
    if (ck::Is_NOT_Valid(InItem))
    { return ECk_Inventory_OperationResult_Add::Failed_InvalidItem; }

    if (FInventoryItemRecordUtils::Get_ContainsEntry(InInventory, InItem))
    { return ECk_Inventory_OperationResult_Add::Failed_ItemAlreadyInInventory; }

    if (NOT Get_PassesCustomAcceptValidation(InInventory, InItem))
    { return ECk_Inventory_OperationResult_Add::Failed_RejectedByCustomAcceptanceLogic; }

    const auto IsStackable = UCk_Utils_ItemTrait_Stackable_UE::Get_IsStackable(InItem);
    const auto IncomingUnits = (InIncomingUnits > 0)
        ? InIncomingUnits
        : (IsStackable ? FMath::Max(1, UCk_Utils_ItemTrait_Stackable_UE::Get_StackCount(InItem)) : 1);

    // PreferStacking lets the soft "no room as a new entry" failures (entry bound full, per-entry
    // stack clamp exceeded, Spatial no fit) pass when existing stacks in this inventory can absorb
    // the item's FULL unit count. Count-aware on purpose: room for 1 unit must not green-light a
    // 99-unit item. Computed lazily — Get_StackRoomFor walks the item list, so we only call it on
    // a soft-failure path.
    const auto CanFallBackToStacking = [&]() -> bool
    {
        return InPolicy == ECk_Inventory_AddPolicy::PreferStacking
            && Get_StackRoomFor(InInventory, InItem) >= IncomingUnits;
    };

    // ---- Per-entry stacking clamp: a whole-item add lands as ONE entry, which may not exceed the
    //      inventory's effective max stack size ----

    if (IsStackable
        && IncomingUnits > UCk_Utils_ItemTrait_Stackable_UE::Get_EffectiveMaxStackSize(InInventory, InItem))
    {
        if (CanFallBackToStacking())
        { return ECk_Inventory_OperationResult_Add::Success; }

        return ECk_Inventory_OperationResult_Add::Failed_NoSpaceAvailable;
    }

    // ---- Bounds check for DataOnly inventories ----

    if (Get_IsDataOnly(InInventory))
    {
        if (const auto DataOnlyHandle = UCk_Utils_Inventory_DataOnly_UE::Cast(InInventory);
            ck::IsValid(DataOnlyHandle))
        {
            switch (UCk_Utils_Inventory_DataOnly_UE::Get_EffectiveBoundMode(DataOnlyHandle))
            {
                case ECk_Inventory_DataOnly_BoundMode::BoundedByUniqueEntries:
                {
                    if (UCk_Utils_Inventory_DataOnly_UE::Get_RemainingSlots(DataOnlyHandle) <= 0)
                    {
                        if (CanFallBackToStacking())
                        { return ECk_Inventory_OperationResult_Add::Success; }

                        return ECk_Inventory_OperationResult_Add::Failed_NoSpaceAvailable;
                    }
                    break;
                }
                case ECk_Inventory_DataOnly_BoundMode::BoundedByTotalUnits:
                {
                    // No stacking fallback: units consume the bound no matter which route they
                    // enter through, so a units-full inventory cannot be rescued by merging.
                    if (UCk_Utils_Inventory_DataOnly_UE::Get_RemainingCapacity(DataOnlyHandle) < IncomingUnits)
                    { return ECk_Inventory_OperationResult_Add::Failed_NoSpaceAvailable; }
                    break;
                }
                case ECk_Inventory_DataOnly_BoundMode::Unbounded:
                default:
                { break; }
            }
        }
    }

    // ---- Spatial fit check for Spatial inventories ----

    if (Get_IsSpatial(InInventory))
    {
        if (const auto SpatialHandle = UCk_Utils_Inventory_Spatial_UE::Cast(InInventory);
            ck::IsValid(SpatialHandle))
        {
            if (NOT UCk_Utils_2dGridSystem_UE::Has(InItem))
            { return ECk_Inventory_OperationResult_Add::Failed_MissingDimensionsTrait; }

            const auto Placement = UCk_Utils_Inventory_Spatial_UE::Get_FirstAvailablePlacement(SpatialHandle, InItem);
            if (NOT Placement.Get_Succeeded())
            {
                if (CanFallBackToStacking())
                { return ECk_Inventory_OperationResult_Add::Success; }

                return ECk_Inventory_OperationResult_Add::Failed_NoSpaceAvailable;
            }
        }
    }

    // ---- Custom quantitative quota (weight-style rules). No stacking fallback — absorbed units
    //      count against the quota no matter which route they enter through ----

    if (const auto Quota = Get_CustomAbsorbableUnits(InInventory, UCk_Utils_Item_UE::Get_Definition(InItem), InItem);
        Quota < IncomingUnits)
    { return ECk_Inventory_OperationResult_Add::Failed_NoSpaceAvailable; }

    return ECk_Inventory_OperationResult_Add::Success;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Get_PassesCustomAcceptValidation(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem)
    -> bool
{
    if (ck::Is_NOT_Valid(InInventory))
    { return true; }

    const auto& Params = InInventory.Get<ck::FFragment_Inventory_Params>();

    if (const auto& NativeDelegate = Params.Get_CustomCanAcceptItem();
        NativeDelegate.IsBound())
    {
        if (NOT NativeDelegate.Execute(InInventory, InItem))
        { return false; }
    }

    if (const auto& DynamicDelegate = Params.Get_CustomCanAcceptItemDynamic();
        DynamicDelegate.IsBound())
    {
        auto Result = true;
        DynamicDelegate.ExecuteIfBound(InInventory, InItem, Result);

        if (NOT Result)
        { return false; }
    }

    if (const auto MemberRefResult = Resolve_CanAcceptItem(
            Params.Get_CanAcceptItemRef(), InInventory, InItem);
        MemberRefResult.IsSet())
    {
        if (NOT MemberRefResult.GetValue())
        { return false; }
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Get_CustomAbsorbableUnits(
        const FCk_Handle_Inventory& InInventory,
        const UCk_InventoryItem_Definition* InDefinition,
        const FCk_Handle_Item& InItem)
    -> int32
{
    if (ck::Is_NOT_Valid(InInventory))
    { return TNumericLimits<int32>::Max(); }

    const auto& Params = InInventory.Get<ck::FFragment_Inventory_Params>();
    auto Quota = TNumericLimits<int32>::Max();

    if (const auto& NativeDelegate = Params.Get_CustomGetAbsorbableUnits();
        NativeDelegate.IsBound())
    { Quota = FMath::Min(Quota, NativeDelegate.Execute(InInventory, InDefinition, InItem)); }

    if (const auto& DynamicDelegate = Params.Get_CustomGetAbsorbableUnitsDynamic();
        DynamicDelegate.IsBound())
    {
        auto Result = TNumericLimits<int32>::Max();
        DynamicDelegate.ExecuteIfBound(InInventory, InDefinition, InItem, Result);
        Quota = FMath::Min(Quota, Result);
    }

    if (const auto MemberRefResult = Resolve_GetAbsorbableUnits(
            Params.Get_GetAbsorbableUnitsRef(), InInventory, InDefinition, InItem);
        MemberRefResult.IsSet())
    { Quota = FMath::Min(Quota, MemberRefResult.GetValue()); }

    return FMath::Max(0, Quota);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Get_AbsorbableUnits(
        const FCk_Handle_Inventory& InInventory,
        const UCk_InventoryItem_Definition* InDefinition)
    -> int32
{
    if (ck::Is_NOT_Valid(InInventory))
    { return 0; }

    if (ck::Is_NOT_Valid(InDefinition, ck::IsValid_Policy_NullptrOnly{}))
    { return 0; }

    const auto SaturatingAdd = [](int32 InA, int32 InB) -> int32
    {
        return (InA > TNumericLimits<int32>::Max() - InB)
            ? TNumericLimits<int32>::Max()
            : InA + InB;
    };

    const auto SaturatingMul = [](int32 InA, int32 InB) -> int32
    {
        if (InA == 0 || InB == 0)
        { return 0; }

        return (InA > TNumericLimits<int32>::Max() / InB)
            ? TNumericLimits<int32>::Max()
            : InA * InB;
    };

    const auto IsStackable = InDefinition->Has_ItemTrait<UCk_ItemTrait_Stackable>();
    const auto EffectiveMaxPerEntry = UCk_Utils_ItemTrait_Stackable_UE::Get_EffectiveMaxStackSize_ByDefinition(
        InInventory, InDefinition);

    // ---- Room in existing same-definition stacks (mirrors Request_FillExistingStacks: definition
    //      match + custom stack validation with no source item, since this is a definition-level
    //      planning query) ----

    auto StackRoom = 0;

    if (IsStackable)
    {
        for (const auto& ExistingItem : Get_Items(InInventory))
        {
            if (UCk_Utils_Item_UE::Get_Definition(ExistingItem) != InDefinition)
            { continue; }

            const auto NoSourceItem = FCk_Handle_Item{};
            if (NOT UCk_Utils_ItemTrait_Stackable_UE::Get_PassesCustomStackValidation(
                    InInventory, NoSourceItem, ExistingItem))
            { continue; }

            StackRoom = SaturatingAdd(StackRoom,
                UCk_Utils_ItemTrait_Stackable_UE::Get_RemainingStackCapacity_InInventory(InInventory, ExistingItem));
        }
    }

    // ---- Room from creating new entries ----

    auto NewEntryUnits = TNumericLimits<int32>::Max();

    if (Get_IsDataOnly(InInventory))
    {
        if (const auto DataOnlyHandle = UCk_Utils_Inventory_DataOnly_UE::Cast(InInventory);
            ck::IsValid(DataOnlyHandle)
            && UCk_Utils_Inventory_DataOnly_UE::Get_EffectiveBoundMode(DataOnlyHandle)
                == ECk_Inventory_DataOnly_BoundMode::BoundedByUniqueEntries)
        {
            NewEntryUnits = SaturatingMul(
                UCk_Utils_Inventory_DataOnly_UE::Get_RemainingSlots(DataOnlyHandle), EffectiveMaxPerEntry);
        }
        // TotalUnits / Unbounded modes leave entry count unconstrained — the metric ceiling below governs.
    }
    else if (Get_IsSpatial(InInventory))
    {
        const auto SpatialHandle = UCk_Utils_Inventory_Spatial_UE::Cast(InInventory);
        const auto* DimensionsTrait = InDefinition->Get_ItemTrait<UCk_ItemTrait_Dimensions>();

        if (ck::Is_NOT_Valid(SpatialHandle) || ck::Is_NOT_Valid(DimensionsTrait, ck::IsValid_Policy_NullptrOnly{}))
        { NewEntryUnits = 0; }
        else
        {
            // Free cells / footprint area is an UPPER BOUND on placements — it ignores
            // fragmentation. Good enough for planning; placement still gates at execution time.
            const auto Footprint = DimensionsTrait->Get_Dimensions();
            const auto FootprintArea = FMath::Max(1, Footprint.X * Footprint.Y);
            const auto MaxPlacements = UCk_Utils_Inventory_Spatial_UE::Get_NumFreeCells(SpatialHandle) / FootprintArea;
            NewEntryUnits = SaturatingMul(MaxPlacements, EffectiveMaxPerEntry);
        }
    }

    auto Total = SaturatingAdd(StackRoom, NewEntryUnits);

    // ---- Metric ceiling: under BoundedByTotalUnits every absorbed unit consumes the bound,
    //      regardless of whether it enters via stacking or a new entry ----

    if (Get_IsDataOnly(InInventory))
    {
        if (const auto DataOnlyHandle = UCk_Utils_Inventory_DataOnly_UE::Cast(InInventory);
            ck::IsValid(DataOnlyHandle)
            && UCk_Utils_Inventory_DataOnly_UE::Get_EffectiveBoundMode(DataOnlyHandle)
                == ECk_Inventory_DataOnly_BoundMode::BoundedByTotalUnits)
        {
            Total = FMath::Min(Total, UCk_Utils_Inventory_DataOnly_UE::Get_RemainingCapacity(DataOnlyHandle));
        }
    }

    // ---- Custom quantitative quota ----

    const auto NoItem = FCk_Handle_Item{};
    return FMath::Min(Total, Get_CustomAbsorbableUnits(InInventory, InDefinition, NoItem));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    BindTo_OnItemsChanged(
        FCk_Handle_Inventory& InInventory,
        const FCk_Delegate_Inventory_OnItemsChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Inventory
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_Inventory_OnItemsChanged, InInventory, InDelegate, InBindingPolicy, InPostFireBehavior);
    return InInventory;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    UnbindFrom_OnItemsChanged(
        FCk_Handle_Inventory& InInventory,
        const FCk_Delegate_Inventory_OnItemsChanged& InDelegate)
    -> FCk_Handle_Inventory
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_Inventory_OnItemsChanged, InInventory, InDelegate);
    return InInventory;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Resolve_CanAcceptItem(
        const FMemberReference& InRef,
        FCk_Handle_Inventory InInventory,
        FCk_Handle_Item InItem)
    -> TOptional<bool>
{
    auto* const MemberClass = InRef.GetMemberParentClass();

    if (ck::Is_NOT_Valid(MemberClass))
    { return {}; }

    auto* const Function = InRef.ResolveMember<UFunction>(MemberClass);

    if (ck::Is_NOT_Valid(Function))
    { return {}; }

    struct
    {
        FCk_Handle_Inventory Inventory;
        FCk_Handle_Item Item;
        bool ReturnValue = false;
    } Args = { MoveTemp(InInventory), MoveTemp(InItem) };

    auto* const Context = MemberClass->GetDefaultObject();
    Context->ProcessEvent(Function, &Args);

    return Args.ReturnValue;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Resolve_GetAbsorbableUnits(
        const FMemberReference& InRef,
        FCk_Handle_Inventory InInventory,
        const UCk_InventoryItem_Definition* InDefinition,
        FCk_Handle_Item InItem)
    -> TOptional<int32>
{
    auto* const MemberClass = InRef.GetMemberParentClass();

    if (ck::Is_NOT_Valid(MemberClass))
    { return {}; }

    auto* const Function = InRef.ResolveMember<UFunction>(MemberClass);

    if (ck::Is_NOT_Valid(Function))
    { return {}; }

    struct
    {
        FCk_Handle_Inventory Inventory;
        const UCk_InventoryItem_Definition* Definition = nullptr;
        FCk_Handle_Item Item;
        int32 ReturnValue = 0;
    } Args = { MoveTemp(InInventory), InDefinition, MoveTemp(InItem) };

    auto* const Context = MemberClass->GetDefaultObject();
    Context->ProcessEvent(Function, &Args);

    return Args.ReturnValue;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Request_TryReplicateInventory(
        FCk_Handle_Inventory& InInventory)
    -> void
{
    InInventory.AddOrGet<ck::FTag_Inventory_MayRequireReplication>();
}

auto
    UCk_Utils_Inventory_UE::
    Request_MarkInventory_AsMayHaveChanged(
        FCk_Handle_Inventory& InInventory)
    -> void
{
    InInventory.AddOrGet<ck::FTag_Inventory_MayHaveChanged>();
}


// --------------------------------------------------------------------------------------------------------------------
// Item Resolution
// --------------------------------------------------------------------------------------------------------------------

namespace ck_item_resolution
{
    struct FCandidateScore
    {
        FCk_Handle_Inventory Inventory;
        int32 StackRoom = 0;
        int32 RemainingCapacity = 0;
    };

    // Maps each inventory subtype to its remaining-capacity metric.
    // DataOnly: Get_RemainingCapacity (metric-aware: entries or units left; MAX_int32 for unbounded).
    // Spatial:  Get_NumFreeCells  (active unoccupied cells; ignores item footprints, but exact for cells).
    // Unknown subtype: MAX_int32 so it doesn't bias ordering against typed inventories.
    auto Compute_RemainingCapacity(const FCk_Handle_Inventory& InCandidate) -> int32
    {
        if (UCk_Utils_Inventory_UE::Get_IsDataOnly(InCandidate))
        {
            if (const auto DataOnlyHandle = UCk_Utils_Inventory_DataOnly_UE::Cast(InCandidate);
                ck::IsValid(DataOnlyHandle))
            { return UCk_Utils_Inventory_DataOnly_UE::Get_RemainingCapacity(DataOnlyHandle); }
        }

        if (UCk_Utils_Inventory_UE::Get_IsSpatial(InCandidate))
        {
            if (const auto SpatialHandle = UCk_Utils_Inventory_Spatial_UE::Cast(InCandidate);
                ck::IsValid(SpatialHandle))
            { return UCk_Utils_Inventory_Spatial_UE::Get_NumFreeCells(SpatialHandle); }
        }

        return TNumericLimits<int32>::Max();
    }
}

auto
    UCk_Utils_ItemResolution_UE::
    ResolveBestTransferTarget(
        FCk_Handle_Item& InItem,
        const FCk_BestTransferTargetParams& InRequest)
    -> FCk_Handle_Inventory
{
    CK_ENSURE_IF_NOT(ck::IsValid(InItem),
        TEXT("Cannot resolve best transfer target — InItem [{}] is invalid"),
        InItem)
    { return {}; }

    const auto StackingPreference = InRequest.Get_StackingPreference();
    const auto AddPolicy = InRequest.Get_AddPolicy();

    auto Survivors = TArray<ck_item_resolution::FCandidateScore>{};
    Survivors.Reserve(InRequest.Get_Candidates().Num());

    for (const auto& Candidate : ck::algo::Filter(InRequest.Get_Candidates(), ck::algo::IsValidEntityHandle{}))
    {
        if (UCk_Utils_Inventory_UE::Get_CanAcceptItem(Candidate, InItem, AddPolicy)
            != ECk_Inventory_OperationResult_Add::Success)
        { continue; }

        const auto StackRoom = UCk_Utils_Inventory_UE::Get_StackRoomFor(Candidate, InItem);

        if (StackingPreference == ECk_ItemResolution_StackingPreference::Require && StackRoom == 0)
        { continue; }

        Survivors.Add({Candidate, StackRoom, ck_item_resolution::Compute_RemainingCapacity(Candidate)});
    }

    if (Survivors.IsEmpty())
    { return {}; }

    const auto& CustomSort = InRequest.Get_CustomSort();
    const auto& CustomSortDynamic = InRequest.Get_CustomSortDynamic();

    // Within-tier comparator: custom sort if bound, otherwise built-in (stack room → capacity for
    // Prefer/Require, capacity only for Ignore). Returns true if A should rank ahead of B.
    const auto WithinTierCompare = [&](const ck_item_resolution::FCandidateScore& InA,
                                       const ck_item_resolution::FCandidateScore& InB) -> bool
    {
        if (CustomSort.IsBound())
        { return CustomSort.Execute(InA.Inventory, InB.Inventory, InItem); }

        if (CustomSortDynamic.IsBound())
        {
            auto AIsBetter = false;
            CustomSortDynamic.ExecuteIfBound(InA.Inventory, InB.Inventory, InItem, AIsBetter);
            return AIsBetter;
        }

        if (StackingPreference != ECk_ItemResolution_StackingPreference::Ignore
            && InA.StackRoom != InB.StackRoom)
        { return InA.StackRoom > InB.StackRoom; }

        return InA.RemainingCapacity > InB.RemainingCapacity;
    };

    // Layered comparator: stacking preference defines tier boundaries; custom sort orders within a tier.
    // - Prefer:  has-stack-room is the primary tier boundary; tiebreaker = WithinTierCompare.
    // - Require: filter pass already dropped zero-stack candidates → single tier; defer entirely to WithinTierCompare.
    // - Ignore:  no tiering; defer entirely to WithinTierCompare.
    // StableSort preserves input order for fully-tied candidates so callers see deterministic results.
    Survivors.StableSort([&](const ck_item_resolution::FCandidateScore& InA, const ck_item_resolution::FCandidateScore& InB)
    {
        if (StackingPreference == ECk_ItemResolution_StackingPreference::Prefer)
        {
            const auto AHasStack = InA.StackRoom > 0;
            const auto BHasStack = InB.StackRoom > 0;

            if (AHasStack != BHasStack)
            { return AHasStack; }
        }

        return WithinTierCompare(InA, InB);
    });

    return Survivors[0].Inventory;
}

// --------------------------------------------------------------------------------------------------------------------
