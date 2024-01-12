#include "CkByteAttribute_Utils.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"

#include "CkEcsBasics/Transform/CkTransform_Fragment.h"

#include "CkLabel/CkLabel_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ByteAttribute_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_ByteAttribute_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> void
{
    RecordOfByteAttributes_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);

    const auto& AddNewByteAttributeToEntity = [&](FCk_Handle InAttributeOwner, const FGameplayTag& InAttributeName, uint8 InAttributeBaseValue)
    {
        const auto NewAttributeEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttributeOwner);

        ByteAttribute_Utils::Add(NewAttributeEntity, InAttributeBaseValue);
        UCk_Utils_GameplayLabel_UE::Add(NewAttributeEntity, InAttributeName);

        RecordOfByteAttributes_Utils::Request_Connect(InAttributeOwner, NewAttributeEntity);
    };

    AddNewByteAttributeToEntity(InHandle, InParams.Get_AttributeName(), InParams.Get_AttributeBaseValue());

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::attribute::VeryVerbose
        (
            TEXT("Skipping creation of Byte Attribute Rep Fragment on Entity [{}] because it's set to [{}]"),
            InHandle,
            InReplicates
        );

        return;
    }

    UCk_Utils_Ecs_Net_UE::TryAddReplicatedFragment<UCk_Fragment_ByteAttribute_Rep>(InHandle);
}

auto
    UCk_Utils_ByteAttribute_UE::
    AddMultiple(
        FCk_Handle InHandle,
        const FCk_Fragment_MultipleByteAttribute_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> void
{
    for (const auto& Param : InParams.Get_ByteAttributeParams())
    {
        Add(InHandle, Param, InReplicates);
    }
}

auto
    UCk_Utils_ByteAttribute_UE::
    Has(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> bool
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <ByteAttribute_Utils, RecordOfByteAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    return ck::IsValid(AttributeEntity);
}

auto
    UCk_Utils_ByteAttribute_UE::
    Has_Any(
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    return RecordOfByteAttributes_Utils::Has(InAttributeOwnerEntity);
}

auto
    UCk_Utils_ByteAttribute_UE::
    Ensure(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InAttributeOwnerEntity, InAttributeName), TEXT("Handle [{}] does NOT have a Byte Attribute [{}]"), InAttributeOwnerEntity, InAttributeName)
    { return false; }

    return true;
}

auto
    UCk_Utils_ByteAttribute_UE::
    Ensure_Any(
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    CK_ENSURE_IF_NOT(Has_Any(InAttributeOwnerEntity), TEXT("Handle [{}] does NOT have any Byte Attribute"), InAttributeOwnerEntity)
    { return false; }

    return true;
}

auto
    UCk_Utils_ByteAttribute_UE::
    ForEach_ByteAttribute(
        FCk_Handle                 InAttributeOwner,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle>
{
    auto ToRet = TArray<FCk_Handle>{};

    ForEach_ByteAttribute(InAttributeOwner, [&](const FCk_Handle& InAttribute)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InAttribute); }
        else
        { ToRet.Emplace(InAttribute); }
    });

    return ToRet;
}

auto
    UCk_Utils_ByteAttribute_UE::
    ForEach_ByteAttribute(
        const FCk_Handle&                  InAttributeOwner,
        const TFunction<void(FCk_Handle)>& InFunc)
    -> void
{
    if (NOT Ensure_Any(InAttributeOwner))
    { return; }

    RecordOfByteAttributes_Utils::ForEach_ValidEntry
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
    UCk_Utils_ByteAttribute_UE::
    ForEach_ByteAttribute_If(
        FCk_Handle                              InAttributeOwner,
        const FCk_Lambda_InHandle&              InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate)
    -> TArray<FCk_Handle>
{
    auto ToRet = TArray<FCk_Handle>{};

    ForEach_ByteAttribute_If
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
    UCk_Utils_ByteAttribute_UE::
    ForEach_ByteAttribute_If(
        FCk_Handle InAttributeOwner,
        const TFunction<void(FCk_Handle)>& InFunc,
        const TFunction<bool(FCk_Handle)>& InPredicate)
    -> void
{
    if (NOT Ensure_Any(InAttributeOwner))
    { return; }

    RecordOfByteAttributes_Utils::ForEach_ValidEntry_If
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
    UCk_Utils_ByteAttribute_UE::
    Get_BaseValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> uint8
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <ByteAttribute_Utils, RecordOfByteAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    return ByteAttribute_Utils::Get_BaseValue(AttributeEntity);
}

auto
    UCk_Utils_ByteAttribute_UE::
    Get_BonusValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> uint8
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <ByteAttribute_Utils, RecordOfByteAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    return ByteAttribute_Utils::Get_FinalValue(AttributeEntity) - ByteAttribute_Utils::Get_BaseValue(AttributeEntity);
}

auto
    UCk_Utils_ByteAttribute_UE::
    Get_FinalValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> uint8
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <ByteAttribute_Utils, RecordOfByteAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
    return ByteAttribute_Utils::Get_FinalValue(AttributeEntity);
}

auto
    UCk_Utils_ByteAttribute_UE::
    BindTo_OnValueChanged(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ByteAttribute_OnValueChanged& InDelegate)
    -> void
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <ByteAttribute_Utils, RecordOfByteAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);

    if (InPostFireBehavior == ECk_Signal_PostFireBehavior::DoNothing)
    { ck::UUtils_Signal_OnByteAttributeValueChanged::Bind(AttributeEntity, InDelegate, InBehavior); }
    else
    { ck::UUtils_Signal_OnByteAttributeValueChanged_PostFireUnbind::Bind(AttributeEntity, InDelegate, InBehavior); }
}

auto
    UCk_Utils_ByteAttribute_UE::
    UnbindFrom_OnValueChanged(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        const FCk_Delegate_ByteAttribute_OnValueChanged& InDelegate)
    -> void
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <ByteAttribute_Utils, RecordOfByteAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);

    ck::UUtils_Signal_OnByteAttributeValueChanged::Unbind(AttributeEntity, InDelegate);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ByteAttributeModifier_UE::
    Add(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InModifierName,
        const FCk_Fragment_ByteAttributeModifier_ParamsData& InParams)
    -> void
{
    const auto& AttributeName = InParams.Get_TargetAttributeName();

    const auto NewModifierEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttributeOwnerEntity);
    UCk_Utils_GameplayLabel_UE::Add(NewModifierEntity, InModifierName);

    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <UCk_Utils_ByteAttribute_UE::ByteAttribute_Utils, UCk_Utils_ByteAttribute_UE::RecordOfByteAttributes_Utils>(
            InAttributeOwnerEntity, AttributeName);

    ByteAttributeModifier_Utils::Add
    (
        NewModifierEntity,
        InParams.Get_ModifierDelta(),
        AttributeEntity,
        InParams.Get_ModifierOperation(),
        InParams.Get_ModifierOperation_RevokablePolicy()
    );

    if (NOT InAttributeOwnerEntity.Has<TObjectPtr<UCk_Fragment_ByteAttribute_Rep>>())
    { return; }

    if (NOT UCk_Utils_Net_UE::Get_HasAuthority(InAttributeOwnerEntity))
    { return; }

    InAttributeOwnerEntity.Get<TObjectPtr<UCk_Fragment_ByteAttribute_Rep>>()->Broadcast_AddModifier(InModifierName, InParams);
}

auto
    UCk_Utils_ByteAttributeModifier_UE::
    Has(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName)
    -> bool
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <UCk_Utils_ByteAttribute_UE::ByteAttribute_Utils, UCk_Utils_ByteAttribute_UE::RecordOfByteAttributes_Utils>(
            InAttributeOwnerEntity, InAttributeName);
    const auto& AttributeModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <ByteAttributeModifier_Utils, ByteAttributeModifier_Utils::RecordOfAttributeModifiers_Utils>(
            AttributeEntity, InModifierName);

    return ck::IsValid(AttributeModifierEntity);
}

auto
    UCk_Utils_ByteAttributeModifier_UE::
    Ensure(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InAttributeOwnerEntity, InAttributeName, InModifierName), TEXT("Handle [{}] does NOT have a Byte Attribute Modifier with name [{}]"), InAttributeOwnerEntity, InModifierName)
    { return false; }

    return true;
}

auto
    UCk_Utils_ByteAttributeModifier_UE::
    Remove(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName)
    -> void
{
    const auto& AttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <UCk_Utils_ByteAttribute_UE::ByteAttribute_Utils, UCk_Utils_ByteAttribute_UE::RecordOfByteAttributes_Utils>(
            InAttributeOwnerEntity, InAttributeName);
    const auto& AttributeModifierEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <ByteAttributeModifier_Utils, ByteAttributeModifier_Utils::RecordOfAttributeModifiers_Utils>(
            AttributeEntity, InModifierName);

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(AttributeModifierEntity);

    if (NOT InAttributeOwnerEntity.Has<TObjectPtr<UCk_Fragment_ByteAttribute_Rep>>())
    { return; }

    if (NOT UCk_Utils_Net_UE::Get_HasAuthority(InAttributeOwnerEntity))
    { return; }

    InAttributeOwnerEntity.Get<TObjectPtr<UCk_Fragment_ByteAttribute_Rep>>()->
        Broadcast_RemoveModifier(InModifierName, InAttributeName);
}

// --------------------------------------------------------------------------------------------------------------------
