#pragma once

#include "CkAttribute/MeterAttribute/CkMeterAttribute_Fragment_Data.h"

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkProvider/CkProvider_Data.h"

#include "CkSignal/CkSignal_Fragment.h"
#include "CkSignal/CkSignal_Utils.h"

#include "CkNumericAttribute_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ECk_NumericAttribute_ConstraintsPolicy : uint8
{
    HasMinimumValue = 1 << 0,
    HasMaximumValue = 1 << 1,

    None = 0 UMETA(Hidden),
};

ENUM_CLASS_FLAGS(ECk_NumericAttribute_ConstraintsPolicy)
ENABLE_ENUM_BITWISE_OPERATORS(ECk_NumericAttribute_ConstraintsPolicy);
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_NumericAttribute_ConstraintsPolicy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Fragment_NumericAttribute_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_NumericAttribute_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _AttributeName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _AttributeStartingValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Bitmask, BitmaskEnum = "ECk_NumericAttribute_ConstraintsPolicy"))
    int32 _ConstraintsPolicyFlags = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_AttachmentPolicyFlags == 1"))
    float _AttributeMinimumValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_AttachmentPolicyFlags == 2"))
    float _AttributeMaximumValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_MeterAttribute_ConstructionScript_PDA> _MeterAttributeConstructionScript;

public:
    auto Get_ConstraintsPolicy() const -> ECk_NumericAttribute_ConstraintsPolicy;

public:
    CK_PROPERTY_GET(_AttributeName)
    CK_PROPERTY_GET(_AttributeStartingValue)
    CK_PROPERTY_GET(_AttributeMaximumValue)
    CK_PROPERTY_GET(_AttributeMinimumValue)
    CK_PROPERTY_GET(_MeterAttributeConstructionScript)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_NumericAttribute_ParamsData, _AttributeName, _AttributeStartingValue, _ConstraintsPolicyFlags, _AttributeMinimumValue, _AttributeMaximumValue);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Fragment_MultipleNumericAttribute_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleNumericAttribute_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Fragment_NumericAttribute_ParamsData> _NumericAttributeParams;

public:
    CK_PROPERTY_GET(_NumericAttributeParams);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleNumericAttribute_ParamsData, _NumericAttributeParams);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class CKATTRIBUTE_API UCk_Provider_NumericAttribute_ParamsData_PDA : public UCk_Provider_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_NumericAttribute_ParamsData_PDA);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|Provider|NumericAttribute")
    FCk_Fragment_NumericAttribute_ParamsData
    Get_Value(
        FCk_Handle InHandle) const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Provider_NumericAttribute_ParamsData_Literal_PDA : public UCk_Provider_NumericAttribute_ParamsData_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_NumericAttribute_ParamsData_Literal_PDA);

private:
    auto Get_Value_Implementation(
        FCk_Handle InHandle) const -> FCk_Fragment_NumericAttribute_ParamsData override;

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    FCk_Fragment_NumericAttribute_ParamsData _Value;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Provider_NumericAttribute_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Provider_NumericAttribute_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_Provider_NumericAttribute_ParamsData_PDA> _Provider;

public:
    CK_PROPERTY_GET(_Provider);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class CKATTRIBUTE_API UCk_Provider_MultipleNumericAttribute_ParamsData_PDA : public UCk_Provider_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_MultipleNumericAttribute_ParamsData_PDA);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|Provider|NumericAttribute")
    FCk_Fragment_MultipleNumericAttribute_ParamsData
    Get_Value(
        FCk_Handle InHandle) const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Provider_MultipleNumericAttribute_ParamsData_Literal_PDA : public UCk_Provider_MultipleNumericAttribute_ParamsData_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_MultipleNumericAttribute_ParamsData_Literal_PDA);

private:
    auto Get_Value_Implementation(
        FCk_Handle InHandle) const -> FCk_Fragment_MultipleNumericAttribute_ParamsData override;

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    FCk_Fragment_MultipleNumericAttribute_ParamsData _Value;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Provider_MultipleNumericAttribute_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Provider_MultipleNumericAttribute_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_Provider_MultipleNumericAttribute_ParamsData_PDA> _Provider;

public:
    CK_PROPERTY_GET(_Provider);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Payload_NumericAttribute_OnValueChanged
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Payload_NumericAttribute_OnValueChanged);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Handle  _Handle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float  _PreviousValue;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float  _NewValue;

public:
    CK_PROPERTY_GET(_Handle);
    CK_PROPERTY_GET(_PreviousValue);
    CK_PROPERTY_GET(_NewValue);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Payload_NumericAttribute_OnValueChanged, _Handle, _PreviousValue, _NewValue);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_NumericAttribute_OnValueChanged,
    FCk_Handle, InHandle,
    FCk_Payload_NumericAttribute_OnValueChanged, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_NumericAttribute_OnValueChanged_MC,
    FCk_Handle, InHandle,
    FCk_Payload_NumericAttribute_OnValueChanged, InPayload);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct FFragment_Signal_OnNumericAttributeValueChanged : public TFragment_Signal<FCk_Handle, FCk_Payload_NumericAttribute_OnValueChanged> {};

    // --------------------------------------------------------------------------------------------------------------------

    struct FFragment_Signal_UnrealMulticast_OnNumericAttributeValueChanged : public TFragment_Signal_UnrealMulticast
    <
        FCk_Delegate_NumericAttribute_OnValueChanged_MC,
        FCk_Handle,
        FCk_Payload_NumericAttribute_OnValueChanged
    > {};

    // --------------------------------------------------------------------------------------------------------------------

    class UUtils_Signal_NumericOnAttributeValueChanged : public TUtils_Signal_UnrealMulticast
    <
        FFragment_Signal_OnNumericAttributeValueChanged,
        FFragment_Signal_UnrealMulticast_OnNumericAttributeValueChanged
    > {};
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Fragment_NumericAttributeModifier_ParamsData : public FCk_Fragment_MeterAttributeModifier_ParamsData
{
    GENERATED_BODY()
};

// --------------------------------------------------------------------------------------------------------------------
