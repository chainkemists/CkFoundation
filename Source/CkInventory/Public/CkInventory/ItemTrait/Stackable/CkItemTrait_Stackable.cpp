#include "CkItemTrait_Stackable.h"

#include "CkInventory/ItemTrait/Stackable/CkItemTrait_Stackable_Utils.h"
#include "CkInventory/Item/CkItem_Utils.h"

#include "CkCore/Format/CkFormat.h"
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
    auto ItemHandle = UCk_Utils_Item_UE::CastChecked(InOwnerEntity);
    ck::UUtils_Signal_Stackable_OnStackCountChanged::Broadcast(
        ItemHandle,
        ck::MakePayload(ItemHandle, FCk_Payload_Item_OnStackCountChanged(ItemHandle, InPayload.Get_FinalValue(), InPayload.Get_FinalValue_Previous())));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ItemTrait_Stackable::
    DoConstruct_Implementation(
        FCk_Handle& InHandle) const
    -> void
{
    auto ItemHandle = UCk_Utils_Item_UE::CastChecked(InHandle);

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

    auto Attribute = UCk_Utils_IntegerAttribute_UE::Add(ItemHandle, Params);

    // ---- Connect relay: attribute OnValueChanged -> stackable OnStackCountChanged ----

    std::ignore = ck::UUtils_Signal_OnIntegerAttributeValueChanged_Current::Bind<
        &DoRelayAttributeChangeToStackableSignal,
        ECk_Signal_BindingPolicy::IgnorePayloadInFlight,
        ECk_Signal_PostFireBehavior::DoNothing>(Attribute);
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR

auto
    UCk_ItemTrait_Stackable::
    DoValidate_Implementation(
        const UCk_InventoryItem_Definition* InDefinition,
        TArray<FText>& OutErrors) const
    -> EDataValidationResult
{
    auto Result = EDataValidationResult::Valid;

    if (_HasMaxStackSize && _InitialCount > _MaxStackSize)
    {
        OutErrors.Add(FText::FromString(ck::Format_UE(
            TEXT("Stackable: InitialCount ({}) exceeds MaxStackSize ({})."),
            _InitialCount, _MaxStackSize)));

        Result = EDataValidationResult::Invalid;
    }

    return Result;
}

#endif

// --------------------------------------------------------------------------------------------------------------------
