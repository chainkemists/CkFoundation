#pragma once

#include "CkAttribute/CkAttribute_Fragment_Data.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkProvider/CkProvider_Data.h"

#include "CkFloatAttribute_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

CKATTRIBUTE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Label_FloatAttribute);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKATTRIBUTE_API FCk_Handle_FloatAttribute : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_FloatAttribute); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_FloatAttribute);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKATTRIBUTE_API FCk_Handle_FloatAttributeModifier : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_FloatAttributeModifier); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_FloatAttributeModifier);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKATTRIBUTE_API FCk_Handle_FloatAttributeRefill : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_FloatAttributeRefill); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_FloatAttributeRefill);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_FloatAttributeMagnitude_CalculationPolicy : uint8
{
    Linear,
    Curve,
    Breakpoints,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_FloatAttributeMagnitude_CalculationPolicy);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Struct representing a float whose magnitude is dictated by a backing attribute and a calculation policy, follows one of the following forms:
 * Linear:         (Coefficient * (PreMultiplyAdditiveValue + [Eval'd Attribute Value According to Policy])) + PostMultiplyAdditiveValue
 * Curve:          Coefficient * Curve([Eval'd Attribute Value According to Policy])
 * Breakpoints:    Breakpoints[floor([Eval'd Attribute Value According to Policy])],   0 if indexing below 0 and the last breakpoint if indexing above it
 */
USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_FloatAttribute_Magnitude
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_FloatAttribute_Magnitude);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "FloatAttribute"))
    FGameplayTag _AttributeName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_MinMaxCurrent _AttributeComponent = ECk_MinMaxCurrent::Current;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Attribute_BaseBonusFinal _MagnitudeComponent = ECk_Attribute_BaseBonusFinal::Final;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_FloatAttributeMagnitude_CalculationPolicy _CalculationMode = ECk_FloatAttributeMagnitude_CalculationPolicy::Linear;

    // Coefficient to the attribute calculation
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, EditConditionHides,
            EditCondition = "_CalculationMode == ECk_FloatAttributeMagnitude_CalculationPolicy::Linear || _CalculationMode == ECk_FloatAttributeMagnitude_CalculationPolicy::Curve"))
    float _Coefficient = 1.0f;

    // Additive value to the attribute calculation, added in BEFORE the coefficient applies
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, EditConditionHides,
            EditCondition = "_CalculationMode == ECk_FloatAttributeMagnitude_CalculationPolicy::Linear"))
    float _PreMultiplyAdditiveValue = 0.0f;

    // Additive value to the attribute calculation, added in AFTER the coefficient applies
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, EditConditionHides,
            EditCondition = "_CalculationMode == ECk_FloatAttributeMagnitude_CalculationPolicy::Linear"))
    float _PostMultiplyAdditiveValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
         meta = (AllowPrivateAccess = true, EditConditionHides,
            EditCondition = "_CalculationMode == ECk_FloatAttributeMagnitude_CalculationPolicy::Curve"))
    FRuntimeFloatCurve _Curve;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, EditConditionHides,
            EditCondition = "_CalculationMode == ECk_FloatAttributeMagnitude_CalculationPolicy::Breakpoints"))
    TArray<float> _Breakpoints;

public:
    CK_PROPERTY_GET(_AttributeName);
    CK_PROPERTY(_AttributeComponent);
    CK_PROPERTY(_MagnitudeComponent);
    CK_PROPERTY(_CalculationMode);
    CK_PROPERTY(_Coefficient);
    CK_PROPERTY(_PreMultiplyAdditiveValue);
    CK_PROPERTY(_PostMultiplyAdditiveValue);
    CK_PROPERTY(_Curve);
    CK_PROPERTY(_Breakpoints);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_FloatAttribute_Magnitude, _AttributeName);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Fragment_FloatAttributeRefill_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_FloatAttributeRefill_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "FloatAttribute"))
    FGameplayTag _RefillAttributeName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    ECk_Attribute_Refill_Policy _RefillBehavior = ECk_Attribute_Refill_Policy::Variable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    float _FillRate = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    ECk_Attribute_RefillState _StartingState = ECk_Attribute_RefillState::Paused;

public:
    CK_PROPERTY_GET(_RefillAttributeName);
    CK_PROPERTY_GET(_RefillBehavior);
    CK_PROPERTY_GET(_FillRate);
    CK_PROPERTY_GET(_StartingState);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Fragment_FloatAttribute_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_FloatAttribute_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "FloatAttribute"))
    FGameplayTag _Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _BaseValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    ECk_MinMax _MinMax = ECk_MinMax::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditConditionHides, EditCondition = "_MinMax == ECk_MinMax::Min || _MinMax == ECk_MinMax::MinMax"))
    float _MinValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditConditionHides, EditCondition = "_MinMax == ECk_MinMax::Max || _MinMax == ECk_MinMax::MinMax"))
    float _MaxValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, InlineEditConditionToggle))
    bool _EnableRefill = false;

    // Non-Replicated fill rate
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess, EditCondition = "_EnableRefill"))
    FCk_Fragment_FloatAttributeRefill_ParamsData _RefillParams;

public:
    auto Get_MinValue() const -> float;
    auto Get_MaxValue() const -> float;

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_BaseValue);
    CK_PROPERTY_GET(_EnableRefill);
    CK_PROPERTY_GET(_RefillParams);

    CK_PROPERTY(_MinMax);
    CK_PROPERTY_SET(_MinValue);
    CK_PROPERTY_SET(_MaxValue);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_FloatAttribute_ParamsData, _Name, _BaseValue);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Fragment_MultipleFloatAttribute_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleFloatAttribute_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, TitleProperty = "_Name"))
    TArray<FCk_Fragment_FloatAttribute_ParamsData> _FloatAttributeParams;

public:
    CK_PROPERTY_GET(_FloatAttributeParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleFloatAttribute_ParamsData, _FloatAttributeParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Fragment_FloatAttributeModifier_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_FloatAttributeModifier_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _ModifierDelta = 0.0f;

    UPROPERTY()
    FGameplayTag _TargetAttributeName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_MinMaxCurrent _Component = ECk_MinMaxCurrent::Current;

public:
    CK_PROPERTY(_ModifierDelta);
    CK_PROPERTY(_TargetAttributeName);
    CK_PROPERTY_GET(_Component);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_FloatAttributeModifier_ParamsData, _ModifierDelta, _Component);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Payload_FloatAttribute_OnValueChanged
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Payload_FloatAttribute_OnValueChanged);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Handle_FloatAttribute  _AttributeEntity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float  _BaseValue = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float  _FinalValue = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float  _BaseValue_Previous = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float  _FinalValue_Previous = 0.0f;

public:
    CK_PROPERTY_GET(_AttributeEntity);
    CK_PROPERTY_GET(_BaseValue);
    CK_PROPERTY_GET(_FinalValue);

    CK_PROPERTY_GET(_BaseValue_Previous);
    CK_PROPERTY_GET(_FinalValue_Previous);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Payload_FloatAttribute_OnValueChanged,
        _AttributeEntity, _BaseValue, _FinalValue, _BaseValue_Previous, _FinalValue_Previous);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_FloatAttribute_OnValueChanged,
    FCk_Handle, InAttributeOwnerEntity,
    FCk_Payload_FloatAttribute_OnValueChanged, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_FloatAttribute_OnValueChanged_MC,
    FCk_Handle, InAttributeOwnerEntity,
    FCk_Payload_FloatAttribute_OnValueChanged, InPayload);

// --------------------------------------------------------------------------------------------------------------------
