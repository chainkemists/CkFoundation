#include "CkInventoryItem_Utils.h"

#include "CkInventory/Item/CkInventoryItem_Definition.h"
#include "CkInventory/Item/CkInventoryItem_ItemFragment.inl.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

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
        UCk_InventoryItem_Definition* InDefinition)
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
    -> UCk_InventoryItem_Definition*
{
    const auto& Params = InItem.Get<ck::FFragment_InventoryItem_Params>();
    return Params.Get_Definition().Get();
}

// --------------------------------------------------------------------------------------------------------------------
