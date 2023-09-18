#include "CkFloatAttribute_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkLabel/CkLabel_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_FloatAttribute_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Provider_FloatAttributes_ParamsData& InParams)
    -> void
{
    // TODO: Select Record policy that disallow duplicate based on Gameplay Label
    RecordOfFloatAttributes_Utils::AddIfMissing(InHandle);

    const auto& paramsProvider = InParams.Get_Provider();

    CK_ENSURE_IF_NOT(ck::IsValid(paramsProvider), TEXT("Invalid Float Attributes Provider"))
    { return; }

    const auto& AddNewFloatAttributeToEntity = [&](FCk_Handle InAttributeOwner, const FGameplayTag& InAttributeName, float InAttributeBaseValue)
    {
        const auto newAttributeEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttributeOwner);

        ck::UCk_Utils_OwningEntity::Add(newAttributeEntity, InHandle);

        FloatAttribute_Utils::Add(newAttributeEntity, InAttributeBaseValue);
        UCk_Utils_GameplayLabel_UE::Add(newAttributeEntity, InAttributeName);

        RecordOfFloatAttributes_Utils::Request_Connect(InAttributeOwner, newAttributeEntity);
    };

    const auto& floatAttributesParams = paramsProvider->Get_Value();

    for (auto kvp : floatAttributesParams.Get_AttributeBaseValues())
    {
        const auto& attributeName = kvp.Key;
        const auto& attributeBaseValue = kvp.Value;

        AddNewFloatAttributeToEntity(InHandle, attributeName, attributeBaseValue);
    }
}

auto
    UCk_Utils_FloatAttribute_UE::
    Has(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    const auto& attributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    return ck::IsValid(attributeEntity);
}

auto
    UCk_Utils_FloatAttribute_UE::
    Ensure(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InAttributeName, InAttributeOwnerEntity), TEXT("Handle [{}] does NOT have a Float Attribute"), InAttributeOwnerEntity)
    { return false; }

    return true;
}

auto
    UCk_Utils_FloatAttribute_UE::
    Get_BaseValue(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity)
    -> float
{
    const auto& attributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    return FloatAttribute_Utils::Get_BaseValue(attributeEntity);
}

auto
    UCk_Utils_FloatAttribute_UE::
    Get_BonusValue(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity)
    -> float
{
    const auto& attributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    return FloatAttribute_Utils::Get_FinalValue(attributeEntity) - FloatAttribute_Utils::Get_BaseValue(attributeEntity);
}

auto
    UCk_Utils_FloatAttribute_UE::
    Get_FinalValue(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity)
    -> float
{
    const auto& attributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    return FloatAttribute_Utils::Get_FinalValue(attributeEntity);
}

auto
    UCk_Utils_FloatAttribute_UE::
    BindTo_OnValueChanged(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity,
        ECk_Signal_BindingPolicy InBehavior,
        const FCk_Delegate_FloatAttribute_OnValueChanged& InDelegate)
    -> void
{
    const auto& attributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);

    ck::UUtils_Signal_OnFloatAttributeValueChanged::Bind(attributeEntity, InDelegate, InBehavior);
}

auto
    UCk_Utils_FloatAttribute_UE::
    UnbindFrom_OnValueChanged(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity,
        const FCk_Delegate_FloatAttribute_OnValueChanged& InDelegate)
    -> void
{
    const auto& attributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);

    ck::UUtils_Signal_OnFloatAttributeValueChanged::Unbind(attributeEntity, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_FloatAttributeModifier_UE::
    Add(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InModifierName,
        const FCk_Fragment_FloatAttributeModifier_ParamsData& InParams)
    -> void
{
    const auto& attributeName = InParams.Get_TargetAttributeName();

    const auto newModifierEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttributeOwnerEntity);
    UCk_Utils_GameplayLabel_UE::Add(newModifierEntity, InModifierName);

    const auto& attributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<UCk_Utils_FloatAttribute_UE::FloatAttribute_Utils, UCk_Utils_FloatAttribute_UE::RecordOfFloatAttributes_Utils>(InAttributeOwnerEntity, attributeName);

    FloatAttributeModifier_Utils::Add
    (
        newModifierEntity,
        InParams.Get_ModifierDelta(),
        attributeEntity,
        InParams.Get_ModifierOperation()
    );
}

auto
    UCk_Utils_FloatAttributeModifier_UE::
    Has(
        FGameplayTag InModifierName,
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    const auto& attributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<UCk_Utils_FloatAttribute_UE::FloatAttribute_Utils, UCk_Utils_FloatAttribute_UE::RecordOfFloatAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    const auto& attributeModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<FloatAttributeModifier_Utils, FloatAttributeModifier_Utils::RecordOfAttributeModifiers_Utils>(attributeEntity, InModifierName);

    return ck::IsValid(attributeModifierEntity);
}

auto
    UCk_Utils_FloatAttributeModifier_UE::
    Ensure(
        FGameplayTag InModifierName,
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InModifierName, InAttributeName, InAttributeOwnerEntity), TEXT("Handle [{}] does NOT have a Float Attribute Modifier with name [{}]"), InAttributeOwnerEntity, InModifierName)
    { return false; }

    return true;
}

auto
    UCk_Utils_FloatAttributeModifier_UE::
    Remove(
        FGameplayTag InModifierName,
        FGameplayTag InAttributeName,
        FCk_Handle   InAttributeOwnerEntity)
    -> void
{
    const auto& attributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<UCk_Utils_FloatAttribute_UE::FloatAttribute_Utils, UCk_Utils_FloatAttribute_UE::RecordOfFloatAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    const auto& attributeModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<FloatAttributeModifier_Utils, FloatAttributeModifier_Utils::RecordOfAttributeModifiers_Utils>(attributeEntity, InModifierName);

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(attributeModifierEntity);
}

// --------------------------------------------------------------------------------------------------------------------
