#include "CkInventory_DataOnly_Utils.h"

#include "CkInventory/Inventory/CkInventory_Fragment.h"
#include "CkInventory/Inventory/CkInventory_Utils.h"
#include "CkInventory/Inventory/DataOnly/CkInventory_DataOnly_RequestTraits.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Utils.h"

#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_DataOnly_UE::
    Make_Params(
        FGameplayTag InName,
        const FCk_Delegate_Inventory_CustomCanAcceptItem_Dynamic& InCustomCanAcceptItem,
        const FCk_Delegate_Inventory_CustomCanStackItems_Dynamic& InCustomCanStackItems)
    -> FCk_Fragment_Inventory_DataOnly_ParamsData
{
    auto Params = FCk_Fragment_Inventory_DataOnly_ParamsData(InName);
    Params.Set_CustomCanAcceptItemDynamic(InCustomCanAcceptItem);
    Params.Set_CustomCanStackItemsDynamic(InCustomCanStackItems);
    return Params;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_DataOnly_UE::
    Make_Params_Bounded(
        FGameplayTag InName,
        int32 InBoundLimit,
        const FCk_Delegate_Inventory_CustomCanAcceptItem_Dynamic& InCustomCanAcceptItem,
        const FCk_Delegate_Inventory_CustomCanStackItems_Dynamic& InCustomCanStackItems)
    -> FCk_Fragment_Inventory_DataOnly_ParamsData
{
    auto Params = FCk_Fragment_Inventory_DataOnly_ParamsData(InName, InBoundLimit);
    Params.Set_CustomCanAcceptItemDynamic(InCustomCanAcceptItem);
    Params.Set_CustomCanStackItemsDynamic(InCustomCanStackItems);
    return Params;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_DataOnly_UE::
    Add(
        FCk_Handle& InOwnerEntity,
        const FCk_Fragment_Inventory_DataOnly_ParamsData& InParams,
        ECk_Replication InReplicates,
        const UObject* InWorldContextObject)
    -> FCk_Handle_Inventory_DataOnly
{
    auto Base = ck::CreateInventory<ck::TInventoryRequestTraits<FCk_Handle_Inventory_DataOnly>>(
        InOwnerEntity, FCk_Fragment_Inventory_ParamsData{InParams}, InReplicates, InWorldContextObject);

    // Auto-name the new inventory entity from the Params' name tag so the ECS
    // Debugger shows a meaningful identifier instead of NONAME. Callers that
    // need a different debug name can still call Set_DebugName afterward
    // (default Override mode replaces this).
    UCk_Utils_Handle_UE::Set_DebugName(Base, InParams.Get_Name().GetTagName());

    return Cast(Base);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Inventory_DataOnly_UE::
    AddMultiple(
        FCk_Handle& InOwnerEntity,
        const FCk_Fragment_MultipleInventory_DataOnly_ParamsData& InParams,
        ECk_Replication InReplicates,
        const UObject* InWorldContextObject)
    -> TArray<FCk_Handle_Inventory_DataOnly>
{
    return ck::algo::Transform<TArray<FCk_Handle_Inventory_DataOnly>>(
        InParams.Get_InventoryParams(),
        [&](const FCk_Fragment_Inventory_DataOnly_ParamsData& InParam)
    {
        return Add(InOwnerEntity, InParam, InReplicates, InWorldContextObject);
    });
}

// --------------------------------------------------------------------------------------------------------------------

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
    Get_BoundsInfo(
        const FCk_Handle_Inventory_DataOnly& InInventory)
    -> FCk_Inventory_DataOnly_BoundsInfo
{
    const auto Maybe = Get_BoundMax(InInventory);
    return Maybe.IsSet() ? FCk_Inventory_DataOnly_BoundsInfo{Maybe.GetValue()}
                         : FCk_Inventory_DataOnly_BoundsInfo{};
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
