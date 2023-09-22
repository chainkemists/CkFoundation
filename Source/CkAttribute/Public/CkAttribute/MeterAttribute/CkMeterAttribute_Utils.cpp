#include "CkMeterAttribute_Utils.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkLabel/CkLabel_Utils.h"

#include <GameplayTagsManager.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{

FMeterAttribute_Tags FMeterAttribute_Tags::_Tags;

auto
    FMeterAttribute_Tags::AddTags() -> void
{
    auto& Manager = UGameplayTagsManager::Get();

    _MinCapacity = Manager.AddNativeGameplayTag(TEXT("Ck.Attribute.Meter.MinCapacity"));
    _MaxCapacity = Manager.AddNativeGameplayTag(TEXT("Ck.Attribute.Meter.MaxCapacity"));
    _Current = Manager.AddNativeGameplayTag(TEXT("Ck.Attribute.Meter.Current"));
}

auto
    FMeterAttribute_Tags::
    Get_MinCapacity()
    -> FGameplayTag
{
    return _Tags._MinCapacity;
}

auto
    FMeterAttribute_Tags::
    Get_MaxCapacity()
    -> FGameplayTag
{
    return _Tags._MaxCapacity;
}

auto
    FMeterAttribute_Tags::
    Get_Current()
    -> FGameplayTag
{
    return _Tags._Current;
}

}

auto
    UCk_Utils_MeterAttribute_UE::
    Add(
        const FCk_Handle InHandle,
        const FCk_Provider_MeterAttributes_ParamsData& InParams)
    -> void
{
    // TODO: Select Record policy that disallow duplicate based on Gameplay Label
    RecordOfMeterAttributes_Utils::AddIfMissing(InHandle);

    const auto& ParamsProvider = InParams.Get_Provider();

    CK_ENSURE_IF_NOT(ck::IsValid(ParamsProvider), TEXT("Invalid Meter Attributes Provider"))
    { return; }

    const auto& AddNewMeterAttributeToEntity = [&](FCk_Handle InAttributeOwner,
        const FGameplayTag& InAttributeName, FCk_Meter InAttributeBaseValue)
    {
        const auto NewAttributeEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttributeOwner);

        ck::UCk_Utils_OwningEntity::Add(NewAttributeEntity, InHandle);

        const auto& MeterParams = InAttributeBaseValue.Get_Params();
        const auto& MeterCapacity = MeterParams.Get_Capacity();
        const auto& MeterStartingPercentage = MeterParams.Get_StartingPercentage();

        UCk_Utils_FloatAttribute_UE::Add(NewAttributeEntity,
            TMap<FGameplayTag, float>
            {
                {ck::FMeterAttribute_Tags::Get_MinCapacity(), MeterCapacity.Get_MinCapacity()},
                {ck::FMeterAttribute_Tags::Get_MaxCapacity(), MeterCapacity.Get_MaxCapacity()},
                {ck::FMeterAttribute_Tags::Get_Current(), MeterStartingPercentage.Get_Value()}
            });

        UCk_Utils_GameplayLabel_UE::Add(NewAttributeEntity, InAttributeName);

        RecordOfMeterAttributes_Utils::Request_Connect(InAttributeOwner, NewAttributeEntity);
    };

    for (const auto& AttributesParams = ParamsProvider->Get_Value();
        auto Kvp : AttributesParams.Get_AttributeBaseValues())
    {
        const auto& AttributeName = Kvp.Key;
        const auto& AttributeBaseValue = Kvp.Value;

        AddNewMeterAttributeToEntity(InHandle, AttributeName, AttributeBaseValue);
    }
}

auto
    UCk_Utils_MeterAttribute_UE::
    Has(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    const auto FoundEntity = RecordOfMeterAttributes_Utils::Get_RecordEntryIf(InAttributeOwnerEntity,
    [&](FCk_Handle InHandle)
    {
        return UCk_Utils_GameplayLabel_UE::Get_Label(InHandle) == InAttributeName;
    });

    if (NOT ck::IsValid(FoundEntity))
    { return false; }

    const auto& FloatAttributeEntiyy = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InAttributeOwnerEntity, ck::FMeterAttribute_Tags::Get_MinCapacity());
    return ck::IsValid(FloatAttributeEntiyy);
}

auto
    UCk_Utils_MeterAttribute_UE::
    Ensure(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InAttributeName, InAttributeOwnerEntity), TEXT("Handle [{}] does NOT have a Meter Attribute"), InAttributeOwnerEntity)
    { return false; }

    return true;
}

auto
    UCk_Utils_MeterAttribute_UE::
    Get_BaseValue(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity)
    -> FCk_Meter
{
    const auto [MinCapacity, MaxCapacity, Current] = Get_MinMaxAndCurrentAttributeEntities(InAttributeName, InAttributeOwnerEntity);

    const auto& MinCapacityValue =  FloatAttribute_Utils::Get_BaseValue(MinCapacity);
    const auto& MaxCapacityValue = FloatAttribute_Utils::Get_BaseValue(MaxCapacity);
    const auto& CurrentValue = FloatAttribute_Utils::Get_BaseValue(Current);

    return FCk_Meter
    {
        FCk_Meter_Params{FCk_Meter_Capacity{MaxCapacityValue}.Set_MinCapacity(MinCapacityValue)}
        .Set_StartingPercentage(FCk_FloatRange_0to1{CurrentValue})
    };
}

auto
    UCk_Utils_MeterAttribute_UE::
    Get_BonusValue(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity)
    -> FCk_Meter
{
    return Get_FinalValue(InAttributeName, InAttributeOwnerEntity) - Get_BaseValue(InAttributeName, InAttributeOwnerEntity);
}

auto
    UCk_Utils_MeterAttribute_UE::
    Get_FinalValue(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity)
    -> FCk_Meter
{
    const auto [MinCapacity, MaxCapacity, Current] = Get_MinMaxAndCurrentAttributeEntities(InAttributeName, InAttributeOwnerEntity);

    const auto& MinCapacityValue =  FloatAttribute_Utils::Get_FinalValue(MinCapacity);
    const auto& MaxCapacityValue = FloatAttribute_Utils::Get_FinalValue(MaxCapacity);
    const auto& CurrentValue = FloatAttribute_Utils::Get_FinalValue(Current);

    return FCk_Meter
    {
        FCk_Meter_Params{FCk_Meter_Capacity{MaxCapacityValue}.Set_MinCapacity(MinCapacityValue)}
        .Set_StartingPercentage(FCk_FloatRange_0to1{CurrentValue})
    };
}

auto
    UCk_Utils_MeterAttribute_UE::
    BindTo_OnValueChanged(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_MeterAttribute_OnValueChanged& InDelegate)
    -> void
{
    const auto FoundEntity = RecordOfMeterAttributes_Utils::Get_RecordEntryIf(InAttributeOwnerEntity,
    [&](FCk_Handle InHandle)
    {
        return UCk_Utils_GameplayLabel_UE::Get_Label(InHandle) == InAttributeName;
    });

    const auto [MinCapacity, MaxCapacity, Current] = Get_MinMaxAndCurrentAttributeEntities(InAttributeName, InAttributeOwnerEntity);

    switch(InBehavior)
    {
    case ECk_Signal_BindingPolicy::FireIfPayloadInFlight:
        ck::UUtils_Signal_OnFloatAttributeValueChanged::Bind
            <&UCk_Utils_MeterAttribute_UE::OnMinCapacityChanged, ECk_Signal_BindingPolicy::FireIfPayloadInFlight>(MinCapacity);
        ck::UUtils_Signal_OnFloatAttributeValueChanged::Bind
            <&UCk_Utils_MeterAttribute_UE::OnMaxCapacityChanged, ECk_Signal_BindingPolicy::FireIfPayloadInFlight>(MaxCapacity);
        ck::UUtils_Signal_OnFloatAttributeValueChanged::Bind
            <&UCk_Utils_MeterAttribute_UE::OnCurrentValueCanged, ECk_Signal_BindingPolicy::FireIfPayloadInFlight>(Current);
        break;
    case ECk_Signal_BindingPolicy::IgnorePayloadInFlight:
        ck::UUtils_Signal_OnFloatAttributeValueChanged::Bind
            <&UCk_Utils_MeterAttribute_UE::OnMinCapacityChanged, ECk_Signal_BindingPolicy::IgnorePayloadInFlight>(MinCapacity);
        ck::UUtils_Signal_OnFloatAttributeValueChanged::Bind
            <&UCk_Utils_MeterAttribute_UE::OnMaxCapacityChanged, ECk_Signal_BindingPolicy::IgnorePayloadInFlight>(MaxCapacity);
        ck::UUtils_Signal_OnFloatAttributeValueChanged::Bind
            <&UCk_Utils_MeterAttribute_UE::OnCurrentValueCanged, ECk_Signal_BindingPolicy::IgnorePayloadInFlight>(Current);
        break;
    default:
        CK_INVALID_ENUM(InBehavior);
    }

    ck::UUtils_Signal_OnMeterAttributeValueChanged::Bind(FoundEntity, InDelegate, InBehavior);
}

auto
    UCk_Utils_MeterAttribute_UE::
    UnbindFrom_OnValueChanged(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity,
        const FCk_Delegate_MeterAttribute_OnValueChanged& InDelegate)
    -> void
{
    const auto FoundEntity = RecordOfMeterAttributes_Utils::Get_RecordEntryIf(InAttributeOwnerEntity,
    [&](FCk_Handle InHandle)
    {
        return UCk_Utils_GameplayLabel_UE::Get_Label(InHandle) == InAttributeName;
    });

    const auto [MinCapacity, MaxCapacity, Current] = Get_MinMaxAndCurrentAttributeEntities(InAttributeName, FoundEntity);

    ck::UUtils_Signal_OnFloatAttributeValueChanged::Unbind<OnMinCapacityChanged>(MinCapacity);
    ck::UUtils_Signal_OnFloatAttributeValueChanged::Unbind<OnMaxCapacityChanged>(MaxCapacity);
    ck::UUtils_Signal_OnFloatAttributeValueChanged::Unbind<OnCurrentValueCanged>(Current);

    ck::UUtils_Signal_OnMeterAttributeValueChanged::Unbind(FoundEntity, InDelegate);
}

auto
    UCk_Utils_MeterAttribute_UE::
    Get_MinMaxAndCurrentAttributeEntities(
        FGameplayTag InAttributeName,
        FCk_Handle InOwnerEntity)
    -> std::tuple<FCk_Handle, FCk_Handle, FCk_Handle>
{
    const auto FoundEntity = RecordOfMeterAttributes_Utils::Get_RecordEntryIf(InOwnerEntity,
    [&](FCk_Handle InHandle)
    {
        return UCk_Utils_GameplayLabel_UE::Get_Label(InHandle) == InAttributeName;
    });

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
    const auto MeterAttributeOwnerEntity = ck::UCk_Utils_OwningEntity::Get_StoredEntity(MeterAttributeEntity);

    ck::UUtils_Signal_OnMeterAttributeValueChanged::Broadcast
    (
        MeterAttributeEntity,
        ck::MakePayload
        (
            MeterAttributeOwnerEntity,
            ck::TPayload_Attribute_OnValueChanged<ck::FFragment_MeterAttribute>
            {
                MeterAttributeOwnerEntity,
                Get_BaseValue(MeterAttributeLabel, MeterAttributeOwnerEntity),
                Get_FinalValue(MeterAttributeLabel, MeterAttributeOwnerEntity)
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
    const auto MeterAttributeOwnerEntity = ck::UCk_Utils_OwningEntity::Get_StoredEntity(MeterAttributeEntity);

    ck::UUtils_Signal_OnMeterAttributeValueChanged::Broadcast
    (
        MeterAttributeEntity,
        ck::MakePayload
        (
            MeterAttributeOwnerEntity,
            ck::TPayload_Attribute_OnValueChanged<ck::FFragment_MeterAttribute>
            {
                MeterAttributeOwnerEntity,
                Get_BaseValue(MeterAttributeLabel, MeterAttributeOwnerEntity),
                Get_FinalValue(MeterAttributeLabel, MeterAttributeOwnerEntity)
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
    const auto MeterAttributeOwnerEntity = ck::UCk_Utils_OwningEntity::Get_StoredEntity(MeterAttributeEntity);

    ck::UUtils_Signal_OnMeterAttributeValueChanged::Broadcast
    (
        MeterAttributeEntity,
        ck::MakePayload
        (
            MeterAttributeOwnerEntity,
            ck::TPayload_Attribute_OnValueChanged<ck::FFragment_MeterAttribute>
            {
                MeterAttributeOwnerEntity,
                Get_BaseValue(MeterAttributeLabel, MeterAttributeOwnerEntity),
                Get_FinalValue(MeterAttributeLabel, MeterAttributeOwnerEntity)
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
    const auto& AttributeName = InParams.Get_TargetAttributeName();

    const auto FoundEntity = UCk_Utils_MeterAttribute_UE::RecordOfMeterAttributes_Utils::Get_RecordEntryIf(InAttributeOwnerEntity,
    [&](FCk_Handle InHandle)
    {
        return UCk_Utils_GameplayLabel_UE::Get_Label(InHandle) == AttributeName;
    });

    UCk_Utils_FloatAttributeModifier_UE::Add(FoundEntity, InModifierName,
        FCk_Fragment_FloatAttributeModifier_ParamsData{FCk_Fragment_FloatAttributeModifier_ParamsData
        {
            InParams.Get_ModifierDelta().Get_Params().Get_Capacity().Get_MinCapacity(),
            ck::FMeterAttribute_Tags::Get_MinCapacity(),
            InParams.Get_ModifierOperation()
        }});

    UCk_Utils_FloatAttributeModifier_UE::Add(FoundEntity, InModifierName,
        FCk_Fragment_FloatAttributeModifier_ParamsData{FCk_Fragment_FloatAttributeModifier_ParamsData
        {
            InParams.Get_ModifierDelta().Get_Params().Get_Capacity().Get_MaxCapacity(),
            ck::FMeterAttribute_Tags::Get_MaxCapacity(),
            InParams.Get_ModifierOperation()
        }});

    UCk_Utils_FloatAttributeModifier_UE::Add(FoundEntity, InModifierName,
        FCk_Fragment_FloatAttributeModifier_ParamsData{FCk_Fragment_FloatAttributeModifier_ParamsData
        {
            InParams.Get_ModifierDelta().Get_Params().Get_StartingPercentage().Get_Value(),
            ck::FMeterAttribute_Tags::Get_Current(),
            InParams.Get_ModifierOperation()
        }});
}

auto
    UCk_Utils_MeterAttributeModifier_UE::
    Has(
        FGameplayTag InModifierName,
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    const auto& AttributeName = InAttributeName;

    const auto FoundEntity = UCk_Utils_MeterAttribute_UE::RecordOfMeterAttributes_Utils::Get_RecordEntryIf(InAttributeOwnerEntity,
    [&](FCk_Handle InHandle)
    {
        return UCk_Utils_GameplayLabel_UE::Get_Label(InHandle) == AttributeName;
    });

    return UCk_Utils_FloatAttributeModifier_UE::Has(InModifierName, ck::FMeterAttribute_Tags::Get_MinCapacity(), FoundEntity);
}

auto
    UCk_Utils_MeterAttributeModifier_UE::
    Ensure(
        FGameplayTag InModifierName,
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InModifierName, InAttributeName, InAttributeOwnerEntity), TEXT("Handle [{}] does NOT have a Meter Attribute Modifier with name [{}]"), InAttributeOwnerEntity, InModifierName)
    { return false; }

    return true;
}

auto
    UCk_Utils_MeterAttributeModifier_UE::
    Remove(
        FGameplayTag InModifierName,
        FGameplayTag InAttributeName,
        FCk_Handle   InAttributeOwnerEntity)
    -> void
{
    const auto FoundEntity = UCk_Utils_MeterAttribute_UE::RecordOfMeterAttributes_Utils::Get_RecordEntryIf(InAttributeOwnerEntity,
    [&](FCk_Handle InHandle)
    {
        return UCk_Utils_GameplayLabel_UE::Get_Label(InHandle) == InAttributeName;
    });

    UCk_Utils_FloatAttributeModifier_UE::Remove(InModifierName, ck::FMeterAttribute_Tags::Get_MinCapacity(), FoundEntity);
    UCk_Utils_FloatAttributeModifier_UE::Remove(InModifierName, ck::FMeterAttribute_Tags::Get_MaxCapacity(), FoundEntity);
    UCk_Utils_FloatAttributeModifier_UE::Remove(InModifierName, ck::FMeterAttribute_Tags::Get_Current(), FoundEntity);
}

// --------------------------------------------------------------------------------------------------------------------
