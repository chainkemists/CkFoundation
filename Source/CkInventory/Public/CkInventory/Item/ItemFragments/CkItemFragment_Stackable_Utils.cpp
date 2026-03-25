#include "CkItemFragment_Stackable_Utils.h"

#include "CkInventory/Item/CkInventoryItem_Definition.h"
#include "CkInventory/Item/CkInventoryItem_ItemFragment.inl.h"
#include "CkInventory/Item/CkInventoryItem_Utils.h"
#include "CkInventory/Item/ItemFragments/CkItemFragment_Stackable.h"
#include "CkInventory/Inventory/CkInventory_Utils.h"

#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemFragment_Stackable_UE::
    Get_IsStackable(
        const FCk_Handle_Item& InItem)
    -> bool
{
    return FCk_ItemFragment::Has<FCk_ItemFragment_Stackable>(InItem);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemFragment_Stackable_UE::
    Get_StackCount(
        const FCk_Handle_Item& InItem)
    -> int32
{
    const auto Attribute = UCk_Utils_IntegerAttribute_UE::TryGet(
        InItem, TAG_IntegerAttribute_InventoryItem_StackCount);

    if (ck::Is_NOT_Valid(Attribute))
    { return 0; }

    return UCk_Utils_IntegerAttribute_UE::Get_FinalValue(Attribute);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemFragment_Stackable_UE::
    Get_MaxStackSize(
        const FCk_Handle_Item& InItem)
    -> int32
{
    const auto* Fragment = FCk_ItemFragment::Get<FCk_ItemFragment_Stackable>(InItem);

    if (ck::Is_NOT_Valid(Fragment, ck::IsValid_Policy_NullptrOnly{}))
    { return 0; }

    if (NOT Fragment->Get_HasMaxStackSize())
    { return -1; }

    return Fragment->Get_MaxStackSize();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemFragment_Stackable_UE::
    Get_HasMaxStackSize(
        const FCk_Handle_Item& InItem)
    -> bool
{
    const auto* Fragment = FCk_ItemFragment::Get<FCk_ItemFragment_Stackable>(InItem);

    if (ck::Is_NOT_Valid(Fragment, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    return Fragment->Get_HasMaxStackSize();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemFragment_Stackable_UE::
    Get_IsStackFull(
        const FCk_Handle_Item& InItem)
    -> bool
{
    const auto* Fragment = FCk_ItemFragment::Get<FCk_ItemFragment_Stackable>(InItem);

    if (ck::Is_NOT_Valid(Fragment, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    if (NOT Fragment->Get_HasMaxStackSize())
    { return false; }

    return Get_StackCount(InItem) >= Fragment->Get_MaxStackSize();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemFragment_Stackable_UE::
    Get_RemainingStackCapacity(
        const FCk_Handle_Item& InItem)
    -> int32
{
    if (NOT Get_IsStackable(InItem))
    { return 0; }

    if (NOT Get_HasMaxStackSize(InItem))
    { return MAX_int32; }

    return FMath::Max(0, Get_MaxStackSize(InItem) - Get_StackCount(InItem));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemFragment_Stackable_UE::
    Get_CanStackItems(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InSourceItem,
        const FCk_Handle_Item& InTargetItem)
    -> ECk_Inventory_OperationResult_Stack
{
    if (ck::Is_NOT_Valid(InSourceItem))
    { return ECk_Inventory_OperationResult_Stack::Failed_InvalidSourceItem; }

    if (ck::Is_NOT_Valid(InTargetItem))
    { return ECk_Inventory_OperationResult_Stack::Failed_InvalidTargetItem; }

    if (NOT Get_IsStackable(InSourceItem) || NOT Get_IsStackable(InTargetItem))
    { return ECk_Inventory_OperationResult_Stack::Failed_ItemsNotStackable; }

    if (ck::IsValid(InInventory))
    {
        if (NOT FInventoryItemRecordUtils::Get_ContainsEntry(InInventory, InSourceItem))
        { return ECk_Inventory_OperationResult_Stack::Failed_SourceNotInInventory; }

        if (NOT FInventoryItemRecordUtils::Get_ContainsEntry(InInventory, InTargetItem))
        { return ECk_Inventory_OperationResult_Stack::Failed_TargetNotInInventory; }
    }

    const auto* Definition = UCk_Utils_InventoryItem_UE::Get_Definition(InSourceItem);

    if (Definition != UCk_Utils_InventoryItem_UE::Get_Definition(InTargetItem))
    { return ECk_Inventory_OperationResult_Stack::Failed_DefinitionMismatch; }

    if (NOT Definition->CanStackWith(InSourceItem, InTargetItem))
    { return ECk_Inventory_OperationResult_Stack::Failed_IncompatibleFragments; }

    if (NOT Get_PassesCustomStackValidation(InInventory, InSourceItem, InTargetItem))
    { return ECk_Inventory_OperationResult_Stack::Failed_RejectedByCustomStackLogic; }

    if (Get_IsStackFull(InTargetItem))
    { return ECk_Inventory_OperationResult_Stack::Failed_TargetStackFull; }

    return ECk_Inventory_OperationResult_Stack::Success;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemFragment_Stackable_UE::
    Resolve_CanStackItems(
        const FMemberReference& InRef,
        FCk_Handle_Inventory InInventory,
        FCk_Handle_Item InSourceItem,
        FCk_Handle_Item InTargetItem)
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
        FCk_Handle_Item SourceItem;
        FCk_Handle_Item TargetItem;
        bool ReturnValue = false;
    } Args = { MoveTemp(InInventory), MoveTemp(InSourceItem), MoveTemp(InTargetItem) };

    auto* const Context = MemberClass->GetDefaultObject();
    Context->ProcessEvent(Function, &Args);

    return Args.ReturnValue;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemFragment_Stackable_UE::
    Get_PassesCustomStackValidation(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InSourceItem,
        const FCk_Handle_Item& InTargetItem)
    -> bool
{
    if (ck::Is_NOT_Valid(InInventory))
    { return true; }

    const auto& Params = InInventory.Get<ck::FFragment_Inventory_Params>();

    if (const auto& NativeDelegate = Params.Get_CustomCanStackItems();
        NativeDelegate.IsBound())
    {
        if (NOT NativeDelegate.Execute(InInventory, InSourceItem, InTargetItem))
        { return false; }
    }

    if (const auto& DynamicDelegate = Params.Get_CustomCanStackItemsDynamic();
        DynamicDelegate.IsBound())
    {
        auto Result = true;
        DynamicDelegate.ExecuteIfBound(InInventory, InSourceItem, InTargetItem, Result);

        if (NOT Result)
        { return false; }
    }

    if (const auto MemberRefResult = Resolve_CanStackItems(
            Params.Get_CanStackItemsRef(), InInventory, InSourceItem, InTargetItem);
        MemberRefResult.IsSet())
    {
        if (NOT MemberRefResult.GetValue())
        { return false; }
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemFragment_Stackable_UE::
    Request_OverrideStackCount(
        const FCk_Handle_Item& InItem,
        int32 InNewCount)
    -> void
{
    auto Attr = UCk_Utils_IntegerAttribute_UE::TryGet(InItem, TAG_IntegerAttribute_InventoryItem_StackCount);
    UCk_Utils_IntegerAttribute_UE::Request_Override(Attr, InNewCount);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemFragment_Stackable_UE::
    Request_FillExistingStacks(
        const FCk_Handle_Inventory& InInventory,
        const UCk_InventoryItem_Definition* InDefinition,
        int32 InCount)
    -> int32
{
    const auto* StackableFragment = InDefinition->Get_ItemFragment<FCk_ItemFragment_Stackable>();

    if (ck::Is_NOT_Valid(StackableFragment, ck::IsValid_Policy_NullptrOnly{}))
    { return 0; }

    auto Filled = int32{0};
    auto Remaining = InCount;

    for (const auto ExistingItems = UCk_Utils_Inventory_UE::Get_Items(InInventory);
        const auto& ExistingItem : ExistingItems)
    {
        if (Remaining <= 0)
        { break; }

        if (UCk_Utils_InventoryItem_UE::Get_Definition(ExistingItem) != InDefinition)
        { continue; }

        const auto Space = Get_RemainingStackCapacity(ExistingItem);

        if (Space <= 0)
        { continue; }

        const auto Transfer = FMath::Min(Remaining, Space);

        if (Transfer <= 0)
        { continue; }

        Request_OverrideStackCount(ExistingItem, Get_StackCount(ExistingItem) + Transfer);

        Remaining -= Transfer;
        Filled    += Transfer;
    }

    return Filled;
}

// --------------------------------------------------------------------------------------------------------------------
// Signals
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemFragment_Stackable_UE::
    BindTo_OnStackCountChanged(
        FCk_Handle_Item& InItem,
        const FCk_Delegate_Stackable_OnStackCountChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Item
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_Stackable_OnStackCountChanged, InItem, InDelegate, InBindingPolicy, InPostFireBehavior);

    return InItem;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemFragment_Stackable_UE::
    UnbindFrom_OnStackCountChanged(
        FCk_Handle_Item& InItem,
        const FCk_Delegate_Stackable_OnStackCountChanged& InDelegate)
    -> FCk_Handle_Item
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_Stackable_OnStackCountChanged, InItem, InDelegate);

    return InItem;
}

// --------------------------------------------------------------------------------------------------------------------
