#include "CkMeterAttribute_Utils.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"
#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkLabel/CkLabel_Utils.h"
#include "Kismet/KismetMathLibrary.h"

#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

#include <GameplayTagsManager.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FMeterAttribute_Tags FMeterAttribute_Tags::_Tags;

    auto
        FMeterAttribute_Tags::
        AddTags()
        -> void
    {
        auto& Manager = UGameplayTagsManager::Get();

        _MinCapacity = Manager.AddNativeGameplayTag(TEXT("Ck.Attribute.Meter.MinCapacity"));
        _MaxCapacity = Manager.AddNativeGameplayTag(TEXT("Ck.Attribute.Meter.MaxCapacity"));
        _Current     = Manager.AddNativeGameplayTag(TEXT("Ck.Attribute.Meter.Current"));
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_MeterAttribute_ConstructionScript_PDA::
    DoConstruct_Implementation(
        const FCk_Handle& InHandle)
    -> void
{
    using RecordOfMeterAttributes_Utils = UCk_Utils_MeterAttribute_UE::RecordOfMeterAttributes_Utils;

    const auto& MeterParams = Get_Params().Get_AttributeBaseValue().Get_Params();
    const auto& MeterCapacity = MeterParams.Get_Capacity();
    const auto& MeterStartingPercentage = MeterParams.Get_StartingPercentage();

    UCk_Utils_FloatAttribute_UE::AddMultiple
    (
        InHandle,
        FCk_Fragment_MultipleFloatAttribute_ParamsData
        {
            {
                { ck::FMeterAttribute_Tags::Get_MinCapacity(), MeterCapacity.Get_MinCapacity() },
                { ck::FMeterAttribute_Tags::Get_MaxCapacity(), MeterCapacity.Get_MaxCapacity() },
                { ck::FMeterAttribute_Tags::Get_Current(), MeterCapacity.Get_MaxCapacity() * MeterStartingPercentage.Get_Value() }
            }
        }
    );

    UCk_Utils_GameplayLabel_UE::Add(InHandle, Get_Params().Get_AttributeName());
    const auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

    RecordOfMeterAttributes_Utils::AddIfMissing(
        LifetimeOwner, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfMeterAttributes_Utils::Request_Connect(LifetimeOwner, InHandle);
}

auto
    UCk_Utils_MeterAttribute_UE::
    Add(
        FCk_Handle InHandle,
        UCk_MeterAttribute_ConstructionScript_PDA* InDataAsset)
    -> void
{
    if (NOT UCk_Utils_Net_UE::Get_HasAuthority(InHandle))
    { return;}

    // Meter is an Entity that is made up of sub-entities (FloatAttribute) and thus
    UCk_Utils_EntityReplicationDriver_UE::Request_Replicate(InHandle, FCk_EntityReplicationDriver_ConstructionInfo{InDataAsset});
}

auto
    UCk_Utils_MeterAttribute_UE::
    AddMultiple(
        FCk_Handle InHandle,
        const FCk_Fragment_MultipleMeterAttribute_ParamsData& InParams)
    -> void
{
    for (const auto& Param : InParams.Get_MeterAttributeParams())
    {
        Add(InHandle, Param);
    }
}

auto
    UCk_Utils_MeterAttribute_UE::
    Has(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> bool
{
    if (NOT RecordOfMeterAttributes_Utils::Has(InAttributeOwnerEntity))
    { return false; }

    const auto FoundEntity = RecordOfMeterAttributes_Utils::Get_RecordEntryIf(InAttributeOwnerEntity, ck::algo::MatchesGameplayLabelExact{InAttributeName});

    return ck::IsValid(FoundEntity);
}

auto
    UCk_Utils_MeterAttribute_UE::
    Has_Any(
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    return RecordOfMeterAttributes_Utils::Has(InAttributeOwnerEntity);
}

auto
    UCk_Utils_MeterAttribute_UE::
    Ensure(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InAttributeOwnerEntity, InAttributeName), TEXT("Handle [{}] does NOT have a Meter Attribute [{}]"), InAttributeOwnerEntity, InAttributeName)
    { return false; }

    return true;
}

auto
    UCk_Utils_MeterAttribute_UE::
    Ensure_Any(
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    CK_ENSURE_IF_NOT(Has_Any(InAttributeOwnerEntity), TEXT("Handle [{}] does NOT have any Meter Attribute [{}]"), InAttributeOwnerEntity)
    { return false; }

    return true;
}

auto
    UCk_Utils_MeterAttribute_UE::
    Get_All(
        FCk_Handle InAttributeOwnerEntity)
    -> TArray<FGameplayTag>
{
    if (NOT RecordOfMeterAttributes_Utils::Has(InAttributeOwnerEntity))
    { return {}; }

    const auto& MeterAttributeEntities = RecordOfMeterAttributes_Utils::Get_AllRecordEntries(InAttributeOwnerEntity);

    return ck::algo::Transform<TArray<FGameplayTag>>(MeterAttributeEntities, [&](FCk_Handle InMeterAttributeEntity)
    {
        return UCk_Utils_GameplayLabel_UE::Get_Label(InMeterAttributeEntity);
    });
}

auto
    UCk_Utils_MeterAttribute_UE::
    Get_BaseValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> FCk_Meter
{
    const auto [MinCapacity, MaxCapacity, Current] = Get_MinMaxAndCurrentAttributeEntities(InAttributeOwnerEntity, InAttributeName);

    const auto& MinCapacityValue =  FloatAttribute_Utils::Get_BaseValue(MinCapacity);
    const auto& MaxCapacityValue = FloatAttribute_Utils::Get_BaseValue(MaxCapacity);
    const auto& CurrentValue = FloatAttribute_Utils::Get_BaseValue(Current);

    const auto& CurrentPercentage = UKismetMathLibrary::MapRangeClamped(CurrentValue, MinCapacityValue, MaxCapacityValue, 0.0f, 1.0f);

    return FCk_Meter
    {
        FCk_Meter_Params{FCk_Meter_Capacity{MaxCapacityValue}.Set_MinCapacity(MinCapacityValue)}
        .Set_StartingPercentage(FCk_FloatRange_0to1{static_cast<float>(CurrentPercentage)})
    };
}

auto
    UCk_Utils_MeterAttribute_UE::
    Get_BonusValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> FCk_Meter
{
    return Get_FinalValue(InAttributeOwnerEntity, InAttributeName) - Get_BaseValue(InAttributeOwnerEntity, InAttributeName);
}

auto
    UCk_Utils_MeterAttribute_UE::
    Get_FinalValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> FCk_Meter
{
    const auto [MinCapacity, MaxCapacity, Current] = Get_MinMaxAndCurrentAttributeEntities(InAttributeOwnerEntity, InAttributeName);

    const auto& MinCapacityValue =  FloatAttribute_Utils::Get_FinalValue(MinCapacity);
    const auto& MaxCapacityValue = FloatAttribute_Utils::Get_FinalValue(MaxCapacity);
    const auto& CurrentValue = FloatAttribute_Utils::Get_FinalValue(Current);

    const auto& CurrentPercentage = UKismetMathLibrary::MapRangeClamped(CurrentValue, MinCapacityValue, MaxCapacityValue, 0.0f, 1.0f);

    return FCk_Meter
    {
        FCk_Meter_Params{FCk_Meter_Capacity{MaxCapacityValue}.Set_MinCapacity(MinCapacityValue)}
        .Set_StartingPercentage(FCk_FloatRange_0to1{static_cast<float>(CurrentPercentage)})
    };
}

auto
    UCk_Utils_MeterAttribute_UE::
    BindTo_OnValueChanged(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_MeterAttribute_OnValueChanged& InDelegate)
    -> void
{
    const auto FoundEntity = RecordOfMeterAttributes_Utils::Get_RecordEntryIf(InAttributeOwnerEntity, ck::algo::MatchesGameplayLabelExact{InAttributeName});

    const auto [MinCapacity, MaxCapacity, Current] = Get_MinMaxAndCurrentAttributeEntities(InAttributeOwnerEntity, InAttributeName);

    ck::UUtils_Signal_OnFloatAttributeValueChanged::Bind<&UCk_Utils_MeterAttribute_UE::OnMinCapacityChanged>(MinCapacity, InBehavior);
    ck::UUtils_Signal_OnFloatAttributeValueChanged::Bind<&UCk_Utils_MeterAttribute_UE::OnMaxCapacityChanged>(MaxCapacity, InBehavior);
    ck::UUtils_Signal_OnFloatAttributeValueChanged::Bind<&UCk_Utils_MeterAttribute_UE::OnCurrentValueCanged>(Current, InBehavior);

    ck::UUtils_Signal_OnMeterAttributeValueChanged::Bind(FoundEntity, InDelegate, InBehavior);
}

auto
    UCk_Utils_MeterAttribute_UE::
    UnbindFrom_OnValueChanged(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        const FCk_Delegate_MeterAttribute_OnValueChanged& InDelegate)
    -> void
{
    const auto FoundEntity = RecordOfMeterAttributes_Utils::Get_RecordEntryIf(InAttributeOwnerEntity, ck::algo::MatchesGameplayLabelExact{InAttributeName});

    const auto [MinCapacity, MaxCapacity, Current] = Get_MinMaxAndCurrentAttributeEntities(FoundEntity, InAttributeName);

    ck::UUtils_Signal_OnFloatAttributeValueChanged::Unbind<OnMinCapacityChanged>(MinCapacity);
    ck::UUtils_Signal_OnFloatAttributeValueChanged::Unbind<OnMaxCapacityChanged>(MaxCapacity);
    ck::UUtils_Signal_OnFloatAttributeValueChanged::Unbind<OnCurrentValueCanged>(Current);

    ck::UUtils_Signal_OnMeterAttributeValueChanged::Unbind(FoundEntity, InDelegate);
}

auto
    UCk_Utils_MeterAttribute_UE::
    Get_MinMaxAndCurrentAttributeEntities(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> std::tuple<FCk_Handle, FCk_Handle, FCk_Handle>
{
    const auto FoundEntity = RecordOfMeterAttributes_Utils::Get_RecordEntryIf(InAttributeOwnerEntity, ck::algo::MatchesGameplayLabelExact{InAttributeName});

    const auto& MinCapacity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(FoundEntity, ck::FMeterAttribute_Tags::Get_MinCapacity());

    const auto& MaxCapacity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(FoundEntity, ck::FMeterAttribute_Tags::Get_MaxCapacity());

    const auto& Current = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(FoundEntity, ck::FMeterAttribute_Tags::Get_Current());

    return {MinCapacity, MaxCapacity, Current};
}

auto
    UCk_Utils_MeterAttribute_UE::
    OnMinCapacityChanged(
        FCk_Handle InFloatAttributeEntity,
        ck::TPayload_Attribute_OnValueChanged<ck::FFragment_FloatAttribute> InValueChanged)
    -> void
{
    const auto MeterAttributeEntity = InFloatAttributeEntity;
    const auto MeterAttributeLabel = UCk_Utils_GameplayLabel_UE::Get_Label(MeterAttributeEntity);
    const auto MeterAttributeOwnerEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(MeterAttributeEntity);

    ck::UUtils_Signal_OnMeterAttributeValueChanged::Broadcast
    (
        MeterAttributeEntity,
        ck::MakePayload
        (
            MeterAttributeOwnerEntity,
            ck::TPayload_Attribute_OnValueChanged<ck::FFragment_MeterAttribute>
            {
                MeterAttributeOwnerEntity,
                Get_BaseValue(MeterAttributeOwnerEntity, MeterAttributeLabel),
                Get_FinalValue(MeterAttributeOwnerEntity, MeterAttributeLabel)
            }
        )
    );
}

auto
    UCk_Utils_MeterAttribute_UE::
    OnMaxCapacityChanged(
        FCk_Handle InFloatAttributeEntity,
        ck::TPayload_Attribute_OnValueChanged<ck::FFragment_FloatAttribute> InValueChanged)
    -> void
{
    const auto MeterAttributeEntity = InFloatAttributeEntity;
    const auto MeterAttributeLabel = UCk_Utils_GameplayLabel_UE::Get_Label(MeterAttributeEntity);
    const auto MeterAttributeOwnerEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(MeterAttributeEntity);

    ck::UUtils_Signal_OnMeterAttributeValueChanged::Broadcast
    (
        MeterAttributeEntity,
        ck::MakePayload
        (
            MeterAttributeOwnerEntity,
            ck::TPayload_Attribute_OnValueChanged<ck::FFragment_MeterAttribute>
            {
                MeterAttributeOwnerEntity,
                Get_BaseValue(MeterAttributeOwnerEntity, MeterAttributeLabel),
                Get_FinalValue(MeterAttributeOwnerEntity, MeterAttributeLabel)
            }
        )
    );
}

auto
    UCk_Utils_MeterAttribute_UE::
    OnCurrentValueCanged(
        FCk_Handle InFloatAttributeEntity,
        ck::TPayload_Attribute_OnValueChanged<ck::FFragment_FloatAttribute> InValueChanged)
    -> void
{
    const auto MeterAttributeEntity = InFloatAttributeEntity;
    const auto MeterAttributeLabel = UCk_Utils_GameplayLabel_UE::Get_Label(MeterAttributeEntity);
    const auto MeterAttributeOwnerEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(MeterAttributeEntity);

    ck::UUtils_Signal_OnMeterAttributeValueChanged::Broadcast
    (
        MeterAttributeEntity,
        ck::MakePayload
        (
            MeterAttributeOwnerEntity,
            ck::TPayload_Attribute_OnValueChanged<ck::FFragment_MeterAttribute>
            {
                MeterAttributeOwnerEntity,
                Get_BaseValue(MeterAttributeOwnerEntity, MeterAttributeLabel),
                Get_FinalValue(MeterAttributeOwnerEntity, MeterAttributeLabel)
            }
        )
    );
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_MeterAttributeModifier_UE::
    Add(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InModifierName,
        const FCk_Fragment_MeterAttributeModifier_ParamsData& InParams)
    -> void
{
    const auto& ModifierPolicy     = InParams.Get_ModifierPolicy();
    const auto& ModifyMinCapacity  = EnumHasAnyFlags(ModifierPolicy, ECk_MeterAttributeModifier_Policy::MinCapacity);
    const auto& ModifyMaxCapacity  = EnumHasAnyFlags(ModifierPolicy, ECk_MeterAttributeModifier_Policy::MaxCapacity);
    const auto& ModifyCurrentValue = EnumHasAnyFlags(ModifierPolicy, ECk_MeterAttributeModifier_Policy::CurrentValue);

    CK_ENSURE_IF_NOT(ModifyMinCapacity || ModifyMaxCapacity || ModifyCurrentValue,
        TEXT("Cannot Meter Attribute Modifier [{}] to Entity [{}] because it targets NO meter component "
            "(MinCapacity, MaxCapacity or CurrentValue)"), InModifierName, InAttributeOwnerEntity)
    { return; }

    const auto FoundEntity = UCk_Utils_MeterAttribute_UE::RecordOfMeterAttributes_Utils::Get_RecordEntryIf
    (
        InAttributeOwnerEntity,
        ck::algo::MatchesGameplayLabelExact{InParams.Get_TargetAttributeName()}
    );

    const auto& ModifierDeltaParams = InParams.Get_ModifierDelta().Get_Params();

    if (ModifyMinCapacity)
    {
        UCk_Utils_FloatAttributeModifier_UE::Add
        (
            FoundEntity,
            InModifierName,
            FCk_Fragment_FloatAttributeModifier_ParamsData{FCk_Fragment_FloatAttributeModifier_ParamsData
            {
                ModifierDeltaParams.Get_Capacity().Get_MinCapacity(),
                ck::FMeterAttribute_Tags::Get_MinCapacity(),
                InParams.Get_ModifierOperation(),
                InParams.Get_ModifierOperation_RevokablePolicy()
            }}
        );
    }

    if (ModifyMaxCapacity)
    {
        UCk_Utils_FloatAttributeModifier_UE::Add
        (
            FoundEntity,
            InModifierName,
            FCk_Fragment_FloatAttributeModifier_ParamsData{FCk_Fragment_FloatAttributeModifier_ParamsData
            {
                ModifierDeltaParams.Get_Capacity().Get_MaxCapacity(),
                ck::FMeterAttribute_Tags::Get_MaxCapacity(),
                InParams.Get_ModifierOperation(),
                InParams.Get_ModifierOperation_RevokablePolicy()
            }}
        );
    }

    if (ModifyCurrentValue)
    {
        const auto& MeterCurrentDelta = UKismetMathLibrary::MapRangeClamped
        (
            ModifierDeltaParams.Get_StartingPercentage().Get_Value(),
            0.0f,
            1.0f,
            ModifierDeltaParams.Get_Capacity().Get_MinCapacity(),
            ModifierDeltaParams.Get_Capacity().Get_MaxCapacity()
        );

        UCk_Utils_FloatAttributeModifier_UE::Add
        (
            FoundEntity,
            InModifierName,
            FCk_Fragment_FloatAttributeModifier_ParamsData{FCk_Fragment_FloatAttributeModifier_ParamsData
            {
                static_cast<float>(MeterCurrentDelta),
                ck::FMeterAttribute_Tags::Get_Current(),
                InParams.Get_ModifierOperation(),
                InParams.Get_ModifierOperation_RevokablePolicy()
            }}
        );
    }
}

auto
    UCk_Utils_MeterAttributeModifier_UE::
    Has(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName)
    -> bool
{
    const auto FoundEntity = UCk_Utils_MeterAttribute_UE::RecordOfMeterAttributes_Utils::Get_RecordEntryIf(InAttributeOwnerEntity, ck::algo::MatchesGameplayLabelExact{InAttributeName});

    const auto& HasMeterModifier_MinCapacity  = UCk_Utils_FloatAttributeModifier_UE::Has(FoundEntity, ck::FMeterAttribute_Tags::Get_MinCapacity(), InModifierName);
    const auto& HasMeterModifier_MaxCapacity  = UCk_Utils_FloatAttributeModifier_UE::Has(FoundEntity, ck::FMeterAttribute_Tags::Get_MaxCapacity(), InModifierName);
    const auto& HasMeterModifier_CurrentValue = UCk_Utils_FloatAttributeModifier_UE::Has(FoundEntity, ck::FMeterAttribute_Tags::Get_Current(),     InModifierName);

    return HasMeterModifier_MinCapacity || HasMeterModifier_MaxCapacity || HasMeterModifier_CurrentValue;
}

auto
    UCk_Utils_MeterAttributeModifier_UE::
    Ensure(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InAttributeOwnerEntity, InAttributeName, InModifierName), TEXT("Handle [{}] does NOT have a Meter Attribute Modifier with Name [{}]"), InAttributeOwnerEntity, InModifierName)
    { return false; }

    return true;
}

auto
    UCk_Utils_MeterAttributeModifier_UE::
    Remove(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName)
    -> void
{
    const auto FoundEntity = UCk_Utils_MeterAttribute_UE::RecordOfMeterAttributes_Utils::Get_RecordEntryIf(InAttributeOwnerEntity, ck::algo::MatchesGameplayLabelExact{InAttributeName});

    const auto& HasMeterModifier_MinCapacity  = UCk_Utils_FloatAttributeModifier_UE::Has(FoundEntity, ck::FMeterAttribute_Tags::Get_MinCapacity(), InModifierName);
    const auto& HasMeterModifier_MaxCapacity  = UCk_Utils_FloatAttributeModifier_UE::Has(FoundEntity, ck::FMeterAttribute_Tags::Get_MaxCapacity(), InModifierName);
    const auto& HasMeterModifier_CurrentValue = UCk_Utils_FloatAttributeModifier_UE::Has(FoundEntity, ck::FMeterAttribute_Tags::Get_Current(),     InModifierName);

    if (HasMeterModifier_MinCapacity)
    {
        UCk_Utils_FloatAttributeModifier_UE::Remove(FoundEntity, ck::FMeterAttribute_Tags::Get_MinCapacity(), InModifierName);
    }

    if (HasMeterModifier_MaxCapacity)
    {
        UCk_Utils_FloatAttributeModifier_UE::Remove(FoundEntity, ck::FMeterAttribute_Tags::Get_MaxCapacity(), InModifierName);
    }

    if (HasMeterModifier_CurrentValue)
    {
        UCk_Utils_FloatAttributeModifier_UE::Remove(FoundEntity, ck::FMeterAttribute_Tags::Get_Current(), InModifierName);
    }
}

// --------------------------------------------------------------------------------------------------------------------
