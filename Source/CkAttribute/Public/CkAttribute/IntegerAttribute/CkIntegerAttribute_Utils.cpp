#include "CkIntegerAttribute_Utils.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkLabel/CkLabel_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IntegerAttribute_UE::
    Add(
        FCk_Handle& InAttributeOwnerEntity,
        const FCk_Fragment_IntegerAttribute_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_IntegerAttribute
{
    RecordOfIntegerAttributes_Utils::AddIfMissing(InAttributeOwnerEntity, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);

    auto NewAttributeEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_AsTypeSafe<FCk_Handle_IntegerAttribute>(InAttributeOwnerEntity);

    IntegerAttribute_Utils_Current::Add(NewAttributeEntity, InParams.Get_BaseValue());

    switch (InParams.Get_MinMax())
    {
        case ECk_MinMax::None:
        {
            break;
        }
        case ECk_MinMax::Min:
        {
            IntegerAttribute_Utils_Min::Add(NewAttributeEntity, InParams.Get_MinValue());
            break;
        }
        case ECk_MinMax::Max:
        {
            IntegerAttribute_Utils_Max::Add(NewAttributeEntity, InParams.Get_MaxValue());
            break;
        }
        case ECk_MinMax::MinMax:
        {
            IntegerAttribute_Utils_Min::Add(NewAttributeEntity, InParams.Get_MinValue());
            IntegerAttribute_Utils_Max::Add(NewAttributeEntity, InParams.Get_MaxValue());
            break;
        }
    }

    if (InReplicates == ECk_Replication::Replicates)
    { NewAttributeEntity.Add<ck::FTag_ReplicatedAttribute>(); }

    UCk_Utils_GameplayLabel_UE::Add(NewAttributeEntity, InParams.Get_Name());
    RecordOfIntegerAttributes_Utils::Request_Connect(InAttributeOwnerEntity, NewAttributeEntity);

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::attribute::VeryVerbose
        (
            TEXT("Skipping creation of Integer Attribute Rep Fragment on Entity [{}] because it's set to [{}]"),
            InAttributeOwnerEntity,
            InReplicates
        );
    }
    else
    {
        UCk_Utils_Net_UE::TryAddContainerFragment<FCk_RepData_IntegerAttributes>(InAttributeOwnerEntity);
    }

    // Apply any pending replication data that arrived before this attribute was constructed.
    if (NOT UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InAttributeOwnerEntity))
    {
        if (const auto* RepData = UCk_Utils_Net_UE::TryGetPendingReplicationData<FCk_RepData_IntegerAttributes>(InAttributeOwnerEntity))
        {
            for (const auto& Entry : RepData->Attributes)
            {
                auto AttributeEntity = TryGet(InAttributeOwnerEntity, Entry.Get_AttributeName());
                if (ck::Is_NOT_Valid(AttributeEntity))
                { continue; }

                UCk_Utils_IntegerAttributeModifier_UE::Request_ClearAllModifiers(AttributeEntity, Entry.Get_Component());
                Request_Override(AttributeEntity, Entry.Get_Base(), Entry.Get_Component());

                const auto& MaybeModifier = UCk_Utils_IntegerAttributeModifier_UE::TryGet(AttributeEntity,
                    ck::FAttributeModifier_ReplicationTags::Get_FinalTag(), Entry.Get_Component());

                if (ck::Is_NOT_Valid(MaybeModifier))
                {
                    UCk_Utils_IntegerAttributeModifier_UE::Add_Revocable
                    (
                        AttributeEntity,
                        ck::FAttributeModifier_ReplicationTags::Get_FinalTag(),
                        ECk_AttributeModifier_Operation::Add,
                        FCk_Fragment_IntegerAttributeModifier_ParamsData
                        {
                            Entry.Get_Final() - Entry.Get_Base(),
                            Entry.Get_Component()
                        }
                    );
                }
            }
        }
    }

    if (InParams.Get_EnableRefill())
    {
        const auto& RefillParams = InParams.Get_RefillParams();

        CK_ENSURE_IF_NOT(ck::IsValid(RefillParams.Get_RefillAttributeName()),
            TEXT("Invalid RefillAttribute Name supplied to INTEGER Attribute [{}]. A unique and valid Name is required"),
            NewAttributeEntity)
        { return NewAttributeEntity; }

        auto RefillAttributeEntity = UCk_Utils_FloatAttribute_UE::Add(InAttributeOwnerEntity, FCk_Fragment_FloatAttribute_ParamsData
            {
                RefillParams.Get_RefillAttributeName(),
                RefillParams.Get_RefillBehavior() == ECk_Attribute_Refill_Policy::Variable ? RefillParams.Get_FillRate() : FMath::Abs(RefillParams.Get_FillRate())
            }, InReplicates);

        if (RefillParams.Get_RefillBehavior() == ECk_Attribute_Refill_Policy::AlwaysReturnToZero)
        {
            CK_ENSURE(RefillParams.Get_FillRate() >= 0.0f,
                TEXT("Refill Rate for INTEGER Attribute [{}] with Absolute Refill Behavior MUST be positive. Current Value [{}]"),
                NewAttributeEntity, RefillParams.Get_FillRate());

            RefillAttributeEntity.Add<ck::FTag_RefillBehaviorAlwaysToZero>();
        }

        RefillAttributeEntity.Add<ck::FFragment_RefillAccumulator>();

        auto RefillAttributeEntityTypeSafe = UCk_Utils_IntegerAttributeRefill_UE::Add(RefillAttributeEntity, RefillParams.Get_StartingState());

        ck::RefillAttribute_Utils::AddOrReplace(NewAttributeEntity, RefillAttributeEntityTypeSafe);
        ck::RefillAttributeTarget_Utils::AddOrReplace(RefillAttributeEntityTypeSafe, NewAttributeEntity);
    }

    return NewAttributeEntity;
}

auto
    UCk_Utils_IntegerAttribute_UE::
    AddMultiple(
        FCk_Handle& InHandle,
        const FCk_Fragment_MultipleIntegerAttribute_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> TArray<FCk_Handle_IntegerAttribute>
{
    return ck::algo::Transform<TArray<FCk_Handle_IntegerAttribute>>(
        InParams.Get_IntegerAttributeParams(), [&](const FCk_Fragment_IntegerAttribute_ParamsData& InParam)
    {
        return Add(InHandle, InParam, InReplicates);
    });
}

auto
    UCk_Utils_IntegerAttribute_UE::
    Has_Any(
        const FCk_Handle& InAttributeOwnerEntity)
    -> bool
{
    return RecordOfIntegerAttributes_Utils::Has(InAttributeOwnerEntity);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_IntegerAttribute_UE, FCk_Handle_IntegerAttribute, ck::FFragment_IntegerAttribute_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IntegerAttribute_UE::
    TryGet(
        const FCk_Handle& InAttributeOwnerEntity,
        FGameplayTag      InAttributeName)
    -> FCk_Handle_IntegerAttribute
{
    QUICK_SCOPE_CYCLE_COUNTER(TryGet_IntegerAttribute)
    return RecordOfIntegerAttributes_Utils::Get_ValidEntry_ByTag(InAttributeOwnerEntity, InAttributeName);
}

auto
    UCk_Utils_IntegerAttribute_UE::
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
    UCk_Utils_IntegerAttribute_UE::
    ForEach(
        FCk_Handle& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_IntegerAttribute>
{
    auto ToRet = TArray<FCk_Handle_IntegerAttribute>{};

    ForEach(InAttributeOwner, [&](const FCk_Handle_IntegerAttribute& InAttribute)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InAttribute, InOptionalPayload); }
        else
        { ToRet.Emplace(InAttribute); }
    });

    return ToRet;
}

auto
    UCk_Utils_IntegerAttribute_UE::
    ForEach(
        FCk_Handle& InAttributeOwner,
        const TFunction<void(FCk_Handle_IntegerAttribute)>& InFunc)
    -> void
{
    RecordOfIntegerAttributes_Utils::ForEach_ValidEntry(InAttributeOwner, InFunc);
}

auto
    UCk_Utils_IntegerAttribute_UE::
    ForEach_If(
        FCk_Handle& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate)
    -> TArray<FCk_Handle_IntegerAttribute>
{
    auto ToRet = TArray<FCk_Handle_IntegerAttribute>{};

    ForEach_If
    (
        InAttributeOwner,
        [&](FCk_Handle_IntegerAttribute InAttribute)
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
    UCk_Utils_IntegerAttribute_UE::
    ForEach_If(
        FCk_Handle& InAttributeOwner,
        const TFunction<void(FCk_Handle_IntegerAttribute)>& InFunc,
        const TFunction<bool(FCk_Handle_IntegerAttribute)>& InPredicate)
    -> void
{
    RecordOfIntegerAttributes_Utils::ForEach_ValidEntry_If(InAttributeOwner, InFunc, InPredicate);
}

auto
    UCk_Utils_IntegerAttribute_UE::
    ForEach_ByName(
        FCk_Handle& InAttributeOwner,
        FGameplayTag InAttributeName,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_IntegerAttribute>
{
    auto Ret = TArray<FCk_Handle_IntegerAttribute>{};

    ForEach_ByName(InAttributeOwner, InAttributeName, [&](FCk_Handle_IntegerAttribute InAttribute)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InAttribute, InOptionalPayload); }

        Ret.Emplace(InAttribute);
    });

    return Ret;
}

auto
    UCk_Utils_IntegerAttribute_UE::
    ForEach_ByName(
        FCk_Handle& InAttributeOwner,
        FGameplayTag InAttributeName,
        const TFunction<void(FCk_Handle_IntegerAttribute)>& InFunc)
    -> void
{
    ForEach_If(InAttributeOwner, InFunc, ck::algo::MatchesGameplayLabelExact{InAttributeName});
}

auto
    UCk_Utils_IntegerAttribute_UE::
    Has_Component(
        const FCk_Handle_IntegerAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent)
    -> bool
{
    switch (InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            return IntegerAttribute_Utils_Min::Has(InAttribute);
        }
        case ECk_MinMaxCurrent::Max:
        {
            return IntegerAttribute_Utils_Max::Has(InAttribute);
        }
        case ECk_MinMaxCurrent::Current:
        {
            return IntegerAttribute_Utils_Current::Has(InAttribute);
        }
        default:
        {
            CK_INVALID_ENUM(InAttributeComponent);
            return {};
        }
    }
}

auto
    UCk_Utils_IntegerAttribute_UE::
    Has_RefillAttribute(
        const FCk_Handle_IntegerAttribute& InAttribute)
    -> bool
{
    return ck::RefillAttribute_Utils::Has(InAttribute);
}

auto
    UCk_Utils_IntegerAttribute_UE::
    TryGet_RefillAttribute(
        const FCk_Handle_IntegerAttribute& InAttribute)
    -> FCk_Handle_IntegerAttributeRefill
{
    if (NOT ck::RefillAttribute_Utils::Has(InAttribute))
    { return {}; }

    return ck::RefillAttribute_Utils::Get_StoredEntity_AsTypeSafe<FCk_Handle_IntegerAttributeRefill>(InAttribute);
}

auto
    UCk_Utils_IntegerAttribute_UE::
    Get_BaseValue(
        const FCk_Handle_IntegerAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent)
    -> int32
{
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Integer Attribute [{}] with Owner [{}] does NOT have a [{}] component"),
        InAttribute,
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute), InAttributeComponent)
    { return {}; }

    switch (InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            return IntegerAttribute_Utils_Min::Get_BaseValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Max:
        {
            return IntegerAttribute_Utils_Max::Get_BaseValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Current:
        {
            return IntegerAttribute_Utils_Current::Get_BaseValue(InAttribute);
        }
        default:
        {
            CK_INVALID_ENUM(InAttributeComponent);
            return {};
        }
    }
}

auto
    UCk_Utils_IntegerAttribute_UE::
    Get_BonusValue(
        const FCk_Handle_IntegerAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent)
    -> int32
{
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Integer Attribute [{}] with Owner [{}] does NOT have a [{}] component"),
        InAttribute,
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute), InAttributeComponent)
    { return {}; }

    switch (InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            return IntegerAttribute_Utils_Min::Get_FinalValue(InAttribute) - IntegerAttribute_Utils_Min::Get_BaseValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Max:
        {
            return IntegerAttribute_Utils_Max::Get_FinalValue(InAttribute) - IntegerAttribute_Utils_Max::Get_BaseValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Current:
        {
            return IntegerAttribute_Utils_Current::Get_FinalValue(InAttribute) - IntegerAttribute_Utils_Current::Get_BaseValue(InAttribute);
        }
        default:
        {
            CK_INVALID_ENUM(InAttributeComponent);
            return {};
        }
    }
}

auto
    UCk_Utils_IntegerAttribute_UE::
    Get_FinalValue(
        const FCk_Handle_IntegerAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent)
    -> int32
{
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Integer Attribute [{}] with Owner [{}] does NOT have a [{}] component"),
        InAttribute,
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute), InAttributeComponent)
    { return {}; }

    switch (InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            return IntegerAttribute_Utils_Min::Get_FinalValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Max:
        {
            return IntegerAttribute_Utils_Max::Get_FinalValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Current:
        {
            return IntegerAttribute_Utils_Current::Get_FinalValue(InAttribute);
        }
        default:
        {
            CK_INVALID_ENUM(InAttributeComponent);
            return {};
        }
    }
}

auto
    UCk_Utils_IntegerAttribute_UE::
    Request_Override(
        UPARAM(ref) FCk_Handle_IntegerAttribute& InAttribute,
        int32 InNewBaseValue,
        ECk_MinMaxCurrent InAttributeComponent)
    -> FCk_Handle_IntegerAttribute
{
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Integer Attribute [{}] with Owner [{}] does NOT have a [{}] component"),
        InAttribute,
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute),
        InAttributeComponent)
    { return InAttribute; }

    UCk_Utils_IntegerAttributeModifier_UE::Add_NotRevocable
    (
        InAttribute,
        ECk_AttributeModifier_Operation::Override,
        FCk_Fragment_IntegerAttributeModifier_ParamsData
        {
            InNewBaseValue,
            InAttributeComponent
        }
    );

    return InAttribute;
}

auto
    UCk_Utils_IntegerAttribute_UE::
    BindTo_OnValueChanged(
        FCk_Handle_IntegerAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent,
        const FCk_Delegate_IntegerAttribute_OnValueChanged& InDelegate,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_IntegerAttribute
{
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Integer Attribute [{}] does NOT have a [{}] component. Cannot BIND to OnValueChanged"),
        InAttribute,
        InAttributeComponent)
    { return InAttribute; }

    switch(InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            CK_SIGNAL_BIND(ck::UUtils_Signal_OnIntegerAttributeValueChanged_Min, InAttribute, InDelegate, InBehavior, InPostFireBehavior);
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            CK_SIGNAL_BIND(ck::UUtils_Signal_OnIntegerAttributeValueChanged_Max, InAttribute, InDelegate, InBehavior, InPostFireBehavior);
            break;
        }
        case ECk_MinMaxCurrent::Current:
        {
            CK_SIGNAL_BIND(ck::UUtils_Signal_OnIntegerAttributeValueChanged_Current, InAttribute, InDelegate, InBehavior, InPostFireBehavior);
            break;
        }
    };

    return InAttribute;
}

auto
    UCk_Utils_IntegerAttribute_UE::
    UnbindFrom_OnValueChanged(
        FCk_Handle_IntegerAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent,
        const FCk_Delegate_IntegerAttribute_OnValueChanged& InDelegate)
    -> FCk_Handle_IntegerAttribute
{
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Integer Attribute [{}] does NOT have a [{}] component. Cannot UNBIND from OnValueChanged"),
        InAttribute,
        InAttributeComponent)
    { return InAttribute; }

    switch(InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnIntegerAttributeValueChanged_Min, InAttribute, InDelegate);
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnIntegerAttributeValueChanged_Max, InAttribute, InDelegate);
            break;
        }
        case ECk_MinMaxCurrent::Current:
        {
            CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnIntegerAttributeValueChanged_Current, InAttribute, InDelegate);
            break;
        }
    };

    return InAttribute;
}

auto
    UCk_Utils_IntegerAttribute_UE::
    BindTo_OnMinClamped(
        FCk_Handle_IntegerAttribute& InAttribute,
        const FCk_Delegate_IntegerAttribute_OnClamped& InDelegate,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_IntegerAttribute
{
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, ECk_MinMaxCurrent::Min),
        TEXT("Integer Attribute [{}] does NOT have a [{}] component. Cannot BIND to OnMinClamped"),
        InAttribute,
        ECk_MinMaxCurrent::Min)
    { return InAttribute; }

    CK_SIGNAL_BIND(ck::UUtils_Signal_OnIntegerAttributeMinClamped, InAttribute, InDelegate, InBehavior, InPostFireBehavior);

    return InAttribute;
}

auto
    UCk_Utils_IntegerAttribute_UE::
    UnbindFrom_OnMinClamped(
        FCk_Handle_IntegerAttribute& InAttribute,
        const FCk_Delegate_IntegerAttribute_OnClamped& InDelegate)
    -> FCk_Handle_IntegerAttribute
{
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, ECk_MinMaxCurrent::Min),
        TEXT("Integer Attribute [{}] does NOT have a [{}] component. Cannot UNBIND from OnMinClamped"),
        InAttribute,
        ECk_MinMaxCurrent::Min)
    { return InAttribute; }

    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnIntegerAttributeMinClamped, InAttribute, InDelegate);

    return InAttribute;
}

auto
    UCk_Utils_IntegerAttribute_UE::
    BindTo_OnMaxClamped(
        FCk_Handle_IntegerAttribute& InAttribute,
        const FCk_Delegate_IntegerAttribute_OnClamped& InDelegate,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_IntegerAttribute
{
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, ECk_MinMaxCurrent::Max),
        TEXT("Integer Attribute [{}] does NOT have a [{}] component. Cannot BIND to OnMaxClamped"),
        InAttribute,
        ECk_MinMaxCurrent::Max)
    { return InAttribute; }

    CK_SIGNAL_BIND(ck::UUtils_Signal_OnIntegerAttributeMaxClamped, InAttribute, InDelegate, InBehavior, InPostFireBehavior);

    return InAttribute;
}

auto
    UCk_Utils_IntegerAttribute_UE::
    UnbindFrom_OnMaxClamped(
        FCk_Handle_IntegerAttribute& InAttribute,
        const FCk_Delegate_IntegerAttribute_OnClamped& InDelegate)
    -> FCk_Handle_IntegerAttribute
{
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, ECk_MinMaxCurrent::Max),
        TEXT("Integer Attribute [{}] does NOT have a [{}] component. Cannot UNBIND from OnMaxClamped"),
        InAttribute,
        ECk_MinMaxCurrent::Max)
    { return InAttribute; }

    CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnIntegerAttributeMaxClamped, InAttribute, InDelegate);

    return InAttribute;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IntegerAttributeRefill_UE::
    Add(
        FCk_Handle_FloatAttribute& InAttributeRefillEntity,
        ECk_Attribute_RefillState InStartingState)
    -> FCk_Handle_IntegerAttributeRefill
{
    InAttributeRefillEntity.Add<ck::FTag_IsRefillAttribute>();

    if (InStartingState == ECk_Attribute_RefillState::Running)
    {
        InAttributeRefillEntity.Add<ck::FTag_IsRefillRunning>();
    }

    return ck::StaticCast<FCk_Handle_IntegerAttributeRefill>(InAttributeRefillEntity);
}

auto
    UCk_Utils_IntegerAttributeRefill_UE::
    Has(
        const FCk_Handle& InAttributeOwnerEntity)
    -> bool
{
    return InAttributeOwnerEntity.Has_Any<ck::FTag_IsRefillAttribute>();
}

auto
    UCk_Utils_IntegerAttributeRefill_UE::
    Get_FillRate(
        const FCk_Handle_IntegerAttributeRefill& InAttributeRefill)
    -> float
{
    const auto& RefillAttributeAsFloatAttribute = UCk_Utils_FloatAttribute_UE::CastChecked(InAttributeRefill);
    return UCk_Utils_FloatAttribute_UE::Get_FinalValue(RefillAttributeAsFloatAttribute);
}

auto
    UCk_Utils_IntegerAttributeRefill_UE::
    Get_RefillState(
        const FCk_Handle_IntegerAttributeRefill& InAttributeRefill)
    -> ECk_Attribute_RefillState
{
    return InAttributeRefill.Has<ck::FTag_IsRefillRunning>()
            ? ECk_Attribute_RefillState::Running
            : ECk_Attribute_RefillState::Paused;
}

auto
    UCk_Utils_IntegerAttributeRefill_UE::
    Request_Pause(
        FCk_Handle_IntegerAttributeRefill& InAttributeRefill)
    -> FCk_Handle_IntegerAttributeRefill
{
    InAttributeRefill.Try_Remove<ck::FTag_IsRefillRunning>();
    return InAttributeRefill;
}

auto
    UCk_Utils_IntegerAttributeRefill_UE::
    Request_Resume(
        FCk_Handle_IntegerAttributeRefill& InAttributeRefill)
    -> FCk_Handle_IntegerAttributeRefill
{
    InAttributeRefill.AddOrGet<ck::FTag_IsRefillRunning>();
    return InAttributeRefill;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_IntegerAttributeModifier_UE::
    Add_Revocable(
        FCk_Handle_IntegerAttribute& InAttribute,
        FGameplayTag InModifierName,
        ECk_AttributeModifier_Operation InModifierOperation,
        const FCk_Fragment_IntegerAttributeModifier_ParamsData& InParams)
    -> FCk_Handle_IntegerAttributeModifier
{
    QUICK_SCOPE_CYCLE_COUNTER(Add_Integer_Modifier_Revocable)

    auto ParamsToUse = InParams;
    ParamsToUse.Set_TargetAttributeName(UCk_Utils_GameplayLabel_UE::Get_Label(InAttribute));

    const auto& AttributeComponent = ParamsToUse.Get_Component();

    CK_ENSURE_IF_NOT(UCk_Utils_IntegerAttribute_UE::Has_Component(InAttribute, AttributeComponent),
        TEXT("Integer Attribute [{}] with Owner [{}] does NOT have a [{}] component. Cannot Add Revocable Modifier"),
        InAttribute,
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute),
        AttributeComponent)
    { return {}; }

    CK_ENSURE_IF_NOT((InModifierOperation == ECk_AttributeModifier_Operation::Divide ? ParamsToUse.Get_ModifierDelta() != 0 : true),
        TEXT("Trying to ADD a new Revocable Modifier [{}] for Integer Attribute [{}] which DIVIDES by 0. Setting it to 1 in non-shipping build"),
        AttributeComponent, InAttribute)
    {
        ParamsToUse.Set_ModifierDelta(1.0f);
    }

    CK_ENSURE_IF_NOT(InModifierOperation != ECk_AttributeModifier_Operation::Override, TEXT("Override Revocable Integer Attribute Modifier is NOT supported!"))
    { return {}; }

    switch (AttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            auto ModifierEntity = IntegerAttributeModifier_Utils_Min::Add_Revocable(InAttribute, ParamsToUse.Get_ModifierDelta(), InModifierOperation, InModifierName);
            return ModifierEntity;
        }
        case ECk_MinMaxCurrent::Max:
        {
            auto ModifierEntity =  IntegerAttributeModifier_Utils_Max::Add_Revocable(InAttribute, ParamsToUse.Get_ModifierDelta(), InModifierOperation, InModifierName);
            return ModifierEntity;
        }
        case ECk_MinMaxCurrent::Current:
        {
            auto ModifierEntity =  IntegerAttributeModifier_Utils_Current::Add_Revocable(InAttribute, ParamsToUse.Get_ModifierDelta(), InModifierOperation, InModifierName);
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
    UCk_Utils_IntegerAttributeModifier_UE::
    Add_NotRevocable(
        FCk_Handle_IntegerAttribute& InAttribute,
        ECk_AttributeModifier_Operation InModifierOperation,
        const FCk_Fragment_IntegerAttributeModifier_ParamsData& InParams)
    -> void
{
    QUICK_SCOPE_CYCLE_COUNTER(Add_Integer_Modifier_NotRevocable)

    auto ParamsToUse = InParams;
    ParamsToUse.Set_TargetAttributeName(UCk_Utils_GameplayLabel_UE::Get_Label(InAttribute));

    const auto& AttributeComponent = ParamsToUse.Get_Component();

    CK_ENSURE_IF_NOT(UCk_Utils_IntegerAttribute_UE::Has_Component(InAttribute, AttributeComponent),
        TEXT("Integer Attribute [{}] with Owner [{}] does NOT have a [{}] component. Cannot Add NotRevocable Modifier"),
        InAttribute,
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute),
        AttributeComponent)
    { return; }

    if (ParamsToUse.Get_ModifierDelta() == 0 &&
        (InModifierOperation == ECk_AttributeModifier_Operation::Add || InModifierOperation  == ECk_AttributeModifier_Operation::Subtract))
    { return; }

    if (ParamsToUse.Get_ModifierDelta() == 1 &&
        (InModifierOperation == ECk_AttributeModifier_Operation::Multiply || InModifierOperation == ECk_AttributeModifier_Operation::Divide))
    { return; }

    CK_ENSURE_IF_NOT((InModifierOperation == ECk_AttributeModifier_Operation::Divide ? ParamsToUse.Get_ModifierDelta() != 0 : true),
        TEXT("Trying to ADD a new NotRevocable Modifier [{}] for Integer Attribute [{}] which DIVIDES by 0. Setting it to 1 in non-shipping build"),
        AttributeComponent, InAttribute)
    {
        ParamsToUse.Set_ModifierDelta(1);
    }

    switch (AttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            IntegerAttributeModifier_Utils_Min::Add_NotRevocable(InAttribute, ParamsToUse.Get_ModifierDelta(), InModifierOperation);
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            IntegerAttributeModifier_Utils_Max::Add_NotRevocable(InAttribute, ParamsToUse.Get_ModifierDelta(), InModifierOperation);
            break;
        }
        case ECk_MinMaxCurrent::Current:
        {
            IntegerAttributeModifier_Utils_Current::Add_NotRevocable(InAttribute, ParamsToUse.Get_ModifierDelta(), InModifierOperation);
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
    UCk_Utils_IntegerAttributeModifier_UE::
    Override(
        FCk_Handle_IntegerAttributeModifier& InAttributeModifierEntity,
        int32 InNewDelta)
    -> FCk_Handle_IntegerAttributeModifier
{
    switch (const auto Component = InAttributeModifierEntity.Get<ECk_MinMaxCurrent>())
    {
        case ECk_MinMaxCurrent::Min:
        {
            if (UCk_Utils_Arithmetic_UE::Get_IsNearlyEqual(IntegerAttributeModifier_Utils_Min::Get_ModifierDeltaValue(InAttributeModifierEntity), InNewDelta))
            { return InAttributeModifierEntity; }

            const auto& ModifierOperation = IntegerAttributeModifier_Utils_Min::Get_ModifierOperation(InAttributeModifierEntity);

            CK_ENSURE_IF_NOT((ModifierOperation == ECk_ArithmeticOperations_Basic::Divide ? InNewDelta != 0 : true),
                TEXT("Trying to OVERRIDE existing Integer Attribute Modifier [{}][{}] with new value which would DIVIDE by 0. Ignoring the change in non-shipping build"),
                InAttributeModifierEntity, Component)
            { return InAttributeModifierEntity; }

            IntegerAttributeModifier_Utils_Min::Override(InAttributeModifierEntity, InNewDelta);
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            if (UCk_Utils_Arithmetic_UE::Get_IsNearlyEqual(IntegerAttributeModifier_Utils_Max::Get_ModifierDeltaValue(InAttributeModifierEntity), InNewDelta))
            { return InAttributeModifierEntity; }

            const auto& ModifierOperation = IntegerAttributeModifier_Utils_Max::Get_ModifierOperation(InAttributeModifierEntity);

            CK_ENSURE_IF_NOT((ModifierOperation == ECk_ArithmeticOperations_Basic::Divide ? InNewDelta != 0 : true),
                TEXT("Trying to OVERRIDE existing Integer Attribute Modifier [{}][{}] with new value which would DIVIDE by 0. Ignoring the change in non-shipping build"),
                InAttributeModifierEntity, Component)
            { return InAttributeModifierEntity; }

            IntegerAttributeModifier_Utils_Max::Override(InAttributeModifierEntity, InNewDelta);
            break;
        }
        case ECk_MinMaxCurrent::Current:
        {
            if (UCk_Utils_Arithmetic_UE::Get_IsNearlyEqual(IntegerAttributeModifier_Utils_Current::Get_ModifierDeltaValue(InAttributeModifierEntity), InNewDelta))
            { return InAttributeModifierEntity; }

            const auto& ModifierOperation = IntegerAttributeModifier_Utils_Current::Get_ModifierOperation(InAttributeModifierEntity);

            CK_ENSURE_IF_NOT((ModifierOperation == ECk_ArithmeticOperations_Basic::Divide ? InNewDelta != 0 : true),
                TEXT("Trying to OVERRIDE existing Integer Attribute Modifier [{}][{}] with new value which would DIVIDE by 0. Ignoring the change in non-shipping build"),
                InAttributeModifierEntity, Component)
            { return InAttributeModifierEntity; }

            IntegerAttributeModifier_Utils_Current::Override(InAttributeModifierEntity, InNewDelta);
            break;
        }
    }

    return InAttributeModifierEntity;
}

auto
    UCk_Utils_IntegerAttributeModifier_UE::
    Get_Delta(
        const FCk_Handle_IntegerAttributeModifier& InAttributeModifierEntity,
        ECk_MinMaxCurrent InComponent)
    -> int32
{
    switch (InComponent)
    {
        case ECk_MinMaxCurrent::Min: return IntegerAttributeModifier_Utils_Min::Get_ModifierDeltaValue(InAttributeModifierEntity);
        case ECk_MinMaxCurrent::Max: return IntegerAttributeModifier_Utils_Max::Get_ModifierDeltaValue(InAttributeModifierEntity);
        case ECk_MinMaxCurrent::Current: return IntegerAttributeModifier_Utils_Current::Get_ModifierDeltaValue(InAttributeModifierEntity);
    }

    return {};
}

auto
    UCk_Utils_IntegerAttributeModifier_UE::
    TryGet(
        const FCk_Handle_IntegerAttribute& InAttribute,
        FGameplayTag InModifierName,
        ECk_MinMaxCurrent InComponent)
    -> FCk_Handle_IntegerAttributeModifier
{
    QUICK_SCOPE_CYCLE_COUNTER(TryGet_IntegerAttributeModifier)
    switch(InComponent)
    {
        case ECk_MinMaxCurrent::Current:
        {
            return RecordOfIntegerAttributeModifiers_Utils_Current::Get_ValidEntry_ByTag(InAttribute, InModifierName);
        }
        case ECk_MinMaxCurrent::Min:
        {
            return RecordOfIntegerAttributeModifiers_Utils_Min::Get_ValidEntry_ByTag(InAttribute, InModifierName);
        }
        case ECk_MinMaxCurrent::Max:
        {
            return RecordOfIntegerAttributeModifiers_Utils_Max::Get_ValidEntry_ByTag(InAttribute, InModifierName);
        }
        default:
        {
            CK_INVALID_ENUM(InComponent);
            return {};
        }
    }
}

auto
    UCk_Utils_IntegerAttributeModifier_UE::
    TryGet_If(
        const FCk_Handle_IntegerAttribute& InAttribute,
        FGameplayTag InModifierName,
        ECk_MinMaxCurrent InComponent,
        const TFunction<bool(FCk_Handle_IntegerAttributeModifier)>& InPredicate)
    -> FCk_Handle_IntegerAttributeModifier
{
    switch(InComponent)
    {
        case ECk_MinMaxCurrent::Current:
        {
            return RecordOfIntegerAttributeModifiers_Utils_Current::Get_ValidEntry_If(InAttribute, [&](const FCk_Handle& InHandle) -> bool
            {
                if (NOT ck::algo::MatchesGameplayLabel{InModifierName}(InHandle))
                { return false; }

                return InPredicate(Cast(InHandle));
            });
        }
        case ECk_MinMaxCurrent::Min:
        {
            return RecordOfIntegerAttributeModifiers_Utils_Min::Get_ValidEntry_If(InAttribute, [&](const FCk_Handle& InHandle) -> bool
            {
                if (NOT ck::algo::MatchesGameplayLabel{InModifierName}(InHandle))
                { return false; }

                return InPredicate(Cast(InHandle));
            });
        }
        case ECk_MinMaxCurrent::Max:
        {
            return RecordOfIntegerAttributeModifiers_Utils_Max::Get_ValidEntry_If(InAttribute, [&](const FCk_Handle& InHandle) -> bool
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
    UCk_Utils_IntegerAttributeModifier_UE::
    Remove(
        FCk_Handle_IntegerAttributeModifier& InAttributeModifierEntity)
    -> FCk_Handle_IntegerAttribute
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAttributeModifierEntity),
        TEXT("Trying to Remove and INVALID Integer Attribute Modifier. Is this on a Client and targeting a Replicated Attribute?\n"
             "If yes, then it's possible it was destroyed as a consequence of a sync from the Server"))
    { return {}; }

    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InAttributeModifierEntity);

    auto AttributeModifierOwnerEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttributeModifierEntity);

    return UCk_Utils_IntegerAttribute_UE::CastChecked(AttributeModifierOwnerEntity);
}

auto
    UCk_Utils_IntegerAttributeModifier_UE::
    Has(
        const FCk_Handle& InModifierEntity)
    -> bool
{
    return InModifierEntity.Has_Any<ck::FFragment_IntegerAttributeModifier_Min, ck::FFragment_IntegerAttributeModifier_Current,ck::FFragment_IntegerAttributeModifier_Max>();
}

auto
    UCk_Utils_IntegerAttributeModifier_UE::
    ForEach(
        FCk_Handle_IntegerAttribute& InAttributeOwner,
        const FInstancedStruct&    InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        ECk_MinMaxCurrent InAttributeComponent)
    -> TArray<FCk_Handle_IntegerAttributeModifier>
{
    auto ToRet = TArray<FCk_Handle_IntegerAttributeModifier>{};

    ForEach(InAttributeOwner, [&](const FCk_Handle_IntegerAttributeModifier& InAttribute)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InAttribute, InOptionalPayload); }
        else
        { ToRet.Emplace(InAttribute); }
    }, InAttributeComponent);

    return ToRet;
}

auto
    UCk_Utils_IntegerAttributeModifier_UE::
    ForEach(
        FCk_Handle_IntegerAttribute& InAttribute,
        const TFunction<void(FCk_Handle_IntegerAttributeModifier)>& InFunc,
        ECk_MinMaxCurrent InAttributeComponent)
    -> void
{
    switch(InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Current:
        {
            RecordOfIntegerAttributeModifiers_Utils_Current::ForEach_ValidEntry(InAttribute, InFunc);
            break;
        }
        case ECk_MinMaxCurrent::Min:
        {
            RecordOfIntegerAttributeModifiers_Utils_Min::ForEach_ValidEntry(InAttribute, InFunc);
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            RecordOfIntegerAttributeModifiers_Utils_Max::ForEach_ValidEntry(InAttribute, InFunc);
            break;
        }
    }
}

auto
    UCk_Utils_IntegerAttributeModifier_UE::
    ForEach_If(
        FCk_Handle_IntegerAttribute& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate,
        ECk_MinMaxCurrent InAttributeComponent)
    -> TArray<FCk_Handle_IntegerAttributeModifier>
{
    auto ToRet = TArray<FCk_Handle_IntegerAttributeModifier>{};

    ForEach_If
    (
        InAttributeOwner,
        [&](FCk_Handle_IntegerAttributeModifier InAttribute)
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
    UCk_Utils_IntegerAttributeModifier_UE::
    ForEach_If(
        FCk_Handle_IntegerAttribute& InAttribute,
        const TFunction<void(FCk_Handle_IntegerAttributeModifier)>& InFunc,
        const TFunction<bool(FCk_Handle_IntegerAttributeModifier)>& InPredicate,
        ECk_MinMaxCurrent InAttributeComponent)
    -> void
{
    switch(InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Current:
        {
            RecordOfIntegerAttributeModifiers_Utils_Current::ForEach_ValidEntry_If(InAttribute, InFunc, InPredicate);
            break;
        }
        case ECk_MinMaxCurrent::Min:
        {
            RecordOfIntegerAttributeModifiers_Utils_Min::ForEach_ValidEntry_If(InAttribute, InFunc, InPredicate);
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            RecordOfIntegerAttributeModifiers_Utils_Max::ForEach_ValidEntry_If(InAttribute, InFunc, InPredicate);
            break;
        }
    }
}

auto
    UCk_Utils_IntegerAttributeModifier_UE::
    Request_ClearAllModifiers(
        FCk_Handle_IntegerAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent)
    -> void
{
    switch (InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Current:
        {
            IntegerAttributeModifier_Utils_Current::Request_ClearAllModifiers(InAttribute);
            break;
        }
        case ECk_MinMaxCurrent::Min:
        {
            IntegerAttributeModifier_Utils_Min::Request_ClearAllModifiers(InAttribute);
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            IntegerAttributeModifier_Utils_Max::Request_ClearAllModifiers(InAttribute);
            break;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
