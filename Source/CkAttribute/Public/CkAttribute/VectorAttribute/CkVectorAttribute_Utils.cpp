#include "CkVectorAttribute_Utils.h"

#include "CkAttribute/CkAttribute_Log.h"

#include "CkCore/Math/Vector/CkVector_Utils.h"
#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

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

    auto NewAttributeEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_VectorAttribute>(InAttributeOwnerEntity);

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

    // it's possible that we have pending replication info
    if (UCk_Utils_Net_UE::Get_IsEntityNetMode_Client(InAttributeOwnerEntity))
    {
        if (UCk_Utils_Ecs_Net_UE::Get_HasReplicatedFragment<UCk_Fragment_VectorAttribute_Rep>(InAttributeOwnerEntity))
        {
            InAttributeOwnerEntity.Try_Transform<TObjectPtr<UCk_Fragment_VectorAttribute_Rep>>(
            [&]( const TObjectPtr<UCk_Fragment_VectorAttribute_Rep>& InRepComp)
            {
                InRepComp->Request_TryUpdateReplicatedAttributes();
            });
        }
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

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_VectorAttribute_UE, FCk_Handle_VectorAttribute, ck::FFragment_VectorAttribute_Current);

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
        return ck::IsValid(TryGet(Handle, InAttributeName));
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
    ForEach_ByName(
        FCk_Handle& InAttributeOwner,
        FGameplayTag InAttributeName,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_VectorAttribute>
{
    auto Ret = TArray<FCk_Handle_VectorAttribute>{};

    ForEach_ByName(InAttributeOwner, InAttributeName, [&](FCk_Handle_VectorAttribute InAttribute)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InAttribute, InOptionalPayload); }

        Ret.Emplace(InAttribute);
    });

    return Ret;
}

auto
    UCk_Utils_VectorAttribute_UE::
    ForEach_ByName(
        FCk_Handle& InAttributeOwner,
        FGameplayTag InAttributeName,
        const TFunction<void(FCk_Handle_VectorAttribute)>& InFunc)
    -> void
{
    ForEach_If(InAttributeOwner, InFunc, ck::algo::MatchesGameplayLabelExact{InAttributeName});
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
        {
            CK_INVALID_ENUM(InAttributeComponent);
            return {};
        }
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
        {
            CK_INVALID_ENUM(InAttributeComponent);
            return {};
        }
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
        {
            CK_INVALID_ENUM(InAttributeComponent);
            return {};
        }
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
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Vector Attribute [{}] with Owner [{}] does NOT have a [{}] component"),
        InAttribute,
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute),
        InAttributeComponent)
    { return InAttribute; }

    if (InNewBaseValue == Get_BaseValue(InAttribute, InAttributeComponent))
    { return InAttribute; }

    UCk_Utils_VectorAttributeModifier_UE::Add_NotRevocable
    (
        InAttribute,
        ECk_AttributeModifier_Operation::Override,
        FCk_Fragment_VectorAttributeModifier_ParamsData
        {
            InNewBaseValue,
            InAttributeComponent
        }
    );

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
    Add_Revocable(
        FCk_Handle_VectorAttribute& InAttribute,
        FGameplayTag InModifierName,
        ECk_AttributeModifier_Operation InModifierOperation,
        const FCk_Fragment_VectorAttributeModifier_ParamsData& InParams)
    -> FCk_Handle_VectorAttributeModifier
{
    QUICK_SCOPE_CYCLE_COUNTER(Add_Vector_Modifier_Revocable)

    auto ParamsToUse = InParams;
    ParamsToUse.Set_TargetAttributeName(UCk_Utils_GameplayLabel_UE::Get_Label(InAttribute));

    const auto& AttributeComponent = ParamsToUse.Get_Component();

    CK_ENSURE_IF_NOT(UCk_Utils_VectorAttribute_UE::Has_Component(InAttribute, AttributeComponent),
        TEXT("Vector Attribute [{}] with Owner [{}] does NOT have a [{}] component. Cannot Add Revocable Modifier"),
        InAttribute,
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute),
        AttributeComponent)
    { return {}; }

    CK_ENSURE_IF_NOT((InModifierOperation == ECk_AttributeModifier_Operation::Divide ? NOT UCk_Utils_Vector3_UE::Get_IsAnyAxisNearlyZero(ParamsToUse.Get_ModifierDelta()) : true),
        TEXT("Trying to ADD a new Revocable Modifier [{}] for Vector Attribute [{}] which DIVIDES by 0. Setting it to 1 in non-shipping build"),
        AttributeComponent, InAttribute)
    {
        ParamsToUse.Set_ModifierDelta(FVector::OneVector);
    }

    CK_ENSURE_IF_NOT(InModifierOperation != ECk_AttributeModifier_Operation::Override, TEXT("Override Revocable Vector Attribute Modifier is NOT supported!"))
    { return {}; }

    switch (AttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            auto ModifierEntity = VectorAttributeModifier_Utils_Min::Add_Revocable(InAttribute, ParamsToUse.Get_ModifierDelta(), InModifierOperation);
            UCk_Utils_GameplayLabel_UE::Add(ModifierEntity, InModifierName);
            return ModifierEntity;
        }
        case ECk_MinMaxCurrent::Max:
        {
            auto ModifierEntity =  VectorAttributeModifier_Utils_Max::Add_Revocable(InAttribute, ParamsToUse.Get_ModifierDelta(), InModifierOperation);
            UCk_Utils_GameplayLabel_UE::Add(ModifierEntity, InModifierName);
            return ModifierEntity;
        }
        case ECk_MinMaxCurrent::Current:
        {
            auto ModifierEntity =  VectorAttributeModifier_Utils_Current::Add_Revocable(InAttribute, ParamsToUse.Get_ModifierDelta(), InModifierOperation);
            UCk_Utils_GameplayLabel_UE::Add(ModifierEntity, InModifierName);
            return ModifierEntity;
        }
        default:
        {
            CK_INVALID_ENUM(AttributeComponent);
            return {};
        }
    }
}

auto
    UCk_Utils_VectorAttributeModifier_UE::
    Add_NotRevocable(
        FCk_Handle_VectorAttribute& InAttribute,
        ECk_AttributeModifier_Operation InModifierOperation,
        const FCk_Fragment_VectorAttributeModifier_ParamsData& InParams)
    -> void
{
    QUICK_SCOPE_CYCLE_COUNTER(Add_Vector_Modifier_NotRevocable)

    auto ParamsToUse = InParams;
    ParamsToUse.Set_TargetAttributeName(UCk_Utils_GameplayLabel_UE::Get_Label(InAttribute));

    const auto& AttributeComponent = ParamsToUse.Get_Component();

    CK_ENSURE_IF_NOT(UCk_Utils_VectorAttribute_UE::Has_Component(InAttribute, AttributeComponent),
        TEXT("Vector Attribute [{}] with Owner [{}] does NOT have a [{}] component. Cannot Add NotRevocable Modifier"),
        InAttribute,
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute),
        AttributeComponent)
    { return; }

    if (ParamsToUse.Get_ModifierDelta().IsNearlyZero() &&
        (InModifierOperation == ECk_AttributeModifier_Operation::Add || InModifierOperation  == ECk_AttributeModifier_Operation::Subtract))
    { return; }

    if (ParamsToUse.Get_ModifierDelta().Equals(FVector::OneVector) &&
        (InModifierOperation == ECk_AttributeModifier_Operation::Multiply || InModifierOperation == ECk_AttributeModifier_Operation::Divide))
    { return; }

    CK_ENSURE_IF_NOT((InModifierOperation == ECk_AttributeModifier_Operation::Divide ? NOT UCk_Utils_Vector3_UE::Get_IsAnyAxisNearlyZero(ParamsToUse.Get_ModifierDelta()) : true),
        TEXT("Trying to ADD a new NotRevocable Modifier [{}] for Vector Attribute [{}] which DIVIDES by 0. Setting it to 1 in non-shipping build"),
        AttributeComponent, InAttribute)
    {
        ParamsToUse.Set_ModifierDelta(FVector::OneVector);
    }

    switch (AttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            VectorAttributeModifier_Utils_Min::Add_NotRevocable(InAttribute, ParamsToUse.Get_ModifierDelta(), InModifierOperation);
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            VectorAttributeModifier_Utils_Max::Add_NotRevocable(InAttribute, ParamsToUse.Get_ModifierDelta(), InModifierOperation);
            break;
        }
        case ECk_MinMaxCurrent::Current:
        {
            VectorAttributeModifier_Utils_Current::Add_NotRevocable(InAttribute, ParamsToUse.Get_ModifierDelta(), InModifierOperation);
            break;
        }
        default:
        {
            CK_INVALID_ENUM(AttributeComponent);
            break;
        }
    }
}

auto
    UCk_Utils_VectorAttributeModifier_UE::
    Override(
        FCk_Handle_VectorAttributeModifier& InAttributeModifierEntity,
        const FVector InNewDelta)
    -> FCk_Handle_VectorAttributeModifier
{
    switch (const auto Component = InAttributeModifierEntity.Get<ECk_MinMaxCurrent>())
    {
        case ECk_MinMaxCurrent::Min:
        {
            if (UCk_Utils_Arithmetic_UE::Get_IsNearlyEqual(VectorAttributeModifier_Utils_Min::Get_ModifierDeltaValue(InAttributeModifierEntity), InNewDelta))
            { return InAttributeModifierEntity; }

            const auto& ModifierOperation = VectorAttributeModifier_Utils_Min::Get_ModifierOperation(InAttributeModifierEntity);

            CK_ENSURE_IF_NOT((ModifierOperation == ECk_ArithmeticOperations_Basic::Divide ? NOT UCk_Utils_Vector3_UE::Get_IsAnyAxisNearlyZero(InNewDelta) : true),
                TEXT("Trying to OVERRIDE existing Vector Attribute Modifier [{}][{}] with new value which would DIVIDE by 0. Ignoring the change in non-shipping build"),
                InAttributeModifierEntity, Component)
            { return InAttributeModifierEntity; }

            VectorAttributeModifier_Utils_Min::Override(InAttributeModifierEntity, InNewDelta);
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            if (UCk_Utils_Arithmetic_UE::Get_IsNearlyEqual(VectorAttributeModifier_Utils_Max::Get_ModifierDeltaValue(InAttributeModifierEntity), InNewDelta))
            { return InAttributeModifierEntity; }

            const auto& ModifierOperation = VectorAttributeModifier_Utils_Max::Get_ModifierOperation(InAttributeModifierEntity);

            CK_ENSURE_IF_NOT((ModifierOperation == ECk_ArithmeticOperations_Basic::Divide ? NOT UCk_Utils_Vector3_UE::Get_IsAnyAxisNearlyZero(InNewDelta) : true),
                TEXT("Trying to OVERRIDE existing Vector Attribute Modifier [{}][{}] with new value which would DIVIDE by 0. Ignoring the change in non-shipping build"),
                InAttributeModifierEntity, Component)
            { return InAttributeModifierEntity; }

            VectorAttributeModifier_Utils_Max::Override(InAttributeModifierEntity, InNewDelta);
            break;
        }
        case ECk_MinMaxCurrent::Current:
        {
            if (UCk_Utils_Arithmetic_UE::Get_IsNearlyEqual(VectorAttributeModifier_Utils_Current::Get_ModifierDeltaValue(InAttributeModifierEntity), InNewDelta))
            { return InAttributeModifierEntity; }

            const auto& ModifierOperation = VectorAttributeModifier_Utils_Current::Get_ModifierOperation(InAttributeModifierEntity);

            CK_ENSURE_IF_NOT((ModifierOperation == ECk_ArithmeticOperations_Basic::Divide ? NOT UCk_Utils_Vector3_UE::Get_IsAnyAxisNearlyZero(InNewDelta) : true),
                TEXT("Trying to OVERRIDE existing Vector Attribute Modifier [{}][{}] with new value which would DIVIDE by 0. Ignoring the change in non-shipping build"),
                InAttributeModifierEntity, Component)
            { return InAttributeModifierEntity; }

            VectorAttributeModifier_Utils_Current::Override(InAttributeModifierEntity, InNewDelta);
            break;
        }
    }

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
            CK_INVALID_ENUM(InComponent);
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
            CK_INVALID_ENUM(InComponent);
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
    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InAttributeModifierEntity);

    auto AttributeModifierOwnerEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttributeModifierEntity);

    return UCk_Utils_VectorAttribute_UE::CastChecked(AttributeModifierOwnerEntity);
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
