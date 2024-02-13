//#include "CkNumericAttribute_Utils.h"
//
//#include "CkAttribute/CkAttribute_Log.h"
//#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"
//#include "CkAttribute/MeterAttribute/CkMeterAttribute_Utils.h"
//#include "CkCore/Object/CkObject_Utils.h"
//
//#include <Kismet/KismetMathLibrary.h>
//
//// --------------------------------------------------------------------------------------------------------------------
//
//auto
//    UCk_Utils_NumericAttribute_UE::
//    Add(
//        FCk_Handle InHandle,
//        const FCk_Fragment_NumericAttribute_ParamsData& InParams,
//        ECk_Replication InReplicates)
//    -> void
//{
//    const auto& ConstraintsPolicy = InParams.Get_ConstraintsPolicy();
//    const auto& AttributeName = InParams.Get_AttributeName();
//    const auto& AttributeStartingValue = InParams.Get_AttributeStartingValue();
//
//    const auto& HasMinimumValue = EnumHasAnyFlags(ConstraintsPolicy, ECk_NumericAttribute_ConstraintsPolicy::HasMinimumValue);
//    const auto& HasMaximumValue = EnumHasAnyFlags(ConstraintsPolicy, ECk_NumericAttribute_ConstraintsPolicy::HasMaximumValue);
//
//    if (HasMinimumValue || HasMaximumValue)
//    {
//        constexpr float DefaultMinimumValue = -100000.0f;
//        constexpr float DefaultMaximumValue = 100000.0f;
//
//        const auto& MinimumValue = HasMinimumValue ? InParams.Get_AttributeMinimumValue() : DefaultMinimumValue;
//        const auto& MaximumValue = HasMaximumValue ? InParams.Get_AttributeMaximumValue() : DefaultMaximumValue;
//
//        const auto& StartingPercentage = UKismetMathLibrary::MapRangeClamped(AttributeStartingValue, MinimumValue, MaximumValue, 0.0f, 1.0f);
//
//        UCk_Utils_MeterAttribute_UE::Add
//        (
//            InHandle,
//            FCk_Fragment_MeterAttribute_ParamsData
//            {
//                AttributeName,
//                FCk_Meter
//                {
//                    FCk_Meter_Params
//                    {
//                        FCk_Meter_Capacity{MaximumValue}
//                        .Set_MinCapacity(MinimumValue)
//                    }
//                    .Set_StartingPercentage(FCk_FloatRange_0to1{static_cast<ck::FFragment_FloatAttribute::AttributeDataType>(StartingPercentage)})
//                }
//            },
//            InReplicates
//        );
//    }
//    else
//    {
//        UCk_Utils_FloatAttribute_UE::Add(InHandle, FCk_Fragment_FloatAttribute_ParamsData{AttributeName, AttributeStartingValue});
//    }
//}
//
//auto
//    UCk_Utils_NumericAttribute_UE::
//    AddMultiple(
//        FCk_Handle InHandle,
//        const FCk_Fragment_MultipleNumericAttribute_ParamsData& InParams,
//        ECk_Replication InReplicates)
//    -> void
//{
//    for (const auto& Param : InParams.Get_NumericAttributeParams())
//    {
//        Add(InHandle, Param, InReplicates);
//    }
//}
//
//auto
//    UCk_Utils_NumericAttribute_UE::
//    Has(
//        FCk_Handle InAttributeOwnerEntity,
//        FGameplayTag InAttributeName)
//    -> bool
//{
//    return UCk_Utils_FloatAttribute_UE::Has_Attribute(InAttributeOwnerEntity, InAttributeName) ||
//        UCk_Utils_MeterAttribute_UE::Has(InAttributeOwnerEntity, InAttributeName);
//}
//
//auto
//    UCk_Utils_NumericAttribute_UE::
//    Has_Any(
//        FCk_Handle InAttributeOwnerEntity)
//    -> bool
//{
//    return UCk_Utils_FloatAttribute_UE::Has_Any_Attribute(InAttributeOwnerEntity) || UCk_Utils_MeterAttribute_UE::Has_Any(InAttributeOwnerEntity);
//}
//
//auto
//    UCk_Utils_NumericAttribute_UE::
//    Ensure(
//        FCk_Handle InAttributeOwnerEntity,
//        FGameplayTag InAttributeName)
//    -> bool
//{
//    CK_ENSURE_IF_NOT(Has(InAttributeOwnerEntity, InAttributeName), TEXT("Handle [{}] does NOT have a Numeric Attribute [{}]"), InAttributeOwnerEntity, InAttributeName)
//    { return false; }
//
//    return true;
//}
//
//auto
//    UCk_Utils_NumericAttribute_UE::
//    Ensure_Any(
//        FCk_Handle InAttributeOwnerEntity)
//    -> bool
//{
//    CK_ENSURE_IF_NOT(Has_Any(InAttributeOwnerEntity), TEXT("Handle [{}] does NOT have any Numeric Attribute [{}]"), InAttributeOwnerEntity)
//    { return false; }
//
//    return true;
//}
//
//auto
//    UCk_Utils_NumericAttribute_UE::
//    Get_CurrentValue(
//        FCk_Handle   InAttributeOwnerEntity,
//        FGameplayTag InAttributeName)
//    -> float
//{
//    if (NOT Ensure(InAttributeOwnerEntity, InAttributeName))
//    { return {}; }
//
//    if (UCk_Utils_FloatAttribute_UE::Has_Attribute(InAttributeOwnerEntity, InAttributeName))
//    { return UCk_Utils_FloatAttribute_UE::Get_FinalValue(InAttributeOwnerEntity, InAttributeName); }
//
//    return UCk_Utils_MeterAttribute_UE::Get_FinalValue(InAttributeOwnerEntity, InAttributeName).Get_Value().Get_CurrentValue();
//}
//
//auto
//    UCk_Utils_NumericAttribute_UE::
//    Get_MinValue(
//        FCk_Handle   InAttributeOwnerEntity,
//        FGameplayTag InAttributeName,
//        bool&        OutHasMinValue)
//    -> float
//{
//    OutHasMinValue = UCk_Utils_MeterAttribute_UE::Has(InAttributeOwnerEntity, InAttributeName);
//
//    if (OutHasMinValue)
//    {
//        return UCk_Utils_MeterAttribute_UE::Get_FinalValue(InAttributeOwnerEntity, InAttributeName).Get_Params().Get_Capacity().Get_MinCapacity();
//    }
//
//    return {};
//}
//
//auto
//    UCk_Utils_NumericAttribute_UE::
//    Get_MaxValue(
//        FCk_Handle   InAttributeOwnerEntity,
//        FGameplayTag InAttributeName,
//        bool&        OutHasMaxValue)
//    -> float
//{
//    OutHasMaxValue = UCk_Utils_MeterAttribute_UE::Has(InAttributeOwnerEntity, InAttributeName);
//
//    if (OutHasMaxValue)
//    {
//        return UCk_Utils_MeterAttribute_UE::Get_FinalValue(InAttributeOwnerEntity, InAttributeName).Get_Params().Get_Capacity().Get_MaxCapacity();
//    }
//
//    return {};
//}
//
//auto
//    UCk_Utils_NumericAttribute_UE::
//    BindTo_OnValueChanged(
//        FCk_Handle InAttributeOwnerEntity,
//        FGameplayTag InAttributeName,
//        ECk_Signal_BindingPolicy InBehavior,
//        ECk_Signal_PostFireBehavior InPostFireBehavior,
//        const FCk_Delegate_NumericAttribute_OnValueChanged& InDelegate)
//    -> void
//{
//    if (UCk_Utils_FloatAttribute_UE::Has_Attribute(InAttributeOwnerEntity, InAttributeName))
//    {
//        const auto& FloatAttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
//
//        ck::UUtils_Signal_OnFloatAttributeValueChanged::Bind<&ThisType::OnFloatAttribute_ValueChanged>(FloatAttributeEntity, InBehavior, InPostFireBehavior);
//        ck::UUtils_Signal_NumericOnAttributeValueChanged::Bind(InAttributeOwnerEntity, InDelegate, InBehavior);
//
//        return;
//    }
//
//    if (UCk_Utils_MeterAttribute_UE::Has(InAttributeOwnerEntity, InAttributeName))
//    {
//        const auto& MeterAttributeEntity = RecordOfMeterAttributes_Utils::Get_ValidEntry_If(InAttributeOwnerEntity, ck::algo::MatchesGameplayLabelExact{InAttributeName});
//        const auto& MeterCurrenValueFloatAttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(MeterAttributeEntity, ck::FMeterAttribute_Tags::Get_Current());
//
//        ck::UUtils_Signal_OnFloatAttributeValueChanged::Bind<&ThisType::OnMeterAttribute_ValueChanged>(MeterCurrenValueFloatAttributeEntity, InBehavior, InPostFireBehavior);
//        ck::UUtils_Signal_NumericOnAttributeValueChanged::Bind(InAttributeOwnerEntity, InDelegate, InBehavior);
//
//        return;
//    }
//
//    CK_TRIGGER_ENSURE(TEXT("Failed to Bind to OnValueChanged. Entity [{}] does NOT have a Numeric Attribute [{}]"), InAttributeOwnerEntity, InAttributeName);
//}
//
//auto
//    UCk_Utils_NumericAttribute_UE::
//    UnbindFrom_OnValueChanged(
//        FCk_Handle InAttributeOwnerEntity,
//        FGameplayTag InAttributeName,
//        const FCk_Delegate_NumericAttribute_OnValueChanged& InDelegate)
//    -> void
//{
//    if (UCk_Utils_FloatAttribute_UE::Has_Attribute(InAttributeOwnerEntity, InAttributeName))
//    {
//        const auto& FloatAttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(InAttributeOwnerEntity, InAttributeName);
//
//        ck::UUtils_Signal_OnFloatAttributeValueChanged::Unbind<&ThisType::OnFloatAttribute_ValueChanged>(FloatAttributeEntity);
//        ck::UUtils_Signal_NumericOnAttributeValueChanged::Unbind(InAttributeOwnerEntity, InDelegate);
//
//        return;
//    }
//
//    if (UCk_Utils_MeterAttribute_UE::Has(InAttributeOwnerEntity, InAttributeName))
//    {
//        const auto& MeterAttributeEntity = RecordOfMeterAttributes_Utils::Get_ValidEntry_If(InAttributeOwnerEntity, ck::algo::MatchesGameplayLabelExact{InAttributeName});
//        const auto& MeterCurrenValueFloatAttributeEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<FloatAttribute_Utils, RecordOfFloatAttributes_Utils>(MeterAttributeEntity, ck::FMeterAttribute_Tags::Get_Current());
//
//        ck::UUtils_Signal_OnFloatAttributeValueChanged::Unbind<&ThisType::OnMeterAttribute_ValueChanged>(MeterCurrenValueFloatAttributeEntity);
//        ck::UUtils_Signal_NumericOnAttributeValueChanged::Unbind(InAttributeOwnerEntity, InDelegate);
//
//        return;
//    }
//
//    CK_TRIGGER_ENSURE(TEXT("Failed to Unbind from OnValueChanged. Entity [{}] does NOT have a Numeric Attribute [{}]"), InAttributeOwnerEntity, InAttributeName);
//}
//
//auto
//    UCk_Utils_NumericAttribute_UE::
//    OnFloatAttribute_ValueChanged(
//        FCk_Handle FloatAttributeOwnerEntity,
//        ck::TPayload_Attribute_OnValueChanged<ck::FFragment_FloatAttribute> InValueChanged)
//    -> void
//{
//    ck::UUtils_Signal_NumericOnAttributeValueChanged::Broadcast
//    (
//        FloatAttributeOwnerEntity,
//        ck::MakePayload
//        (
//            FloatAttributeOwnerEntity,
//            FCk_Payload_NumericAttribute_OnValueChanged
//            {
//                FloatAttributeOwnerEntity,
//                InValueChanged.Get_BaseValue(),
//                InValueChanged.Get_FinalValue()
//            }
//        )
//    );
//}
//
//auto
//    UCk_Utils_NumericAttribute_UE::
//    OnMeterAttribute_ValueChanged(
//        FCk_Handle InMeterAttributeEntity,
//        ck::TPayload_Attribute_OnValueChanged<ck::FFragment_FloatAttribute> InValueChanged)
//    -> void
//{
//    auto MeterAttributeLifetimeOwner = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(InMeterAttributeEntity);
//
//    ck::UUtils_Signal_NumericOnAttributeValueChanged::Broadcast
//    (
//        MeterAttributeLifetimeOwner,
//        ck::MakePayload
//        (
//            MeterAttributeLifetimeOwner,
//            FCk_Payload_NumericAttribute_OnValueChanged
//            {
//                MeterAttributeLifetimeOwner,
//                InValueChanged.Get_BaseValue(),
//                InValueChanged.Get_FinalValue()
//            }
//        )
//    );
//}
//
//auto
//    UCk_Utils_NumericAttributeModifier_UE::
//    Add(
//        FCk_Handle                                            InAttributeOwnerEntity,
//        FGameplayTag                                          InModifierName,
//        const FCk_Fragment_NumericAttributeModifier_ParamsData& InParams)
//    -> void
//{
//    if (NOT UCk_Utils_Net_UE::Get_IsEntityNetMode_Host(InAttributeOwnerEntity))
//    { return; }
//
//    if (UCk_Utils_FloatAttribute_UE::Has_Attribute(InAttributeOwnerEntity, InParams.Get_TargetAttributeName()))
//    {
//        UCk_Utils_FloatAttributeModifier_UE::Add(InAttributeOwnerEntity, InModifierName,
//            FCk_Fragment_FloatAttributeModifier_ParamsData
//            {
//                InParams.Get_ModifierDelta().Get_Value().Get_CurrentValue(),
//                InParams.Get_TargetAttributeName(),
//                InParams.Get_ModifierOperation(),
//                InParams.Get_ModifierOperation_RevocablePolicy()
//            });
//    }
//    else if (UCk_Utils_MeterAttribute_UE::Has(InAttributeOwnerEntity, InParams.Get_TargetAttributeName()))
//    {
//        UCk_Utils_MeterAttributeModifier_UE::Add(InAttributeOwnerEntity, InModifierName, InParams);
//    }
//    else
//    {
//        CK_TRIGGER_ENSURE(TEXT("Unable to add modifier. Entity [{}] does NOT have a Numeric Attribute (float/meter) [{}]"),
//            InAttributeOwnerEntity, InParams.Get_TargetAttributeName());
//    }
//}
//
//// --------------------------------------------------------------------------------------------------------------------
