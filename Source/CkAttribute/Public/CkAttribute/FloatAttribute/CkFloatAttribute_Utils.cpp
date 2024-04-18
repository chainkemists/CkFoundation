#include "CkFloatAttribute_Utils.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"

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
    -> FCk_Handle_FloatAttribute
{
    RecordOfFloatAttributes_Utils::AddIfMissing(InAttributeOwnerEntity, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);

    auto NewAttributeEntity = [&]
    {
        auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttributeOwnerEntity);
        return ck::StaticCast<FCk_Handle_FloatAttribute>(NewEntity);
    }();

    FloatAttribute_Utils_Current::Add(NewAttributeEntity, InParams.Get_BaseValue());

    switch (InParams.Get_MinMax())
    {
        case ECk_MinMax::None:
        {
            break;
        }
        case ECk_MinMax::Min:
        {
            FloatAttribute_Utils_Min::Add(NewAttributeEntity, InParams.Get_MinValue());
            break;
        }
        case ECk_MinMax::Max:
        {
            FloatAttribute_Utils_Max::Add(NewAttributeEntity, InParams.Get_MaxValue());
            break;
        }
        case ECk_MinMax::MinMax:
        {
            FloatAttribute_Utils_Min::Add(NewAttributeEntity, InParams.Get_MinValue());
            FloatAttribute_Utils_Max::Add(NewAttributeEntity, InParams.Get_MaxValue());
            break;
        }
    }

    UCk_Utils_GameplayLabel_UE::Add(NewAttributeEntity, InParams.Get_Name());
    RecordOfFloatAttributes_Utils::Request_Connect(InAttributeOwnerEntity, NewAttributeEntity);

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

    return NewAttributeEntity;
}

auto
    UCk_Utils_FloatAttribute_UE::
    AddMultiple(
        FCk_Handle& InHandle,
        const FCk_Fragment_MultipleFloatAttribute_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> TArray<FCk_Handle_FloatAttribute>
{
    return ck::algo::Transform<TArray<FCk_Handle_FloatAttribute>>(
        InParams.Get_FloatAttributeParams(), [&](const FCk_Fragment_FloatAttribute_ParamsData& InParam)
    {
        return Add(InHandle, InParam, InReplicates);
    });
}

auto
    UCk_Utils_FloatAttribute_UE::
    Has_Any(
        const FCk_Handle& InAttributeOwnerEntity)
    -> bool
{
    return RecordOfFloatAttributes_Utils::Has(InAttributeOwnerEntity);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(FloatAttribute, UCk_Utils_FloatAttribute_UE, FCk_Handle_FloatAttribute, ck::FFragment_FloatAttribute_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_FloatAttribute_UE::
    TryGet(
        const FCk_Handle& InAttributeOwnerEntity,
        FGameplayTag      InAttributeName)
    -> FCk_Handle_FloatAttribute
{
    return RecordOfFloatAttributes_Utils::Get_ValidEntry_If(InAttributeOwnerEntity,
        ck::algo::MatchesGameplayLabelExact{InAttributeName});
}

auto
    UCk_Utils_FloatAttribute_UE::
    ForEach(
        FCk_Handle& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_FloatAttribute>
{
    auto ToRet = TArray<FCk_Handle_FloatAttribute>{};

    ForEach(InAttributeOwner, [&](const FCk_Handle_FloatAttribute& InAttribute)
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
    ForEach(
        FCk_Handle& InAttributeOwner,
        const TFunction<void(FCk_Handle_FloatAttribute)>& InFunc)
    -> void
{
    RecordOfFloatAttributes_Utils::ForEach_ValidEntry
    (
        InAttributeOwner,
        InFunc,
        ECk_Record_ForEach_Policy::IgnoreRecordMissing
    );
}

auto
    UCk_Utils_FloatAttribute_UE::
    ForEach_If(
        FCk_Handle& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate)
    -> TArray<FCk_Handle_FloatAttribute>
{
    auto ToRet = TArray<FCk_Handle_FloatAttribute>{};

    ForEach_If
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
    ForEach_If(
        FCk_Handle& InAttributeOwner,
        const TFunction<void(FCk_Handle_FloatAttribute)>& InFunc,
        const TFunction<bool(FCk_Handle_FloatAttribute)>& InPredicate)
    -> void
{
    RecordOfFloatAttributes_Utils::ForEach_ValidEntry_If
    (
        InAttributeOwner,
        InFunc,
        InPredicate,
        ECk_Record_ForEach_Policy::IgnoreRecordMissing
    );
}

auto
    UCk_Utils_FloatAttribute_UE::
    Has_Component(
        const FCk_Handle_FloatAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent)
    -> bool
{
    switch (InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            return FloatAttribute_Utils_Min::Has(InAttribute);
        }
        case ECk_MinMaxCurrent::Max:
        {
            return FloatAttribute_Utils_Max::Has(InAttribute);
        }
        case ECk_MinMaxCurrent::Current:
        {
            return FloatAttribute_Utils_Current::Has(InAttribute);
        }
        default:
        {
            CK_INVALID_ENUM(InAttributeComponent);
            return {};
        }
    }
}

auto
    UCk_Utils_FloatAttribute_UE::
    Get_BaseValue(
        const FCk_Handle_FloatAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent)
    -> float
{
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Float Attribute [{}] with Owner [{}] does NOT have a [{}] component"),
        InAttribute,
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute), InAttributeComponent)
    { return {}; }

    switch (InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            return FloatAttribute_Utils_Min::Get_BaseValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Max:
        {
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
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Float Attribute [{}] with Owner [{}] does NOT have a [{}] component"),
        InAttribute,
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute), InAttributeComponent)
    { return {}; }

    switch (InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            return FloatAttribute_Utils_Min::Get_FinalValue(InAttribute) - FloatAttribute_Utils_Min::Get_BaseValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Max:
        {
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
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Float Attribute [{}] with Owner [{}] does NOT have a [{}] component"),
        InAttribute,
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute), InAttributeComponent)
    { return {}; }


    switch (InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            return FloatAttribute_Utils_Min::Get_FinalValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Max:
        {
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
        float InNewBaseValue,
        ECk_MinMaxCurrent InAttributeComponent)
    -> FCk_Handle_FloatAttribute
{
    const auto CurrentBaseValue = Get_BaseValue(InAttribute, InAttributeComponent);
    const auto Delta = InNewBaseValue - CurrentBaseValue;

    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Float Attribute [{}] with Owner [{}] does NOT have a [{}] component"),
        InAttribute,
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute), InAttributeComponent)
    { return {}; }

    UCk_Utils_FloatAttributeModifier_UE::Add(InAttribute, {}, FCk_Fragment_FloatAttributeModifier_ParamsData{
        Delta, ECk_ArithmeticOperations_Basic::Add, ECk_ModifierOperation_RevocablePolicy::NotRevocable, InAttributeComponent});

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
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Float Attribute [{}] does NOT have a [{}] component. Cannot BIND to OnValueChanged"),
        InAttribute,
        InAttributeComponent)
    { return InAttribute; }

    switch(InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            CK_SIGNAL_BIND(ck::UUtils_Signal_OnFloatAttributeValueChanged_Min, InAttribute, InDelegate, InBehavior, InPostFireBehavior);
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            CK_SIGNAL_BIND(ck::UUtils_Signal_OnFloatAttributeValueChanged_Max, InAttribute, InDelegate, InBehavior, InPostFireBehavior);
            break;
        }
        case ECk_MinMaxCurrent::Current:
        {
            CK_SIGNAL_BIND(ck::UUtils_Signal_OnFloatAttributeValueChanged_Current, InAttribute, InDelegate, InBehavior, InPostFireBehavior);
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
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Float Attribute [{}] does NOT have a [{}] component. Cannot UNBIND from OnValueChanged"),
        InAttribute,
        InAttributeComponent)
    { return InAttribute; }

    switch(InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnFloatAttributeValueChanged_Min, InAttribute, InDelegate);
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnFloatAttributeValueChanged_Max, InAttribute, InDelegate);
            break;
        }
        case ECk_MinMaxCurrent::Current:
        {
            CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnFloatAttributeValueChanged_Current, InAttribute, InDelegate);
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

    auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute);
    const auto& ModifierOperation = ParamsToUse.Get_ModifierOperation();
    const auto& AttributeComponent = ParamsToUse.Get_Component();

    CK_ENSURE_IF_NOT(UCk_Utils_FloatAttribute_UE::Has_Component(InAttribute, AttributeComponent),
        TEXT("Float Attribute [{}] with Owner [{}] does NOT have a [{}] component. Cannot Add Modifier"),
        InAttribute,
        LifetimeOwner,
        AttributeComponent)
    { return {}; }

    if (FMath::IsNearlyZero(ParamsToUse.Get_ModifierDelta()) &&
        (ModifierOperation == ECk_ArithmeticOperations_Basic::Add || ModifierOperation  == ECk_ArithmeticOperations_Basic::Subtract))
    { return {}; }

    if (FMath::IsNearlyEqual(ParamsToUse.Get_ModifierDelta(), 1.0f) &&
        (ModifierOperation == ECk_ArithmeticOperations_Basic::Multiply || ModifierOperation == ECk_ArithmeticOperations_Basic::Divide))
    { return {}; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttribute);
    auto NewModifierEntity = ck::StaticCast<FCk_Handle_FloatAttributeModifier>(NewEntity);
    UCk_Utils_GameplayLabel_UE::Add(NewModifierEntity, InModifierName);

    switch (AttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            FloatAttributeModifier_Utils_Min::Add
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
            FloatAttributeModifier_Utils_Max::Add
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
            FloatAttributeModifier_Utils_Current::Add
            (
                NewModifierEntity,
                ParamsToUse.Get_ModifierDelta(),
                ParamsToUse.Get_ModifierOperation(),
                ParamsToUse.Get_ModifierOperation_RevocablePolicy()
            );

            break;
        }
    }

    UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_FloatAttribute_Rep>(
        LifetimeOwner, [&](UCk_Fragment_FloatAttribute_Rep* InRepComp)
    {
        InRepComp->Broadcast_AddModifier(InModifierName, ParamsToUse);
    });

    return NewModifierEntity;
}

auto
    UCk_Utils_FloatAttributeModifier_UE::
    Override(
        FCk_Handle_FloatAttributeModifier& InAttributeModifierEntity,
        float InNewDelta,
        ECk_MinMaxCurrent InComponent)
    -> FCk_Handle_FloatAttributeModifier
{
#define ENSURE_IS_UNIQUE(_Utils_)\
    CK_ENSURE_IF_NOT(_Utils_::Get_IsModifierUnique(InAttributeModifierEntity) == ECk_Unique::Unique,\
        TEXT("Modifier [{}] is NOT unique for Attribute [{}][{}] with Owner [{}].\n"\
             "Overriding non-unique Modifiers affect only the last non-unique Modifier on Clients."),\
        InAttributeModifierEntity,\
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttributeModifierEntity),\
        InComponent,\
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttributeModifierEntity)))\
    { return InAttributeModifierEntity; }

    switch (InComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            ENSURE_IS_UNIQUE(FloatAttributeModifier_Utils_Min);
            FloatAttributeModifier_Utils_Min::Override
            (
                InAttributeModifierEntity,
                InNewDelta
            );
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            ENSURE_IS_UNIQUE(FloatAttributeModifier_Utils_Max);
            FloatAttributeModifier_Utils_Max::Override
            (
                InAttributeModifierEntity,
                InNewDelta
            );
            break;
        }
        case ECk_MinMaxCurrent::Current:
        {
            ENSURE_IS_UNIQUE(FloatAttributeModifier_Utils_Current);
            FloatAttributeModifier_Utils_Current::Override
            (
                InAttributeModifierEntity,
                InNewDelta
            );
            break;
        }
    }
#undef ENSURE_IS_UNIQUE

    const auto AttributeEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttributeModifierEntity);
    auto ReplicatedEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(AttributeEntity);

    UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_FloatAttribute_Rep>(
        ReplicatedEntity, [&](UCk_Fragment_FloatAttribute_Rep* InRepComp)
    {
        InRepComp->Broadcast_OverrideModifier(
            UCk_Utils_GameplayLabel_UE::Get_Label(InAttributeModifierEntity),
            UCk_Utils_GameplayLabel_UE::Get_Label(AttributeEntity),
            InNewDelta, InComponent);
    });

    return InAttributeModifierEntity;
}

auto
    UCk_Utils_FloatAttributeModifier_UE::
    Get_Delta(
        const FCk_Handle_FloatAttributeModifier& InAttributeModifierEntity,
        ECk_MinMaxCurrent InComponent)
    -> float
{
    switch (InComponent)
    {
        case ECk_MinMaxCurrent::Min: return FloatAttributeModifier_Utils_Min::Get_ModifierDeltaValue(InAttributeModifierEntity);
        case ECk_MinMaxCurrent::Max: return FloatAttributeModifier_Utils_Max::Get_ModifierDeltaValue(InAttributeModifierEntity);
        case ECk_MinMaxCurrent::Current: return FloatAttributeModifier_Utils_Current::Get_ModifierDeltaValue(InAttributeModifierEntity);
    }

    return {};
}

auto
    UCk_Utils_FloatAttributeModifier_UE::
    TryGet(
        const FCk_Handle_FloatAttribute& InAttribute,
        FGameplayTag InModifierName,
        ECk_MinMaxCurrent InComponent)
    -> FCk_Handle_FloatAttributeModifier
{
    switch(InComponent)
    {
        case ECk_MinMaxCurrent::Current:
        {
            return RecordOfFloatAttributeModifiers_Utils_Current::Get_ValidEntry_If(InAttribute, ck::algo::MatchesGameplayLabel{InModifierName});
        }
        case ECk_MinMaxCurrent::Min:
        {
            return RecordOfFloatAttributeModifiers_Utils_Min::Get_ValidEntry_If(InAttribute, ck::algo::MatchesGameplayLabel{InModifierName});
        }
        case ECk_MinMaxCurrent::Max:
        {
            return RecordOfFloatAttributeModifiers_Utils_Max::Get_ValidEntry_If(InAttribute, ck::algo::MatchesGameplayLabel{InModifierName});
        }
        default:
        {
            return {};
        }
    }
}

auto
    UCk_Utils_FloatAttributeModifier_UE::
    TryGet_If(
        const FCk_Handle_FloatAttribute& InAttribute,
        FGameplayTag InModifierName,
        ECk_MinMaxCurrent InComponent,
        const TFunction<bool(FCk_Handle_FloatAttributeModifier)>& InPredicate)
    -> FCk_Handle_FloatAttributeModifier
{
    switch(InComponent)
    {
        case ECk_MinMaxCurrent::Current:
        {
            return RecordOfFloatAttributeModifiers_Utils_Current::Get_ValidEntry_If(InAttribute, [&](const FCk_Handle& InHandle) -> bool
            {
                if (NOT ck::algo::MatchesGameplayLabel{InModifierName}(InHandle))
                { return false; }

                return InPredicate(Cast(InHandle));
            });
        }
        case ECk_MinMaxCurrent::Min:
        {
            return RecordOfFloatAttributeModifiers_Utils_Min::Get_ValidEntry_If(InAttribute, [&](const FCk_Handle& InHandle) -> bool
            {
                if (NOT ck::algo::MatchesGameplayLabel{InModifierName}(InHandle))
                { return false; }

                return InPredicate(Cast(InHandle));
            });
        }
        case ECk_MinMaxCurrent::Max:
        {
            return RecordOfFloatAttributeModifiers_Utils_Max::Get_ValidEntry_If(InAttribute, [&](const FCk_Handle& InHandle) -> bool
            {
                if (NOT ck::algo::MatchesGameplayLabel{InModifierName}(InHandle))
                { return false; }

                return InPredicate(Cast(InHandle));
            });
        }
        default:
        {
            return {};
        }
    }
}

auto
    UCk_Utils_FloatAttributeModifier_UE::
    Remove(
        FCk_Handle_FloatAttributeModifier& InAttributeModifierEntity)
    -> FCk_Handle_FloatAttribute
{
    auto AttributeModifierOwnerEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttributeModifierEntity);
    auto AttributeEntity = UCk_Utils_FloatAttribute_UE::CastChecked(AttributeModifierOwnerEntity);
    auto AttributeOwnerEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(AttributeEntity);

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InAttributeModifierEntity);

    const auto& AttributeComponent = [InAttributeModifierEntity]() -> ECk_MinMaxCurrent
    {
        if (FloatAttributeModifier_Utils_Current::Has(InAttributeModifierEntity))
        { return ECk_MinMaxCurrent::Current; }

        if (FloatAttributeModifier_Utils_Min::Has(InAttributeModifierEntity))
        { return ECk_MinMaxCurrent::Min; }

        if (FloatAttributeModifier_Utils_Max::Has(InAttributeModifierEntity))
        { return ECk_MinMaxCurrent::Max; }

        CK_TRIGGER_ENSURE(TEXT("Float Attribute Modifier Entity [{}] does NOT have Min, Max or Current"), InAttributeModifierEntity);
        return ECk_MinMaxCurrent::Current;
    }();

    UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_FloatAttribute_Rep>(AttributeOwnerEntity,
    [&](UCk_Fragment_FloatAttribute_Rep* InRepComp)
    {
        InRepComp->Broadcast_RemoveModifier(
            UCk_Utils_GameplayLabel_UE::Get_Label(InAttributeModifierEntity),
            UCk_Utils_GameplayLabel_UE::Get_Label(AttributeEntity),
            AttributeComponent);
    });

    return AttributeEntity;
}

auto
    UCk_Utils_FloatAttributeModifier_UE::
    Has(
        const FCk_Handle& InModifierEntity)
    -> bool
{
    return InModifierEntity.Has_Any<ck::FFragment_FloatAttributeModifier_Min, ck::FFragment_FloatAttributeModifier_Current,ck::FFragment_FloatAttributeModifier_Max>();
}

auto
    UCk_Utils_FloatAttributeModifier_UE::
    ForEach(
        FCk_Handle_FloatAttribute& InAttributeOwner,
        const FInstancedStruct&    InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        ECk_MinMaxCurrent InAttributeComponent)
    -> TArray<FCk_Handle_FloatAttributeModifier>
{
    auto ToRet = TArray<FCk_Handle_FloatAttributeModifier>{};

    ForEach(InAttributeOwner, [&](const FCk_Handle_FloatAttributeModifier& InAttribute)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InAttribute, InOptionalPayload); }
        else
        { ToRet.Emplace(InAttribute); }
    }, InAttributeComponent);

    return ToRet;
}

auto
    UCk_Utils_FloatAttributeModifier_UE::
    ForEach(
        FCk_Handle_FloatAttribute& InAttribute,
        const TFunction<void(FCk_Handle_FloatAttributeModifier)>& InFunc,
        ECk_MinMaxCurrent InAttributeComponent)
    -> void
{
    switch(InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Current:
        {
            RecordOfFloatAttributeModifiers_Utils_Current::ForEach_ValidEntry
            (
                InAttribute,
                InFunc,
                ECk_Record_ForEach_Policy::IgnoreRecordMissing
            );
            break;
        }
        case ECk_MinMaxCurrent::Min:
        {
            RecordOfFloatAttributeModifiers_Utils_Min::ForEach_ValidEntry
            (
                InAttribute,
                InFunc,
                ECk_Record_ForEach_Policy::IgnoreRecordMissing
            );
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            RecordOfFloatAttributeModifiers_Utils_Max::ForEach_ValidEntry
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
    UCk_Utils_FloatAttributeModifier_UE::
    ForEach_If(
        FCk_Handle_FloatAttribute& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate,
        ECk_MinMaxCurrent InAttributeComponent)
    -> TArray<FCk_Handle_FloatAttributeModifier>
{
    auto ToRet = TArray<FCk_Handle_FloatAttributeModifier>{};

    ForEach_If
    (
        InAttributeOwner,
        [&](FCk_Handle_FloatAttributeModifier InAttribute)
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
    UCk_Utils_FloatAttributeModifier_UE::
    ForEach_If(
        FCk_Handle_FloatAttribute& InAttribute,
        const TFunction<void(FCk_Handle_FloatAttributeModifier)>& InFunc,
        const TFunction<bool(FCk_Handle_FloatAttributeModifier)>& InPredicate,
        ECk_MinMaxCurrent InAttributeComponent)
    -> void
{
    switch(InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Current:
        {
            RecordOfFloatAttributeModifiers_Utils_Current::ForEach_ValidEntry_If
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
            RecordOfFloatAttributeModifiers_Utils_Min::ForEach_ValidEntry_If
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
            RecordOfFloatAttributeModifiers_Utils_Max::ForEach_ValidEntry_If
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
