#include "CkFloatAttribute_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"

#include "CkEcsBasics/Transform/CkTransform_Fragment.h"

#include "CkLabel/CkLabel_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_FloatAttribute_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Provider_FloatAttributes_ParamsData& InParams)
    -> void
{
    const auto& ParamsProvider = InParams.Get_Provider();

    CK_ENSURE_IF_NOT(ck::IsValid(ParamsProvider), TEXT("Invalid Float Attributes Provider"))
    { return; }

    Add(InHandle, ParamsProvider->Get_Value().Get_AttributeBaseValues());

    UCk_Utils_Ecs_Net_UE::TryAddReplicatedFragment<UCk_Fragment_FloatAttribute_Rep>(InHandle);
}

auto
    UCk_Utils_FloatAttribute_UE::
    Has(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    return ck::IsValid(AttributeEntity);
}

auto
    UCk_Utils_FloatAttribute_UE::
    Has_Any(
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    return RecordOfFloatAttributes_Utils::Has(InAttributeOwnerEntity);
}

auto
    UCk_Utils_FloatAttribute_UE::
    Ensure(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InAttributeName, InAttributeOwnerEntity), TEXT("Handle [{}] does NOT have a Float Attribute [{}]"), InAttributeOwnerEntity, InAttributeName)
    { return false; }

    return true;
}

auto
    UCk_Utils_FloatAttribute_UE::
    Ensure_Any(
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    CK_ENSURE_IF_NOT(Has_Any(InAttributeOwnerEntity), TEXT("Handle [{}] does NOT have any Float Attribute"), InAttributeOwnerEntity)
    { return false; }

    return true;
}

auto
    UCk_Utils_FloatAttribute_UE::
    Get_All(
        FCk_Handle InAttributeOwnerEntity)
    -> TArray<FGameplayTag>
{
    if (NOT RecordOfFloatAttributes_Utils::Has(InAttributeOwnerEntity))
    { return {}; }

    const auto& floatAttributeEntities = RecordOfFloatAttributes_Utils::Get_AllRecordEntries(InAttributeOwnerEntity);

    return ck::algo::Transform<TArray<FGameplayTag>>(floatAttributeEntities, [&](FCk_Handle InFloatAttributeEntity)
    {
        return UCk_Utils_GameplayLabel_UE::Get_Label(InFloatAttributeEntity);
    });
}

auto
    UCk_Utils_FloatAttribute_UE::
    Get_BaseValue(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity)
    -> float
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    return FloatAttribute_Utils::Get_BaseValue(AttributeEntity);
}

auto
    UCk_Utils_FloatAttribute_UE::
    Get_BonusValue(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity)
    -> float
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    return FloatAttribute_Utils::Get_FinalValue(AttributeEntity) - FloatAttribute_Utils::Get_BaseValue(AttributeEntity);
}

auto
    UCk_Utils_FloatAttribute_UE::
    Get_FinalValue(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity)
    -> float
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    return FloatAttribute_Utils::Get_FinalValue(AttributeEntity);
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
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);

    ck::UUtils_Signal_OnFloatAttributeValueChanged::Bind(AttributeEntity, InDelegate, InBehavior);
}

auto
    UCk_Utils_FloatAttribute_UE::
    UnbindFrom_OnValueChanged(
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity,
        const FCk_Delegate_FloatAttribute_OnValueChanged& InDelegate)
    -> void
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);

    ck::UUtils_Signal_OnFloatAttributeValueChanged::Unbind(AttributeEntity, InDelegate);
}

auto
    UCk_Utils_FloatAttribute_UE::
    Add(
        FCk_Handle                InHandle,
        TMap<FGameplayTag, float> InAttributeBaseValues)
    -> void
{
    // TODO: Select Record policy that disallow duplicate based on Gameplay Label
    RecordOfFloatAttributes_Utils::AddIfMissing(InHandle);

    const auto& AddNewFloatAttributeToEntity = [&](FCk_Handle InAttributeOwner, const FGameplayTag& InAttributeName, float InAttributeBaseValue)
    {
        const auto NewAttributeEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttributeOwner);

        ck::UCk_Utils_OwningEntity::Add(NewAttributeEntity, InHandle);

        FloatAttribute_Utils::Add(NewAttributeEntity, InAttributeBaseValue);
        UCk_Utils_GameplayLabel_UE::Add(NewAttributeEntity, InAttributeName);

        RecordOfFloatAttributes_Utils::Request_Connect(InAttributeOwner, NewAttributeEntity);
    };

    for (auto Kvp : InAttributeBaseValues)
    {
        const auto& AttributeName = Kvp.Key;
        const auto& AttributeBaseValue = Kvp.Value;

        AddNewFloatAttributeToEntity(InHandle, AttributeName, AttributeBaseValue);
    }
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
    const auto& AttributeName = InParams.Get_TargetAttributeName();

    const auto NewModifierEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttributeOwnerEntity);
    UCk_Utils_GameplayLabel_UE::Add(NewModifierEntity, InModifierName);

    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <UCk_Utils_FloatAttribute_UE::FloatAttribute_Utils, UCk_Utils_FloatAttribute_UE::RecordOfFloatAttributes_Utils>(
            InAttributeOwnerEntity, AttributeName);

    FloatAttributeModifier_Utils::Add
    (
        NewModifierEntity,
        InParams.Get_ModifierDelta(),
        AttributeEntity,
        InParams.Get_ModifierOperation()
    );

    if (NOT InAttributeOwnerEntity.Has<TObjectPtr<UCk_Fragment_FloatAttribute_Rep>>())
    { return; }

    InAttributeOwnerEntity.Get<TObjectPtr<UCk_Fragment_FloatAttribute_Rep>>()->Broadcast_AddModifier(InModifierName, InParams);
}

auto
    UCk_Utils_FloatAttributeModifier_UE::
    Has(
        FGameplayTag InModifierName,
        FGameplayTag InAttributeName,
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <UCk_Utils_FloatAttribute_UE::FloatAttribute_Utils, UCk_Utils_FloatAttribute_UE::RecordOfFloatAttributes_Utils>(
            InAttributeOwnerEntity, InAttributeName);
    const auto& AttributeModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttributeModifier_Utils, FloatAttributeModifier_Utils::RecordOfAttributeModifiers_Utils>(
            AttributeEntity, InModifierName);

    return ck::IsValid(AttributeModifierEntity);
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
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <UCk_Utils_FloatAttribute_UE::FloatAttribute_Utils, UCk_Utils_FloatAttribute_UE::RecordOfFloatAttributes_Utils>(
            InAttributeOwnerEntity, InAttributeName);
    const auto& AttributeModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttributeModifier_Utils, FloatAttributeModifier_Utils::RecordOfAttributeModifiers_Utils>(
            AttributeEntity, InModifierName);

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(AttributeModifierEntity);

    if (NOT InAttributeOwnerEntity.Has<TObjectPtr<UCk_Fragment_FloatAttribute_Rep>>())
    { return; }

    InAttributeOwnerEntity.Get<TObjectPtr<UCk_Fragment_FloatAttribute_Rep>>()->Broadcast_RemoveModifier(InModifierName, InAttributeName);
}

// --------------------------------------------------------------------------------------------------------------------
