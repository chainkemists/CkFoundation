#include "CkFloatAttribute_Utils.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"

#include "CkEcsBasics/Transform/CkTransform_Fragment.h"

#include "CkLabel/CkLabel_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_FloatAttribute_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_FloatAttribute_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> void
{
    RecordOfFloatAttributes_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);

    const auto& AddNewFloatAttributeToEntity =
    [&](const FCk_Handle& InAttributeOwner, const FGameplayTag& InAttributeName, float InAttributeBaseValue)
    {
        const auto NewAttributeEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttributeOwner);

        FloatAttribute_Utils::Add(NewAttributeEntity, InAttributeBaseValue);
        UCk_Utils_GameplayLabel_UE::Add(NewAttributeEntity, InAttributeName);

        RecordOfFloatAttributes_Utils::Request_Connect(InAttributeOwner, NewAttributeEntity);
    };

    AddNewFloatAttributeToEntity(InHandle, InParams.Get_AttributeName(), InParams.Get_AttributeBaseValue());

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::attribute::VeryVerbose
        (
            TEXT("Skipping creation of Float Attribute Rep Fragment on Entity [{}] because it's set to [{}]"),
            InHandle,
            InReplicates
        );

        return;
    }

    UCk_Utils_Ecs_Net_UE::TryAddReplicatedFragment<UCk_Fragment_FloatAttribute_Rep>(InHandle);
}

auto
    UCk_Utils_FloatAttribute_UE::
    AddMultiple(
        FCk_Handle InHandle,
        const FCk_Fragment_MultipleFloatAttribute_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> void
{
    for (const auto& Param : InParams.Get_FloatAttributeParams())
    {
        Add(InHandle, Param, InReplicates);
    }
}

auto
    UCk_Utils_FloatAttribute_UE::
    Has(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
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
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InAttributeOwnerEntity, InAttributeName), TEXT("Handle [{}] does NOT have a Float Attribute [{}]"), InAttributeOwnerEntity, InAttributeName)
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
    ForEach_FloatAttribute(
        FCk_Handle                 InAttributeOwner,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle>
{
    auto ToRet = TArray<FCk_Handle>{};

    ForEach_FloatAttribute(InAttributeOwner, [&](const FCk_Handle& InAttribute)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InAttribute); }
        else
        { ToRet.Emplace(InAttribute); }
    });

    return ToRet;
}

auto
    UCk_Utils_FloatAttribute_UE::
    ForEach_FloatAttribute(
        const FCk_Handle&                  InAttributeOwner,
        const TFunction<void(FCk_Handle)>& InFunc)
    -> void
{
    if (NOT Ensure_Any(InAttributeOwner))
    { return; }

    RecordOfFloatAttributes_Utils::ForEach_ValidEntry
    (
        InAttributeOwner,
        [&](const FCk_Handle& InAttribute)
        {
            if (InAttribute == InAttributeOwner)
            { return; }

            InFunc(InAttribute);
        }
    );
}

auto
    UCk_Utils_FloatAttribute_UE::
    ForEach_FloatAttribute_If(
        FCk_Handle                              InAttributeOwner,
        const FCk_Lambda_InHandle&              InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate)
    -> TArray<FCk_Handle>
{
    auto ToRet = TArray<FCk_Handle>{};

    ForEach_FloatAttribute_If
    (
        InAttributeOwner,
        [&](FCk_Handle InAttribute)
        {
            if (InDelegate.IsBound())
            { InDelegate.Execute(InAttribute); }
            else
            { ToRet.Emplace(InAttribute); }
        },
        [&](const FCk_Handle& InAttribute)  -> bool
        {
            const FCk_SharedBool PredicateResult;

            if (InPredicate.IsBound())
            {
                InPredicate.Execute(InAttribute, PredicateResult);
            }

            return *PredicateResult;

        }
    );

    return ToRet;
}

auto
    UCk_Utils_FloatAttribute_UE::
    ForEach_FloatAttribute_If(
        const FCk_Handle&                  InAttributeOwner,
        const TFunction<void(FCk_Handle)>& InFunc,
        const TFunction<bool(FCk_Handle)>& InPredicate)
    -> void
{
    if (NOT Ensure_Any(InAttributeOwner))
    { return; }

    RecordOfFloatAttributes_Utils::ForEach_ValidEntry_If
    (
        InAttributeOwner,
        [&](const FCk_Handle& InAttribute)
        {
            if (InAttribute == InAttributeOwner)
            { return; }

            InFunc(InAttribute);
        },
        InPredicate
    );
}

auto
    UCk_Utils_FloatAttribute_UE::
    Get_All(
        FCk_Handle InAttributeOwnerEntity)
    -> TArray<FGameplayTag>
{
    if (NOT RecordOfFloatAttributes_Utils::Has(InAttributeOwnerEntity))
    { return {}; }

    auto AllAttributes = TArray<FGameplayTag>{};

    RecordOfFloatAttributes_Utils::ForEach_ValidEntry(InAttributeOwnerEntity, [&](FCk_Handle InFloatAttributeEntity)
    {
        AllAttributes.Add(UCk_Utils_GameplayLabel_UE::Get_Label(InFloatAttributeEntity));
    });

    return AllAttributes;
}

auto
    UCk_Utils_FloatAttribute_UE::
    Get_BaseValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> float
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    return FloatAttribute_Utils::Get_BaseValue(AttributeEntity);
}

auto
    UCk_Utils_FloatAttribute_UE::
    Get_BonusValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> float
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    return FloatAttribute_Utils::Get_FinalValue(AttributeEntity) - FloatAttribute_Utils::Get_BaseValue(AttributeEntity);
}

auto
    UCk_Utils_FloatAttribute_UE::
    Get_FinalValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> float
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    return FloatAttribute_Utils::Get_FinalValue(AttributeEntity);
}

auto
    UCk_Utils_FloatAttribute_UE::
    BindTo_OnValueChanged(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_FloatAttribute_OnValueChanged& InDelegate)
    -> void
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);

    if (InPostFireBehavior == ECk_Signal_PostFireBehavior::DoNothing)
    { ck::UUtils_Signal_OnFloatAttributeValueChanged::Bind(AttributeEntity, InDelegate, InBehavior); }
    else
    { ck::UUtils_Signal_OnFloatAttributeValueChanged_PostFireUnbind::Bind(AttributeEntity, InDelegate, InBehavior); }
}

auto
    UCk_Utils_FloatAttribute_UE::
    UnbindFrom_OnValueChanged(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        const FCk_Delegate_FloatAttribute_OnValueChanged& InDelegate)
    -> void
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);

    ck::UUtils_Signal_OnFloatAttributeValueChanged::Unbind(AttributeEntity, InDelegate);
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
        InParams.Get_ModifierOperation(),
        InParams.Get_ModifierOperation_RevokablePolicy()
    );

    if (NOT InAttributeOwnerEntity.Has<TObjectPtr<UCk_Fragment_FloatAttribute_Rep>>())
    { return; }

    if (NOT UCk_Utils_Net_UE::Get_HasAuthority(InAttributeOwnerEntity))
    { return; }

    InAttributeOwnerEntity.Get<TObjectPtr<UCk_Fragment_FloatAttribute_Rep>>()->Broadcast_AddModifier(InModifierName, InParams);
}

auto
    UCk_Utils_FloatAttributeModifier_UE::
    Has(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName)
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
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InAttributeOwnerEntity, InAttributeName, InModifierName), TEXT("Handle [{}] does NOT have a Float Attribute Modifier with name [{}]"), InAttributeOwnerEntity, InModifierName)
    { return false; }

    return true;
}

auto
    UCk_Utils_FloatAttributeModifier_UE::
    Remove(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName)
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

    if (NOT UCk_Utils_Net_UE::Get_HasAuthority(InAttributeOwnerEntity))
    { return; }

    InAttributeOwnerEntity.Get<TObjectPtr<UCk_Fragment_FloatAttribute_Rep>>()->
        Broadcast_RemoveModifier(InModifierName, InAttributeName);
}

// --------------------------------------------------------------------------------------------------------------------
