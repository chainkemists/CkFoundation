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

#include "CkInventory/ItemTrait/Stackable/CkItemTrait_Stackable_Utils.h"

namespace
{
    // Auth check + runtime-branch on the inventory's shape tag, hands the typed handle to
    // the caller-supplied lambda. The shape-branch lives ONLY here at the public Utils boundary.
    template <typename TEnqueueFn>
    auto DispatchEnqueue(
        FCk_Handle_Inventory& InInventory,
        const TCHAR* InContext,
        FName InDelegateFunctionName,
        TEnqueueFn InEnqueueFn) -> FCk_Handle_Inventory
    {
        CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_HasAuthority(InInventory),
            TEXT("{}: No authority over inventory [{}].{}"),
            InContext, InInventory, ck::Context(InDelegateFunctionName))
        { return InInventory; }

        if (UCk_Utils_Inventory_Spatial_UE::Has(InInventory))
        {
            auto Typed = UCk_Utils_Inventory_Spatial_UE::CastChecked(InInventory);
            InEnqueueFn(Typed);
            return InInventory;
        }
        if (UCk_Utils_Inventory_DataOnly_UE::Has(InInventory))
        {
            auto Typed = UCk_Utils_Inventory_DataOnly_UE::CastChecked(InInventory);
            InEnqueueFn(Typed);
            return InInventory;
        }

        CK_TRIGGER_ENSURE(TEXT("{}: Inventory [{}] has no recognized shape tag (neither Spatial nor DataOnly)"),
            InContext, InInventory);
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
    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_Add,
        InRequest.PopulateRequestHandle(InInventory), InDelegate);

    return DispatchEnqueue(
        InInventory, TEXT("Request_AddItem"), InDelegate.GetFunctionName(),
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
    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_Remove,
        InRequest.PopulateRequestHandle(InInventory), InDelegate);

    return DispatchEnqueue(
        InInventory, TEXT("Request_RemoveItem"), InDelegate.GetFunctionName(),
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
    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_Stack,
        InRequest.PopulateRequestHandle(InInventory), InDelegate);

    return DispatchEnqueue(
        InInventory, TEXT("Request_StackItems"), InDelegate.GetFunctionName(),
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
    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_Split,
        InRequest.PopulateRequestHandle(InInventory), InDelegate);

    return DispatchEnqueue(
        InInventory, TEXT("Request_SplitStack"), InDelegate.GetFunctionName(),
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
    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_AddByDefinition,
        InRequest.PopulateRequestHandle(InInventory), InDelegate);

    return DispatchEnqueue(
        InInventory, TEXT("Request_AddItemByDefinition"), InDelegate.GetFunctionName(),
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
    auto Request = InRequest;  // Sort carries native delegates that need a mutable copy for PopulateRequestHandle.
    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_Sort,
        Request.PopulateRequestHandle(InInventory), InDelegate);

    return DispatchEnqueue(
        InInventory, TEXT("Request_Sort"), InDelegate.GetFunctionName(),
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
    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_Transfer,
        InRequest.PopulateRequestHandle(InSourceInventory), InDelegate);

    return DispatchEnqueue(
        InSourceInventory, TEXT("Request_TransferItem_ToSpatial"), InDelegate.GetFunctionName(),
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
    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_Transfer,
        InRequest.PopulateRequestHandle(InSourceInventory), InDelegate);

    return DispatchEnqueue(
        InSourceInventory, TEXT("Request_TransferItem_ToDataOnly"), InDelegate.GetFunctionName(),
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

        const auto Remaining = UCk_Utils_ItemTrait_Stackable_UE::Get_RemainingStackCapacity(ExistingItem);

        // Get_RemainingStackCapacity returns MAX_int32 when the stack has no max size.
        // Any single such match means the inventory has effectively unbounded stack room.
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
    if (ck::Is_NOT_Valid(InItem))
    { return ECk_Inventory_OperationResult_Add::Failed_InvalidItem; }

    if (FInventoryItemRecordUtils::Get_ContainsEntry(InInventory, InItem))
    { return ECk_Inventory_OperationResult_Add::Failed_ItemAlreadyInInventory; }

    if (NOT Get_PassesCustomAcceptValidation(InInventory, InItem))
    { return ECk_Inventory_OperationResult_Add::Failed_RejectedByCustomAcceptanceLogic; }

    // PreferStacking lets the soft "no room" failures (DataOnly bound full, Spatial no fit) pass
    // when an existing item in this inventory can absorb InItem via stacking. Computed lazily —
    // Get_StackRoomFor walks the item list, so we only call it on a soft-failure path.
    const auto CanFallBackToStacking = [&]() -> bool
    {
        return InPolicy == ECk_Inventory_AddPolicy::PreferStacking
            && Get_StackRoomFor(InInventory, InItem) > 0;
    };

    // ---- Bounds check for DataOnly inventories ----

    if (Get_IsDataOnly(InInventory))
    {
        if (const auto DataOnlyHandle = UCk_Utils_Inventory_DataOnly_UE::Cast(InInventory);
            ck::IsValid(DataOnlyHandle))
        {
            if (const auto BoundMax = UCk_Utils_Inventory_DataOnly_UE::Get_BoundMax(DataOnlyHandle);
                BoundMax.IsSet() && Get_NumItems(InInventory) >= BoundMax.GetValue())
            {
                if (CanFallBackToStacking())
                { return ECk_Inventory_OperationResult_Add::Success; }

                return ECk_Inventory_OperationResult_Add::Failed_NoSpaceAvailable;
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
    // DataOnly: Get_RemainingSlots (BoundMax - NumItems, MAX_int32 for unbounded).
    // Spatial:  Get_NumFreeCells  (active unoccupied cells; ignores item footprints, but exact for cells).
    // Unknown subtype: MAX_int32 so it doesn't bias ordering against typed inventories.
    auto Compute_RemainingCapacity(const FCk_Handle_Inventory& InCandidate) -> int32
    {
        if (UCk_Utils_Inventory_UE::Get_IsDataOnly(InCandidate))
        {
            if (const auto DataOnlyHandle = UCk_Utils_Inventory_DataOnly_UE::Cast(InCandidate);
                ck::IsValid(DataOnlyHandle))
            { return UCk_Utils_Inventory_DataOnly_UE::Get_RemainingSlots(DataOnlyHandle); }
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

    auto Survivors = TArray<ck_item_resolution::FCandidateScore>{};
    Survivors.Reserve(InRequest.Get_Candidates().Num());

    for (const auto& Candidate : ck::algo::Filter(InRequest.Get_Candidates(), ck::algo::IsValidEntityHandle{}))
    {
        // PreferStacking accepts candidates whose only failure mode is "no room for new entry"
        // when an existing stack can absorb the item. Centralized in Get_CanAcceptItem so this
        // utility and Request_AddItemByDefinition / Request_TransferItem share one validator.
        if (UCk_Utils_Inventory_UE::Get_CanAcceptItem(Candidate, InItem, ECk_Inventory_AddPolicy::PreferStacking)
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
