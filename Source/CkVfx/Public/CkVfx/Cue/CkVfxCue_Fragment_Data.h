#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTagContainer.h>

#include "CkVfxCue_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKVFX_API FCk_Handle_VfxCue : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_VfxCue); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_VfxCue);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_VfxCue_PlaybackBehavior : uint8
{
    AutoPlay,
    Manual,
    DelayedPlay
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_VfxCue_PlaybackBehavior);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_VfxCue_DurationMode : uint8
{
    UseSystemDuration,
    Override,
    Infinite
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_VfxCue_DurationMode);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_VfxCue_PoolingBehavior : uint8
{
    AutoDestroy,
    AutoPool,
    Manual
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_VfxCue_PoolingBehavior);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_VfxCue_ParameterType : uint8
{
    Float,
    Vector,
    Color,
    Bool,
    Int
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_VfxCue_ParameterType);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVFX_API FCk_VfxCue_UserParameter
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_VfxCue_UserParameter);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FName _ParameterName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_VfxCue_ParameterType _ParameterType = ECk_VfxCue_ParameterType::Float;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_ParameterType == ECk_VfxCue_ParameterType::Float", EditConditionHides))
    float _FloatValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_ParameterType == ECk_VfxCue_ParameterType::Vector", EditConditionHides))
    FVector _VectorValue = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_ParameterType == ECk_VfxCue_ParameterType::Color", EditConditionHides))
    FLinearColor _ColorValue = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_ParameterType == ECk_VfxCue_ParameterType::Bool", EditConditionHides))
    bool _BoolValue = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_ParameterType == ECk_VfxCue_ParameterType::Int", EditConditionHides))
    int32 _IntValue = 0;

public:
    CK_PROPERTY(_ParameterName);
    CK_PROPERTY(_ParameterType);
    CK_PROPERTY(_FloatValue);
    CK_PROPERTY(_VectorValue);
    CK_PROPERTY(_ColorValue);
    CK_PROPERTY(_BoolValue);
    CK_PROPERTY(_IntValue);

public:
    FCk_VfxCue_UserParameter() = default;
    FCk_VfxCue_UserParameter(FName InParameterName, float InValue);
    FCk_VfxCue_UserParameter(FName InParameterName, const FVector& InValue);
    FCk_VfxCue_UserParameter(FName InParameterName, FLinearColor InValue);
    FCk_VfxCue_UserParameter(FName InParameterName, bool InValue);
    FCk_VfxCue_UserParameter(FName InParameterName, int32 InValue);

    CK_ANGELSCRIPT_CTOR_REGISTRATION(FCk_VfxCue_UserParameter, _ParameterName, _FloatValue)
    CK_ANGELSCRIPT_CTOR_REGISTRATION(FCk_VfxCue_UserParameter, _ParameterName, _VectorValue)
    CK_ANGELSCRIPT_CTOR_REGISTRATION(FCk_VfxCue_UserParameter, _ParameterName, _ColorValue)
    CK_ANGELSCRIPT_CTOR_REGISTRATION(FCk_VfxCue_UserParameter, _ParameterName, _BoolValue)
    CK_ANGELSCRIPT_CTOR_REGISTRATION(FCk_VfxCue_UserParameter, _ParameterName, _IntValue)
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVFX_API FCk_Request_VfxCue_Play : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VfxCue_Play);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VfxCue_Play);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVFX_API FCk_Request_VfxCue_Stop : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VfxCue_Stop);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VfxCue_Stop);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_VfxCue_OnStarted,
    FCk_Handle_VfxCue, InVfxCue);

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_VfxCue_OnFinished,
    FCk_Handle_VfxCue, InVfxCue);

// --------------------------------------------------------------------------------------------------------------------
