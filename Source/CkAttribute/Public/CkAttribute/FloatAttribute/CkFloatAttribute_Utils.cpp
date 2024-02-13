#include "CkFloatAttribute_Utils.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkEcsBasics/Transform/CkTransform_Fragment.h"

#include "CkLabel/CkLabel_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_FloatAttribute_UE::
    Add(
        FCk_Handle& InAttributeOwnerEntity,
        const FCk_Fragment_FloatAttribute_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle
{
    RecordOfFloatAttributes_Utils::AddIfMissing(InAttributeOwnerEntity, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);

    // TODO: Attribute<T> should be taking Handle by ref and this NewAttributeEntity variable should NOT be const
    auto NewAttributeEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttributeOwnerEntity);

    FloatAttribute_Utils_Current::Add(NewAttributeEntity, InParams.Get_BaseValue());

    switch (InParams.Get_Component())
    {
        case ECk_MinMax_Mask::None:
        {
            break;
        }
        case ECk_MinMax_Mask::Min:
        {
            FloatAttribute_Utils_Min::Add(NewAttributeEntity, InParams.Get_BaseValue());
            break;
        }
        case ECk_MinMax_Mask::Max:
        {
            FloatAttribute_Utils_Max::Add(NewAttributeEntity, InParams.Get_BaseValue());
            break;
        }
        case ECk_MinMax_Mask::MinMax:
        {
            FloatAttribute_Utils_Min::Add(NewAttributeEntity, InParams.Get_BaseValue());
            FloatAttribute_Utils_Max::Add(NewAttributeEntity, InParams.Get_BaseValue());
            break;
        }
    }

    UCk_Utils_GameplayLabel_UE::Add(NewAttributeEntity, InParams.Get_Name());

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::attribute::VeryVerbose
        (
            TEXT("Skipping creation of Float Attribute Rep Fragment on Entity [{}] because it's set to [{}]"),
            InAttributeOwnerEntity,
            InReplicates
        );
    }
    else
    {
        UCk_Utils_Ecs_Net_UE::TryAddReplicatedFragment<UCk_Fragment_FloatAttribute_Rep>(InAttributeOwnerEntity);
    }

    return InAttributeOwnerEntity;
}

auto
    UCk_Utils_FloatAttribute_UE::
    AddMultiple(
        FCk_Handle& InHandle,
        const FCk_Fragment_MultipleFloatAttribute_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle
{
    for (const auto& Param : InParams.Get_FloatAttributeParams())
    {
        Add(InHandle, Param, InReplicates);
    }

    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(FloatAttribute, UCk_Utils_FloatAttribute_UE,
    FCk_Handle_FloatAttribute, UCk_Utils_FloatAttribute_UE::RecordOfFloatAttributes_Utils::RecordType);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_FloatAttribute_UE::
    ForEach_FloatAttribute(
        FCk_Handle& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_FloatAttribute>
{
    auto ToRet = TArray<FCk_Handle_FloatAttribute>{};

    ForEach_FloatAttribute(InAttributeOwner, [&](const FCk_Handle_FloatAttribute& InAttribute)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InAttribute, InOptionalPayload); }
        else
        { ToRet.Emplace(InAttribute); }
    });

    return ToRet;
}

auto
    UCk_Utils_FloatAttribute_UE::
    ForEach_FloatAttribute(
        FCk_Handle& InAttributeOwner,
        const TFunction<void(FCk_Handle_FloatAttribute)>& InFunc)
    -> void
{
    RecordOfFloatAttributes_Utils::ForEach_ValidEntry
    (
        InAttributeOwner,
        [&](const FCk_Handle_FloatAttribute& InAttribute)
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
        FCk_Handle& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate)
    -> TArray<FCk_Handle_FloatAttribute>
{
    auto ToRet = TArray<FCk_Handle_FloatAttribute>{};

    ForEach_FloatAttribute_If
    (
        InAttributeOwner,
        [&](FCk_Handle_FloatAttribute InAttribute)
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
    UCk_Utils_FloatAttribute_UE::
    ForEach_FloatAttribute_If(
        FCk_Handle& InAttributeOwner,
        const TFunction<void(FCk_Handle_FloatAttribute)>& InFunc,
        const TFunction<bool(FCk_Handle_FloatAttribute)>& InPredicate)
    -> void
{
    RecordOfFloatAttributes_Utils::ForEach_ValidEntry_If
    (
        InAttributeOwner,
        [&](const FCk_Handle_FloatAttribute& InAttribute)
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
    Get_BaseValue(
        const FCk_Handle_FloatAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent)
    -> float
{
    switch (InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            CK_ENSURE_IF_NOT(Has_BaseValue(InAttribute, InAttributeComponent),
                TEXT("Float Attribute [{}] with Owner [{}] does NOT have a [{}] component"),
                InAttribute,
                UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute), InAttributeComponent)
            { return {}; }

            return FloatAttribute_Utils_Min::Get_BaseValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Max:
        {
            CK_ENSURE_IF_NOT(Has_BaseValue(InAttribute, InAttributeComponent),
                TEXT("Float Attribute [{}] with Owner [{}] does NOT have a MAX component"),
                InAttribute,
                UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute))
            { return {}; }

            return FloatAttribute_Utils_Max::Get_BaseValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Current:
        {
            return FloatAttribute_Utils_Current::Get_BaseValue(InAttribute);
        }
        default:
            return {};

    }
}

auto
    UCk_Utils_FloatAttribute_UE::
    Get_BonusValue(
        const FCk_Handle_FloatAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent)
    -> float
{
    switch (InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            CK_ENSURE_IF_NOT(Has_BaseValue(InAttribute, InAttributeComponent),
                TEXT("Float Attribute [{}] with Owner [{}] does NOT have a [{}] component"),
                InAttribute,
                UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute), InAttributeComponent)
            { return {}; }

            return FloatAttribute_Utils_Min::Get_FinalValue(InAttribute) - FloatAttribute_Utils_Min::Get_BaseValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Max:
        {
            CK_ENSURE_IF_NOT(Has_BaseValue(InAttribute, InAttributeComponent),
                TEXT("Float Attribute [{}] with Owner [{}] does NOT have a MAX component"),
                InAttribute,
                UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute))
            { return {}; }

            return FloatAttribute_Utils_Max::Get_FinalValue(InAttribute) - FloatAttribute_Utils_Max::Get_BaseValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Current:
        {
            return FloatAttribute_Utils_Current::Get_FinalValue(InAttribute) - FloatAttribute_Utils_Current::Get_BaseValue(InAttribute);
        }
        default:
            return {};
    }
}

auto
    UCk_Utils_FloatAttribute_UE::
    Get_FinalValue(
        const FCk_Handle_FloatAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent)
    -> float
{
    switch (InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            CK_ENSURE_IF_NOT(Has_BaseValue(InAttribute, InAttributeComponent),
                TEXT("Float Attribute [{}] with Owner [{}] does NOT have a [{}] component"),
                InAttribute,
                UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute), InAttributeComponent)
            { return {}; }

            return FloatAttribute_Utils_Min::Get_FinalValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Max:
        {
            CK_ENSURE_IF_NOT(Has_BaseValue(InAttribute, InAttributeComponent),
                TEXT("Float Attribute [{}] with Owner [{}] does NOT have a MAX component"),
                InAttribute,
                UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute))
            { return {}; }

            return FloatAttribute_Utils_Max::Get_FinalValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Current:
        {
            return FloatAttribute_Utils_Current::Get_FinalValue(InAttribute);
        }
        default:
            return {};
    }
}

auto
    UCk_Utils_FloatAttribute_UE::
    Request_Override(
        UPARAM(ref) FCk_Handle_FloatAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent,
        float InNewBaseValue)
    -> FCk_Handle_FloatAttribute
{
    const auto CurrentBaseValue = Get_BaseValue(InAttribute, InAttributeComponent);
    const auto Delta = InNewBaseValue - CurrentBaseValue;

    CK_ENSURE_IF_NOT(Has_BaseValue(InAttribute, InAttributeComponent),
        TEXT("Float Attribute [{}] with Owner [{}] does NOT have a [{}] component"),
        InAttribute,
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute), InAttributeComponent)
    { return {}; }

    UCk_Utils_FloatAttributeModifier_UE::Add(InAttribute, {}, FCk_Fragment_FloatAttributeModifier_ParamsData{
        Delta, ECk_ModifierOperation::Additive, ECk_ModifierOperation_RevocablePolicy::NotRevocable, InAttributeComponent});

    return InAttribute;
}

auto
    UCk_Utils_FloatAttribute_UE::
    BindTo_OnValueChanged(
        FCk_Handle_FloatAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_FloatAttribute_OnValueChanged& InDelegate)
    -> FCk_Handle_FloatAttribute
{
    switch(InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            CK_SIGNAL_BIND(InAttribute, InDelegate, ck::UUtils_Signal_OnFloatAttributeValueChanged_Min, InBehavior, InPostFireBehavior);
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            CK_SIGNAL_BIND(InAttribute, InDelegate, ck::UUtils_Signal_OnFloatAttributeValueChanged_Max, InBehavior, InPostFireBehavior);
            break;
        }
        case ECk_MinMaxCurrent::Current:
        {
            CK_SIGNAL_BIND(InAttribute, InDelegate, ck::UUtils_Signal_OnFloatAttributeValueChanged_Current, InBehavior, InPostFireBehavior);
            break;
        }
    };

    return InAttribute;
}

auto
    UCk_Utils_FloatAttribute_UE::
    UnbindFrom_OnValueChanged(
        FCk_Handle_FloatAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent,
        const FCk_Delegate_FloatAttribute_OnValueChanged& InDelegate)
    -> FCk_Handle_FloatAttribute
{
    switch(InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            CK_SIGNAL_UNBIND(InAttribute, InDelegate, ck::UUtils_Signal_OnFloatAttributeValueChanged_Min);
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            CK_SIGNAL_UNBIND(InAttribute, InDelegate, ck::UUtils_Signal_OnFloatAttributeValueChanged_Max);
            break;
        }
        case ECk_MinMaxCurrent::Current:
        {
            CK_SIGNAL_UNBIND(InAttribute, InDelegate, ck::UUtils_Signal_OnFloatAttributeValueChanged_Current);
            break;
        }
    };

    return InAttribute;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_FloatAttributeModifier_UE::
    Add(
        FCk_Handle_FloatAttribute& InAttribute,
        FGameplayTag InModifierName,
        const FCk_Fragment_FloatAttributeModifier_ParamsData& InParams)
    -> FCk_Handle_FloatAttributeModifier
{
    auto ParamsToUse = InParams;
    ParamsToUse.Set_TargetAttributeName(UCk_Utils_GameplayLabel_UE::Get_Label(InAttribute));

    const auto& LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute);

    if (ParamsToUse.Get_ModifierDelta() == 0 && ParamsToUse.Get_ModifierOperation() == ECk_ModifierOperation::Additive)
    { return {}; }

    if (ParamsToUse.Get_ModifierDelta() == 1 && ParamsToUse.Get_ModifierOperation() == ECk_ModifierOperation::Multiplicative)
    { return {}; }

    auto NewModifierEntity = ck::StaticCast<FCk_Handle_FloatAttributeModifier>(UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttribute));
    UCk_Utils_GameplayLabel_UE::Add(NewModifierEntity, InModifierName);

    switch (ParamsToUse.Get_Component())
    {
        case ECk_MinMaxCurrent::Min:
        {
            RecordOfFloatAttributeModifiers_Utils_Min::AddIfMissing(InAttribute);
            RecordOfFloatAttributeModifiers_Utils_Min::Request_Connect(InAttribute, NewModifierEntity);

            FloatAttributeModifier_Utils_Min::Add
            (
                NewModifierEntity,
                ParamsToUse.Get_ModifierDelta(),
                InAttribute,
                ParamsToUse.Get_ModifierOperation(),
                ParamsToUse.Get_ModifierOperation_RevocablePolicy()
            );

            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            RecordOfFloatAttributeModifiers_Utils_Max::AddIfMissing(InAttribute);
            RecordOfFloatAttributeModifiers_Utils_Max::Request_Connect(InAttribute, NewModifierEntity);

            FloatAttributeModifier_Utils_Max::Add
            (
                NewModifierEntity,
                ParamsToUse.Get_ModifierDelta(),
                InAttribute,
                ParamsToUse.Get_ModifierOperation(),
                ParamsToUse.Get_ModifierOperation_RevocablePolicy()
            );

            break;
        }
        case ECk_MinMaxCurrent::Current:
        {
            RecordOfFloatAttributeModifiers_Utils_Current::AddIfMissing(InAttribute);
            RecordOfFloatAttributeModifiers_Utils_Current::Request_Connect(InAttribute, NewModifierEntity);

            FloatAttributeModifier_Utils_Current::Add
            (
                NewModifierEntity,
                ParamsToUse.Get_ModifierDelta(),
                InAttribute,
                ParamsToUse.Get_ModifierOperation(),
                ParamsToUse.Get_ModifierOperation_RevocablePolicy()
            );

            break;

        }
    }

    if (NOT LifetimeOwner.Has<TObjectPtr<UCk_Fragment_FloatAttribute_Rep>>())
    { return NewModifierEntity; }

    if (NOT UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(LifetimeOwner))
    { return NewModifierEntity; }

    LifetimeOwner.Get<TObjectPtr<UCk_Fragment_FloatAttribute_Rep>>()->Broadcast_AddModifier(InModifierName, ParamsToUse);
    return NewModifierEntity;
}

auto
    UCk_Utils_FloatAttributeModifier_UE::
    TryGet(
        const FCk_Handle_FloatAttribute& InAttribute,
        FGameplayTag InModifierName,
        ECk_MinMaxCurrent _Component)
    -> FCk_Handle_FloatAttributeModifier
{
    switch(_Component)
    {
        case ECk_MinMaxCurrent::Min:
            return RecordOfFloatAttributeModifiers_Utils_Min::Get_ValidEntry_If(InAttribute, ck::algo::MatchesGameplayLabel{InModifierName});
        case ECk_MinMaxCurrent::Max:
            return RecordOfFloatAttributeModifiers_Utils_Max::Get_ValidEntry_If(InAttribute, ck::algo::MatchesGameplayLabel{InModifierName});
        case ECk_MinMaxCurrent::Current:
            return RecordOfFloatAttributeModifiers_Utils_Current::Get_ValidEntry_If(InAttribute, ck::algo::MatchesGameplayLabel{InModifierName});
        default:
            return {};
    }
}

auto
    UCk_Utils_FloatAttributeModifier_UE::
    Remove(
        FCk_Handle_FloatAttributeModifier& InAttributeModifierEntity)
    -> FCk_Handle_FloatAttribute
{
    const auto& AttributeEntity = UCk_Utils_FloatAttribute_UE::CastChecked(
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttributeModifierEntity));
    const auto& AttributeOwnerEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(AttributeEntity);

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InAttributeModifierEntity);

    if (NOT AttributeOwnerEntity.Has<TObjectPtr<UCk_Fragment_FloatAttribute_Rep>>())
    { return AttributeEntity; }

    if (NOT UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(AttributeOwnerEntity))
    { return AttributeEntity; }

    AttributeOwnerEntity.Get<TObjectPtr<UCk_Fragment_FloatAttribute_Rep>>()->Broadcast_RemoveModifier
    (
        UCk_Utils_GameplayLabel_UE::Get_Label(InAttributeModifierEntity),
            UCk_Utils_GameplayLabel_UE::Get_Label(AttributeEntity)
    );

    return AttributeEntity;
}

// --------------------------------------------------------------------------------------------------------------------
