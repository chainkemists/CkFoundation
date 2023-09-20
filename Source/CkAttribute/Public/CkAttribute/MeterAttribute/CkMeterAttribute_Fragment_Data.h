#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Meter/CkMeter.h"

#include "CkProvider/CkProvider_Data.h"

#include "CkMeterAttribute_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Fragment_MeterAttributes_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MeterAttributes_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ForceInlineRow))
    TMap<FGameplayTag, FCk_Meter> _AttributeBaseValues;

public:
    CK_PROPERTY_GET(_AttributeBaseValues);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MeterAttributes_ParamsData, _AttributeBaseValues);
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

public:
    CK_PROPERTY_GET(_ModifierDelta)
    CK_PROPERTY_GET(_TargetAttributeName)
    CK_PROPERTY_GET(_ModifierOperation)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MeterAttributeModifier_ParamsData, _ModifierDelta, _TargetAttributeName, _ModifierOperation);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class CKATTRIBUTE_API UCk_Provider_MeterAttributes_ParamsData_PDA : public UCk_Provider_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_MeterAttributes_ParamsData_PDA);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|Provider|MeterAttributes")
    FCk_Fragment_MeterAttributes_ParamsData Get_Value() const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Provider_MeterAttributes_ParamsData_Literal_PDA : public UCk_Provider_MeterAttributes_ParamsData_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_MeterAttributes_ParamsData_Literal_PDA);

private:
    auto Get_Value_Implementation() const -> FCk_Fragment_MeterAttributes_ParamsData override;

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    FCk_Fragment_MeterAttributes_ParamsData _Value;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Provider_MeterAttributes_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Provider_MeterAttributes_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_Provider_MeterAttributes_ParamsData_PDA> _Provider;

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
    FCk_Handle  _Handle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Meter  _BaseValue;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Meter  _FinalValue;

public:
    CK_PROPERTY_GET(_Handle);
    CK_PROPERTY_GET(_BaseValue);
    CK_PROPERTY_GET(_FinalValue);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Payload_MeterAttribute_OnValueChanged, _Handle, _BaseValue, _FinalValue);
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
