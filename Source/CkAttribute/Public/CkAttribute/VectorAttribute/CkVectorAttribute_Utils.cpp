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
        FCk_Handle& InAttributeOwnerEntity,
        const FCk_Fragment_VectorAttribute_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_VectorAttribute
{
    RecordOfVectorAttributes_Utils::AddIfMissing(InAttributeOwnerEntity, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);

    auto NewAttributeEntity = [&]
    {
        auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttributeOwnerEntity);
        return ck::StaticCast<FCk_Handle_VectorAttribute>(NewEntity);
    }();

    VectorAttribute_Utils_Current::Add(NewAttributeEntity, InParams.Get_BaseValue());

    switch (InParams.Get_MinMax())
    {
        case ECk_MinMax::None:
        {
            break;
        }
        case ECk_MinMax::Min:
        {
            VectorAttribute_Utils_Min::Add(NewAttributeEntity, InParams.Get_MinValue());
            break;
        }
        case ECk_MinMax::Max:
        {
            VectorAttribute_Utils_Max::Add(NewAttributeEntity, InParams.Get_MaxValue());
            break;
        }
        case ECk_MinMax::MinMax:
        {
            VectorAttribute_Utils_Min::Add(NewAttributeEntity, InParams.Get_MinValue());
            VectorAttribute_Utils_Max::Add(NewAttributeEntity, InParams.Get_MaxValue());
            break;
        }
    }

    UCk_Utils_GameplayLabel_UE::Add(NewAttributeEntity, InParams.Get_Name());
    RecordOfVectorAttributes_Utils::Request_Connect(InAttributeOwnerEntity, NewAttributeEntity);

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::attribute::VeryVerbose
        (
            TEXT("Skipping creation of Vector Attribute Rep Fragment on Entity [{}] because it's set to [{}]"),
            InAttributeOwnerEntity,
            InReplicates
        );
    }
    else
    {
        UCk_Utils_Ecs_Net_UE::TryAddReplicatedFragment<UCk_Fragment_VectorAttribute_Rep>(InAttributeOwnerEntity);
    }

    return NewAttributeEntity;
}

auto
    UCk_Utils_VectorAttribute_UE::
    AddMultiple(
        FCk_Handle& InHandle,
        const FCk_Fragment_MultipleVectorAttribute_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> TArray<FCk_Handle_VectorAttribute>
{
    return ck::algo::Transform<TArray<FCk_Handle_VectorAttribute>>(
        InParams.Get_VectorAttributeParams(), [&](const FCk_Fragment_VectorAttribute_ParamsData& InParam)
    {
        return Add(InHandle, InParam, InReplicates);
    });
}

auto
    UCk_Utils_VectorAttribute_UE::
    Has_Any(
        const FCk_Handle& InAttributeOwnerEntity)
    -> bool
{
    return RecordOfVectorAttributes_Utils::Has(InAttributeOwnerEntity);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(VectorAttribute, UCk_Utils_VectorAttribute_UE, FCk_Handle_VectorAttribute, ck::FFragment_VectorAttribute_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VectorAttribute_UE::
    TryGet(
        const FCk_Handle& InAttributeOwnerEntity,
        FGameplayTag      InAttributeName)
    -> FCk_Handle_VectorAttribute
{
    CK_ENSURE_IF_NOT(RecordOfVectorAttributes_Utils::Has(InAttributeOwnerEntity),
        TEXT("Cannot get the VectorAttribute with name [{}] from Entity [{}] because it is MISSING the VectorAttribute feature!"),
        InAttributeName, InAttributeOwnerEntity)
    { return {}; }

    return RecordOfVectorAttributes_Utils::Get_ValidEntry_If(InAttributeOwnerEntity,
        ck::algo::MatchesGameplayLabelExact{InAttributeName});
}

auto
    UCk_Utils_VectorAttribute_UE::
    ForEach(
        FCk_Handle& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_VectorAttribute>
{
    auto ToRet = TArray<FCk_Handle_VectorAttribute>{};

    ForEach(InAttributeOwner, [&](const FCk_Handle_VectorAttribute& InAttribute)
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
    ForEach(
        FCk_Handle& InAttributeOwner,
        const TFunction<void(FCk_Handle_VectorAttribute)>& InFunc)
    -> void
{
    RecordOfVectorAttributes_Utils::ForEach_ValidEntry
    (
        InAttributeOwner,
        InFunc,
        ECk_Record_ForEach_Policy::IgnoreRecordMissing
    );
}

auto
    UCk_Utils_VectorAttribute_UE::
    ForEach_If(
        FCk_Handle& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate)
    -> TArray<FCk_Handle_VectorAttribute>
{
    auto ToRet = TArray<FCk_Handle_VectorAttribute>{};

    ForEach_If
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
    ForEach_If(
        FCk_Handle& InAttributeOwner,
        const TFunction<void(FCk_Handle_VectorAttribute)>& InFunc,
        const TFunction<bool(FCk_Handle_VectorAttribute)>& InPredicate)
    -> void
{
    RecordOfVectorAttributes_Utils::ForEach_ValidEntry_If
    (
        InAttributeOwner,
        InFunc,
        InPredicate,
        ECk_Record_ForEach_Policy::IgnoreRecordMissing
    );
}

auto
    UCk_Utils_VectorAttribute_UE::
    Has_Component(
        const FCk_Handle_VectorAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent)
    -> bool
{
    switch (InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            return VectorAttribute_Utils_Min::Has(InAttribute);
        }
        case ECk_MinMaxCurrent::Max:
        {
            return VectorAttribute_Utils_Max::Has(InAttribute);
        }
        case ECk_MinMaxCurrent::Current:
        {
            return VectorAttribute_Utils_Current::Has(InAttribute);
        }
        default:
        {
            CK_INVALID_ENUM(InAttributeComponent);
            return {};
        }
    }
}

auto
    UCk_Utils_VectorAttribute_UE::
    Get_BaseValue(
        const FCk_Handle_VectorAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent)
    -> FVector
{
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Vector Attribute [{}] with Owner [{}] does NOT have a [{}] component"),
        InAttribute,
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute), InAttributeComponent)
    { return {}; }

    switch (InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            return VectorAttribute_Utils_Min::Get_BaseValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Max:
        {
            return VectorAttribute_Utils_Max::Get_BaseValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Current:
        {
            return VectorAttribute_Utils_Current::Get_BaseValue(InAttribute);
        }
        default:
            return {};

    }
}

auto
    UCk_Utils_VectorAttribute_UE::
    Get_BonusValue(
        const FCk_Handle_VectorAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent)
    -> FVector
{
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Vector Attribute [{}] with Owner [{}] does NOT have a [{}] component"),
        InAttribute,
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute), InAttributeComponent)
    { return {}; }

    switch (InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            return VectorAttribute_Utils_Min::Get_FinalValue(InAttribute) - VectorAttribute_Utils_Min::Get_BaseValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Max:
        {
            return VectorAttribute_Utils_Max::Get_FinalValue(InAttribute) - VectorAttribute_Utils_Max::Get_BaseValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Current:
        {
            return VectorAttribute_Utils_Current::Get_FinalValue(InAttribute) - VectorAttribute_Utils_Current::Get_BaseValue(InAttribute);
        }
        default:
            return {};
    }
}

auto
    UCk_Utils_VectorAttribute_UE::
    Get_FinalValue(
        const FCk_Handle_VectorAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent)
    -> FVector
{
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Vector Attribute [{}] with Owner [{}] does NOT have a [{}] component"),
        InAttribute,
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute), InAttributeComponent)
    { return {}; }


    switch (InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            return VectorAttribute_Utils_Min::Get_FinalValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Max:
        {
            return VectorAttribute_Utils_Max::Get_FinalValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Current:
        {
            return VectorAttribute_Utils_Current::Get_FinalValue(InAttribute);
        }
        default:
            return {};
    }
}

auto
    UCk_Utils_VectorAttribute_UE::
    Request_Override(
        UPARAM(ref) FCk_Handle_VectorAttribute& InAttribute,
        FVector InNewBaseValue,
        ECk_MinMaxCurrent InAttributeComponent)
    -> FCk_Handle_VectorAttribute
{
    const auto CurrentBaseValue = Get_BaseValue(InAttribute, InAttributeComponent);
    const auto Delta = InNewBaseValue - CurrentBaseValue;

    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Vector Attribute [{}] with Owner [{}] does NOT have a [{}] component"),
        InAttribute,
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute), InAttributeComponent)
    { return {}; }

    UCk_Utils_VectorAttributeModifier_UE::Add(InAttribute, {}, FCk_Fragment_VectorAttributeModifier_ParamsData{
        Delta, ECk_ArithmeticOperations_Basic::Add, ECk_ModifierOperation_RevocablePolicy::NotRevocable, InAttributeComponent});

    return InAttribute;
}

auto
    UCk_Utils_VectorAttribute_UE::
    BindTo_OnValueChanged(
        FCk_Handle_VectorAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_VectorAttribute_OnValueChanged& InDelegate)
    -> FCk_Handle_VectorAttribute
{
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Vector Attribute [{}] does NOT have a [{}] component. Cannot BIND to OnValueChanged"),
        InAttribute,
        InAttributeComponent)
    { return InAttribute; }

    switch(InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            CK_SIGNAL_BIND(ck::UUtils_Signal_OnVectorAttributeValueChanged_Min, InAttribute, InDelegate, InBehavior, InPostFireBehavior);
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            CK_SIGNAL_BIND(ck::UUtils_Signal_OnVectorAttributeValueChanged_Max, InAttribute, InDelegate, InBehavior, InPostFireBehavior);
            break;
        }
        case ECk_MinMaxCurrent::Current:
        {
            CK_SIGNAL_BIND(ck::UUtils_Signal_OnVectorAttributeValueChanged_Current, InAttribute, InDelegate, InBehavior, InPostFireBehavior);
            break;
        }
    };

    return InAttribute;
}

auto
    UCk_Utils_VectorAttribute_UE::
    UnbindFrom_OnValueChanged(
        FCk_Handle_VectorAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent,
        const FCk_Delegate_VectorAttribute_OnValueChanged& InDelegate)
    -> FCk_Handle_VectorAttribute
{
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Vector Attribute [{}] does NOT have a [{}] component. Cannot UNBIND from OnValueChanged"),
        InAttribute,
        InAttributeComponent)
    { return InAttribute; }

    switch(InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnVectorAttributeValueChanged_Min, InAttribute, InDelegate);
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnVectorAttributeValueChanged_Max, InAttribute, InDelegate);
            break;
        }
        case ECk_MinMaxCurrent::Current:
        {
            CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnVectorAttributeValueChanged_Current, InAttribute, InDelegate);
            break;
        }
    };

    return InAttribute;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_VectorAttributeModifier_UE::
    Add(
        FCk_Handle_VectorAttribute& InAttribute,
        FGameplayTag InModifierName,
        const FCk_Fragment_VectorAttributeModifier_ParamsData& InParams)
    -> FCk_Handle_VectorAttributeModifier
{
    auto ParamsToUse = InParams;
    ParamsToUse.Set_TargetAttributeName(UCk_Utils_GameplayLabel_UE::Get_Label(InAttribute));

    auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute);
    const auto& ModifierOperation = ParamsToUse.Get_ModifierOperation();
    const auto& AttributeComponent = ParamsToUse.Get_Component();

    CK_ENSURE_IF_NOT(UCk_Utils_VectorAttribute_UE::Has_Component(InAttribute, AttributeComponent),
        TEXT("Vector Attribute [{}] with Owner [{}] does NOT have a [{}] component. Cannot Add Modifier"),
        InAttribute,
        LifetimeOwner,
        AttributeComponent)
    { return {}; }

    if (ParamsToUse.Get_ModifierDelta().IsNearlyZero() &&
        (ModifierOperation == ECk_ArithmeticOperations_Basic::Add || ModifierOperation  == ECk_ArithmeticOperations_Basic::Subtract))
    { return {}; }

    if (ParamsToUse.Get_ModifierDelta().Equals(FVector::OneVector) &&
        (ModifierOperation == ECk_ArithmeticOperations_Basic::Add || ModifierOperation  == ECk_ArithmeticOperations_Basic::Subtract))
    { return {}; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttribute);
    auto NewModifierEntity = ck::StaticCast<FCk_Handle_VectorAttributeModifier>(NewEntity);
    UCk_Utils_GameplayLabel_UE::Add(NewModifierEntity, InModifierName);

    switch (AttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            VectorAttributeModifier_Utils_Min::Add
            (
                NewModifierEntity,
                ParamsToUse.Get_ModifierDelta(),
                ParamsToUse.Get_ModifierOperation(),
                ParamsToUse.Get_ModifierOperation_RevocablePolicy()
            );

            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            VectorAttributeModifier_Utils_Max::Add
            (
                NewModifierEntity,
                ParamsToUse.Get_ModifierDelta(),
                ParamsToUse.Get_ModifierOperation(),
                ParamsToUse.Get_ModifierOperation_RevocablePolicy()
            );

            break;
        }
        case ECk_MinMaxCurrent::Current:
        {
            VectorAttributeModifier_Utils_Current::Add
            (
                NewModifierEntity,
                ParamsToUse.Get_ModifierDelta(),
                ParamsToUse.Get_ModifierOperation(),
                ParamsToUse.Get_ModifierOperation_RevocablePolicy()
            );

            break;
        }
    }

    if (NOT UCk_Utils_Ecs_Net_UE::Get_HasReplicatedFragment<UCk_Fragment_VectorAttribute_Rep>(LifetimeOwner))
    { return NewModifierEntity; }

    if (NOT UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(LifetimeOwner))
    { return NewModifierEntity; }

    UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_VectorAttribute_Rep>(
        LifetimeOwner, [&](UCk_Fragment_VectorAttribute_Rep* InRepComp)
    {
        InRepComp->Broadcast_AddModifier(InModifierName, ParamsToUse);
    });

    return NewModifierEntity;
}

auto
    UCk_Utils_VectorAttributeModifier_UE::
    TryGet(
        const FCk_Handle_VectorAttribute& InAttribute,
        FGameplayTag InModifierName,
        ECk_MinMaxCurrent InComponent)
    -> FCk_Handle_VectorAttributeModifier
{
    switch(InComponent)
    {
        case ECk_MinMaxCurrent::Current:
        {
            if (NOT RecordOfVectorAttributeModifiers_Utils_Current::Has(InAttribute))
            { return {}; }

            return RecordOfVectorAttributeModifiers_Utils_Current::Get_ValidEntry_If(InAttribute, ck::algo::MatchesGameplayLabel{InModifierName});
        }
        case ECk_MinMaxCurrent::Min:
        {
            if (NOT RecordOfVectorAttributeModifiers_Utils_Min::Has(InAttribute))
            { return {}; }

            return RecordOfVectorAttributeModifiers_Utils_Min::Get_ValidEntry_If(InAttribute, ck::algo::MatchesGameplayLabel{InModifierName});
        }
        case ECk_MinMaxCurrent::Max:
        {
            if (NOT RecordOfVectorAttributeModifiers_Utils_Max::Has(InAttribute))
            { return {}; }

            return RecordOfVectorAttributeModifiers_Utils_Max::Get_ValidEntry_If(InAttribute, ck::algo::MatchesGameplayLabel{InModifierName});
        }
        default:
            return {};
    }
}

auto
    UCk_Utils_VectorAttributeModifier_UE::
    Remove(
        FCk_Handle_VectorAttributeModifier& InAttributeModifierEntity)
    -> FCk_Handle_VectorAttribute
{
    auto AttributeModifierOwnerEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttributeModifierEntity);
    auto AttributeEntity = UCk_Utils_VectorAttribute_UE::CastChecked(AttributeModifierOwnerEntity);
    auto AttributeOwnerEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(AttributeEntity);

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InAttributeModifierEntity);

    if (NOT UCk_Utils_Ecs_Net_UE::Get_HasReplicatedFragment<UCk_Fragment_VectorAttribute_Rep>(AttributeOwnerEntity))
    { return AttributeEntity; }

    if (NOT UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(AttributeOwnerEntity))
    { return AttributeEntity; }

    UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_VectorAttribute_Rep>(AttributeOwnerEntity,
    [&](UCk_Fragment_VectorAttribute_Rep* InRepComp)
    {
        InRepComp->Broadcast_RemoveModifier(UCk_Utils_GameplayLabel_UE::Get_Label(InAttributeModifierEntity),
            UCk_Utils_GameplayLabel_UE::Get_Label(AttributeEntity));
    });

    return AttributeEntity;
}

auto
    UCk_Utils_VectorAttributeModifier_UE::
    ForEach(
        FCk_Handle_VectorAttribute& InAttributeOwner,
        const FInstancedStruct&    InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        ECk_MinMaxCurrent InAttributeComponent)
    -> TArray<FCk_Handle_VectorAttributeModifier>
{
    auto ToRet = TArray<FCk_Handle_VectorAttributeModifier>{};

    ForEach(InAttributeOwner, [&](const FCk_Handle_VectorAttributeModifier& InAttribute)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InAttribute, InOptionalPayload); }
        else
        { ToRet.Emplace(InAttribute); }
    }, InAttributeComponent);

    return ToRet;
}

auto
    UCk_Utils_VectorAttributeModifier_UE::
    ForEach(
        FCk_Handle_VectorAttribute& InAttribute,
        const TFunction<void(FCk_Handle_VectorAttributeModifier)>& InFunc,
        ECk_MinMaxCurrent InAttributeComponent)
    -> void
{
    switch(InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Current:
        {
            RecordOfVectorAttributeModifiers_Utils_Current::ForEach_ValidEntry
            (
                InAttribute,
                InFunc,
                ECk_Record_ForEach_Policy::IgnoreRecordMissing
            );
            break;
        }
        case ECk_MinMaxCurrent::Min:
        {
            RecordOfVectorAttributeModifiers_Utils_Min::ForEach_ValidEntry
            (
                InAttribute,
                InFunc,
                ECk_Record_ForEach_Policy::IgnoreRecordMissing
            );
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            RecordOfVectorAttributeModifiers_Utils_Max::ForEach_ValidEntry
            (
                InAttribute,
                InFunc,
                ECk_Record_ForEach_Policy::IgnoreRecordMissing
            );
            break;
        }
    }
}

auto
    UCk_Utils_VectorAttributeModifier_UE::
    ForEach_If(
        FCk_Handle_VectorAttribute& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate,
        ECk_MinMaxCurrent InAttributeComponent)
    -> TArray<FCk_Handle_VectorAttributeModifier>
{
    auto ToRet = TArray<FCk_Handle_VectorAttributeModifier>{};

    ForEach_If
    (
        InAttributeOwner,
        [&](FCk_Handle_VectorAttributeModifier InAttribute)
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
        },
        InAttributeComponent
    );

    return ToRet;
}

auto
    UCk_Utils_VectorAttributeModifier_UE::
    ForEach_If(
        FCk_Handle_VectorAttribute& InAttribute,
        const TFunction<void(FCk_Handle_VectorAttributeModifier)>& InFunc,
        const TFunction<bool(FCk_Handle_VectorAttributeModifier)>& InPredicate,
        ECk_MinMaxCurrent InAttributeComponent)
    -> void
{
    switch(InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Current:
        {
            RecordOfVectorAttributeModifiers_Utils_Current::ForEach_ValidEntry_If
            (
                InAttribute,
                InFunc,
                InPredicate,
                ECk_Record_ForEach_Policy::IgnoreRecordMissing
            );
            break;
        }
        case ECk_MinMaxCurrent::Min:
        {
            RecordOfVectorAttributeModifiers_Utils_Min::ForEach_ValidEntry_If
            (
                InAttribute,
                InFunc,
                InPredicate,
                ECk_Record_ForEach_Policy::IgnoreRecordMissing
            );
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            RecordOfVectorAttributeModifiers_Utils_Max::ForEach_ValidEntry_If
            (
                InAttribute,
                InFunc,
                InPredicate,
                ECk_Record_ForEach_Policy::IgnoreRecordMissing
            );
            break;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
