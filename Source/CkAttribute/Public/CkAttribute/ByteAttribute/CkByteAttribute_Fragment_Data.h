#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include "CkProvider/CkProvider_Data.h"

#include "CkByteAttribute_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

CKATTRIBUTE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Label_ByteAttribute);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKATTRIBUTE_API FCk_Handle_ByteAttribute : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_ByteAttribute); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_ByteAttribute);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKATTRIBUTE_API FCk_Handle_ByteAttributeModifier : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_ByteAttributeModifier); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_ByteAttributeModifier);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKATTRIBUTE_API FCk_Fragment_ByteAttribute_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_ByteAttribute_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "ByteAttribute"))
    FGameplayTag _Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    uint8 _BaseValue = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta=(AllowPrivateAccess))
    ECk_MinMax _MinMax = ECk_MinMax::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditConditionHides, EditCondition = "_MinMax == ECk_MinMax::Min || _MinMax == ECk_MinMax::MinMax"))
    uint8 _MinValue = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditConditionHides, EditCondition = "_MinMax == ECk_MinMax::Max || _MinMax == ECk_MinMax::MinMax"))
    uint8 _MaxValue = 0;

public:
    auto Get_MinValue() const -> uint8;
    auto Get_MaxValue() const -> uint8;

public:
    CK_PROPERTY_GET(_Name);
    CK_PROPERTY_GET(_BaseValue);

    CK_PROPERTY(_MinMax);
    CK_PROPERTY_SET(_MinValue);
    CK_PROPERTY_SET(_MaxValue);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_ByteAttribute_ParamsData, _Name, _BaseValue);
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
              meta = (AllowPrivateAccess = true, TitleProperty = "_Name"))
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
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_ByteAttributeModifier_ParamsData, _ModifierDelta, _Component);
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
    FCk_Handle_ByteAttribute  _AttributeEntity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    uint8  _BaseValue = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    uint8  _FinalValue = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    uint8  _BaseValue_Previous = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    uint8  _FinalValue_Previous = 0;

public:
    CK_PROPERTY_GET(_AttributeEntity);
    CK_PROPERTY_GET(_BaseValue);
    CK_PROPERTY_GET(_FinalValue);
    CK_PROPERTY_GET(_BaseValue_Previous);
    CK_PROPERTY_GET(_FinalValue_Previous);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Payload_ByteAttribute_OnValueChanged,
        _AttributeEntity, _BaseValue, _FinalValue, _BaseValue_Previous, _FinalValue_Previous);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_ByteAttribute_OnValueChanged,
    FCk_Handle, InAttributeOwnerEntity,
    FCk_Payload_ByteAttribute_OnValueChanged, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_ByteAttribute_OnValueChanged_MC,
    FCk_Handle, InAttributeOwnerEntity,
    FCk_Payload_ByteAttribute_OnValueChanged, InPayload);

// --------------------------------------------------------------------------------------------------------------------
