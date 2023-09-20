#include "CkMeterAttribute_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkLabel/CkLabel_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_MeterAttribute_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Provider_MeterAttributes_ParamsData& InParams)
    -> void
{
    // TODO: Select Record policy that disallow duplicate based on Gameplay Label
    RecordOfMeterAttributes_Utils::AddIfMissing(InHandle);

    const auto& paramsProvider = InParams.Get_Provider();

    CK_ENSURE_IF_NOT(ck::IsValid(paramsProvider), TEXT("Invalid Meter Attributes Provider"))
    { return; }

    const auto& AddNewMeterAttributeToEntity = [&](FCk_Handle InAttributeOwner,
        const FGameplayTag& InAttributeName, FCk_Meter InAttributeBaseValue)
    {
        const auto newAttributeEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttributeOwner);

        ck::UCk_Utils_OwningEntity::Add(newAttributeEntity, InHandle);

        MeterAttribute_Utils::Add(newAttributeEntity, InAttributeBaseValue);
        UCk_Utils_GameplayLabel_UE::Add(newAttributeEntity, InAttributeName);

        RecordOfMeterAttributes_Utils::Request_Connect(InAttributeOwner, newAttributeEntity);
    };

    for (const auto& AttributesParams = paramsProvider->Get_Value();
        auto kvp : AttributesParams.Get_AttributeBaseValues())
    {
        const auto& attributeName = kvp.Key;
        const auto& attributeBaseValue = kvp.Value;

        AddNewMeterAttributeToEntity(InHandle, attributeName, attributeBaseValue);
    }
}

auto
    UCk_Utils_MeterAttribute_UE::
    Has(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <MeterAttribute_Utils, RecordOfMeterAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    return ck::IsValid(AttributeEntity);
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
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <MeterAttribute_Utils, RecordOfMeterAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    return MeterAttribute_Utils::Get_BaseValue(AttributeEntity);
}

auto
    UCk_Utils_MeterAttribute_UE::
    Get_BonusValue(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity)
    -> FCk_Meter
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <MeterAttribute_Utils, RecordOfMeterAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    return MeterAttribute_Utils::Get_FinalValue(AttributeEntity) - MeterAttribute_Utils::Get_BaseValue(AttributeEntity);
}

auto
    UCk_Utils_MeterAttribute_UE::
    Get_FinalValue(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity)
    -> FCk_Meter
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <MeterAttribute_Utils, RecordOfMeterAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    return MeterAttribute_Utils::Get_FinalValue(AttributeEntity);
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
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <MeterAttribute_Utils, RecordOfMeterAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);

    ck::UUtils_Signal_OnMeterAttributeValueChanged::Bind(AttributeEntity, InDelegate, InBehavior);
}

auto
    UCk_Utils_MeterAttribute_UE::
    UnbindFrom_OnValueChanged(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity,
        const FCk_Delegate_MeterAttribute_OnValueChanged& InDelegate)
    -> void
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <MeterAttribute_Utils, RecordOfMeterAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);

    ck::UUtils_Signal_OnMeterAttributeValueChanged::Unbind(AttributeEntity, InDelegate);
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

    const auto NewModifierEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttributeOwnerEntity);
    UCk_Utils_GameplayLabel_UE::Add(NewModifierEntity, InModifierName);

    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <UCk_Utils_MeterAttribute_UE::MeterAttribute_Utils, UCk_Utils_MeterAttribute_UE::RecordOfMeterAttributes_Utils>(
            InAttributeOwnerEntity, AttributeName);

    MeterAttributeModifier_Utils::Add
    (
        NewModifierEntity,
        InParams.Get_ModifierDelta(),
        AttributeEntity,
        InParams.Get_ModifierOperation()
    );
}

auto
    UCk_Utils_MeterAttributeModifier_UE::
    Has(
        FGameplayTag InModifierName,
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <UCk_Utils_MeterAttribute_UE::MeterAttribute_Utils, UCk_Utils_MeterAttribute_UE::RecordOfMeterAttributes_Utils>(
            InAttributeOwnerEntity, InAttributeName);
    const auto& AttributeModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <MeterAttributeModifier_Utils, MeterAttributeModifier_Utils::RecordOfAttributeModifiers_Utils>(AttributeEntity, InModifierName);

    return ck::IsValid(AttributeModifierEntity);
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
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <UCk_Utils_MeterAttribute_UE::MeterAttribute_Utils, UCk_Utils_MeterAttribute_UE::RecordOfMeterAttributes_Utils>(
            InAttributeOwnerEntity, InAttributeName);
    const auto& AttributeModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <MeterAttributeModifier_Utils, MeterAttributeModifier_Utils::RecordOfAttributeModifiers_Utils>(AttributeEntity, InModifierName);

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(AttributeModifierEntity);
}

// --------------------------------------------------------------------------------------------------------------------
