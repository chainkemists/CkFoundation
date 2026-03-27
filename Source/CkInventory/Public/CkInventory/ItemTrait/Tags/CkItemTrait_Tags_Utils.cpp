#include "CkItemTrait_Tags_Utils.h"

#include "CkInventory/Item/CkItem_Definition.h"
#include "CkInventory/Item/CkItem_Utils.h"
#include "CkInventory/ItemTrait/CkItemTrait.inl.h"
#include "CkInventory/ItemTrait/Tags/CkItemTrait_Tags.h"

#include "CkTagSet/CkTagSet_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemTrait_Tags_UE::
    Get_HasTagsFeature(
        const FCk_Handle_Item& InItem)
    -> bool
{
    return UCk_ItemTrait::Has<UCk_ItemTrait_Tags>(InItem);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemTrait_Tags_UE::
    Get_Tags(
        const FCk_Handle_Item& InItem)
    -> FGameplayTagContainer
{
    if (NOT UCk_Utils_TagSet_UE::Has(InItem))
    { return {}; }

    auto TagSetHandle = UCk_Utils_TagSet_UE::CastChecked(InItem);
    return UCk_Utils_TagSet_UE::Get_Tags(TagSetHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemTrait_Tags_UE::
    HasTag(
        const FCk_Handle_Item& InItem,
        FGameplayTag InTag)
    -> bool
{
    if (NOT UCk_Utils_TagSet_UE::Has(InItem))
    { return false; }

    auto TagSetHandle = UCk_Utils_TagSet_UE::CastChecked(InItem);
    return UCk_Utils_TagSet_UE::HasTag(TagSetHandle, InTag);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemTrait_Tags_UE::
    HasTagExact(
        const FCk_Handle_Item& InItem,
        FGameplayTag InTag)
    -> bool
{
    if (NOT UCk_Utils_TagSet_UE::Has(InItem))
    { return false; }

    auto TagSetHandle = UCk_Utils_TagSet_UE::CastChecked(InItem);
    return UCk_Utils_TagSet_UE::HasTagExact(TagSetHandle, InTag);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemTrait_Tags_UE::
    HasAny(
        const FCk_Handle_Item& InItem,
        FGameplayTagContainer InTags)
    -> bool
{
    if (NOT UCk_Utils_TagSet_UE::Has(InItem))
    { return false; }

    auto TagSetHandle = UCk_Utils_TagSet_UE::CastChecked(InItem);
    return UCk_Utils_TagSet_UE::HasAny(TagSetHandle, InTags);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemTrait_Tags_UE::
    HasAll(
        const FCk_Handle_Item& InItem,
        FGameplayTagContainer InTags)
    -> bool
{
    if (NOT UCk_Utils_TagSet_UE::Has(InItem))
    { return false; }

    auto TagSetHandle = UCk_Utils_TagSet_UE::CastChecked(InItem);
    return UCk_Utils_TagSet_UE::HasAll(TagSetHandle, InTags);
}

// --------------------------------------------------------------------------------------------------------------------
// Requests (Authority Only)
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemTrait_Tags_UE::
    Request_AddTags(
        FCk_Handle_Item& InItem,
        const FGameplayTagContainer& InTags)
    -> void
{
    if (NOT UCk_Utils_TagSet_UE::Has(InItem))
    { return; }

    auto TagSetHandle = UCk_Utils_TagSet_UE::CastChecked(InItem);
    UCk_Utils_TagSet_UE::Request_AddTags(TagSetHandle, InTags);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemTrait_Tags_UE::
    Request_AddTag(
        FCk_Handle_Item& InItem,
        FGameplayTag InTag)
    -> void
{
    if (NOT UCk_Utils_TagSet_UE::Has(InItem))
    { return; }

    auto TagSetHandle = UCk_Utils_TagSet_UE::CastChecked(InItem);
    UCk_Utils_TagSet_UE::Request_AddTag(TagSetHandle, InTag);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemTrait_Tags_UE::
    Request_RemoveTags(
        FCk_Handle_Item& InItem,
        const FGameplayTagContainer& InTags)
    -> void
{
    if (NOT UCk_Utils_TagSet_UE::Has(InItem))
    { return; }

    auto TagSetHandle = UCk_Utils_TagSet_UE::CastChecked(InItem);
    UCk_Utils_TagSet_UE::Request_RemoveTags(TagSetHandle, InTags);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemTrait_Tags_UE::
    Request_RemoveTag(
        FCk_Handle_Item& InItem,
        FGameplayTag InTag)
    -> void
{
    if (NOT UCk_Utils_TagSet_UE::Has(InItem))
    { return; }

    auto TagSetHandle = UCk_Utils_TagSet_UE::CastChecked(InItem);
    UCk_Utils_TagSet_UE::Request_RemoveTag(TagSetHandle, InTag);
}

// --------------------------------------------------------------------------------------------------------------------
// Signals
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemTrait_Tags_UE::
    BindTo_OnTagsChanged(
        FCk_Handle_Item& InItem,
        const FCk_Delegate_ItemTags_OnTagsChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_Item
{
    CK_SIGNAL_BIND(ck::UUtils_Signal_ItemTags_OnTagsChanged, InItem, InDelegate, InBindingPolicy, InPostFireBehavior);

    return InItem;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ItemTrait_Tags_UE::
    UnbindFrom_OnTagsChanged(
        FCk_Handle_Item& InItem,
        const FCk_Delegate_ItemTags_OnTagsChanged& InDelegate)
    -> FCk_Handle_Item
{
    CK_SIGNAL_UNBIND(ck::UUtils_Signal_ItemTags_OnTagsChanged, InItem, InDelegate);

    return InItem;
}

// --------------------------------------------------------------------------------------------------------------------
