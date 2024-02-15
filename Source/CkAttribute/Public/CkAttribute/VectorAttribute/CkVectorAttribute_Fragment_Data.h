#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkProvider/CkProvider_Data.h"

#include "CkVectorAttribute_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKATTRIBUTE_API FCk_Handle_VectorAttribute : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_VectorAttribute); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_VectorAttribute);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKATTRIBUTE_API FCk_Handle_VectorAttributeModifier : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_VectorAttributeModifier); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_VectorAttributeModifier);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Fragment_VectorAttribute_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_VectorAttribute_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _BaseValue = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    ECk_MinMax _MinMax = ECk_MinMax::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditConditionHides, EditCondition = "_MinMax == ECk_MinMax::Min || _MinMax == ECk_MinMax::MinMax"))
    FVector _MinValue = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditConditionHides, EditCondition = "_MinMax == ECk_MinMax::Max || _MinMax == ECk_MinMax::MinMax"))
    FVector _MaxValue = FVector::ZeroVector;

public:
    auto Get_MinValue() const -> FVector;
    auto Get_MaxValue() const -> FVector;

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_BaseValue);
    CK_PROPERTY_GET(_MinMax);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_VectorAttribute_ParamsData, _Name, _BaseValue);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Fragment_MultipleVectorAttribute_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleVectorAttribute_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, TitleProperty = "_Name"))
    TArray<FCk_Fragment_VectorAttribute_ParamsData> _VectorAttributeParams;

public:
    CK_PROPERTY_GET(_VectorAttributeParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleVectorAttribute_ParamsData, _VectorAttributeParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Fragment_VectorAttributeModifier_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_VectorAttributeModifier_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _ModifierDelta = FVector::ZeroVector;

    UPROPERTY()
    FGameplayTag _TargetAttributeName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ArithmeticOperations_Basic _ModifierOperation = ECk_ArithmeticOperations_Basic::Add;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ModifierOperation_RevocablePolicy _ModifierOperation_RevocablePolicy = ECk_ModifierOperation_RevocablePolicy::Revocable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_MinMaxCurrent _Component = ECk_MinMaxCurrent::Current;

public:
    CK_PROPERTY_GET(_ModifierDelta);
    CK_PROPERTY(_TargetAttributeName);
    CK_PROPERTY_GET(_ModifierOperation);
    CK_PROPERTY_GET(_ModifierOperation_RevocablePolicy);
    CK_PROPERTY_GET(_Component);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_VectorAttributeModifier_ParamsData, _ModifierDelta,
        _ModifierOperation, _ModifierOperation_RevocablePolicy, _Component);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class CKATTRIBUTE_API UCk_Provider_VectorAttribute_ParamsData_PDA : public UCk_Provider_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_VectorAttribute_ParamsData_PDA);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|Provider|VectorAttribute")
    FCk_Fragment_VectorAttribute_ParamsData
    Get_Value(
        FCk_Handle InHandle) const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Provider_VectorAttribute_ParamsData_Literal_PDA : public UCk_Provider_VectorAttribute_ParamsData_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_VectorAttribute_ParamsData_Literal_PDA);

private:
    auto Get_Value_Implementation(
        FCk_Handle InHandle) const -> FCk_Fragment_VectorAttribute_ParamsData override;

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    FCk_Fragment_VectorAttribute_ParamsData _Value;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Provider_VectorAttribute_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Provider_VectorAttribute_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_Provider_VectorAttribute_ParamsData_PDA> _Provider;

public:
    CK_PROPERTY_GET(_Provider);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class CKATTRIBUTE_API UCk_Provider_MultipleVectorAttribute_ParamsData_PDA : public UCk_Provider_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_MultipleVectorAttribute_ParamsData_PDA);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|Provider|VectorAttribute")
    FCk_Fragment_MultipleVectorAttribute_ParamsData
    Get_Value(
        FCk_Handle InHandle) const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Provider_MultipleVectorAttribute_ParamsData_Literal_PDA
    : public UCk_Provider_MultipleVectorAttribute_ParamsData_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_MultipleVectorAttribute_ParamsData_Literal_PDA);

private:
    auto Get_Value_Implementation(
        FCk_Handle InHandle) const -> FCk_Fragment_MultipleVectorAttribute_ParamsData override;

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    FCk_Fragment_MultipleVectorAttribute_ParamsData _Value;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Provider_MultipleVectorAttribute_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Provider_MultipleVectorAttribute_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_Provider_MultipleVectorAttribute_ParamsData_PDA> _Provider;

public:
    CK_PROPERTY_GET(_Provider);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Payload_VectorAttribute_OnValueChanged
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Payload_VectorAttribute_OnValueChanged);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Handle_VectorAttribute  _AttributeHandle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector  _BaseValue = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector  _FinalValue = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_AttributeHandle);
    CK_PROPERTY_GET(_BaseValue);
    CK_PROPERTY_GET(_FinalValue);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Payload_VectorAttribute_OnValueChanged, _AttributeHandle, _BaseValue, _FinalValue);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_VectorAttribute_OnValueChanged,
    FCk_Handle, InHandle,
    FCk_Payload_VectorAttribute_OnValueChanged, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_VectorAttribute_OnValueChanged_MC,
    FCk_Handle, InHandle,
    FCk_Payload_VectorAttribute_OnValueChanged, InPayload);

// --------------------------------------------------------------------------------------------------------------------
