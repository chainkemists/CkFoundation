#include "CkItemTrait_Tags.h"

#include "CkInventory/ItemTrait/Tags/CkItemTrait_Tags_Utils.h"
#include "CkInventory/Item/CkItem_Utils.h"

#include "CkCore/Payload/CkPayload.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkTagSet/CkTagSet_Utils.h"
#include "CkTagSet/CkTagSet_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

static auto
DoRelayTagSetChangeToItemSignal(
    FCk_Handle_TagSet InTagSet,
    FGameplayTagContainer InTagsAdded,
    FGameplayTagContainer InTagsRemoved) -> void
{
    auto ItemHandle = UCk_Utils_Item_UE::CastChecked(InTagSet);
    ck::UUtils_Signal_ItemTags_OnTagsChanged::Broadcast(
        ItemHandle,
        ck::MakePayload(ItemHandle, FCk_Payload_Item_OnTagsChanged(ItemHandle, InTagsAdded, InTagsRemoved)));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ItemTrait_Tags::
    DoConstruct_Implementation(
        FCk_Handle& InHandle) const
    -> void
{
    auto ItemHandle = UCk_Utils_Item_UE::CastChecked(InHandle);

    auto TagSetHandle = UCk_Utils_TagSet_UE::Add(ItemHandle, _Tags);

    // ---- Connect relay: TagSet OnTagsChanged -> Item OnTagsChanged ----

    std::ignore = ck::UUtils_Signal_TagSet_OnTagsChanged::Bind<
        &DoRelayTagSetChangeToItemSignal,
        ECk_Signal_BindingPolicy::IgnorePayloadInFlight,
        ECk_Signal_PostFireBehavior::DoNothing>(TagSetHandle);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ItemTrait_Tags::
    DoOnSplit_Implementation(
        const FCk_Handle_Item& InSourceItem,
        FCk_Handle_Item& InNewItem) const
    -> void
{
    const auto SourceTags = UCk_Utils_ItemTrait_Tags_UE::Get_Tags(InSourceItem);
    UCk_Utils_ItemTrait_Tags_UE::Request_AddTags(InNewItem, SourceTags);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ItemTrait_Tags::
    DoCanStackWith_Implementation(
        const FCk_Handle_Item& InSource,
        const FCk_Handle_Item& InTarget) const
    -> bool
{
    return UCk_Utils_ItemTrait_Tags_UE::Get_Tags(InSource) ==
           UCk_Utils_ItemTrait_Tags_UE::Get_Tags(InTarget);
}

// --------------------------------------------------------------------------------------------------------------------
