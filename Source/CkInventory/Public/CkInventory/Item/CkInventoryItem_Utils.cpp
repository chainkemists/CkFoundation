#include "CkInventoryItem_Utils.h"

#include "CkInventory/Item/CkInventoryItem_Definition.h"

#include "CkInventory/CkInventory_Log.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(
    UCk_Utils_InventoryItem_UE,
    FCk_Handle_Item,
    ck::FFragment_InventoryItem_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InventoryItem_UE::
    Create(
        FCk_Handle& InOwnerEntity,
        const TSoftObjectPtr<UCk_InventoryItem_Definition>& InDefinition)
    -> FCk_Handle_Item
{
    if (ck::Is_NOT_Valid(InOwnerEntity))
    {
        ck::inventory::Error(TEXT("Create: Invalid owner entity"));
        return {};
    }

    if (InDefinition.IsNull())
    {
        ck::inventory::Error(TEXT("Create: Null item definition"));
        return {};
    }

    auto ItemEntity = InOwnerEntity.Create();

    ItemEntity.Add<ck::FFragment_InventoryItem_Params>(
        FCk_Fragment_InventoryItem_ParamsData(InDefinition));

    return Cast(ItemEntity);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InventoryItem_UE::
    Get_Definition(
        const FCk_Handle_Item& InItem)
    -> TSoftObjectPtr<UCk_InventoryItem_Definition>
{
    const auto& Params = InItem.Get<ck::FFragment_InventoryItem_Params>();
    return Params.Get_Params().Get_Definition();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InventoryItem_UE::
    Get_Dimensions(
        const FCk_Handle_Item& InItem)
    -> FIntPoint
{
    if (NOT InItem.Has<ck::FFragment_InventoryItem_Dimensions>())
    { return FIntPoint(1, 1); }

    const auto& Dimensions = InItem.Get<ck::FFragment_InventoryItem_Dimensions>();
    return Dimensions.Get_Params().Get_Dimensions();
}

// --------------------------------------------------------------------------------------------------------------------
