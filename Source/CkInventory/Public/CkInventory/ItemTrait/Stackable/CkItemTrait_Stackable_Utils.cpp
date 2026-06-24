#include "CkItemTrait_Stackable_Utils.h"

#include "CkInventory/Item/CkItem_Definition.h"
#include "CkInventory/Item/CkItem_Utils.h"
#include "CkInventory/ItemTrait/CkItemTrait.inl.h"
#include "CkInventory/ItemTrait/Stackable/CkItemTrait_Stackable.h"
#include "CkInventory/Inventory/CkInventory_Utils.h"

#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemTrait_Stackable_UE::
    Get_IsStackable(
        const FCk_Handle_Item& InItem)
    -> bool
{
    return UCk_ItemTrait::Has<UCk_ItemTrait_Stackable>(InItem);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemTrait_Stackable_UE::
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
    UCk_Utils_ItemTrait_Stackable_UE::
    Get_MaxStackSize(
        const FCk_Handle_Item& InItem)
    -> int32
{
    const auto* Trait = UCk_ItemTrait::Get<UCk_ItemTrait_Stackable>(InItem);

    if (ck::Is_NOT_Valid(Trait, ck::IsValid_Policy_NullptrOnly{}))
    { return 0; }

    if (NOT Trait->Get_HasMaxStackSize())
    { return -1; }

    return Trait->Get_MaxStackSize();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemTrait_Stackable_UE::
    Get_HasMaxStackSize(
        const FCk_Handle_Item& InItem)
    -> bool
{
    const auto* Trait = UCk_ItemTrait::Get<UCk_ItemTrait_Stackable>(InItem);

    if (ck::Is_NOT_Valid(Trait, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    return Trait->Get_HasMaxStackSize();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemTrait_Stackable_UE::
    Get_IsStackFull(
        const FCk_Handle_Item& InItem)
    -> bool
{
    const auto* Trait = UCk_ItemTrait::Get<UCk_ItemTrait_Stackable>(InItem);

    if (ck::Is_NOT_Valid(Trait, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    if (NOT Trait->Get_HasMaxStackSize())
    { return false; }

    return Get_StackCount(InItem) >= Trait->Get_MaxStackSize();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemTrait_Stackable_UE::
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

namespace ck_stackable_details
{
    // Applies the inventory's StackingPolicy to a definition-level max (MAX_int32 = uncapped).
    static auto
    ApplyStackingPolicy(
        const FCk_Handle_Inventory& InInventory,
        int32 InDefinitionMax) -> int32
    {
        if (ck::Is_NOT_Valid(InInventory))
        { return InDefinitionMax; }

        const auto& Params = InInventory.Get<ck::FFragment_Inventory_Params>();

        switch (Params.Get_StackingPolicy())
        {
            case ECk_Inventory_StackingPolicy::ClampMaxStackSize:
            { return FMath::Min(InDefinitionMax, FMath::Max(1, Params.Get_MaxStackSizeClamp())); }
            case ECk_Inventory_StackingPolicy::NoStacking:
            { return 1; }
            case ECk_Inventory_StackingPolicy::UseItemDefinition:
            default:
            { return InDefinitionMax; }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemTrait_Stackable_UE::
    Get_EffectiveMaxStackSize(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem)
    -> int32
{
    if (NOT Get_IsStackable(InItem))
    { return 1; }

    const auto DefinitionMax = Get_HasMaxStackSize(InItem) ? Get_MaxStackSize(InItem) : MAX_int32;
    return ck_stackable_details::ApplyStackingPolicy(InInventory, DefinitionMax);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemTrait_Stackable_UE::
    Get_EffectiveMaxStackSize_ByDefinition(
        const FCk_Handle_Inventory& InInventory,
        const UCk_InventoryItem_Definition* InDefinition)
    -> int32
{
    if (ck::Is_NOT_Valid(InDefinition, ck::IsValid_Policy_NullptrOnly{}))
    { return 1; }

    const auto* Trait = InDefinition->Get_ItemTrait<UCk_ItemTrait_Stackable>();

    if (ck::Is_NOT_Valid(Trait, ck::IsValid_Policy_NullptrOnly{}))
    { return 1; }

    const auto DefinitionMax = Trait->Get_HasMaxStackSize() ? Trait->Get_MaxStackSize() : MAX_int32;
    return ck_stackable_details::ApplyStackingPolicy(InInventory, DefinitionMax);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemTrait_Stackable_UE::
    Get_RemainingStackCapacity_InInventory(
        const FCk_Handle_Inventory& InInventory,
        const FCk_Handle_Item& InItem)
    -> int32
{
    if (NOT Get_IsStackable(InItem))
    { return 0; }

    const auto EffectiveMax = Get_EffectiveMaxStackSize(InInventory, InItem);

    if (EffectiveMax == MAX_int32)
    { return MAX_int32; }

    return FMath::Max(0, EffectiveMax - Get_StackCount(InItem));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemTrait_Stackable_UE::
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

    const auto* Definition = UCk_Utils_Item_UE::Get_Definition(InSourceItem);

    if (Definition != UCk_Utils_Item_UE::Get_Definition(InTargetItem))
    { return ECk_Inventory_OperationResult_Stack::Failed_DefinitionMismatch; }

    if (NOT Definition->CanStackWith(InSourceItem, InTargetItem))
    { return ECk_Inventory_OperationResult_Stack::Failed_IncompatibleFragments; }

    if (NOT Get_PassesCustomStackValidation(InInventory, InSourceItem, InTargetItem))
    { return ECk_Inventory_OperationResult_Stack::Failed_RejectedByCustomStackLogic; }

    if (Get_IsStackFull(InTargetItem))
    { return ECk_Inventory_OperationResult_Stack::Failed_TargetStackFull; }

    // The inventory's StackingPolicy may cap stacks tighter than the item definition does.
    // Only checkable with a valid inventory (callers that pass an invalid inventory to skip the
    // containment check — e.g. Get_StackRoomFor — apply the clamp themselves).
    if (ck::IsValid(InInventory)
        && Get_StackCount(InTargetItem) >= Get_EffectiveMaxStackSize(InInventory, InTargetItem))
    { return ECk_Inventory_OperationResult_Stack::Failed_TargetStackFull; }

    return ECk_Inventory_OperationResult_Stack::Success;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemTrait_Stackable_UE::
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
    UCk_Utils_ItemTrait_Stackable_UE::
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
    UCk_Utils_ItemTrait_Stackable_UE::
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
    UCk_Utils_ItemTrait_Stackable_UE::
    Request_AdjustStackCount(
        const FCk_Handle_Item& InItem,
        int32 InDelta)
    -> void
{
    if (InDelta == 0)
    { return; }

    auto Attr = UCk_Utils_IntegerAttribute_UE::TryGet(InItem, TAG_IntegerAttribute_InventoryItem_StackCount);
    UCk_Utils_IntegerAttributeModifier_UE::Add_NotRevocable(
        Attr,
        ECk_AttributeModifier_Operation::Add,
        FCk_Fragment_IntegerAttributeModifier_ParamsData{InDelta, ECk_MinMaxCurrent::Current});
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemTrait_Stackable_UE::
    Request_FillExistingStacks(
        const FCk_Handle_Inventory& InInventory,
        const UCk_InventoryItem_Definition* InDefinition,
        int32 InCount,
        const FCk_Handle_Item& InSourceItem)
    -> FCk_FillExistingStacksResult
{
    const auto* StackableTrait = InDefinition->Get_ItemTrait<UCk_ItemTrait_Stackable>();

    if (ck::Is_NOT_Valid(StackableTrait, ck::IsValid_Policy_NullptrOnly{}))
    { return {}; }

    auto Filled = int32{0};
    auto Remaining = InCount;
    auto LastFilledItem = FCk_Handle_Item{};

    for (const auto ExistingItems = UCk_Utils_Inventory_UE::Get_Items(InInventory);
        const auto& ExistingItem : ExistingItems)
    {
        if (Remaining <= 0)
        { break; }

        if (UCk_Utils_Item_UE::Get_Definition(ExistingItem) != InDefinition)
        { continue; }

        // Mirror the trait-level stack-compatibility gate that Get_CanStackItems applies on the
        // explicit StackItems path. Same-definition items can still be unstackable when a trait
        // distinguishes runtime state (e.g. the Tags trait — VHS rewound vs NotRewound). Without
        // this, AddByDefinition / Transfer pre-fill silently merges items that a direct StackItems
        // request would reject. InSourceItem is invalid for definition-driven fills (AddByDefinition),
        // which have no runtime source to compare — gate only when it is valid.
        if (ck::IsValid(InSourceItem) && NOT InDefinition->CanStackWith(InSourceItem, ExistingItem))
        { continue; }

        // The inventory's custom stack hooks must also gate this fill path — otherwise a
        // per-inventory stacking restriction is enforced on direct StackItems requests but
        // silently bypassed by AddByDefinition / Transfer pre-fill. InSourceItem may be invalid
        // (definition-driven fills); validators must tolerate that.
        if (NOT Get_PassesCustomStackValidation(InInventory, InSourceItem, ExistingItem))
        { continue; }

        const auto Space = Get_RemainingStackCapacity_InInventory(InInventory, ExistingItem);

        if (Space <= 0)
        { continue; }

        const auto Transfer = FMath::Min(Remaining, Space);

        if (Transfer <= 0)
        { continue; }

        Request_AdjustStackCount(ExistingItem, Transfer);

        Remaining -= Transfer;
        Filled    += Transfer;
        LastFilledItem = ExistingItem;
    }

    return FCk_FillExistingStacksResult{Filled, LastFilledItem};
}

// --------------------------------------------------------------------------------------------------------------------
// Signals
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemTrait_Stackable_UE::
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
    UCk_Utils_ItemTrait_Stackable_UE::
    UnbindFrom_OnStackCountChanged(
        FCk_Handle_Item& InItem,
        const FCk_Delegate_Stackable_OnStackCountChanged& InDelegate)
    -> FCk_Handle_Item
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_Stackable_OnStackCountChanged, InItem, InDelegate);

    return InItem;
}

// --------------------------------------------------------------------------------------------------------------------
