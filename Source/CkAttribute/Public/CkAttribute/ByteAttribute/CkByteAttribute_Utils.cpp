#include "CkByteAttribute_Utils.h"

#include "CkAttribute/CkAttribute_Log.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/Arithmetic/CkArithmetic_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Fragment.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

#include "CkLabel/CkLabel_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ByteAttribute_UE::
    Add(
        FCk_Handle& InAttributeOwnerEntity,
        const FCk_Fragment_ByteAttribute_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> FCk_Handle_ByteAttribute
{
    RecordOfByteAttributes_Utils::AddIfMissing(InAttributeOwnerEntity, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);

    auto NewAttributeEntity = [&]
    {
        auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttributeOwnerEntity);
        return ck::StaticCast<FCk_Handle_ByteAttribute>(NewEntity);
    }();

    ByteAttribute_Utils_Current::Add(NewAttributeEntity, InParams.Get_BaseValue());

    switch (InParams.Get_MinMax())
    {
        case ECk_MinMax::None:
        {
            break;
        }
        case ECk_MinMax::Min:
        {
            ByteAttribute_Utils_Min::Add(NewAttributeEntity, InParams.Get_MinValue());
            break;
        }
        case ECk_MinMax::Max:
        {
            ByteAttribute_Utils_Max::Add(NewAttributeEntity, InParams.Get_MaxValue());
            break;
        }
        case ECk_MinMax::MinMax:
        {
            ByteAttribute_Utils_Min::Add(NewAttributeEntity, InParams.Get_MinValue());
            ByteAttribute_Utils_Max::Add(NewAttributeEntity, InParams.Get_MaxValue());
            break;
        }
    }

    if (InReplicates == ECk_Replication::Replicates)
    { NewAttributeEntity.Add<ck::FTag_ReplicatedAttribute>(); }

    UCk_Utils_GameplayLabel_UE::Add(NewAttributeEntity, InParams.Get_Name());
    RecordOfByteAttributes_Utils::Request_Connect(InAttributeOwnerEntity, NewAttributeEntity);

    if (InReplicates == ECk_Replication::DoesNotReplicate)
    {
        ck::attribute::VeryVerbose
        (
            TEXT("Skipping creation of Byte Attribute Rep Fragment on Entity [{}] because it's set to [{}]"),
            InAttributeOwnerEntity,
            InReplicates
        );
    }
    else
    {
        UCk_Utils_Ecs_Net_UE::TryAddReplicatedFragment<UCk_Fragment_ByteAttribute_Rep>(InAttributeOwnerEntity);
    }

    // it's possible that we have pending replication info
    if (UCk_Utils_Net_UE::Get_IsEntityNetMode_Client(InAttributeOwnerEntity))
    {
        if (UCk_Utils_Ecs_Net_UE::Get_HasReplicatedFragment<UCk_Fragment_ByteAttribute_Rep>(InAttributeOwnerEntity))
        {
            InAttributeOwnerEntity.Try_Transform<TObjectPtr<UCk_Fragment_ByteAttribute_Rep>>([&](TObjectPtr<UCk_Fragment_ByteAttribute_Rep>& InRepComp)
            {
                InRepComp->Request_TryUpdateReplicatedAttributes();
            });
        }
    }

    return NewAttributeEntity;
}

auto
    UCk_Utils_ByteAttribute_UE::
    AddMultiple(
        FCk_Handle& InHandle,
        const FCk_Fragment_MultipleByteAttribute_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> TArray<FCk_Handle_ByteAttribute>
{
    return ck::algo::Transform<TArray<FCk_Handle_ByteAttribute>>(
        InParams.Get_ByteAttributeParams(), [&](const FCk_Fragment_ByteAttribute_ParamsData& InParam)
    {
        return Add(InHandle, InParam, InReplicates);
    });
}

auto
    UCk_Utils_ByteAttribute_UE::
    Has_Any(
        const FCk_Handle& InAttributeOwnerEntity)
    -> bool
{
    return RecordOfByteAttributes_Utils::Has(InAttributeOwnerEntity);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(ByteAttribute, UCk_Utils_ByteAttribute_UE, FCk_Handle_ByteAttribute, ck::FFragment_ByteAttribute_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ByteAttribute_UE::
    TryGet(
        const FCk_Handle& InAttributeOwnerEntity,
        FGameplayTag      InAttributeName)
    -> FCk_Handle_ByteAttribute
{
    return RecordOfByteAttributes_Utils::Get_ValidEntry_If(InAttributeOwnerEntity,
        ck::algo::MatchesGameplayLabelExact{InAttributeName});
}

auto
    UCk_Utils_ByteAttribute_UE::
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
    UCk_Utils_ByteAttribute_UE::
    ForEach(
        FCk_Handle& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_ByteAttribute>
{
    auto ToRet = TArray<FCk_Handle_ByteAttribute>{};

    ForEach(InAttributeOwner, [&](const FCk_Handle_ByteAttribute& InAttribute)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InAttribute, InOptionalPayload); }
        else
        { ToRet.Emplace(InAttribute); }
    });

    return ToRet;
}

auto
    UCk_Utils_ByteAttribute_UE::
    ForEach(
        FCk_Handle& InAttributeOwner,
        const TFunction<void(FCk_Handle_ByteAttribute)>& InFunc)
    -> void
{
    RecordOfByteAttributes_Utils::ForEach_ValidEntry(InAttributeOwner, InFunc);
}

auto
    UCk_Utils_ByteAttribute_UE::
    ForEach_If(
        FCk_Handle& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate)
    -> TArray<FCk_Handle_ByteAttribute>
{
    auto ToRet = TArray<FCk_Handle_ByteAttribute>{};

    ForEach_If
    (
        InAttributeOwner,
        [&](FCk_Handle_ByteAttribute InAttribute)
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
    UCk_Utils_ByteAttribute_UE::
    ForEach_If(
        FCk_Handle& InAttributeOwner,
        const TFunction<void(FCk_Handle_ByteAttribute)>& InFunc,
        const TFunction<bool(FCk_Handle_ByteAttribute)>& InPredicate)
    -> void
{
    RecordOfByteAttributes_Utils::ForEach_ValidEntry_If(InAttributeOwner, InFunc, InPredicate);
}

auto
    UCk_Utils_ByteAttribute_UE::
    Has_Component(
        const FCk_Handle_ByteAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent)
    -> bool
{
    switch (InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            return ByteAttribute_Utils_Min::Has(InAttribute);
        }
        case ECk_MinMaxCurrent::Max:
        {
            return ByteAttribute_Utils_Max::Has(InAttribute);
        }
        case ECk_MinMaxCurrent::Current:
        {
            return ByteAttribute_Utils_Current::Has(InAttribute);
        }
        default:
        {
            CK_INVALID_ENUM(InAttributeComponent);
            return {};
        }
    }
}

auto
    UCk_Utils_ByteAttribute_UE::
    Get_BaseValue(
        const FCk_Handle_ByteAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent)
    -> uint8
{
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Byte Attribute [{}] with Owner [{}] does NOT have a [{}] component"),
        InAttribute,
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute), InAttributeComponent)
    { return {}; }

    switch (InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            return ByteAttribute_Utils_Min::Get_BaseValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Max:
        {
            return ByteAttribute_Utils_Max::Get_BaseValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Current:
        {
            return ByteAttribute_Utils_Current::Get_BaseValue(InAttribute);
        }
        default:
            return {};

    }
}

auto
    UCk_Utils_ByteAttribute_UE::
    Get_BonusValue(
        const FCk_Handle_ByteAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent)
    -> uint8
{
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Byte Attribute [{}] with Owner [{}] does NOT have a [{}] component"),
        InAttribute,
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute), InAttributeComponent)
    { return {}; }

    switch (InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            return ByteAttribute_Utils_Min::Get_FinalValue(InAttribute) - ByteAttribute_Utils_Min::Get_BaseValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Max:
        {
            return ByteAttribute_Utils_Max::Get_FinalValue(InAttribute) - ByteAttribute_Utils_Max::Get_BaseValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Current:
        {
            return ByteAttribute_Utils_Current::Get_FinalValue(InAttribute) - ByteAttribute_Utils_Current::Get_BaseValue(InAttribute);
        }
        default:
            return {};
    }
}

auto
    UCk_Utils_ByteAttribute_UE::
    Get_FinalValue(
        const FCk_Handle_ByteAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent)
    -> uint8
{
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Byte Attribute [{}] with Owner [{}] does NOT have a [{}] component"),
        InAttribute,
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute), InAttributeComponent)
    { return {}; }


    switch (InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            return ByteAttribute_Utils_Min::Get_FinalValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Max:
        {
            return ByteAttribute_Utils_Max::Get_FinalValue(InAttribute);
        }
        case ECk_MinMaxCurrent::Current:
        {
            return ByteAttribute_Utils_Current::Get_FinalValue(InAttribute);
        }
        default:
            return {};
    }
}

auto
    UCk_Utils_ByteAttribute_UE::
    Request_Override(
        UPARAM(ref) FCk_Handle_ByteAttribute& InAttribute,
        uint8 InNewBaseValue,
        ECk_MinMaxCurrent InAttributeComponent)
    -> FCk_Handle_ByteAttribute
{
    const auto CurrentBaseValue = Get_BaseValue(InAttribute, InAttributeComponent);
    const uint8 Delta = InNewBaseValue - CurrentBaseValue;

    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Byte Attribute [{}] with Owner [{}] does NOT have a [{}] component"),
        InAttribute,
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute), InAttributeComponent)
    { return {}; }

    UCk_Utils_ByteAttributeModifier_UE::Add(InAttribute, ck::FAttributeModifier_Tags::Get_Override(),
        FCk_Fragment_ByteAttributeModifier_ParamsData
        {
            Delta,
            ECk_ArithmeticOperations_Basic::Add,
            ECk_ModifierOperation_RevocablePolicy::NotRevocable,
            InAttributeComponent
        });

    return InAttribute;
}

auto
    UCk_Utils_ByteAttribute_UE::
    BindTo_OnValueChanged(
        FCk_Handle_ByteAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_ByteAttribute_OnValueChanged& InDelegate)
    -> FCk_Handle_ByteAttribute
{
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Byte Attribute [{}] does NOT have a [{}] component. Cannot BIND to OnValueChanged"),
        InAttribute,
        InAttributeComponent)
    { return InAttribute; }

    switch(InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            CK_SIGNAL_BIND(ck::UUtils_Signal_OnByteAttributeValueChanged_Min, InAttribute, InDelegate, InBehavior, InPostFireBehavior);
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            CK_SIGNAL_BIND(ck::UUtils_Signal_OnByteAttributeValueChanged_Max, InAttribute, InDelegate, InBehavior, InPostFireBehavior);
            break;
        }
        case ECk_MinMaxCurrent::Current:
        {
            CK_SIGNAL_BIND(ck::UUtils_Signal_OnByteAttributeValueChanged_Current, InAttribute, InDelegate, InBehavior, InPostFireBehavior);
            break;
        }
    };

    return InAttribute;
}

auto
    UCk_Utils_ByteAttribute_UE::
    UnbindFrom_OnValueChanged(
        FCk_Handle_ByteAttribute& InAttribute,
        ECk_MinMaxCurrent InAttributeComponent,
        const FCk_Delegate_ByteAttribute_OnValueChanged& InDelegate)
    -> FCk_Handle_ByteAttribute
{
    CK_ENSURE_IF_NOT(Has_Component(InAttribute, InAttributeComponent),
        TEXT("Byte Attribute [{}] does NOT have a [{}] component. Cannot UNBIND from OnValueChanged"),
        InAttribute,
        InAttributeComponent)
    { return InAttribute; }

    switch(InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnByteAttributeValueChanged_Min, InAttribute, InDelegate);
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnByteAttributeValueChanged_Max, InAttribute, InDelegate);
            break;
        }
        case ECk_MinMaxCurrent::Current:
        {
            CK_SIGNAL_UNBIND(ck::UUtils_Signal_OnByteAttributeValueChanged_Current, InAttribute, InDelegate);
            break;
        }
    };

    return InAttribute;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ByteAttributeModifier_UE::
    Add(
        FCk_Handle_ByteAttribute& InAttribute,
        FGameplayTag InModifierName,
        const FCk_Fragment_ByteAttributeModifier_ParamsData& InParams)
    -> FCk_Handle_ByteAttributeModifier
{
    auto ParamsToUse = InParams;
    ParamsToUse.Set_TargetAttributeName(UCk_Utils_GameplayLabel_UE::Get_Label(InAttribute));

    const auto& ModifierOperation = ParamsToUse.Get_ModifierOperation();
    const auto& AttributeComponent = ParamsToUse.Get_Component();
    const auto& RevocablePolicy = InParams.Get_ModifierOperation_RevocablePolicy();

    CK_ENSURE_IF_NOT(UCk_Utils_ByteAttribute_UE::Has_Component(InAttribute, AttributeComponent),
        TEXT("Byte Attribute [{}] with Owner [{}] does NOT have a [{}] component. Cannot Add Modifier"),
        InAttribute,
        UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttribute),
        AttributeComponent)
    { return {}; }

    if (RevocablePolicy == ECk_ModifierOperation_RevocablePolicy::NotRevocable)
    {
        if (ParamsToUse.Get_ModifierDelta() == 0 &&
            (ModifierOperation == ECk_ArithmeticOperations_Basic::Add || ModifierOperation  == ECk_ArithmeticOperations_Basic::Subtract))
        { return {}; }

        if (ParamsToUse.Get_ModifierDelta() == 1 &&
            (ModifierOperation == ECk_ArithmeticOperations_Basic::Multiply || ModifierOperation == ECk_ArithmeticOperations_Basic::Divide))
        { return {}; }
    }

    CK_ENSURE_IF_NOT((ModifierOperation == ECk_ArithmeticOperations_Basic::Divide ? ParamsToUse.Get_ModifierDelta() != 0 : true),
        TEXT("Trying to ADD a new Modifier [{}][{}] for Byte Attribute [{}] which DIVIDES by 0. Setting it to 1 in non-shipping build"),
        InModifierName, AttributeComponent, InAttribute)
    {
        ParamsToUse.Set_ModifierDelta(1);
    }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAttribute);
    auto NewModifierEntity = ck::StaticCast<FCk_Handle_ByteAttributeModifier>(NewEntity);
    UCk_Utils_GameplayLabel_UE::Add(NewModifierEntity, InModifierName);

    switch (AttributeComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            ByteAttributeModifier_Utils_Min::Add(NewModifierEntity, ParamsToUse.Get_ModifierDelta(),ModifierOperation, RevocablePolicy);
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            ByteAttributeModifier_Utils_Max::Add(NewModifierEntity, ParamsToUse.Get_ModifierDelta(),ModifierOperation, RevocablePolicy);
            break;
        }
        case ECk_MinMaxCurrent::Current:
        {
            ByteAttributeModifier_Utils_Current::Add(NewModifierEntity, ParamsToUse.Get_ModifierDelta(),ModifierOperation, RevocablePolicy);
            break;
        }
    }

    return NewModifierEntity;
}

auto
    UCk_Utils_ByteAttributeModifier_UE::
    Override(
        FCk_Handle_ByteAttributeModifier& InAttributeModifierEntity,
        uint8 InNewDelta,
        ECk_MinMaxCurrent InComponent)
    -> FCk_Handle_ByteAttributeModifier
{
    switch (InComponent)
    {
        case ECk_MinMaxCurrent::Min:
        {
            if (UCk_Utils_Arithmetic_UE::Get_IsNearlyEqual(ByteAttributeModifier_Utils_Min::Get_ModifierDeltaValue(InAttributeModifierEntity), InNewDelta))
            { return InAttributeModifierEntity; }

            const auto& ModifierOperation = ByteAttributeModifier_Utils_Min::Get_ModifierOperation(InAttributeModifierEntity);

            CK_ENSURE_IF_NOT((ModifierOperation == ECk_ArithmeticOperations_Basic::Divide ? InNewDelta != 0 : true),
                TEXT("Trying to OVERRIDE existing Byte Attribute Modifier [{}][{}] with new value which would DIVIDE by 0. Ignoring the change in non-shipping build"),
                InAttributeModifierEntity, InComponent)
            { return InAttributeModifierEntity; }

            ByteAttributeModifier_Utils_Min::Override(InAttributeModifierEntity, InNewDelta);
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            if (UCk_Utils_Arithmetic_UE::Get_IsNearlyEqual(ByteAttributeModifier_Utils_Max::Get_ModifierDeltaValue(InAttributeModifierEntity), InNewDelta))
            { return InAttributeModifierEntity; }

            const auto& ModifierOperation = ByteAttributeModifier_Utils_Max::Get_ModifierOperation(InAttributeModifierEntity);

            CK_ENSURE_IF_NOT((ModifierOperation == ECk_ArithmeticOperations_Basic::Divide ? InNewDelta != 0 : true),
                TEXT("Trying to OVERRIDE existing Byte Attribute Modifier [{}][{}] with new value which would DIVIDE by 0. Ignoring the change in non-shipping build"),
                InAttributeModifierEntity, InComponent)
            { return InAttributeModifierEntity; }

            ByteAttributeModifier_Utils_Max::Override(InAttributeModifierEntity, InNewDelta);
            break;
        }
        case ECk_MinMaxCurrent::Current:
        {
                if (UCk_Utils_Arithmetic_UE::Get_IsNearlyEqual(ByteAttributeModifier_Utils_Current::Get_ModifierDeltaValue(InAttributeModifierEntity), InNewDelta))
            { return InAttributeModifierEntity; }

            const auto& ModifierOperation = ByteAttributeModifier_Utils_Current::Get_ModifierOperation(InAttributeModifierEntity);

            CK_ENSURE_IF_NOT((ModifierOperation == ECk_ArithmeticOperations_Basic::Divide ? InNewDelta != 0 : true),
                TEXT("Trying to OVERRIDE existing Byte Attribute Modifier [{}][{}] with new value which would DIVIDE by 0. Ignoring the change in non-shipping build"),
                InAttributeModifierEntity, InComponent)
            { return InAttributeModifierEntity; }

            ByteAttributeModifier_Utils_Current::Override(InAttributeModifierEntity, InNewDelta);
            break;
        }
    }

    return InAttributeModifierEntity;
}

auto
    UCk_Utils_ByteAttributeModifier_UE::
    Get_Delta(
        const FCk_Handle_ByteAttributeModifier& InAttributeModifierEntity,
        ECk_MinMaxCurrent InComponent)
    -> uint8
{
    switch (InComponent)
    {
        case ECk_MinMaxCurrent::Min: return ByteAttributeModifier_Utils_Min::Get_ModifierDeltaValue(InAttributeModifierEntity);
        case ECk_MinMaxCurrent::Max: return ByteAttributeModifier_Utils_Max::Get_ModifierDeltaValue(InAttributeModifierEntity);
        case ECk_MinMaxCurrent::Current: return ByteAttributeModifier_Utils_Current::Get_ModifierDeltaValue(InAttributeModifierEntity);
    }

    return {};
}

auto
    UCk_Utils_ByteAttributeModifier_UE::
    TryGet(
        const FCk_Handle_ByteAttribute& InAttribute,
        FGameplayTag InModifierName,
        ECk_MinMaxCurrent InComponent)
    -> FCk_Handle_ByteAttributeModifier
{
    switch(InComponent)
    {
        case ECk_MinMaxCurrent::Current:
        {
            return RecordOfByteAttributeModifiers_Utils_Current::Get_ValidEntry_If(InAttribute, ck::algo::MatchesGameplayLabel{InModifierName});
        }
        case ECk_MinMaxCurrent::Min:
        {
            return RecordOfByteAttributeModifiers_Utils_Min::Get_ValidEntry_If(InAttribute, ck::algo::MatchesGameplayLabel{InModifierName});
        }
        case ECk_MinMaxCurrent::Max:
        {
            return RecordOfByteAttributeModifiers_Utils_Max::Get_ValidEntry_If(InAttribute, ck::algo::MatchesGameplayLabel{InModifierName});
        }
        default:
        {
            return {};
        }
    }
}

auto
    UCk_Utils_ByteAttributeModifier_UE::
    TryGet_If(
        const FCk_Handle_ByteAttribute& InAttribute,
        FGameplayTag InModifierName,
        ECk_MinMaxCurrent InComponent,
        const TFunction<bool(FCk_Handle_ByteAttributeModifier)>& InPredicate)
    -> FCk_Handle_ByteAttributeModifier
{
    switch(InComponent)
    {
        case ECk_MinMaxCurrent::Current:
        {
            return RecordOfByteAttributeModifiers_Utils_Current::Get_ValidEntry_If(InAttribute, [&](const FCk_Handle& InHandle) -> bool
            {
                if (NOT ck::algo::MatchesGameplayLabel{InModifierName}(InHandle))
                { return false; }

                return InPredicate(Cast(InHandle));
            });
        }
        case ECk_MinMaxCurrent::Min:
        {
            return RecordOfByteAttributeModifiers_Utils_Min::Get_ValidEntry_If(InAttribute, [&](const FCk_Handle& InHandle) -> bool
            {
                if (NOT ck::algo::MatchesGameplayLabel{InModifierName}(InHandle))
                { return false; }

                return InPredicate(Cast(InHandle));
            });
        }
        case ECk_MinMaxCurrent::Max:
        {
            return RecordOfByteAttributeModifiers_Utils_Max::Get_ValidEntry_If(InAttribute, [&](const FCk_Handle& InHandle) -> bool
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
    UCk_Utils_ByteAttributeModifier_UE::
    Remove(
        FCk_Handle_ByteAttributeModifier& InAttributeModifierEntity)
    -> FCk_Handle_ByteAttribute
{
    UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InAttributeModifierEntity);

    auto AttributeModifierOwnerEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttributeModifierEntity);

    return UCk_Utils_ByteAttribute_UE::CastChecked(AttributeModifierOwnerEntity);
}

auto
    UCk_Utils_ByteAttributeModifier_UE::
    Has(
        const FCk_Handle& InModifierEntity)
    -> bool
{
    return InModifierEntity.Has_Any<ck::FFragment_ByteAttributeModifier_Min, ck::FFragment_ByteAttributeModifier_Current,ck::FFragment_ByteAttributeModifier_Max>();
}

auto
    UCk_Utils_ByteAttributeModifier_UE::
    ForEach(
        FCk_Handle_ByteAttribute& InAttributeOwner,
        const FInstancedStruct&    InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        ECk_MinMaxCurrent InAttributeComponent)
    -> TArray<FCk_Handle_ByteAttributeModifier>
{
    auto ToRet = TArray<FCk_Handle_ByteAttributeModifier>{};

    ForEach(InAttributeOwner, [&](const FCk_Handle_ByteAttributeModifier& InAttribute)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InAttribute, InOptionalPayload); }
        else
        { ToRet.Emplace(InAttribute); }
    }, InAttributeComponent);

    return ToRet;
}

auto
    UCk_Utils_ByteAttributeModifier_UE::
    ForEach(
        FCk_Handle_ByteAttribute& InAttribute,
        const TFunction<void(FCk_Handle_ByteAttributeModifier)>& InFunc,
        ECk_MinMaxCurrent InAttributeComponent)
    -> void
{
    switch(InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Current:
        {
            RecordOfByteAttributeModifiers_Utils_Current::ForEach_ValidEntry(InAttribute, InFunc);
            break;
        }
        case ECk_MinMaxCurrent::Min:
        {
            RecordOfByteAttributeModifiers_Utils_Min::ForEach_ValidEntry(InAttribute, InFunc);
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            RecordOfByteAttributeModifiers_Utils_Max::ForEach_ValidEntry(InAttribute, InFunc);
            break;
        }
    }
}

auto
    UCk_Utils_ByteAttributeModifier_UE::
    ForEach_If(
        FCk_Handle_ByteAttribute& InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate,
        ECk_MinMaxCurrent InAttributeComponent)
    -> TArray<FCk_Handle_ByteAttributeModifier>
{
    auto ToRet = TArray<FCk_Handle_ByteAttributeModifier>{};

    ForEach_If
    (
        InAttributeOwner,
        [&](FCk_Handle_ByteAttributeModifier InAttribute)
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
    UCk_Utils_ByteAttributeModifier_UE::
    ForEach_If(
        FCk_Handle_ByteAttribute& InAttribute,
        const TFunction<void(FCk_Handle_ByteAttributeModifier)>& InFunc,
        const TFunction<bool(FCk_Handle_ByteAttributeModifier)>& InPredicate,
        ECk_MinMaxCurrent InAttributeComponent)
    -> void
{
    switch(InAttributeComponent)
    {
        case ECk_MinMaxCurrent::Current:
        {
            RecordOfByteAttributeModifiers_Utils_Current::ForEach_ValidEntry_If(InAttribute, InFunc, InPredicate);
            break;
        }
        case ECk_MinMaxCurrent::Min:
        {
            RecordOfByteAttributeModifiers_Utils_Min::ForEach_ValidEntry_If(InAttribute, InFunc, InPredicate);
            break;
        }
        case ECk_MinMaxCurrent::Max:
        {
            RecordOfByteAttributeModifiers_Utils_Max::ForEach_ValidEntry_If(InAttribute, InFunc, InPredicate);
            break;
        }
    }
}

auto
    UCk_Utils_ByteAttributeModifier_UE::
    DoGet_IsModifierUnique(
        const FCk_Handle_ByteAttributeModifier& InAttributeModifierEntity,
        ECk_MinMaxCurrent InComponent)
    -> bool
{
    const auto AttributeEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InAttributeModifierEntity);
    const auto& NameOfModifier = UCk_Utils_GameplayLabel_UE::Get_Label(InAttributeModifierEntity);

    const auto Predicate = [&](const FCk_Handle_ByteAttributeModifier& InCurrentModifier)
    {
        return UCk_Utils_GameplayLabel_UE::Matches(InCurrentModifier, NameOfModifier);
    };

    switch(InComponent)
    {
        case ECk_MinMaxCurrent::Current: return RecordOfByteAttributeModifiers_Utils_Current::Get_ValidEntriesCount_If(AttributeEntity, Predicate) < 2;
        case ECk_MinMaxCurrent::Min: return RecordOfByteAttributeModifiers_Utils_Min::Get_ValidEntriesCount_If(AttributeEntity, Predicate) < 2;
        case ECk_MinMaxCurrent::Max: return RecordOfByteAttributeModifiers_Utils_Max::Get_ValidEntriesCount_If(AttributeEntity, Predicate) < 2;
    }

    return {};
}

// --------------------------------------------------------------------------------------------------------------------
