#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkProvider/CkProvider_Data.h"

#include "CkByteAttribute_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Fragment_ByteAttribute_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_ByteAttribute_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ForceInlineRow))
    FGameplayTag _AttributeName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ForceInlineRow))
    uint8 _AttributeBaseValue = 0;

public:
    CK_PROPERTY_GET(_AttributeName);
    CK_PROPERTY_GET(_AttributeBaseValue);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_ByteAttribute_ParamsData, _AttributeName, _AttributeBaseValue);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Fragment_MultipleByteAttribute_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleByteAttribute_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Fragment_ByteAttribute_ParamsData> _ByteAttributeParams;

public:
    CK_PROPERTY_GET(_ByteAttributeParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleByteAttribute_ParamsData, _ByteAttributeParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Fragment_ByteAttributeModifier_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_ByteAttributeModifier_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    uint8 _ModifierDelta = 0;

    UPROPERTY(BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _TargetAttributeName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ModifierOperation _ModifierOperation = ECk_ModifierOperation::Additive;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ModifierOperation_RevocablePolicy _ModifierOperation_RevokablePolicy = ECk_ModifierOperation_RevocablePolicy::Revocable;

public:
    CK_PROPERTY_GET(_ModifierDelta);
    CK_PROPERTY_GET(_TargetAttributeName);
    CK_PROPERTY_GET(_ModifierOperation);
    CK_PROPERTY_GET(_ModifierOperation_RevokablePolicy);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_ByteAttributeModifier_ParamsData, _ModifierDelta, _TargetAttributeName,
        _ModifierOperation, _ModifierOperation_RevokablePolicy);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class CKATTRIBUTE_API UCk_Provider_ByteAttribute_ParamsData_PDA : public UCk_Provider_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_ByteAttribute_ParamsData_PDA);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|Provider|ByteAttribute")
    FCk_Fragment_ByteAttribute_ParamsData
    Get_Value(
        FCk_Handle InHandle) const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Provider_ByteAttribute_ParamsData_Literal_PDA : public UCk_Provider_ByteAttribute_ParamsData_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_ByteAttribute_ParamsData_Literal_PDA);

private:
    auto Get_Value_Implementation(
        FCk_Handle InHandle) const -> FCk_Fragment_ByteAttribute_ParamsData override;

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    FCk_Fragment_ByteAttribute_ParamsData _Value;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Provider_ByteAttribute_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Provider_ByteAttribute_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_Provider_ByteAttribute_ParamsData_PDA> _Provider;

public:
    CK_PROPERTY_GET(_Provider);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class CKATTRIBUTE_API UCk_Provider_MultipleByteAttribute_ParamsData_PDA : public UCk_Provider_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_MultipleByteAttribute_ParamsData_PDA);

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent,
              Category = "Ck|Provider|ByteAttribute")
    FCk_Fragment_MultipleByteAttribute_ParamsData
    Get_Value(
        FCk_Handle InHandle) const;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKATTRIBUTE_API UCk_Provider_MultipleByteAttribute_ParamsData_Literal_PDA
    : public UCk_Provider_MultipleByteAttribute_ParamsData_PDA
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Provider_MultipleByteAttribute_ParamsData_Literal_PDA);

private:
    auto Get_Value_Implementation(
        FCk_Handle InHandle) const -> FCk_Fragment_MultipleByteAttribute_ParamsData override;

private:
    UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = true))
    FCk_Fragment_MultipleByteAttribute_ParamsData _Value;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Provider_MultipleByteAttribute_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Provider_MultipleByteAttribute_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_Provider_MultipleByteAttribute_ParamsData_PDA> _Provider;

public:
    CK_PROPERTY_GET(_Provider);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Payload_ByteAttribute_OnValueChanged
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Payload_ByteAttribute_OnValueChanged);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Handle  _Handle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    uint8  _BaseValue = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    uint8  _FinalValue = 0;

public:
    CK_PROPERTY_GET(_Handle);
    CK_PROPERTY_GET(_BaseValue);
    CK_PROPERTY_GET(_FinalValue);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Payload_ByteAttribute_OnValueChanged, _Handle, _BaseValue, _FinalValue);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_ByteAttribute_OnValueChanged,
    FCk_Handle, InHandle,
    FCk_Payload_ByteAttribute_OnValueChanged, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_ByteAttribute_OnValueChanged_MC,
    FCk_Handle, InHandle,
    FCk_Payload_ByteAttribute_OnValueChanged, InPayload);

// --------------------------------------------------------------------------------------------------------------------
