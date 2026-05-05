#include "CkInventory_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkInventory/Inventory/CkInventory_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkLabel/CkLabel_Utils.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"
#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Utils.h"

#include "CkGrid/CkGrid_Utils.h"

#include "CkInventory/ItemTrait/Stackable/CkItemTrait_Stackable_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_inventory
{
    auto Get_ItemActiveCells(const FCk_Handle_Item& InItem) -> TArray<FIntPoint>
    {
        if (auto GridHandle = UCk_Utils_2dGridSystem_UE::Cast(InItem);
            ck::IsValid(GridHandle))
        {
            return UCk_Utils_2dGridSystem_UE::Get_AllCells_AsCoordinate(
                GridHandle, ECk_2dGridSystem_CellFilter::OnlyActiveCells);
        }

        // Fallback: 1x1 item without a Dimensions fragment
        return { FIntPoint(0, 0) };
    }

    // ---- Rotation helpers ----

    auto CardinalRotationToYaw(ECk_CardinalRotation InRotation) -> float
    {
        switch (InRotation)
        {
            case ECk_CardinalRotation::None:          return 0.0f;
            case ECk_CardinalRotation::Quarter:       return 90.0f;
            case ECk_CardinalRotation::Half:          return 180.0f;
            case ECk_CardinalRotation::ThreeQuarter:  return 270.0f;
            default:                                  return 0.0f;
        }
    }

    auto YawToCardinalRotation(float InYaw) -> ECk_CardinalRotation
    {
        // Normalize to [0, 360)
        auto Yaw = FMath::Fmod(InYaw, 360.0f);
        if (Yaw < 0.0f) { Yaw += 360.0f; }

        // Quantize to nearest 90°
        const auto Quantized = FMath::RoundToInt(Yaw / 90.0f) % 4;

        switch (Quantized)
        {
            case 0:  return ECk_CardinalRotation::None;
            case 1:  return ECk_CardinalRotation::Quarter;
            case 2:  return ECk_CardinalRotation::Half;
            case 3:  return ECk_CardinalRotation::ThreeQuarter;
            default: return ECk_CardinalRotation::None;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(
    UCk_Utils_Inventory_UE,
    FCk_Handle_Inventory,
    ck::FFragment_Inventory_Params);

// --------------------------------------------------------------------------------------------------------------------
// Make Params
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Make_InventoryParams_Spatial(
        FGameplayTag InName,
        FIntPoint InDimensions,
        FCk_Delegate_Inventory_CustomCanAcceptItem_Dynamic InCustomCanAcceptItem,
        FCk_Delegate_Inventory_CustomCanStackItems_Dynamic InCustomCanStackItems)
    -> FCk_Fragment_Inventory_ParamsData
{
    const auto Params = FCk_Fragment_Inventory_ParamsData(InName, InDimensions)
        .Set_CustomCanAcceptItemDynamic(InCustomCanAcceptItem)
        .Set_CustomCanStackItemsDynamic(InCustomCanStackItems);
    return Params;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Make_InventoryParams_DataOnly(
        FGameplayTag InName,
        FCk_Delegate_Inventory_CustomCanAcceptItem_Dynamic InCustomCanAcceptItem,
        FCk_Delegate_Inventory_CustomCanStackItems_Dynamic InCustomCanStackItems)
    -> FCk_Fragment_Inventory_ParamsData
{
    const auto Params = FCk_Fragment_Inventory_ParamsData(InName)
        .Set_CustomCanAcceptItemDynamic(InCustomCanAcceptItem)
        .Set_CustomCanStackItemsDynamic(InCustomCanStackItems);
    return Params;
}

auto
    UCk_Utils_Inventory_UE::
    Make_InventoryParams_DataOnly_Bounded(
        FGameplayTag InName,
        int32 InBoundLimit,
        FCk_Delegate_Inventory_CustomCanAcceptItem_Dynamic InCustomCanAcceptItem,
        FCk_Delegate_Inventory_CustomCanStackItems_Dynamic InCustomCanStackItems)
    -> FCk_Fragment_Inventory_ParamsData
{
    const auto Params = FCk_Fragment_Inventory_ParamsData(InName, TOptional<int32>(InBoundLimit))
        .Set_CustomCanAcceptItemDynamic(InCustomCanAcceptItem)
        .Set_CustomCanStackItemsDynamic(InCustomCanStackItems);
    return Params;
}

// --------------------------------------------------------------------------------------------------------------------
// Creation
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Add(
        FCk_Handle& InOwnerEntity,
        const FCk_Fragment_Inventory_ParamsData& InParams,
        ECk_Replication InReplicates,
        const UObject* InWorldContextObject)
    -> FCk_Handle_Inventory
{
    auto NewInventoryEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_Inventory>(InOwnerEntity);

    // ---- Fix up self-context FMemberReferences using the world context object ----

    auto FixedParams = InParams;

    if (ck::IsValid(InWorldContextObject))
    {
        auto* const ContextClass = InWorldContextObject->GetClass();

        if (auto& AcceptRef = FixedParams.Get_CanAcceptItemRef();
            AcceptRef.IsSelfContext())
        {
            AcceptRef.SetExternalMember(AcceptRef.GetMemberName(), ContextClass);
        }

        if (auto& StackRef = FixedParams.Get_CanStackItemsRef();
            StackRef.IsSelfContext())
        {
            StackRef.SetExternalMember(StackRef.GetMemberName(), ContextClass);
        }
    }

    NewInventoryEntity.Add<ck::FFragment_Inventory_Params>(FixedParams);
    UCk_Utils_GameplayLabel_UE::Add(NewInventoryEntity, InParams.Get_Name());

    // Add inventory type tag
    switch (InParams.Get_InventoryType())
    {
        case ECk_InventoryType::DataOnly:
        {
            NewInventoryEntity.Add<ck::FTag_Inventory_DataOnly>();

            // ---- Create internal integer attribute for bound max ----

            const auto BoundValue = (InParams.Get_BoundMode() == ECk_Inventory_DataOnly_BoundMode::Bounded)
                ? InParams.Get_BoundLimit()
                : ck::Inventory::UnboundedBoundLimit;

            const auto AttrParams = FCk_Fragment_IntegerAttribute_ParamsData(
                TAG_IntegerAttribute_InventoryBoundMax, BoundValue);

            UCk_Utils_IntegerAttribute_UE::Add(NewInventoryEntity, AttrParams, InReplicates);

            break;
        }
        case ECk_InventoryType::Spatial:
        {
            NewInventoryEntity.Add<ck::FTag_Inventory_Spatial>();

            // Add Transform + 2dGridSystem for spatial inventories
            auto TransformHandle = UCk_Utils_Transform_UE::Add(NewInventoryEntity, FTransform::Identity);

            const auto GridParams = FCk_Fragment_2dGridSystem_ParamsData(
                InParams.Get_Dimensions(),
                FVector2D(1.0, 1.0));

            auto TileMapHandle = UCk_Utils_2dGridSystem_UE::Add(TransformHandle, GridParams);

            UCk_Utils_2dGridSystem_UE::ForEach_Cell(TileMapHandle, ECk_2dGridSystem_CellFilter::OnlyActiveCells,
                [&](FCk_Handle_2dGridCell InCell)
            {
                ck::TUtils_InventorySlot_ItemRef::Clear(InCell);
            });

            break;
        }
        default:
        {
            CK_INVALID_ENUM(InParams.Get_InventoryType());
            break;
        }
    }

    // Initialize record of items on the inventory entity
    RecordOfInventoryItems_Utils::AddIfMissing(NewInventoryEntity);

    // Initialize previous items snapshot for change detection
    NewInventoryEntity.Add<ck::FFragment_Inventory_PreviousItems>();

    // Set up replication
    if (InReplicates == ECk_Replication::Replicates)
    {
        UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_InventoryItems>(InOwnerEntity);
    }

    // Connect to owner's record of inventories
    RecordOfInventories_Utils::AddIfMissing(InOwnerEntity, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfInventories_Utils::Request_Connect(InOwnerEntity, NewInventoryEntity);

    return NewInventoryEntity;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    AddMultiple(
        FCk_Handle& InOwnerEntity,
        const FCk_Fragment_MultipleInventory_ParamsData& InParams,
        ECk_Replication InReplicates,
        const UObject* InWorldContextObject)
    -> TArray<FCk_Handle_Inventory>
{
    return ck::algo::Transform<TArray<FCk_Handle_Inventory>>(
        InParams.Get_InventoryParams(), [&](const FCk_Fragment_Inventory_ParamsData& InParam)
    {
        return Add(InOwnerEntity, InParam, InReplicates, InWorldContextObject);
    });
}

// --------------------------------------------------------------------------------------------------------------------
// Queries
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

// --------------------------------------------------------------------------------------------------------------------
// Validation
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
// Requests (Authority Only)
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Request_AddItem(
        FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_AddItem& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Add& InDelegate)
    -> FCk_Handle_Inventory
{
    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_HasAuthority(InInventory),
        TEXT("Request_AddItem: No authority over inventory [{}].{}"), InInventory, ck::Context(InDelegate.GetFunctionName()))
    { return {}; }

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_Add,
        InRequest.PopulateRequestHandle(InInventory), InDelegate);

    InInventory.AddOrGet<ck::FFragment_Inventory_Requests>()._Requests.Emplace(InRequest);
    return InInventory;
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
    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_HasAuthority(InInventory),
        TEXT("Request_RemoveItem: No authority over inventory [{}].{}"), InInventory, ck::Context(InDelegate.GetFunctionName()))
    { return {}; }

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_Remove,
        InRequest.PopulateRequestHandle(InInventory), InDelegate);

    InInventory.AddOrGet<ck::FFragment_Inventory_Requests>()._Requests.Emplace(InRequest);
    return InInventory;
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
    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_HasAuthority(InInventory),
        TEXT("Request_StackItems: No authority over inventory [{}].{}"), InInventory, ck::Context(InDelegate.GetFunctionName()))
    { return {}; }

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_Stack,
        InRequest.PopulateRequestHandle(InInventory), InDelegate);

    InInventory.AddOrGet<ck::FFragment_Inventory_Requests>()._Requests.Emplace(InRequest);
    return InInventory;
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
    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_HasAuthority(InInventory),
        TEXT("Request_SplitStack: No authority over inventory [{}].{}"), InInventory, ck::Context(InDelegate.GetFunctionName()))
    { return {}; }

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_Split,
        InRequest.PopulateRequestHandle(InInventory), InDelegate);

    InInventory.AddOrGet<ck::FFragment_Inventory_Requests>()._Requests.Emplace(InRequest);
    return InInventory;
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
    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_HasAuthority(InInventory),
        TEXT("Request_AddItemByDefinition: No authority over inventory [{}].{}"), InInventory, ck::Context(InDelegate.GetFunctionName()))
    { return {}; }

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_AddByDefinition,
        InRequest.PopulateRequestHandle(InInventory), InDelegate);

    InInventory.AddOrGet<ck::FFragment_Inventory_Requests>()._Requests.Emplace(InRequest);
    return InInventory;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Request_TransferItem(
        FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_TransferItem& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Transfer& InDelegate)
    -> FCk_Handle_Inventory
{
    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_HasAuthority(InInventory),
        TEXT("Request_TransferItem: No authority over inventory [{}].{}"), InInventory, ck::Context(InDelegate.GetFunctionName()))
    { return {}; }

    CK_SIGNAL_BIND_REQUEST_FULFILLED(ck::UUtils_Signal_Inventory_OnOperationResult_Transfer,
        InRequest.PopulateRequestHandle(InInventory), InDelegate);

    InInventory.AddOrGet<ck::FFragment_Inventory_Requests>()._Requests.Emplace(InRequest);
    return InInventory;
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
    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_HasAuthority(InInventory),
        TEXT("No authority on inventory [{}]"), InInventory)
    { return InInventory; }

    auto Request = InRequest;

    CK_SIGNAL_BIND_REQUEST_FULFILLED(
        ck::UUtils_Signal_Inventory_OnOperationResult_Sort,
        Request.PopulateRequestHandle(InInventory),
        InDelegate);

    auto& Requests = InInventory.AddOrGet<ck::FFragment_Inventory_Requests>();
    Requests._Requests.Add(MoveTemp(Request));

    return InInventory;
}

auto
    UCk_Utils_Inventory_UE::
    Request_RelocateItem(
        FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_RelocateItem& InRequest,
        const FCk_Delegate_Inventory_OnOperationResult_Relocate& InDelegate)
    -> FCk_Handle_Inventory
{
    CK_ENSURE_IF_NOT(UCk_Utils_Net_UE::Get_HasAuthority(InInventory),
        TEXT("No authority on inventory [{}]"), InInventory)
    { return InInventory; }

    auto Request = InRequest;

    CK_SIGNAL_BIND_REQUEST_FULFILLED(
        ck::UUtils_Signal_Inventory_OnOperationResult_Relocate,
        Request.PopulateRequestHandle(InInventory),
        InDelegate);

    auto& Requests = InInventory.AddOrGet<ck::FFragment_Inventory_Requests>();
    Requests._Requests.Add(MoveTemp(Request));

    return InInventory;
}

// --------------------------------------------------------------------------------------------------------------------
// Signals
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
// FMemberReference Resolution
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
// Internal
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

// ============================================================================
// Spatial Inventory Utils
// ============================================================================

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(
    UCk_Utils_Inventory_Spatial_UE,
    FCk_Handle_Inventory_Spatial,
    ck::FTag_Inventory_Spatial);

// --------------------------------------------------------------------------------------------------------------------
// Public typed overloads (delegate to internal FCk_Handle_Inventory overloads)
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_Spatial_UE::
    Get_Dimensions(
        const FCk_Handle_Inventory_Spatial& InInventory)
    -> FIntPoint
{
    const auto GridHandle = Get_Grid(InInventory);
    return UCk_Utils_2dGridSystem_UE::Get_Dimensions(GridHandle);
}

auto
    UCk_Utils_Inventory_Spatial_UE::
    Get_NumFreeCells(
        const FCk_Handle_Inventory_Spatial& InInventory)
    -> int32
{
    const auto GridHandle = Get_Grid(InInventory);

    auto FreeCount = 0;

    UCk_Utils_2dGridSystem_UE::ForEach_Cell(GridHandle, ECk_2dGridSystem_CellFilter::OnlyActiveCells,
        [&](const FCk_Handle_2dGridCell& InCell)
    {
        if (ck::Is_NOT_Valid(ck::TUtils_InventorySlot_ItemRef::Get_StoredEntity(InCell)))
        { ++FreeCount; }
    });

    return FreeCount;
}

auto
    UCk_Utils_Inventory_Spatial_UE::
    Get_ItemPlacementRotation(
        const FCk_Handle_Item& InItem)
    -> ECk_CardinalRotation
{
    auto TransformHandle = UCk_Utils_Transform_UE::Cast(InItem);

    if (ck::Is_NOT_Valid(TransformHandle))
    { return ECk_CardinalRotation::None; }

    const auto Rotation = UCk_Utils_Transform_UE::Get_EntityCurrentRotation(TransformHandle);
    return ck_inventory::YawToCardinalRotation(Rotation.Yaw);
}

auto
    UCk_Utils_Inventory_Spatial_UE::
    Get_ItemActiveCells_Rotated(
        const FCk_Handle_Item& InItem,
        ECk_CardinalRotation InRotation)
    -> TArray<FIntPoint>
{
    auto BaseCells = ck_inventory::Get_ItemActiveCells(InItem);
    return UCk_Utils_Grid2D_UE::Get_RotatedShape(BaseCells, InRotation);
}

auto
    UCk_Utils_Inventory_Spatial_UE::
    Get_ItemPlacementCoordinate(
        const FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Handle_Item& InItem)
    -> FIntPoint
{
    auto Coordinate = ck::Inventory::AutoPlaceCoordinate;

    const auto& GridHandle = Get_Grid(InInventory);

    UCk_Utils_2dGridSystem_UE::ForEach_Cell(GridHandle, ECk_2dGridSystem_CellFilter::OnlyActiveCells,
        [&](const FCk_Handle_2dGridCell& InCell)
    {
        if (const auto& StoredEntity = ck::TUtils_InventorySlot_ItemRef::Get_StoredEntity(InCell);
            StoredEntity != InItem)
        { return; }

        const auto Local = UCk_Utils_2dGridCell_UE::Get_Coordinate(InCell, ECk_2dGridSystem_CoordinateType::Local);

        // Multi-cell items mark every occupied cell with the same item ref. ForEach_Cell
        // iteration order is not guaranteed to hit the anchor first, so reduce to the
        // lexicographic minimum (Y then X) — which equals the placement anchor because
        // Get_RotatedShape normalizes rotated shapes so MinX=MinY=0.
        if (Coordinate.X < 0 ||
            Local.Y < Coordinate.Y ||
            (Local.Y == Coordinate.Y && Local.X < Coordinate.X))
        {
            Coordinate = Local;
        }
    });

    return Coordinate;
}

auto
    UCk_Utils_Inventory_Spatial_UE::
    Get_ItemAtCoordinate(
        const FCk_Handle_Inventory_Spatial& InInventory,
        const FIntPoint& InCoordinate)
    -> FCk_Handle_Item
{
    const auto& GridHandle = Get_Grid(InInventory);
    const auto& CellHandle = UCk_Utils_2dGridSystem_UE::Get_CellAt(GridHandle, InCoordinate);
    return ck::TUtils_InventorySlot_ItemRef::Get_StoredEntity(CellHandle);
}

// --------------------------------------------------------------------------------------------------------------------
// Internal spatial helpers (used by processors via friend access)
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_Spatial_UE::
    Get_Grid(
        const FCk_Handle_Inventory_Spatial& InInventory)
    -> FCk_Handle_2dGridSystem
{
    return UCk_Utils_2dGridSystem_UE::CastChecked(InInventory);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_Spatial_UE::
    DoCanPlaceItemAt(
        const FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Handle_Item& InItem,
        const FIntPoint& InCoordinate,
        ECk_CardinalRotation InRotation)
    -> bool
{
    if (NOT UCk_Utils_Inventory_UE::Get_IsSpatial(InInventory))
    { return false; }

    const auto& GridHandle = Get_Grid(InInventory);
    const auto RotatedCells = Get_ItemActiveCells_Rotated(InItem, InRotation);

    // ---- Check bounds and disabled via grid-level utility ----

    if (NOT UCk_Utils_2dGridSystem_UE::Get_CanFitShapeAt(GridHandle, RotatedCells, InCoordinate))
    { return false; }

    // ---- Check occupancy (inventory layer) ----

    return ck::algo::NoneOf(RotatedCells, [&](const FIntPoint& CellOffset)
    {
        const auto& Coord = InCoordinate + CellOffset;
        const auto& CellHandle = UCk_Utils_2dGridSystem_UE::Get_CellAt(GridHandle, Coord);
        return ck::IsValid(ck::TUtils_InventorySlot_ItemRef::Get_StoredEntity(CellHandle));
    });
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_inventory
{
    static constexpr ECk_CardinalRotation AllRotations[] =
    {
        ECk_CardinalRotation::None,
        ECk_CardinalRotation::Quarter,
        ECk_CardinalRotation::Half,
        ECk_CardinalRotation::ThreeQuarter
    };
}

auto
    UCk_Utils_Inventory_Spatial_UE::
    Get_CanPlaceItemAt(
        const FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Handle_Item& InItem,
        const FIntPoint& InCoordinate)
    -> FCk_SpatialPlacementResult
{
    for (const auto Rotation : ck_inventory::AllRotations)
    {
        if (DoCanPlaceItemAt(InInventory, InItem, InCoordinate, Rotation))
        { return FCk_SpatialPlacementResult::Success(InCoordinate, Rotation); }
    }

    return FCk_SpatialPlacementResult::Failed();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_Spatial_UE::
    Get_FirstAvailablePlacement(
        const FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Handle_Item& InItem)
    -> FCk_SpatialPlacementResult
{
    if (NOT UCk_Utils_Inventory_UE::Get_IsSpatial(InInventory))
    { return FCk_SpatialPlacementResult::Failed(); }

    const auto& GridHandle = Get_Grid(InInventory);
    const auto& GridDimensions = UCk_Utils_2dGridSystem_UE::Get_Dimensions(GridHandle);

    for (const auto Rotation : ck_inventory::AllRotations)
    {
        for (auto Y = 0; Y < GridDimensions.Y; ++Y)
        {
            for (auto X = 0; X < GridDimensions.X; ++X)
            {
                if (DoCanPlaceItemAt(InInventory, InItem, FIntPoint{X, Y}, Rotation))
                { return FCk_SpatialPlacementResult::Success(FIntPoint{X, Y}, Rotation); }
            }
        }
    }

    return FCk_SpatialPlacementResult::Failed();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_Spatial_UE::
    Request_PlaceItemOnGrid(
        FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Handle_Item& InItem,
        const FIntPoint& InCoordinate,
        ECk_CardinalRotation InRotation)
    -> void
{
    const auto& GridHandle = Get_Grid(InInventory);
    const auto& RotatedCells = Get_ItemActiveCells_Rotated(InItem, InRotation);

    ck::algo::ForEach(RotatedCells, [&](const FIntPoint& CellOffset)
    {
        const auto& Coord = InCoordinate + CellOffset;

        if (auto CellHandle = UCk_Utils_2dGridSystem_UE::Get_CellAt(GridHandle, Coord);
            ck::IsValid(CellHandle))
        {
            ck::TUtils_InventorySlot_ItemRef::AddOrReplace(CellHandle, InItem);
        }
    });

    // ---- Store rotation on item's Transform ----

    if (auto TransformHandle = UCk_Utils_Transform_UE::Cast(InItem);
        ck::IsValid(TransformHandle))
    {
        const auto Yaw = ck_inventory::CardinalRotationToYaw(InRotation);
        UCk_Utils_Transform_UE::Request_SetRotation(TransformHandle, FCk_Request_Transform_SetRotation{FRotator{0.0, Yaw, 0.0}});
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_Spatial_UE::
    Request_RemoveItemFromGrid(
        FCk_Handle_Inventory_Spatial& InInventory,
        const FCk_Handle_Item& InItem)
    -> void
{
    const auto& GridHandle = Get_Grid(InInventory);

    UCk_Utils_2dGridSystem_UE::ForEach_Cell(GridHandle, ECk_2dGridSystem_CellFilter::OnlyActiveCells,
        [&InItem](FCk_Handle_2dGridCell InCell)
    {
        if (const auto& StoredEntity = ck::TUtils_InventorySlot_ItemRef::Get_StoredEntity(InCell);
            StoredEntity == InItem)
        {
            ck::TUtils_InventorySlot_ItemRef::Clear(InCell);
        }
    });

    // ---- Reset rotation on item's Transform ----

    if (auto TransformHandle = UCk_Utils_Transform_UE::Cast(InItem);
        ck::IsValid(TransformHandle))
    {
        UCk_Utils_Transform_UE::Request_SetRotation(TransformHandle, FCk_Request_Transform_SetRotation{FRotator::ZeroRotator});
    }
}

// ============================================================================
// DataOnly Inventory Utils
// ============================================================================

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(
    UCk_Utils_Inventory_DataOnly_UE,
    FCk_Handle_Inventory_DataOnly,
    ck::FTag_Inventory_DataOnly);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_DataOnly_UE::
    Get_BoundMax(
        const FCk_Handle_Inventory_DataOnly& InInventory)
    -> TOptional<int32>
{
    auto BoundAttr = UCk_Utils_IntegerAttribute_UE::TryGet(InInventory, TAG_IntegerAttribute_InventoryBoundMax);

    if (ck::Is_NOT_Valid(BoundAttr))
    { return {}; }

    const auto Value = UCk_Utils_IntegerAttribute_UE::Get_FinalValue(BoundAttr, ECk_MinMaxCurrent::Current);

    if (Value == ck::Inventory::UnboundedBoundLimit)
    { return {}; }

    return Value;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_DataOnly_UE::
    Get_BoundMax_BP(
        const FCk_Handle_Inventory_DataOnly& InInventory,
        bool& OutIsBounded)
    -> int32
{
    const auto Result = Get_BoundMax(InInventory);
    OutIsBounded = Result.IsSet();
    return Result.IsSet() ? Result.GetValue() : 0;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_DataOnly_UE::
    Get_RemainingSlots(
        const FCk_Handle_Inventory_DataOnly& InInventory)
    -> int32
{
    const auto BoundMax = Get_BoundMax(InInventory);
    if (NOT BoundMax.IsSet())
    { return TNumericLimits<int32>::Max(); }

    return FMath::Max(0, BoundMax.GetValue() - UCk_Utils_Inventory_UE::Get_NumItems(InInventory));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_DataOnly_UE::
    Request_OverrideBounds(
        FCk_Handle_Inventory_DataOnly& InInventory,
        int32 InNewBoundMax)
    -> FCk_Handle_Inventory_DataOnly
{
    auto BoundAttr = UCk_Utils_IntegerAttribute_UE::TryGet(InInventory, TAG_IntegerAttribute_InventoryBoundMax);

    CK_ENSURE_IF_NOT(ck::IsValid(BoundAttr),
        TEXT("Inventory [{}] does not have a bound max attribute"), InInventory)
    { return InInventory; }

    UCk_Utils_IntegerAttribute_UE::Request_Override(BoundAttr, InNewBoundMax, ECk_MinMaxCurrent::Current);

    return InInventory;
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
        const FCk_Request_ItemResolution_BestTransferTarget& InRequest)
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
