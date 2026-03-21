#include "CkItemFragment_Stackable.h"

#include "CkInventory/Item/ItemFragments/CkItemFragment_Stackable_Utils.h"
#include "CkInventory/Item/CkInventoryItem_Utils.h"

#include "CkCore/Payload/CkPayload.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Utils.h"
#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Fragment.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(TAG_IntegerAttribute_InventoryItem_StackCount, TEXT("IntegerAttribute.Inventory.Item.StackCount"));

// --------------------------------------------------------------------------------------------------------------------

static auto
DoRelayAttributeChangeToStackableSignal(
    FCk_Handle InOwnerEntity,
    ck::TPayload_Attribute_OnValueChanged<ck::FFragment_IntegerAttribute_Current> InPayload) -> void
{
    auto ItemHandle = UCk_Utils_InventoryItem_UE::CastChecked(InOwnerEntity);
    ck::UUtils_Signal_Stackable_OnStackCountChanged::Broadcast(
        ItemHandle,
        ck::MakePayload(ItemHandle, FCk_Payload_Item_OnStackCountChanged(ItemHandle, InPayload.Get_FinalValue(), InPayload.Get_FinalValue_Previous())));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_ItemFragment_Stackable::
    OnApplied(
        FCk_Handle_Item& InItem) const
    -> void
{
    auto Params = FCk_Fragment_IntegerAttribute_ParamsData(TAG_IntegerAttribute_InventoryItem_StackCount, _InitialCount);
    Params.Set_MinValue(1);

    if (_HasMaxStackSize)
    {
        Params.Set_MinMax(ECk_MinMax::MinMax);
        Params.Set_MaxValue(_MaxStackSize);
    }
    else
    {
        Params.Set_MinMax(ECk_MinMax::Min);
    }

    UCk_Utils_IntegerAttribute_UE::Add(InItem, Params);

    // ---- Connect relay: attribute OnValueChanged → stackable OnStackCountChanged ----

    auto Attribute = UCk_Utils_IntegerAttribute_UE::TryGet(
        InItem, TAG_IntegerAttribute_InventoryItem_StackCount);

    if (ck::Is_NOT_Valid(Attribute))
    { return; }

    (void)ck::UUtils_Signal_OnIntegerAttributeValueChanged_Current::Bind<
        &DoRelayAttributeChangeToStackableSignal,
        ECk_Signal_BindingPolicy::IgnorePayloadInFlight,
        ECk_Signal_PostFireBehavior::DoNothing>(Attribute);
}

// --------------------------------------------------------------------------------------------------------------------
