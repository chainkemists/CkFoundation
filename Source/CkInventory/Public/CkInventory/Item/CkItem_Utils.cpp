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
    UCk_Utils_Item_UE,
    FCk_Handle_Item,
    ck::FFragment_InventoryItem);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Item_UE::
    Add(
        FCk_Handle& InHandle,
        const UCk_InventoryItem_Definition* InDefinition)
    -> FCk_Handle_Item
{
    CK_ENSURE_IF_NOT(ck::IsValid(InDefinition), TEXT("Create: Null item definition"))
    { return {}; }

    InDefinition->Construct(InHandle);
    return Cast(InHandle);
}

auto
    UCk_Utils_Item_UE::
    Create(
        FCk_Handle& InOwnerEntity,
        const UCk_InventoryItem_Definition* InDefinition)
    -> FCk_Handle_Item
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOwnerEntity), TEXT("Create: Invalid owner entity"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InDefinition), TEXT("Create: Null item definition"))
    { return {}; }

    auto ConstructionInfo = FCk_EntityReplicationDriver_Spec(InDefinition->GetClass());
    ConstructionInfo.Set_ConstructionScriptArchetype(InDefinition);

    auto ItemEntity = UCk_Utils_EntityReplicationDriver_UE::Request_BuildAndReplicate(
        InOwnerEntity,
        ConstructionInfo);

    return Cast(ItemEntity);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Item_UE::
    GetOrCreate_TransientItemDefinition(
        UObject* InOuter,
        FName InName,
        const FCk_InventoryItem_CoreInfo& InCoreInfo,
        const TArray<UCk_ItemTrait*>& InTraits)
    -> UCk_InventoryItem_Definition*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOuter),
        TEXT("GetOrCreate_TransientItemDefinition: Null outer"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InName),
        TEXT("GetOrCreate_TransientItemDefinition: Name is None — runtime definitions need a deterministic name"))
    { return {}; }

    auto Definition = FindObject<UCk_InventoryItem_Definition>(InOuter, *InName.ToString());

    if (ck::Is_NOT_Valid(Definition))
    {
        const auto ExistingOfOtherClass = FindObject<UObject>(InOuter, *InName.ToString());
        CK_ENSURE_IF_NOT(ck::Is_NOT_Valid(ExistingOfOtherClass),
            TEXT("GetOrCreate_TransientItemDefinition: [{}] already exists under [{}] but is a [{}], not an item definition"),
            InName, InOuter, ExistingOfOtherClass->GetClass())
        { return {}; }

        Definition = NewObject<UCk_InventoryItem_Definition>(InOuter, InName, RF_Transient);
    }

    CK_ENSURE_IF_NOT(NOT Definition->IsAsset(),
        TEXT("GetOrCreate_TransientItemDefinition: [{}] is an on-disk/script asset — refusing to repopulate it"),
        Definition)
    { return {}; }

    Definition->_CoreInfo = InCoreInfo;

    Definition->_ItemTraits.Reset();
    for (const auto& Trait : InTraits)
    {
        CK_ENSURE_IF_NOT(ck::IsValid(Trait),
            TEXT("GetOrCreate_TransientItemDefinition: Null trait passed for [{}]"), InName)
        { continue; }

        if (Trait->GetOuter() != Definition)
        {
            constexpr const TCHAR* KeepExistingName = nullptr;
            Trait->Rename(KeepExistingName, Definition, REN_DontCreateRedirectors | REN_NonTransactional | REN_DoNotDirty);
        }

        Definition->_ItemTraits.Add(Trait);
    }

    return Definition;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Item_UE::
    Get_Definition(
        const FCk_Handle_Item& InItem)
    -> const UCk_InventoryItem_Definition*
{
    return InItem.Get<ck::FFragment_InventoryItem>().Get_Definition().Get();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Item_UE::
    Get_ParentInventory(
        const FCk_Handle_Item& InItem)
    -> FCk_Handle_Inventory
{
    if (NOT ck::TUtils_Item_ParentInventory::Has(InItem))
    { return {}; }

    return ck::TUtils_Item_ParentInventory::Get_StoredEntity(InItem);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Item_UE::
    Has_ItemTrait(
        const FCk_Handle_Item& InItem,
        TSubclassOf<UCk_ItemTrait> InTraitClass)
    -> bool
{
    const auto* Definition = Get_Definition(InItem);

    if (ck::Is_NOT_Valid(Definition))
    { return false; }

    return Definition->Has_ItemTrait(InTraitClass.Get());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Item_UE::
    Get_ItemTrait(
        const FCk_Handle_Item& InItem,
        TSubclassOf<UCk_ItemTrait> InTraitClass)
    -> const UCk_ItemTrait*
{
    const auto* Definition = Get_Definition(InItem);

    if (ck::Is_NOT_Valid(Definition))
    { return nullptr; }

    return Definition->Get_ItemTrait(InTraitClass.Get());
}

// --------------------------------------------------------------------------------------------------------------------
