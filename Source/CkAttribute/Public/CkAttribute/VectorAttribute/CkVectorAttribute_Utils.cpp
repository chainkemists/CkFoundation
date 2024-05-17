#include "CkVectorAttribute_Utils.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment.h"

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

    if (InReplicates == ECk_Replication::Replicates)
    { NewAttributeEntity.Add<ck::FTag_ReplicatedAttribute>(); }

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
    return RecordOfVectorAttributes_Utils::Get_ValidEntry_If(InAttributeOwnerEntity,
        ck::algo::MatchesGameplayLabelExact{InAttributeName});
}

auto
    UCk_Utils_VectorAttribute_UE::
    TryGet_Entity_WithAttribute_InOwnershipChain(
        const FCk_Handle& InHandle,
        FGameplayTag InAttributeName)
    -> FCk_Handle
{
    auto MaybeOwner = UCk_Utils_EntityLifetime_UE::Get_EntityInOwnershipChain_If(InHandle,
    [&](const FCk_Handle& Handle)
    {
        if (ck::IsValid(TryGet(Handle, InAttributeName)))
        { return true; }

        return false;
    });

    return MaybeOwner;
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
    RecordOfVectorAttributes_Utils::ForEach_ValidEntry(InAttributeOwner, InFunc);
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
    RecordOfVectorAttributes_Utils::ForEach_ValidEntry_If(InAttributeOwner, InFunc, InPredicate);
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

    if (InParams.Get_ModifierOperation_RevocablePolicy() == ECk_ModifierOperation_RevocablePolicy::NotRevocable &&
        ParamsToUse.Get_ModifierDelta().IsNearlyZero() &&
        (ModifierOperation == ECk_ArithmeticOperations_Basic::Add || ModifierOperation  == ECk_ArithmeticOperations_Basic::Subtract))
    { return {}; }

    if (InParams.Get_ModifierOperation_RevocablePolicy() == ECk_ModifierOperation_RevocablePolicy::NotRevocable &&
        ParamsToUse.Get_ModifierDelta().Equals(FVector::OneVector) &&
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

    UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_VectorAttribute_Rep>(
        LifetimeOwner, [&](UCk_Fragment_VectorAttribute_Rep* InRepComp)
    {
        if (NOT InAttribute.Has<ck::FTag_ReplicatedAttribute>())
        { return; }

        InRepComp->Broadcast_AddModifier(InModifierName, ParamsToUse);
    });

    return NewModifierEntity;
}

auto
    UCk_Utils_VectorAttributeModifier_UE::
    Override(
        FCk_Handle_VectorAttributeModifier& InAttributeModifierEntity,
        FVector InNewDelta,
        ECk_MinMaxCurrent InComponent)
    -> FCk_Handle_VectorAttributeModifier
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
            ENSURE_IS_UNIQUE(VectorAttributeModifier_Utils_Min);
            VectorAttributeModifier_Utils_Min::Override
            (
                InAttributeModifierEntity,
                InNewDelta
            );
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            ENSURE_IS_UNIQUE(VectorAttributeModifier_Utils_Max);
            VectorAttributeModifier_Utils_Max::Override
            (
                InAttributeModifierEntity,
                InNewDelta
            );
            break;
        }
        case ECk_MinMaxCurrent::Current:
        {
            ENSURE_IS_UNIQUE(VectorAttributeModifier_Utils_Current);
            VectorAttributeModifier_Utils_Current::Override
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

    UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_VectorAttribute_Rep>(
        ReplicatedEntity, [&](UCk_Fragment_VectorAttribute_Rep* InRepComp)
    {
        if (NOT AttributeEntity.Has<ck::FTag_ReplicatedAttribute>())
        { return; }

        InRepComp->Broadcast_OverrideModifier(
            UCk_Utils_GameplayLabel_UE::Get_Label(InAttributeModifierEntity),
            UCk_Utils_GameplayLabel_UE::Get_Label(AttributeEntity),
            InNewDelta, InComponent);
    });

    return InAttributeModifierEntity;
}

auto
    UCk_Utils_VectorAttributeModifier_UE::
    Get_Delta(
        const FCk_Handle_VectorAttributeModifier& InAttributeModifierEntity,
        ECk_MinMaxCurrent InComponent)
    -> FVector
{
    switch (InComponent)
    {
        case ECk_MinMaxCurrent::Min: return VectorAttributeModifier_Utils_Min::Get_ModifierDeltaValue(InAttributeModifierEntity);
        case ECk_MinMaxCurrent::Max: return VectorAttributeModifier_Utils_Max::Get_ModifierDeltaValue(InAttributeModifierEntity);
        case ECk_MinMaxCurrent::Current: return VectorAttributeModifier_Utils_Current::Get_ModifierDeltaValue(InAttributeModifierEntity);
    }

    return {};
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
            return RecordOfVectorAttributeModifiers_Utils_Current::Get_ValidEntry_If(InAttribute, ck::algo::MatchesGameplayLabel{InModifierName});
        }
        case ECk_MinMaxCurrent::Min:
        {
            return RecordOfVectorAttributeModifiers_Utils_Min::Get_ValidEntry_If(InAttribute, ck::algo::MatchesGameplayLabel{InModifierName});
        }
        case ECk_MinMaxCurrent::Max:
        {
            return RecordOfVectorAttributeModifiers_Utils_Max::Get_ValidEntry_If(InAttribute, ck::algo::MatchesGameplayLabel{InModifierName});
        }
        default:
        {
            return {};
        }
    }
}

auto
    UCk_Utils_VectorAttributeModifier_UE::
    TryGet_If(
        const FCk_Handle_VectorAttribute& InAttribute,
        FGameplayTag InModifierName,
        ECk_MinMaxCurrent InComponent,
        const TFunction<bool(FCk_Handle_VectorAttributeModifier)>& InPredicate)
    -> FCk_Handle_VectorAttributeModifier
{
    switch(InComponent)
    {
        case ECk_MinMaxCurrent::Current:
        {
            return RecordOfVectorAttributeModifiers_Utils_Current::Get_ValidEntry_If(InAttribute, [&](const FCk_Handle& InHandle) -> bool
            {
                if (NOT ck::algo::MatchesGameplayLabel{InModifierName}(InHandle))
                { return false; }

                return InPredicate(Cast(InHandle));
            });
        }
        case ECk_MinMaxCurrent::Min:
        {
            return RecordOfVectorAttributeModifiers_Utils_Min::Get_ValidEntry_If(InAttribute, [&](const FCk_Handle& InHandle) -> bool
            {
                if (NOT ck::algo::MatchesGameplayLabel{InModifierName}(InHandle))
                { return false; }

                return InPredicate(Cast(InHandle));
            });
        }
        case ECk_MinMaxCurrent::Max:
        {
            return RecordOfVectorAttributeModifiers_Utils_Max::Get_ValidEntry_If(InAttribute, [&](const FCk_Handle& InHandle) -> bool
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
    UCk_Utils_VectorAttributeModifier_UE::
    Remove(
        FCk_Handle_VectorAttributeModifier& InAttributeModifierEntity)
    -> FCk_Handle_VectorAttribute
{
    auto AttributeModifierOwnerEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttributeModifierEntity);
    auto AttributeEntity = UCk_Utils_VectorAttribute_UE::CastChecked(AttributeModifierOwnerEntity);
    auto AttributeOwnerEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(AttributeEntity);

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InAttributeModifierEntity);

    const auto& AttributeComponent = [InAttributeModifierEntity]() -> ECk_MinMaxCurrent
    {
        if (VectorAttributeModifier_Utils_Current::Has(InAttributeModifierEntity))
        { return ECk_MinMaxCurrent::Current; }

        if (VectorAttributeModifier_Utils_Min::Has(InAttributeModifierEntity))
        { return ECk_MinMaxCurrent::Min; }

        if (VectorAttributeModifier_Utils_Max::Has(InAttributeModifierEntity))
        { return ECk_MinMaxCurrent::Max; }

        CK_TRIGGER_ENSURE(TEXT("Vector Attribute Modifier Entity [{}] does NOT have Min, Max or Current"), InAttributeModifierEntity);
        return ECk_MinMaxCurrent::Current;
    }();

    UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_VectorAttribute_Rep>(AttributeOwnerEntity,
    [&](UCk_Fragment_VectorAttribute_Rep* InRepComp)
    {
        if (NOT AttributeEntity.Has<ck::FTag_Replicated>())
        { return; }

        InRepComp->Broadcast_RemoveModifier(
            UCk_Utils_GameplayLabel_UE::Get_Label(InAttributeModifierEntity),
            UCk_Utils_GameplayLabel_UE::Get_Label(AttributeEntity),
            AttributeComponent);
    });

    return AttributeEntity;
}

auto
    UCk_Utils_VectorAttributeModifier_UE::
    Has(
        const FCk_Handle& InModifierEntity)
    -> bool
{
    return InModifierEntity.Has_Any<ck::FFragment_VectorAttributeModifier_Min, ck::FFragment_VectorAttributeModifier_Current,ck::FFragment_VectorAttributeModifier_Max>();
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
            RecordOfVectorAttributeModifiers_Utils_Current::ForEach_ValidEntry(InAttribute, InFunc);
            break;
        }
        case ECk_MinMaxCurrent::Min:
        {
            RecordOfVectorAttributeModifiers_Utils_Min::ForEach_ValidEntry(InAttribute, InFunc);
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            RecordOfVectorAttributeModifiers_Utils_Max::ForEach_ValidEntry(InAttribute, InFunc);
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
            RecordOfVectorAttributeModifiers_Utils_Current::ForEach_ValidEntry_If(InAttribute, InFunc, InPredicate);
            break;
        }
        case ECk_MinMaxCurrent::Min:
        {
            RecordOfVectorAttributeModifiers_Utils_Min::ForEach_ValidEntry_If(InAttribute, InFunc, InPredicate);
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            RecordOfVectorAttributeModifiers_Utils_Max::ForEach_ValidEntry_If(InAttribute, InFunc, InPredicate);
            break;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
