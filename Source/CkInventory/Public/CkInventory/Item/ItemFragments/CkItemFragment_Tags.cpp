#include "CkItemFragment_Tags.h"

#include "CkInventory/Item/ItemFragments/CkItemFragment_Tags_Utils.h"
#include "CkInventory/Item/CkInventoryItem_Utils.h"

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
    auto ItemHandle = UCk_Utils_InventoryItem_UE::CastChecked(InTagSet);
    ck::UUtils_Signal_ItemTags_OnTagsChanged::Broadcast(
        ItemHandle,
        ck::MakePayload(ItemHandle, FCk_Payload_Item_OnTagsChanged(ItemHandle, InTagsAdded, InTagsRemoved)));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_ItemFragment_Tags::
    OnApplied(
        FCk_Handle_Item& InItem) const
    -> void
{
    auto TagSetHandle = UCk_Utils_TagSet_UE::Add(InItem, _Tags);

    if (ck::Is_NOT_Valid(TagSetHandle))
    { return; }

    // ---- Connect relay: TagSet OnTagsChanged → Item OnTagsChanged ----

    (void)ck::UUtils_Signal_TagSet_OnTagsChanged::Bind<
        &DoRelayTagSetChangeToItemSignal,
        ECk_Signal_BindingPolicy::IgnorePayloadInFlight,
        ECk_Signal_PostFireBehavior::DoNothing>(TagSetHandle);
}

// --------------------------------------------------------------------------------------------------------------------
