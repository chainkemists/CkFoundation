#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Meter/CkMeter.h"

#include "CkProvider/CkProvider_Data.h"

#include "CkMeterAttribute_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ECk_MeterAttributeModifier_Policy : uint8
{
    None = 0 UMETA(Hidden),
    MinCapacity = 1 << 0,
    CurrentValue = 1 << 1,
    MaxCapacity = 1 << 2,
};

ENUM_CLASS_FLAGS(ECk_MeterAttributeModifier_Policy)
ENABLE_ENUM_BITWISE_OPERATORS(ECk_MeterAttributeModifier_Policy);
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_MeterAttributeModifier_Policy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Fragment_MeterAttribute_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MeterAttribute_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ForceInlineRow))
    FGameplayTag _AttributeName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ForceInlineRow))
    FCk_Meter _AttributeBaseValue;

public:
    CK_PROPERTY_GET(_AttributeName);
    CK_PROPERTY_GET(_AttributeBaseValue);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MeterAttribute_ParamsData, _AttributeName, _AttributeBaseValue);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Fragment_MultipleMeterAttribute_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleMeterAttribute_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<TObjectPtr<class UCk_MeterAttribute_ConstructionScript_PDA>> _MeterAttributeParams;

public:
    CK_PROPERTY_GET(_MeterAttributeParams);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleMeterAttribute_ParamsData, _MeterAttributeParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Fragment_MeterAttributeModifier_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MeterAttributeModifier_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Meter _ModifierDelta;

    UPROPERTY(BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _TargetAttributeName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ModifierOperation _ModifierOperation = ECk_ModifierOperation::Additive;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ModifierOperation_RevokablePolicy _ModifierOperation_RevokablePolicy = ECk_ModifierOperation_RevokablePolicy::Revokable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Bitmask, BitmaskEnum = "ECk_MeterAttributeModifier_Policy"))
    int32 _ModifierPolicyFlags = 0;

public:
    auto Get_ModifierPolicy() const -> ECk_MeterAttributeModifier_Policy;

public:
    CK_PROPERTY_GET(_ModifierDelta)
    CK_PROPERTY_GET(_TargetAttributeName)
    CK_PROPERTY_GET(_ModifierOperation)
    CK_PROPERTY_GET(_ModifierOperation_RevokablePolicy)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MeterAttributeModifier_ParamsData, _ModifierDelta, _TargetAttributeName, _ModifierOperation, _ModifierOperation_RevokablePolicy, _ModifierPolicyFlags);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class CKATTRIBUTE_API UCk_Provider_MeterAttribute_ParamsData_PDA : public UCk_Provider_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_MeterAttribute_ParamsData_PDA);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|Provider|MeterAttribute")
    FCk_Fragment_MeterAttribute_ParamsData
    Get_Value(
        FCk_Handle InHandle) const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Provider_MeterAttribute_ParamsData_Literal_PDA : public UCk_Provider_MeterAttribute_ParamsData_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_MeterAttribute_ParamsData_Literal_PDA);

private:
    auto Get_Value_Implementation(
        FCk_Handle InHandle) const -> FCk_Fragment_MeterAttribute_ParamsData override;

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    FCk_Fragment_MeterAttribute_ParamsData _Value;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Provider_MeterAttribute_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Provider_MeterAttribute_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_Provider_MeterAttribute_ParamsData_PDA> _Provider;

public:
    CK_PROPERTY_GET(_Provider);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class CKATTRIBUTE_API UCk_Provider_MultipleMeterAttribute_ParamsData_PDA : public UCk_Provider_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_MultipleMeterAttribute_ParamsData_PDA);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|Provider|MeterAttribute")
    FCk_Fragment_MultipleMeterAttribute_ParamsData
    Get_Value(
        FCk_Handle InHandle) const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Provider_MultipleMeterAttribute_ParamsData_Literal_PDA : public UCk_Provider_MultipleMeterAttribute_ParamsData_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_MultipleMeterAttribute_ParamsData_Literal_PDA);

private:
    auto Get_Value_Implementation(
        FCk_Handle InHandle) const -> FCk_Fragment_MultipleMeterAttribute_ParamsData override;

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    FCk_Fragment_MultipleMeterAttribute_ParamsData _Value;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Provider_MultipleMeterAttribute_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Provider_MultipleMeterAttribute_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_Provider_MultipleMeterAttribute_ParamsData_PDA> _Provider;

public:
    CK_PROPERTY_GET(_Provider);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Payload_MeterAttribute_OnValueChanged
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Payload_MeterAttribute_OnValueChanged);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Meter  _BaseValue;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Meter  _FinalValue;

public:
    CK_PROPERTY_GET(_BaseValue);
    CK_PROPERTY_GET(_FinalValue);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Payload_MeterAttribute_OnValueChanged, _BaseValue, _FinalValue);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_MeterAttribute_OnValueChanged,
    FCk_Handle, InHandle,
    FCk_Payload_MeterAttribute_OnValueChanged, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_MeterAttribute_OnValueChanged_MC,
    FCk_Handle, InHandle,
    FCk_Payload_MeterAttribute_OnValueChanged, InPayload);

// --------------------------------------------------------------------------------------------------------------------
