#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include <GameplayTagContainer.h>
#include <NativeGameplayTags.h>

#include "CkTween_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKTWEEN_API FCk_Handle_Tween : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Tween);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Tween);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTWEEN_API FCk_TweenTransformResult
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_TweenTransformResult);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Handle_Tween _LocationTween;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Handle_Tween _RotationTween;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_Handle_Tween _ScaleTween;

public:
    CK_PROPERTY_GET(_LocationTween);
    CK_PROPERTY_GET(_RotationTween);
    CK_PROPERTY_GET(_ScaleTween);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_TweenTransformResult, _LocationTween, _RotationTween, _ScaleTween);
};

// --------------------------------------------------------------------------------------------------------------------

CKTWEEN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Tween);
CKTWEEN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Tween_Location);
CKTWEEN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Tween_Rotation);
CKTWEEN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Tween_Scale);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_TweenEasing : uint8
{
    // ReSharper disable CppInconsistentNaming

    Linear,

    InSine,
    OutSine,
    InOutSine,

    InQuad,
    OutQuad,
    InOutQuad,

    InCubic,
    OutCubic,
    InOutCubic,

    InQuart,
    OutQuart,
    InOutQuart,

    InQuint,
    OutQuint,
    InOutQuint,

    InExpo,
    OutExpo,
    InOutExpo,

    InCirc,
    OutCirc,
    InOutCirc,

    InBack,
    OutBack,
    InOutBack,

    InElastic,
    OutElastic,
    InOutElastic,

    InBounce,
    OutBounce,
    InOutBounce

    // ReSharper restore CppInconsistentNaming
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_TweenEasing);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_TweenState : uint8
{
    Playing,
    Paused,
    Completed,
    Cancelled
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_TweenState);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_TweenLoopType : uint8
{
    None,
    Restart,
    Yoyo
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_TweenLoopType);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_TweenTarget : uint8
{
    Custom,
    Transform_Location,
    Transform_Rotation,
    Transform_Scale
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_TweenTarget);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTWEEN_API FCk_TweenValue
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_TweenValue);

public:
    using VariantType = std::variant<float, FVector, FRotator, FLinearColor, FCk_Handle_Transform>;

private:
    VariantType _Value;

public:
    template<typename T>
    FCk_TweenValue(const T& InValue) : _Value(InValue) {}

    FCk_TweenValue() : _Value(0.0f) {}

    template<typename T>
    auto Get() const -> const T&
    {
        return std::get<T>(_Value);
    }

    template<typename T>
    auto Set(const T& InValue) -> void
    {
        _Value = InValue;
    }

    template<typename T>
    auto Is() const -> bool
    {
        return std::holds_alternative<T>(_Value);
    }

    auto GetVariant() const -> const VariantType& { return _Value; }
    auto GetVariant() -> VariantType& { return _Value; }

public:
    auto IsFloat() const -> bool;
    auto IsVector() const -> bool;
    auto IsRotator() const -> bool;
    auto IsLinearColor() const -> bool;
    auto IsTransformHandle() const -> bool;

    auto GetAsFloat() const -> float;
    auto GetAsVector() const -> FVector;
    auto GetAsRotator() const -> FRotator;
    auto GetAsLinearColor() const -> FLinearColor;
    auto GetAsTransformHandle() const -> FCk_Handle_Transform;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTWEEN_API FCk_Fragment_Tween_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Tween_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_TweenValue _StartValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_TweenValue _EndValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, ClampMin = 0.0f))
    float _Duration = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_TweenEasing _Easing = ECk_TweenEasing::OutCubic;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_TweenLoopType _LoopType = ECk_TweenLoopType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, ClampMin = -1, UIMin = -1,
            ToolTip = "Number of loops. -1 for infinite, 0 for no loops, >0 for specific count"))
    int32 _LoopCount = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true, ClampMin = 0.0f))
    float _YoyoDelay = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    ECk_TweenTarget _Target = ECk_TweenTarget::Custom;

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Tween_ParamsData, _StartValue, _EndValue, _Duration, _Easing);

    CK_PROPERTY_GET(_StartValue);
    CK_PROPERTY_GET(_EndValue);
    CK_PROPERTY_GET(_Duration);
    CK_PROPERTY_GET(_Easing);
    CK_PROPERTY(_LoopType);
    CK_PROPERTY(_LoopCount);
    CK_PROPERTY(_YoyoDelay);
    CK_PROPERTY(_Target);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTWEEN_API FCk_Request_Tween_Pause : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Tween_Pause);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Tween_Pause);
};

USTRUCT(BlueprintType)
struct CKTWEEN_API FCk_Request_Tween_Resume : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Tween_Resume);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Tween_Resume);
};

USTRUCT(BlueprintType)
struct CKTWEEN_API FCk_Request_Tween_Stop : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Tween_Stop);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Tween_Stop);
};

USTRUCT(BlueprintType)
struct CKTWEEN_API FCk_Request_Tween_Restart : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Tween_Restart);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Tween_Restart);
};

USTRUCT(BlueprintType)
struct CKTWEEN_API FCk_Request_Tween_SetTimeMultiplier : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Tween_SetTimeMultiplier);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Tween_SetTimeMultiplier);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    float _Multiplier = 1.0f;

public:
    CK_PROPERTY_GET(_Multiplier);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Tween_SetTimeMultiplier, _Multiplier);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTWEEN_API FCk_Fragment_Tween_Requests
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Tween_Requests);

    using RequestType = std::variant<
        FCk_Request_Tween_Pause,
        FCk_Request_Tween_Resume,
        FCk_Request_Tween_Stop,
        FCk_Request_Tween_Restart,
        FCk_Request_Tween_SetTimeMultiplier
    >;

private:
    TArray<RequestType> _Requests;

public:
    CK_PROPERTY(_Requests);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTWEEN_API FCk_Tween_Payload_OnUpdate
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Tween_Payload_OnUpdate);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_TweenValue _CurrentValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_FloatRange_0to1 _Progress;

public:
    CK_PROPERTY_GET(_CurrentValue);
    CK_PROPERTY_GET(_Progress);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Tween_Payload_OnUpdate, _CurrentValue, _Progress);
};

USTRUCT(BlueprintType)
struct CKTWEEN_API FCk_Tween_Payload_OnComplete
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Tween_Payload_OnComplete);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FCk_TweenValue _FinalValue;

public:
    CK_PROPERTY_GET(_FinalValue);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Tween_Payload_OnComplete, _FinalValue);
};

USTRUCT(BlueprintType)
struct CKTWEEN_API FCk_Tween_Payload_OnLoop
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Tween_Payload_OnLoop);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    int32 _LoopCount = 0;

public:
    CK_PROPERTY_GET(_LoopCount);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Tween_Payload_OnLoop, _LoopCount);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Tween_OnUpdate,
    FCk_Handle_Tween, InHandle,
    FCk_Tween_Payload_OnUpdate, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Tween_OnUpdate_MC,
    FCk_Handle_Tween, InHandle,
    FCk_Tween_Payload_OnUpdate, InPayload);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Tween_OnComplete,
    FCk_Handle_Tween, InHandle,
    FCk_Tween_Payload_OnComplete, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Tween_OnComplete_MC,
    FCk_Handle_Tween, InHandle,
    FCk_Tween_Payload_OnComplete, InPayload);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Tween_OnLoop,
    FCk_Handle_Tween, InHandle,
    FCk_Tween_Payload_OnLoop, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Tween_OnLoop_MC,
    FCk_Handle_Tween, InHandle,
    FCk_Tween_Payload_OnLoop, InPayload);

// --------------------------------------------------------------------------------------------------------------------