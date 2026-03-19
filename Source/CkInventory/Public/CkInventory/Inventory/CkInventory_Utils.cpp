#include "CkInventory_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkInventory/CkInventory_Log.h"

#include "CkInventory/Item/CkInventoryItem_Fragment.h"
#include "CkInventory/Item/CkInventoryItem_Utils.h"
#include "CkInventory/InventorySlot/CkInventorySlot_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkLabel/CkLabel_Utils.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Utils.h"
#include "CkGrid/2dGridSystem/Cell/Ck2dGridCell_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(
    UCk_Utils_Inventory_UE,
    FCk_Handle_Inventory,
    ck::FFragment_Inventory_Params);

// --------------------------------------------------------------------------------------------------------------------
// Creation
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Add(
        FCk_Handle& InOwnerEntity,
        const FCk_Fragment_Inventory_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_Inventory
{
    auto NewInventoryEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_Inventory>(InOwnerEntity);

    NewInventoryEntity.Add<ck::FFragment_Inventory_Params>(InParams);
    UCk_Utils_GameplayLabel_UE::Add(NewInventoryEntity, InParams.Get_Name());

    // Add inventory type tag
    switch (InParams.Get_InventoryType())
    {
        case ECk_InventoryType::DataOnly:
        {
            NewInventoryEntity.Add<ck::FTag_Inventory_DataOnly>();
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

            UCk_Utils_2dGridSystem_UE::Add(TransformHandle, GridParams);
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
    Get_InventoryType(
        const FCk_Handle_Inventory& InInventory)
    -> ECk_InventoryType
{
    const auto& Params = InInventory.Get<ck::FFragment_Inventory_Params>();
    return Params.Get_Params().Get_InventoryType();
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
    Get_Grid(
        const FCk_Handle_Inventory& InInventory)
    -> FCk_Handle_2dGridSystem
{
    if (NOT Get_IsSpatial(InInventory))
    { return {}; }

    return UCk_Utils_2dGridSystem_UE::CastChecked(InInventory);
}

// --------------------------------------------------------------------------------------------------------------------
// Requests
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Request_AddItem(
        FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_AddItem& InRequest)
    -> FCk_Handle_Inventory
{
    InInventory.AddOrGet<ck::FFragment_Inventory_Requests>()._Requests.Emplace(InRequest);
    return InInventory;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Request_RemoveItem(
        FCk_Handle_Inventory& InInventory,
        const FCk_Request_Inventory_RemoveItem& InRequest)
    -> FCk_Handle_Inventory
{
    InInventory.AddOrGet<ck::FFragment_Inventory_Requests>()._Requests.Emplace(InRequest);
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
// Internal
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Request_ItemsUpdated(
        FCk_Handle_Inventory& InInventory)
    -> void
{
    // Currently a no-op; signal is fired by the processor after processing requests
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

// --------------------------------------------------------------------------------------------------------------------
// Spatial Helpers
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Get_CanPlaceItemAt(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem,
        const FIntPoint& InCoordinate)
    -> bool
{
    if (NOT Get_IsSpatial(InInventory))
    { return false; }

    auto GridHandle = Get_Grid(InInventory);
    if (ck::Is_NOT_Valid(GridHandle))
    { return false; }

    const auto ItemDimensions = UCk_Utils_InventoryItem_UE::Get_Dimensions(InItem);
    const auto GridDimensions = UCk_Utils_2dGridSystem_UE::Get_Dimensions(GridHandle);

    for (int32 dy = 0; dy < ItemDimensions.Y; ++dy)
    {
        for (int32 dx = 0; dx < ItemDimensions.X; ++dx)
        {
            const auto Coord = FIntPoint(InCoordinate.X + dx, InCoordinate.Y + dy);

            // Check bounds
            if (Coord.X < 0 || Coord.Y < 0 || Coord.X >= GridDimensions.X || Coord.Y >= GridDimensions.Y)
            { return false; }

            auto CellHandle = UCk_Utils_2dGridSystem_UE::Get_CellAt(GridHandle, Coord);
            if (ck::Is_NOT_Valid(CellHandle))
            { return false; }

            // Check if cell is disabled
            if (UCk_Utils_2dGridCell_UE::Get_IsDisabled(CellHandle))
            { return false; }

            // Check if cell is already occupied
            if (ck::TUtils_InventorySlot_ItemRef::Has(CellHandle))
            {
                auto StoredEntity = ck::TUtils_InventorySlot_ItemRef::Get_StoredEntity(CellHandle);
                if (ck::IsValid(StoredEntity))
                { return false; }
            }
        }
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    Get_FindFirstAvailablePlacement(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem)
    -> FIntPoint
{
    if (NOT Get_IsSpatial(InInventory))
    { return FIntPoint(-1, -1); }

    auto GridHandle = Get_Grid(InInventory);
    if (ck::Is_NOT_Valid(GridHandle))
    { return FIntPoint(-1, -1); }

    const auto GridDimensions = UCk_Utils_2dGridSystem_UE::Get_Dimensions(GridHandle);

    for (int32 y = 0; y < GridDimensions.Y; ++y)
    {
        for (int32 x = 0; x < GridDimensions.X; ++x)
        {
            if (Get_CanPlaceItemAt(InInventory, InItem, FIntPoint(x, y)))
            { return FIntPoint(x, y); }
        }
    }

    return FIntPoint(-1, -1);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    DoPlaceItemOnGrid(
        FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem,
        const FIntPoint& InCoordinate)
    -> void
{
    auto GridHandle = Get_Grid(InInventory);
    if (ck::Is_NOT_Valid(GridHandle))
    { return; }

    const auto ItemDimensions = UCk_Utils_InventoryItem_UE::Get_Dimensions(InItem);

    for (int32 dy = 0; dy < ItemDimensions.Y; ++dy)
    {
        for (int32 dx = 0; dx < ItemDimensions.X; ++dx)
        {
            const auto Coord = FIntPoint(InCoordinate.X + dx, InCoordinate.Y + dy);
            auto CellHandle = UCk_Utils_2dGridSystem_UE::Get_CellAt(GridHandle, Coord);

            if (ck::Is_NOT_Valid(CellHandle))
            { continue; }

            ck::TUtils_InventorySlot_ItemRef::AddOrReplace(CellHandle, InItem);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_UE::
    DoRemoveItemFromGrid(
        FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem)
    -> void
{
    auto GridHandle = Get_Grid(InInventory);
    if (ck::Is_NOT_Valid(GridHandle))
    { return; }

    UCk_Utils_2dGridSystem_UE::ForEach_Cell(GridHandle, ECk_2dGridSystem_CellFilter::OnlyActiveCells,
        [&InItem](FCk_Handle_2dGridCell InCell)
    {
        if (NOT ck::TUtils_InventorySlot_ItemRef::Has(InCell))
        { return; }

        auto StoredEntity = ck::TUtils_InventorySlot_ItemRef::Get_StoredEntity(InCell);
        if (StoredEntity == InItem)
        {
            InCell.Remove<ck::FFragment_InventorySlot_ItemRef>();
        }
    });
}

// --------------------------------------------------------------------------------------------------------------------
