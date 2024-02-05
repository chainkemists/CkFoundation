#include "CkMeterAttribute_Utils.h"

#include "CkAttribute/CkAttribute_Log.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Fragment.h"
#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"
#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkLabel/CkLabel_Utils.h"
#include "Kismet/KismetMathLibrary.h"

#include "CkNet/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"

#include <GameplayTagsManager.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FMeterAttribute_Tags FMeterAttribute_Tags::_Tags;

    auto
        FMeterAttribute_Tags::
        AddTags()
        -> void
    {
        auto& Manager = UGameplayTagsManager::Get();

        _MinCapacity = Manager.AddNativeGameplayTag(TEXT("Ck.Attribute.Meter.MinCapacity"));
        _MaxCapacity = Manager.AddNativeGameplayTag(TEXT("Ck.Attribute.Meter.MaxCapacity"));
        _Current     = Manager.AddNativeGameplayTag(TEXT("Ck.Attribute.Meter.Current"));
    }

    auto
        FMeterAttribute_Tags::
        Get_MinCapacity()
        -> FGameplayTag
    {
        return _Tags._MinCapacity;
    }

    auto
        FMeterAttribute_Tags::
        Get_MaxCapacity()
        -> FGameplayTag
    {
        return _Tags._MaxCapacity;
    }

    auto
        FMeterAttribute_Tags::
        Get_Current()
        -> FGameplayTag
    {
        return _Tags._Current;
    }

}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_MeterAttribute_ConstructionScript_PDA::
    DoConstruct_Implementation(
         FCk_Handle& InHandle)
    -> void
{
    using RecordOfMeterAttributes_Utils = UCk_Utils_MeterAttribute_UE::RecordOfMeterAttributes_Utils;

    // Our owner may be storing the starting Parameters for us. In which case, initialize using those parameters
    auto LifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InHandle);

    const auto& ParamsToUse = [&]
    {
        const auto& MeterAttributeParams = UCk_Utils_MeterAttribute_UE::
            Pop_Parameter<FCk_Fragment_MeterAttribute_ParamsData>(LifetimeOwner,
                [&](const FCk_Fragment_MeterAttribute_ParamsData& InParams)
            {
                if (NOT UCk_Utils_GameplayLabel_UE::Has(InHandle))
                { return true; }

                return UCk_Utils_GameplayLabel_UE::MatchesExact(InHandle, InParams.Get_AttributeName());
            });

        if (ck::IsValid(MeterAttributeParams))
        { return *MeterAttributeParams; }

        return _Params;
    }();

    CK_ENSURE_IF_NOT(NOT UCk_Utils_MeterAttribute_UE::Has(LifetimeOwner, ParamsToUse.Get_AttributeName()),
        TEXT("The Entity [{}] already has the Attribute [{}] and is a DUPLICATE"),
        InHandle, ParamsToUse.Get_AttributeName())
    { return; }

    const auto& MeterParams = ParamsToUse.Get_AttributeBaseValue().Get_Params();
    const auto& MeterCapacity = MeterParams.Get_Capacity();
    const auto& MeterStartingPercentage = MeterParams.Get_StartingPercentage();

    auto FloatAttributeOwner = UCk_Utils_FloatAttribute_UE::AddMultiple
    (
        InHandle,
        FCk_Fragment_MultipleFloatAttribute_ParamsData
        {
            {
                { ck::FMeterAttribute_Tags::Get_MinCapacity(), MeterCapacity.Get_MinCapacity() },
                { ck::FMeterAttribute_Tags::Get_MaxCapacity(), MeterCapacity.Get_MaxCapacity() },
                { ck::FMeterAttribute_Tags::Get_Current(), MeterCapacity.Get_MaxCapacity() * MeterStartingPercentage.Get_Value() }
            }
        },
        ParamsToUse.Get_Replicates()
    );

    // TODO: Adding all 3 tags to the Meter Attribute Entity which may seem redundant. However, this is in preparation
    // for when we have OPTIONAL min/max
    FloatAttributeOwner.AddOrGet<ck::FTag_Meter_MinValue>();
    FloatAttributeOwner.AddOrGet<ck::FTag_Meter_MaxValue>();
    FloatAttributeOwner.AddOrGet<ck::FTag_Meter_CurrentValue>();

    UCk_Utils_GameplayLabel_UE::Add(FloatAttributeOwner, ParamsToUse.Get_AttributeName());

    RecordOfMeterAttributes_Utils::AddIfMissing(
        LifetimeOwner, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfMeterAttributes_Utils::Request_Connect(LifetimeOwner, FloatAttributeOwner);
}

auto
    UCk_Utils_MeterAttribute_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_MeterAttribute_ParamsData& InConstructionScriptData,
        ECk_Replication InReplicates)
    -> void
{
    auto MeterParamsDataToUse = InConstructionScriptData;
    MeterParamsDataToUse.Set_Replicates(InReplicates);

    CK_ENSURE_IF_NOT(ck::IsValid(MeterParamsDataToUse.Get_AttributeName()),
        TEXT("Invalid Attribute Name. Unable to add MeterAttribute to Entity [{}]"), InHandle)
    { return; }

    Store_Parameter(InHandle, MeterParamsDataToUse);

    // Meter is an Entity that is made up of sub-entities (FloatAttribute) and thus requires us constructing it just like
    // we would an Unreal Entity
    UCk_Utils_EntityReplicationDriver_UE::Request_TryBuildAndReplicate(InHandle,
        FCk_EntityReplicationDriver_ConstructionInfo{UCk_MeterAttribute_ConstructionScript_PDA::StaticClass()}
        .Set_Label(MeterParamsDataToUse.Get_AttributeName()));
}

auto
    UCk_Utils_MeterAttribute_UE::
    AddMultiple(
        FCk_Handle InHandle,
        const FCk_Fragment_MultipleMeterAttribute_ParamsData& InParams,
        ECk_Replication InReplicates)
    -> void
{
    for (const auto& Param : InParams.Get_MeterAttributeParams())
    {
        Add(InHandle, Param, InReplicates);
    }
}

auto
    UCk_Utils_MeterAttribute_UE::
    Has(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> bool
{
    if (NOT RecordOfMeterAttributes_Utils::Has(InAttributeOwnerEntity))
    { return false; }

    const auto FoundEntity = RecordOfMeterAttributes_Utils::Get_ValidEntry_If(
        InAttributeOwnerEntity, ck::algo::MatchesGameplayLabelExact{InAttributeName});

    return ck::IsValid(FoundEntity);
}

auto
    UCk_Utils_MeterAttribute_UE::
    Has_Any(
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    return RecordOfMeterAttributes_Utils::Has(InAttributeOwnerEntity);
}

auto
    UCk_Utils_MeterAttribute_UE::
    Ensure(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InAttributeOwnerEntity, InAttributeName), TEXT("Handle [{}] does NOT have a Meter Attribute [{}]"),
        InAttributeOwnerEntity, InAttributeName)
    { return false; }

    return true;
}

auto
    UCk_Utils_MeterAttribute_UE::
    Ensure_Any(
        FCk_Handle InAttributeOwnerEntity)
    -> bool
{
    CK_ENSURE_IF_NOT(Has_Any(InAttributeOwnerEntity), TEXT("Handle [{}] does NOT have any Meter Attribute [{}]"), InAttributeOwnerEntity)
    { return false; }

    return true;
}

auto
    UCk_Utils_MeterAttribute_UE::
    Get_All(
        FCk_Handle InAttributeOwnerEntity)
    -> TArray<FGameplayTag>
{
    if (NOT RecordOfMeterAttributes_Utils::Has(InAttributeOwnerEntity))
    { return {}; }

    auto AllMeters = TArray<FGameplayTag>{};

    RecordOfMeterAttributes_Utils::ForEach_ValidEntry(InAttributeOwnerEntity, [&](FCk_Handle InMeterAttributeEntity)
    {
        AllMeters.Add(UCk_Utils_GameplayLabel_UE::Get_Label(InMeterAttributeEntity));
    });

    return AllMeters;
}

auto
    UCk_Utils_MeterAttribute_UE::
    ForEach(
        FCk_Handle InAttributeOwner,
        const FInstancedStruct&    InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle>
{
    auto ToRet = TArray<FCk_Handle>{};

    ForEach(InAttributeOwner, [&](const FCk_Handle& InAttribute)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InAttribute, InOptionalPayload); }
        else
        { ToRet.Emplace(InAttribute); }
    });

    return ToRet;
}

auto
    UCk_Utils_MeterAttribute_UE::
    ForEach(
        const FCk_Handle& InAttributeOwner,
        const TFunction<void(FCk_Handle)>& InFunc)
    -> void
{
    if (NOT Ensure_Any(InAttributeOwner))
    { return; }

    RecordOfMeterAttributes_Utils::ForEach_ValidEntry
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
    UCk_Utils_MeterAttribute_UE::
    ForEach_If(
        FCk_Handle InAttributeOwner,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate,
        const FCk_Predicate_InHandle_OutResult& InPredicate)
    -> TArray<FCk_Handle>
{
    auto ToRet = TArray<FCk_Handle>{};

    ForEach_If
    (
        InAttributeOwner,
        [&](FCk_Handle InAttribute)
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
    UCk_Utils_MeterAttribute_UE::
    ForEach_If(
        const FCk_Handle& InAttributeOwner,
        const TFunction<void(FCk_Handle)>& InFunc,
        const TFunction<bool(FCk_Handle)>& InPredicate)
    -> void
{
    if (NOT Ensure_Any(InAttributeOwner))
    { return; }

    RecordOfMeterAttributes_Utils::ForEach_ValidEntry_If
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
    UCk_Utils_MeterAttribute_UE::
    Get_BaseValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> FCk_Meter
{
    CK_ENSURE_IF_NOT(ck::IsValid(InAttributeName),
        TEXT("Invalid AttributeName for Entity [{}]. Unable to get the BaseValue"), InAttributeOwnerEntity)
    { return {}; }

    const auto [MinCapacity, MaxCapacity, Current] = Get_MinMaxAndCurrentAttributeEntities(InAttributeOwnerEntity, InAttributeName);

    if (ck::Is_NOT_Valid(MinCapacity) || ck::Is_NOT_Valid(MaxCapacity) || ck::Is_NOT_Valid(Current))
    { return {}; }

    const auto& MinCapacityValue =  FloatAttribute_Utils::Get_BaseValue(MinCapacity);
    const auto& MaxCapacityValue = FloatAttribute_Utils::Get_BaseValue(MaxCapacity);
    const auto& CurrentValue = FloatAttribute_Utils::Get_BaseValue(Current);

    const auto& CurrentPercentage = UKismetMathLibrary::MapRangeClamped(CurrentValue, MinCapacityValue, MaxCapacityValue, 0.0f, 1.0f);

    return FCk_Meter
    {
        FCk_Meter_Params{FCk_Meter_Capacity{MaxCapacityValue}.Set_MinCapacity(MinCapacityValue)}
        .Set_StartingPercentage(FCk_FloatRange_0to1{static_cast<float>(CurrentPercentage)})
    };
}

auto
    UCk_Utils_MeterAttribute_UE::
    Get_BonusValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> FCk_Meter
{
    return Get_FinalValue(InAttributeOwnerEntity, InAttributeName) - Get_BaseValue(InAttributeOwnerEntity, InAttributeName);
}

auto
    UCk_Utils_MeterAttribute_UE::
    Get_FinalValue(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> FCk_Meter
{
    const auto [MinCapacity, MaxCapacity, Current] = Get_MinMaxAndCurrentAttributeEntities(InAttributeOwnerEntity, InAttributeName);

    if (ck::Is_NOT_Valid(MinCapacity) || ck::Is_NOT_Valid(MaxCapacity) || ck::Is_NOT_Valid(Current))
    { return {}; }

    const auto& MinCapacityValue =  FloatAttribute_Utils::Get_FinalValue(MinCapacity);
    const auto& MaxCapacityValue = FloatAttribute_Utils::Get_FinalValue(MaxCapacity);
    const auto& CurrentValue = FloatAttribute_Utils::Get_FinalValue(Current);

    const auto& CurrentPercentage = UKismetMathLibrary::MapRangeClamped(CurrentValue, MinCapacityValue, MaxCapacityValue, 0.0f, 1.0f);

    return FCk_Meter
    {
        FCk_Meter_Params{FCk_Meter_Capacity{MaxCapacityValue}.Set_MinCapacity(MinCapacityValue)}
        .Set_StartingPercentage(FCk_FloatRange_0to1{static_cast<float>(CurrentPercentage)})
    };
}

auto
    UCk_Utils_MeterAttribute_UE::
    Request_Override(
        FCk_Handle   InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FCk_Meter    InNewBaseValue,
        FCk_Meter_Mask InMask)
    -> void
{
    auto FoundEntity = RecordOfMeterAttributes_Utils::Get_ValidEntry_If(
        InAttributeOwnerEntity, ck::algo::MatchesGameplayLabelExact{InAttributeName});

    CK_ENSURE_IF_NOT(ck::IsValid(FoundEntity), TEXT("Could NOT find Attribute [{}] in Entity [{}]. Unable to OVERRIDE the Meter to [{}]."),
        InAttributeName, InAttributeOwnerEntity, InNewBaseValue)
    { return; }

    if (EnumHasAnyFlags(InMask.Get_MeterMask(), ECk_Meter_Mask::Min))
    {
        UCk_Utils_FloatAttribute_UE::Request_Override(FoundEntity, ck::FMeterAttribute_Tags::Get_MinCapacity(),
            InNewBaseValue.Get_Params().Get_Capacity().Get_MinCapacity());
    }

    if (EnumHasAnyFlags(InMask.Get_MeterMask(), ECk_Meter_Mask::Max))
    {
        UCk_Utils_FloatAttribute_UE::Request_Override(FoundEntity, ck::FMeterAttribute_Tags::Get_MaxCapacity(),
            InNewBaseValue.Get_Params().Get_Capacity().Get_MaxCapacity());
    }

    if (EnumHasAnyFlags(InMask.Get_MeterMask(), ECk_Meter_Mask::Current))
    {
        UCk_Utils_FloatAttribute_UE::Request_Override(FoundEntity, ck::FMeterAttribute_Tags::Get_Current(),
            InNewBaseValue.Get_Value().Get_CurrentValue());
    }
}

auto
    UCk_Utils_MeterAttribute_UE::
    BindTo_OnValueChanged(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        ECk_Signal_BindingPolicy InBehavior,
        ECk_Signal_PostFireBehavior InPostFireBehavior,
        const FCk_Delegate_MeterAttribute_OnValueChanged& InDelegate)
    -> void
{
    auto FoundEntity = RecordOfMeterAttributes_Utils::Get_ValidEntry_If(
        InAttributeOwnerEntity, ck::algo::MatchesGameplayLabelExact{InAttributeName});

    CK_ENSURE_IF_NOT(ck::IsValid(FoundEntity), TEXT("Could NOT find Attribute [{}] in Entity [{}]. Unable to BIND on ValueChanged the Delegate [{}]"),
        InAttributeName, InAttributeOwnerEntity, InDelegate.GetFunctionName())
    { return; }

    const auto [MinCapacity, MaxCapacity, Current] = Get_MinMaxAndCurrentAttributeEntities(InAttributeOwnerEntity, InAttributeName);

    if (ck::Is_NOT_Valid(MinCapacity) || ck::Is_NOT_Valid(MaxCapacity) || ck::Is_NOT_Valid(Current))
    { return; }

    using Signal = ck::UUtils_Signal_OnFloatAttributeValueChanged;

    Signal::Bind<&UCk_Utils_MeterAttribute_UE::OnMinCapacityChanged>(MinCapacity, InBehavior, InPostFireBehavior);
    Signal::Bind<&UCk_Utils_MeterAttribute_UE::OnMaxCapacityChanged>(MaxCapacity, InBehavior, InPostFireBehavior);
    Signal::Bind<&UCk_Utils_MeterAttribute_UE::OnCurrentValueCanged>(Current, InBehavior, InPostFireBehavior);

    ck::UUtils_Signal_OnMeterAttributeValueChanged::Bind(FoundEntity, InDelegate, InBehavior);
}

auto
    UCk_Utils_MeterAttribute_UE::
    UnbindFrom_OnValueChanged(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        const FCk_Delegate_MeterAttribute_OnValueChanged& InDelegate)
    -> void
{
    const auto FoundEntity = RecordOfMeterAttributes_Utils::Get_ValidEntry_If(
        InAttributeOwnerEntity, ck::algo::MatchesGameplayLabelExact{InAttributeName});

    CK_ENSURE_IF_NOT(ck::IsValid(FoundEntity), TEXT("Could NOT find Attribute [{}] in Entity [{}]. Unable to UNBIND on ValueChanged the Delegate [{}]"),
        InAttributeName, InAttributeOwnerEntity, InDelegate.GetFunctionName())
    { return; }

    const auto [MinCapacity, MaxCapacity, Current] = Get_MinMaxAndCurrentAttributeEntities(FoundEntity, InAttributeName);

    if (ck::Is_NOT_Valid(MinCapacity) || ck::Is_NOT_Valid(MaxCapacity) || ck::Is_NOT_Valid(Current))
    { return; }

    ck::UUtils_Signal_OnFloatAttributeValueChanged::Unbind<OnMinCapacityChanged>(MinCapacity);
    ck::UUtils_Signal_OnFloatAttributeValueChanged::Unbind<OnMaxCapacityChanged>(MaxCapacity);
    ck::UUtils_Signal_OnFloatAttributeValueChanged::Unbind<OnCurrentValueCanged>(Current);

    ck::UUtils_Signal_OnMeterAttributeValueChanged::Unbind(FoundEntity, InDelegate);
}

auto
    UCk_Utils_MeterAttribute_UE::
    Get_MinAndCurrentAttributeEntities(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> std::tuple<FCk_Handle, FCk_Handle>
{
    const auto FoundEntity = RecordOfMeterAttributes_Utils::Get_ValidEntry_If(InAttributeOwnerEntity,
        ck::algo::MatchesGameplayLabelExact{InAttributeName});

    CK_ENSURE_IF_NOT(ck::IsValid(FoundEntity), TEXT("Could not find Attribute [{}] in Entity [{}]"),
        InAttributeName, InAttributeOwnerEntity)
    { return {}; }

    const auto& MinCapacity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(FoundEntity, ck::FMeterAttribute_Tags::Get_MinCapacity());

    const auto& Current = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(FoundEntity, ck::FMeterAttribute_Tags::Get_Current());

    return {MinCapacity, Current};
}

auto
    UCk_Utils_MeterAttribute_UE::
    Get_MaxAndCurrentAttributeEntities(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> std::tuple<FCk_Handle, FCk_Handle>
{
    const auto FoundEntity = RecordOfMeterAttributes_Utils::Get_ValidEntry_If(InAttributeOwnerEntity,
        ck::algo::MatchesGameplayLabelExact{InAttributeName});

    CK_ENSURE_IF_NOT(ck::IsValid(FoundEntity), TEXT("Could not find Attribute [{}] in Entity [{}]"),
        InAttributeName, InAttributeOwnerEntity)
    { return {}; }

    const auto& MaxCapacity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(FoundEntity, ck::FMeterAttribute_Tags::Get_MaxCapacity());

    const auto& Current = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(FoundEntity, ck::FMeterAttribute_Tags::Get_Current());

    return {MaxCapacity, Current};
}

auto
    UCk_Utils_MeterAttribute_UE::
    Get_MinMaxAndCurrentAttributeEntities(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName)
    -> std::tuple<FCk_Handle, FCk_Handle, FCk_Handle>
{
    const auto FoundEntity = RecordOfMeterAttributes_Utils::Get_ValidEntry_If(InAttributeOwnerEntity,
        ck::algo::MatchesGameplayLabelExact{InAttributeName});

    CK_ENSURE_IF_NOT(ck::IsValid(FoundEntity), TEXT("Could not find Attribute [{}] in Entity [{}]"),
        InAttributeName, InAttributeOwnerEntity)
    { return {}; }

    const auto& MinCapacity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(FoundEntity, ck::FMeterAttribute_Tags::Get_MinCapacity());

    const auto& MaxCapacity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(FoundEntity, ck::FMeterAttribute_Tags::Get_MaxCapacity());

    const auto& Current = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(FoundEntity, ck::FMeterAttribute_Tags::Get_Current());

    return {MinCapacity, MaxCapacity, Current};
}

auto
    UCk_Utils_MeterAttribute_UE::
    OnMinCapacityChanged(
        FCk_Handle InFloatAttributeEntity,
        ck::TPayload_Attribute_OnValueChanged<ck::FFragment_FloatAttribute> InValueChanged)
    -> void
{
    const auto MeterAttributeEntity = InFloatAttributeEntity;
    const auto MeterAttributeLabel = UCk_Utils_GameplayLabel_UE::Get_Label(MeterAttributeEntity);
    const auto MeterAttributeOwnerEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(MeterAttributeEntity);

    ck::UUtils_Signal_OnMeterAttributeValueChanged::Broadcast
    (
        MeterAttributeEntity,
        ck::MakePayload
        (
            MeterAttributeOwnerEntity,
            FCk_Payload_MeterAttribute_OnValueChanged
            {
                Get_BaseValue(MeterAttributeOwnerEntity, MeterAttributeLabel),
                Get_FinalValue(MeterAttributeOwnerEntity, MeterAttributeLabel)
            }
        )
    );
}

auto
    UCk_Utils_MeterAttribute_UE::
    OnMaxCapacityChanged(
        FCk_Handle InFloatAttributeEntity,
        ck::TPayload_Attribute_OnValueChanged<ck::FFragment_FloatAttribute> InValueChanged)
    -> void
{
    const auto MeterAttributeEntity = InFloatAttributeEntity;
    const auto MeterAttributeLabel = UCk_Utils_GameplayLabel_UE::Get_Label(MeterAttributeEntity);
    const auto MeterAttributeOwnerEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(MeterAttributeEntity);

    ck::UUtils_Signal_OnMeterAttributeValueChanged::Broadcast
    (
        MeterAttributeEntity,
        ck::MakePayload
        (
            MeterAttributeOwnerEntity,
            FCk_Payload_MeterAttribute_OnValueChanged
            {
                Get_BaseValue(MeterAttributeOwnerEntity, MeterAttributeLabel),
                Get_FinalValue(MeterAttributeOwnerEntity, MeterAttributeLabel)
            }
        )
    );
}

auto
    UCk_Utils_MeterAttribute_UE::
    OnCurrentValueCanged(
        FCk_Handle InFloatAttributeEntity,
        ck::TPayload_Attribute_OnValueChanged<ck::FFragment_FloatAttribute> InValueChanged)
    -> void
{
    const auto MeterAttributeEntity = InFloatAttributeEntity;
    const auto MeterAttributeLabel = UCk_Utils_GameplayLabel_UE::Get_Label(MeterAttributeEntity);
    const auto MeterAttributeOwnerEntity = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(MeterAttributeEntity);

    ck::UUtils_Signal_OnMeterAttributeValueChanged::Broadcast
    (
        MeterAttributeEntity,
        ck::MakePayload
        (
            MeterAttributeOwnerEntity,
            FCk_Payload_MeterAttribute_OnValueChanged
            {
                Get_BaseValue(MeterAttributeOwnerEntity, MeterAttributeLabel),
                Get_FinalValue(MeterAttributeOwnerEntity, MeterAttributeLabel)
            }
        )
    );
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_MeterAttributeModifier_UE::
    Add(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InModifierName,
        const FCk_Fragment_MeterAttributeModifier_ParamsData& InParams)
    -> void
{
    const auto& ModifierPolicy     = InParams.Get_ModifierPolicy();
    const auto& ModifyMinCapacity  = EnumHasAnyFlags(ModifierPolicy, ECk_MeterAttributeModifier_Policy::MinCapacity);
    const auto& ModifyMaxCapacity  = EnumHasAnyFlags(ModifierPolicy, ECk_MeterAttributeModifier_Policy::MaxCapacity);
    const auto& ModifyCurrentValue = EnumHasAnyFlags(ModifierPolicy, ECk_MeterAttributeModifier_Policy::CurrentValue);

    CK_ENSURE_IF_NOT(ModifyMinCapacity || ModifyMaxCapacity || ModifyCurrentValue,
        TEXT("Cannot Meter Attribute Modifier [{}] to Entity [{}] because it targets NO meter component "
            "(MinCapacity, MaxCapacity or CurrentValue)"), InModifierName, InAttributeOwnerEntity)
    { return; }

    auto FoundEntity = UCk_Utils_MeterAttribute_UE::RecordOfMeterAttributes_Utils::Get_ValidEntry_If
    (
        InAttributeOwnerEntity,
        ck::algo::MatchesGameplayLabelExact{InParams.Get_TargetAttributeName()}
    );

    CK_ENSURE_IF_NOT(ck::IsValid(FoundEntity), TEXT("Could NOT find Attribute [{}] in Entity [{}]. Cannot ADD Modifier [{}]"),
        InParams.Get_TargetAttributeName(), InAttributeOwnerEntity, InModifierName)
    { return; }

    const auto& ModifierDeltaParams = InParams.Get_ModifierDelta().Get_Params();

    if (ModifyMinCapacity)
    {
        UCk_Utils_FloatAttributeModifier_UE::Add
        (
            FoundEntity,
            InModifierName,
            FCk_Fragment_FloatAttributeModifier_ParamsData{FCk_Fragment_FloatAttributeModifier_ParamsData
            {
                ModifierDeltaParams.Get_Capacity().Get_MinCapacity(),
                ck::FMeterAttribute_Tags::Get_MinCapacity(),
                InParams.Get_ModifierOperation(),
                InParams.Get_ModifierOperation_RevocablePolicy()
            }}
        );
    }

    if (ModifyMaxCapacity)
    {
        UCk_Utils_FloatAttributeModifier_UE::Add
        (
            FoundEntity,
            InModifierName,
            FCk_Fragment_FloatAttributeModifier_ParamsData{FCk_Fragment_FloatAttributeModifier_ParamsData
            {
                ModifierDeltaParams.Get_Capacity().Get_MaxCapacity(),
                ck::FMeterAttribute_Tags::Get_MaxCapacity(),
                InParams.Get_ModifierOperation(),
                InParams.Get_ModifierOperation_RevocablePolicy()
            }}
        );
    }

    if (ModifyCurrentValue)
    {
        const auto& MeterCurrentDelta = UKismetMathLibrary::MapRangeClamped
        (
            ModifierDeltaParams.Get_StartingPercentage().Get_Value(),
            0.0f,
            1.0f,
            ModifierDeltaParams.Get_Capacity().Get_MinCapacity(),
            ModifierDeltaParams.Get_Capacity().Get_MaxCapacity()
        );

        UCk_Utils_FloatAttributeModifier_UE::Add
        (
            FoundEntity,
            InModifierName,
            FCk_Fragment_FloatAttributeModifier_ParamsData{FCk_Fragment_FloatAttributeModifier_ParamsData
            {
                static_cast<float>(MeterCurrentDelta),
                ck::FMeterAttribute_Tags::Get_Current(),
                InParams.Get_ModifierOperation(),
                InParams.Get_ModifierOperation_RevocablePolicy()
            }}
        );
    }

    FoundEntity.AddOrGet<ck::FTag_Meter_RequiresUpdate>();
}

auto
    UCk_Utils_MeterAttributeModifier_UE::
    Has(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName)
    -> bool
{
    const auto FoundEntity = UCk_Utils_MeterAttribute_UE::RecordOfMeterAttributes_Utils::Get_ValidEntry_If(
        InAttributeOwnerEntity, ck::algo::MatchesGameplayLabelExact{InAttributeName});

    CK_ENSURE_IF_NOT(ck::IsValid(FoundEntity), TEXT("Could NOT find Attribute [{}] in Entity [{}]. Cannot HAS check for Modifier [{}]"),
        InAttributeName, InAttributeOwnerEntity, InModifierName)
    { return false; }

    const auto& HasMeterModifier_MinCapacity  = UCk_Utils_FloatAttributeModifier_UE::Has(
        FoundEntity, ck::FMeterAttribute_Tags::Get_MinCapacity(), InModifierName);
    const auto& HasMeterModifier_MaxCapacity  = UCk_Utils_FloatAttributeModifier_UE::Has(
        FoundEntity, ck::FMeterAttribute_Tags::Get_MaxCapacity(), InModifierName);
    const auto& HasMeterModifier_CurrentValue = UCk_Utils_FloatAttributeModifier_UE::Has(
        FoundEntity, ck::FMeterAttribute_Tags::Get_Current(),     InModifierName);

    return HasMeterModifier_MinCapacity || HasMeterModifier_MaxCapacity || HasMeterModifier_CurrentValue;
}

auto
    UCk_Utils_MeterAttributeModifier_UE::
    Ensure(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InAttributeOwnerEntity, InAttributeName, InModifierName),
        TEXT("Handle [{}] does NOT have a Meter Attribute Modifier with Name [{}]"), InAttributeOwnerEntity, InModifierName)
    { return false; }

    return true;
}

auto
    UCk_Utils_MeterAttributeModifier_UE::
    Remove(
        FCk_Handle InAttributeOwnerEntity,
        FGameplayTag InAttributeName,
        FGameplayTag InModifierName)
    -> void
{
    auto FoundEntity = UCk_Utils_MeterAttribute_UE::RecordOfMeterAttributes_Utils::Get_ValidEntry_If(
        InAttributeOwnerEntity, ck::algo::MatchesGameplayLabelExact{InAttributeName});

    CK_ENSURE_IF_NOT(ck::IsValid(FoundEntity), TEXT("Could NOT find Attribute [{}] in Entity [{}]. Cannot REMOVE Modifier [{}]"),
        InAttributeName, InAttributeOwnerEntity, InModifierName)
    { return; }

    const auto& HasMeterModifier_MinCapacity  = UCk_Utils_FloatAttributeModifier_UE::Has(
        FoundEntity, ck::FMeterAttribute_Tags::Get_MinCapacity(), InModifierName);
    const auto& HasMeterModifier_MaxCapacity  = UCk_Utils_FloatAttributeModifier_UE::Has(
        FoundEntity, ck::FMeterAttribute_Tags::Get_MaxCapacity(), InModifierName);
    const auto& HasMeterModifier_CurrentValue = UCk_Utils_FloatAttributeModifier_UE::Has(
        FoundEntity, ck::FMeterAttribute_Tags::Get_Current(),     InModifierName);

    if (HasMeterModifier_MinCapacity)
    {
        UCk_Utils_FloatAttributeModifier_UE::Remove(FoundEntity, ck::FMeterAttribute_Tags::Get_MinCapacity(), InModifierName);
    }

    if (HasMeterModifier_MaxCapacity)
    {
        UCk_Utils_FloatAttributeModifier_UE::Remove(FoundEntity, ck::FMeterAttribute_Tags::Get_MaxCapacity(), InModifierName);
    }

    if (HasMeterModifier_CurrentValue)
    {
        UCk_Utils_FloatAttributeModifier_UE::Remove(FoundEntity, ck::FMeterAttribute_Tags::Get_Current(), InModifierName);
    }

    FoundEntity.AddOrGet<ck::FTag_Meter_RequiresUpdate>();
}

// --------------------------------------------------------------------------------------------------------------------
