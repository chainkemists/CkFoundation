#include "CkVectorAttribute_Utils.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkEcsBasics/Transform/CkTransform_Fragment.h"

#include "CkLabel/CkLabel_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VectorAttribute_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_VectorAttribute_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle
{
    RecordOfVectorAttributes_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);

    const auto& AddNewVectorAttributeToEntity = [&](FCk_Handle InAttributeOwner, const FGameplayTag& InAttributeName, FVector InAttributeBaseValue)
    {
        auto NewAttributeEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttributeOwner);

        VectorAttribute_Utils::Add(NewAttributeEntity, InAttributeBaseValue);
        UCk_Utils_GameplayLabel_UE::Add(NewAttributeEntity, InAttributeName);

        // TODO: Remove this Cast once we have type-safe API instead of GameplayTags
        auto VectorAttributeEntity = ck::StaticCast<FCk_Handle_VectorAttribute>(NewAttributeEntity);
        RecordOfVectorAttributes_Utils::Request_Connect(InAttributeOwner, VectorAttributeEntity);
    };

    AddNewVectorAttributeToEntity(InHandle, InParams.Get_AttributeName(), InParams.Get_AttributeBaseValue());

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::attribute::VeryVerbose
        (
            TEXT("Skipping creation of Vector Attribute Rep Fragment on Entity [{}] because it's set to [{}]"),
            InHandle,
            InReplicates
        );
    }
    else
    {
        UCk_Utils_Ecs_Net_UE::TryAddReplicatedFragment<UCk_Fragment_VectorAttribute_Rep>(InHandle);
    }

    return InHandle;
}

auto
    UCk_Utils_VectorAttribute_UE::
    AddMultiple(
        FCk_Handle& InHandle,
        const FCk_Fragment_MultipleVectorAttribute_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle
{
    for (const auto& Param : InParams.Get_VectorAttributeParams())
    {
        Add(InHandle, Param, InReplicates);
    }

    return InHandle;
}

auto
    UCk_Utils_VectorAttribute_UE::
    Has_Attribute(
        const FCk_Handle& InAttributeOwnerEntity,
        FGameplayTag      InAttributeName)
    -> bool
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <VectorAttribute_Utils, RecordOfVectorAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    return ck::IsValid(AttributeEntity);
}

auto
    UCk_Utils_VectorAttribute_UE::
    Has_Any_Attribute(
        const FCk_Handle& InAttributeOwnerEntity)
    -> bool
{
    return RecordOfVectorAttributes_Utils::Has(InAttributeOwnerEntity);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VectorAttribute_UE::
    ForEach_VectorAttribute(
        FCk_Handle& InAttributeOwner,
        const FInstancedStruct&    InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle>
{
    auto ToRet = TArray<FCk_Handle>{};

    ForEach_VectorAttribute(InAttributeOwner, [&](const FCk_Handle_VectorAttribute& InAttribute)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InAttribute, InOptionalPayload); }
        else
        { ToRet.Emplace(InAttribute); }
    });

    return ToRet;
}

auto
    UCk_Utils_VectorAttribute_UE::
    ForEach_VectorAttribute(
        FCk_Handle& InAttributeOwner,
        const TFunction<void(FCk_Handle_VectorAttribute)>& InFunc)
    -> void
{
    RecordOfVectorAttributes_Utils::ForEach_ValidEntry
    (
        InAttributeOwner,
        [&](const FCk_Handle_VectorAttribute& InAttribute)
        {
            if (InAttribute == InAttributeOwner)
            { return; }

            InFunc(InAttribute);
        }
    );
}

auto
    UCk_Utils_VectorAttribute_UE::
    ForEach_VectorAttribute_If(
        FCk_Handle& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate)
    -> TArray<FCk_Handle_VectorAttribute>
{
    auto ToRet = TArray<FCk_Handle_VectorAttribute>{};

    ForEach_VectorAttribute_If
    (
        InAttributeOwner,
        [&](FCk_Handle_VectorAttribute InAttribute)
        {
            if (InDelegate.IsBound())
            { InDelegate.Execute(InAttribute, InOptionalPayload); }
            else
            { ToRet.Emplace(InAttribute); }
        },
        [&](const FCk_Handle& InAttribute)  -> bool
        {
            const FCk_SharedBool PredicateResult;

            if (InPredicate.IsBound())
            {
                InPredicate.Execute(InAttribute, PredicateResult, InOptionalPayload);
            }

            return *PredicateResult;

        }
    );

    return ToRet;
}

auto
    UCk_Utils_VectorAttribute_UE::
    ForEach_VectorAttribute_If(
        FCk_Handle& InAttributeOwner,
        const TFunction<void(FCk_Handle_VectorAttribute)>& InFunc,
        const TFunction<bool(FCk_Handle_VectorAttribute)>& InPredicate)
    -> void
{
    RecordOfVectorAttributes_Utils::ForEach_ValidEntry_If
    (
        InAttributeOwner,
        [&](const FCk_Handle_VectorAttribute& InAttribute)
        {
            if (InAttribute == InAttributeOwner)
            { return; }

            InFunc(InAttribute);
        },
        InPredicate
    );
}

auto
    UCk_Utils_VectorAttribute_UE::
    Get_BaseValue(
        const FCk_Handle& InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> FVector
{
    CK_ENSURE_IF_NOT(Has_Attribute(InAttributeOwnerEntity, InAttributeName), TEXT("Attribute [{}] does NOT exist on Entity [{}]."),
        InAttributeName, InAttributeOwnerEntity)
    { return {}; }

    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <VectorAttribute_Utils, RecordOfVectorAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    return VectorAttribute_Utils::Get_BaseValue(AttributeEntity);
}

auto
    UCk_Utils_VectorAttribute_UE::
    Get_BonusValue(
        const FCk_Handle& InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> FVector
{
    CK_ENSURE_IF_NOT(Has_Attribute(InAttributeOwnerEntity, InAttributeName), TEXT("Attribute [{}] does NOT exist on Entity [{}]."),
        InAttributeName, InAttributeOwnerEntity)
    { return {}; }

    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <VectorAttribute_Utils, RecordOfVectorAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    return VectorAttribute_Utils::Get_FinalValue(AttributeEntity) - VectorAttribute_Utils::Get_BaseValue(AttributeEntity);
}

auto
    UCk_Utils_VectorAttribute_UE::
    Get_FinalValue(
        const FCk_Handle& InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> FVector
{
    CK_ENSURE_IF_NOT(Has_Attribute(InAttributeOwnerEntity, InAttributeName), TEXT("Attribute [{}] does NOT exist on Entity [{}]."),
        InAttributeName, InAttributeOwnerEntity)
    { return {}; }

    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <VectorAttribute_Utils, RecordOfVectorAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    return VectorAttribute_Utils::Get_FinalValue(AttributeEntity);
}

auto
    UCk_Utils_VectorAttribute_UE::
    Request_Override(
        FCk_Handle& InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FVector InNewBaseValue)
        -> void
{
    const auto CurrentBaseValue = Get_BaseValue(InAttributeOwnerEntity, InAttributeName);
    const auto Delta = InNewBaseValue - CurrentBaseValue;

    UCk_Utils_VectorAttributeModifier_UE::Add(InAttributeOwnerEntity, {},
        FCk_Fragment_VectorAttributeModifier_ParamsData{
            Delta, InAttributeName, ECk_ModifierOperation::Additive, ECk_ModifierOperation_RevocablePolicy::NotRevocable});
}

auto
    UCk_Utils_VectorAttribute_UE::
    BindTo_OnValueChanged(
        FCk_Handle& InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_VectorAttribute_OnValueChanged& InDelegate)
    -> void
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <VectorAttribute_Utils, RecordOfVectorAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);

    CK_ENSURE_IF_NOT(ck::IsValid(AttributeEntity),
        TEXT("Could NOT find Attribute [{}] in Entity [{}]. Unable to BIND on ValueChanged the Delegate [{}]"),
        InAttributeName, InAttributeOwnerEntity, InDelegate.GetFunctionName())
    { return; }

    if (InPostFireBehavior == ECk_Signal_PostFireBehavior::DoNothing)
    { ck::UUtils_Signal_OnVectorAttributeValueChanged::Bind(AttributeEntity, InDelegate, InBehavior); }
    else
    { ck::UUtils_Signal_OnVectorAttributeValueChanged_PostFireUnbind::Bind(AttributeEntity, InDelegate, InBehavior); }
}

auto
    UCk_Utils_VectorAttribute_UE::
    UnbindFrom_OnValueChanged(
        FCk_Handle& InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        const FCk_Delegate_VectorAttribute_OnValueChanged& InDelegate)
    -> void
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <VectorAttribute_Utils, RecordOfVectorAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);

    CK_ENSURE_IF_NOT(ck::IsValid(AttributeEntity),
        TEXT("Could NOT find Attribute [{}] in Entity [{}]. Unable to BIND on ValueChanged the Delegate [{}]"),
        InAttributeName, InAttributeOwnerEntity, InDelegate.GetFunctionName())
    { return; }

    ck::UUtils_Signal_OnVectorAttributeValueChanged::Unbind(AttributeEntity, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VectorAttributeModifier_UE::
    Add(
        FCk_Handle& InAttributeOwnerEntity,
        FGameplayTag InModifierName,
        const FCk_Fragment_VectorAttributeModifier_ParamsData& InParams)
    -> void
{
    const auto& AttributeName = InParams.Get_TargetAttributeName();

    if (InParams.Get_ModifierDelta().IsNearlyZero() && InParams.Get_ModifierOperation() == ECk_ModifierOperation::Additive)
    { return; }

    if (InParams.Get_ModifierDelta() == FVector::OneVector && InParams.Get_ModifierOperation() == ECk_ModifierOperation::Multiplicative)
    { return; }

    const auto NewModifierEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttributeOwnerEntity);
    UCk_Utils_GameplayLabel_UE::Add(NewModifierEntity, InModifierName);

    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <UCk_Utils_VectorAttribute_UE::VectorAttribute_Utils, UCk_Utils_VectorAttribute_UE::RecordOfVectorAttributes_Utils>(
            InAttributeOwnerEntity, AttributeName);

    VectorAttributeModifier_Utils::Add
    (
        NewModifierEntity,
        InParams.Get_ModifierDelta(),
        AttributeEntity,
        InParams.Get_ModifierOperation(),
        InParams.Get_ModifierOperation_RevokablePolicy()
    );

    if (NOT InAttributeOwnerEntity.Has<TObjectPtr<UCk_Fragment_VectorAttribute_Rep>>())
    { return; }

    if (NOT UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InAttributeOwnerEntity))
    { return; }

    InAttributeOwnerEntity.Get<TObjectPtr<UCk_Fragment_VectorAttribute_Rep>>()->Broadcast_AddModifier(InModifierName, InParams);
}

auto
    UCk_Utils_VectorAttributeModifier_UE::
    Has(
        const FCk_Handle& InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName)
    -> bool
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <UCk_Utils_VectorAttribute_UE::VectorAttribute_Utils, UCk_Utils_VectorAttribute_UE::RecordOfVectorAttributes_Utils>(
            InAttributeOwnerEntity, InAttributeName);
    const auto& AttributeModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <VectorAttributeModifier_Utils, VectorAttributeModifier_Utils::RecordOfAttributeModifiers_Utils>(
            AttributeEntity, InModifierName);

    return ck::IsValid(AttributeModifierEntity);
}

auto
    UCk_Utils_VectorAttributeModifier_UE::
    Remove(
        FCk_Handle& InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName)
    -> void
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <UCk_Utils_VectorAttribute_UE::VectorAttribute_Utils, UCk_Utils_VectorAttribute_UE::RecordOfVectorAttributes_Utils>(
            InAttributeOwnerEntity, InAttributeName);
    const auto& AttributeModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <VectorAttributeModifier_Utils, VectorAttributeModifier_Utils::RecordOfAttributeModifiers_Utils>(
            AttributeEntity, InModifierName);

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(AttributeModifierEntity);

    if (NOT InAttributeOwnerEntity.Has<TObjectPtr<UCk_Fragment_VectorAttribute_Rep>>())
    { return; }

    if (NOT UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InAttributeOwnerEntity))
    { return; }

    InAttributeOwnerEntity.Get<TObjectPtr<UCk_Fragment_VectorAttribute_Rep>>()->
        Broadcast_RemoveModifier(InModifierName, InAttributeName);
}

// --------------------------------------------------------------------------------------------------------------------
