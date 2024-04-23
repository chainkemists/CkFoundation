#pragma once

#include "CkCore/Chrono/CkChrono.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Format/CkFormat.h"

#include "CkEcs/Handle/CkHandle_Typesafe.h"

#include "CkTimer_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Timer_Manipulate : uint8
{
    Reset,
    Complete,
    Stop,
    Pause,
    Resume
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Timer_Manipulate);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Timer_Behavior : uint8
{
    StopOnDone UMETA(DisplayName = "Reset And Pause On Done"),
    ResetOnDone UMETA(DisplayName = "Reset And Resume On Done"),
    PauseOnDone
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Timer_Behavior);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Timer_State : uint8
{
    Paused,
    Running
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Timer_State);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Timer_CountDirection : uint8
{
    CountUp,
    CountDown
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Timer_CountDirection);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKTIMER_API FCk_Handle_Timer : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Timer); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Timer);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTIMER_API FCk_Fragment_Timer_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Timer_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Timer"))
    FGameplayTag _TimerName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time   _Duration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Timer_CountDirection _CountDirection = ECk_Timer_CountDirection::CountUp;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Timer_Behavior _Behavior = ECk_Timer_Behavior::PauseOnDone;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Timer_State _StartingState = ECk_Timer_State::Paused;

public:
    CK_PROPERTY(_TimerName);
    CK_PROPERTY_GET(_Duration);
    CK_PROPERTY(_CountDirection);
    CK_PROPERTY(_Behavior);
    CK_PROPERTY(_StartingState);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Timer_ParamsData, _Duration);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTIMER_API FCk_Fragment_MultipleTimer_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleTimer_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, TitleProperty = "_TimerName"))
    TArray<FCk_Fragment_Timer_ParamsData> _TimerParams;

public:
    CK_PROPERTY_GET(_TimerParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleTimer_ParamsData, _TimerParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTIMER_API FCk_Request_Timer_Jump
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Timer_Jump);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _JumpDuration;

public:
    CK_PROPERTY_GET(_JumpDuration);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Timer_Jump, _JumpDuration);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTIMER_API FCk_Request_Timer_Consume
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Timer_Consume);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _ConsumeDuration;

public:
    CK_PROPERTY_GET(_ConsumeDuration);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Timer_Consume, _ConsumeDuration);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTIMER_API FCk_Request_Timer_Manipulate
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Timer_Manipulate);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Timer_Manipulate _Manipulate = ECk_Timer_Manipulate::Reset;

public:
    CK_PROPERTY_GET(_Manipulate);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Timer_Manipulate, _Manipulate);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTIMER_API FCk_Request_Timer_ChangeDirection
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Timer_ChangeDirection);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Timer_CountDirection _CountDirection = ECk_Timer_CountDirection::CountUp;

public:
    CK_PROPERTY_GET(_CountDirection);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Timer_ChangeDirection, _CountDirection);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Timer,
    FCk_Handle_Timer, InHandle,
    FCk_Chrono, InChrono);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Timer_MC,
    FCk_Handle_Timer, InHandle,
    FCk_Chrono, InChrono);

// --------------------------------------------------------------------------------------------------------------------

