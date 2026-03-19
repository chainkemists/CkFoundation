#include "CkInventorySlot_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkInventory/CkInventory_Log.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(
    UCk_Utils_InventorySlot_UE,
    FCk_Handle_InventorySlot,
    ck::FFragment_InventorySlot_ItemRef);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InventorySlot_UE::
    Get_Item(
        const FCk_Handle_InventorySlot& InSlot)
    -> FCk_Handle_Item
{
    return ck::TUtils_InventorySlot_ItemRef::Get_StoredEntity_AsTypeSafe<FCk_Handle_Item>(InSlot);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InventorySlot_UE::
    Get_IsOccupied(
        const FCk_Handle_InventorySlot& InSlot)
    -> bool
{
    auto StoredEntity = ck::TUtils_InventorySlot_ItemRef::Get_StoredEntity(InSlot);
    return ck::IsValid(StoredEntity);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InventorySlot_UE::
    Get_Coordinate(
        const FCk_Handle_InventorySlot& InSlot)
    -> FIntPoint
{
    auto CellHandle = ck::StaticCast<FCk_Handle_2dGridCell>(static_cast<const FCk_Handle&>(InSlot));
    return UCk_Utils_2dGridCell_UE::Get_Coordinate(CellHandle, ECk_2dGridSystem_CoordinateType::Local);
}

// --------------------------------------------------------------------------------------------------------------------
