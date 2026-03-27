#include "CkItem_Utils.h"

#include "CkInventory/Item/CkItem_Definition.h"
#include "CkInventory/ItemTrait/CkItemTrait.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

#include "CkInventory/CkInventory_Log.h"
#include "CkInventory/Inventory/CkInventory_Fragment.h"
#include "CkInventory/Inventory/CkInventory_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(
    UCk_Utils_InventoryItem_UE,
    FCk_Handle_Item,
    ck::FFragment_InventoryItem);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InventoryItem_UE::
    Create(
        FCk_Handle& InOwnerEntity,
        const UCk_InventoryItem_Definition* InDefinition)
    -> FCk_Handle_Item
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOwnerEntity), TEXT("Create: Invalid owner entity"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InDefinition), TEXT("Create: Null item definition"))
    { return {}; }

    auto ConstructionInfo = FCk_EntityReplicationDriver_ConstructionInfo(InDefinition->GetClass());
    ConstructionInfo.Set_ConstructionScriptArchetype(InDefinition);

    auto ItemEntity = UCk_Utils_EntityReplicationDriver_UE::Request_BuildAndReplicate(
        InOwnerEntity,
        ConstructionInfo);

    return Cast(ItemEntity);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InventoryItem_UE::
    Get_Definition(
        const FCk_Handle_Item& InItem)
    -> const UCk_InventoryItem_Definition*
{
    return InItem.Get<ck::FFragment_InventoryItem>().Get_Definition().Get();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_InventoryItem_UE::
    Get_ParentInventory(
        const FCk_Handle_Item& InItem)
    -> FCk_Handle_Inventory
{
    if (NOT ck::TUtils_Item_ParentInventory::Has(InItem))
    { return {}; }

    return UCk_Utils_Inventory_UE::Cast(ck::TUtils_Item_ParentInventory::Get_StoredEntity(InItem));
}

// --------------------------------------------------------------------------------------------------------------------
