#pragma once

#include "CkCore/Chrono/CkChrono.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Enums/CkEnums.h"          // ECk_RelativeAbsolute (Timer Jump mode)

#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTags.h>

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

UENUM(BlueprintType)
enum class ECk_Timer_JumpDirection : uint8
{
    Forwards,
    Backwards
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Timer_JumpDirection);

// --------------------------------------------------------------------------------------------------------------------

// Save-only persistence payload for a Timer child entity (spec Phase 3.2). The runtime position lives in the chrono's
// _CurrentValue, which is UPROPERTY(Transient) and therefore does NOT survive the v3 rebuild+hydrate load — the entity
// is re-Constructed from its spawn recipe (Params re-derive GoalValue/direction/behavior) but the chrono resets to its
// start (0 for CountUp, GoalValue for CountDown-after-Setup). This payload captures the three pieces the rebuild loses:
//   - _Elapsed        : the chrono position (Get_TimeElapsed(None) == _CurrentValue) at capture.
//   - _CountDirection : the RUNTIME direction (the FTag_Timer_Countdown tag), which Request_ChangeCountDirection /
//                       Request_ReverseDirection flip WITHOUT touching Params — so a runtime flip is lost on rebuild.
//   - _RunState       : running vs paused (the FTag_Timer_NeedsUpdate tag, mirrors Get_CurrentState). Done/terminal is
//                       NOT a distinct run-state here — it is encoded by _Elapsed reaching GoalValue.
// FCk_SaveData_ prefix (not FCk_RepData_): this type never rides the wire (timers are unreplicated) and stays off the
// RepData census. Handler registered in CkTimer_Fragment.cpp (Produce + HydrationApply, no net Apply).
USTRUCT()
struct CKTIMER_API FCk_SaveData_Timer
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SaveData_Timer);

private:
    UPROPERTY()
    FCk_Time _Elapsed;

    UPROPERTY()
    ECk_Timer_CountDirection _CountDirection = ECk_Timer_CountDirection::CountUp;

    UPROPERTY()
    ECk_Timer_State _RunState = ECk_Timer_State::Paused;

public:
    CK_PROPERTY(_Elapsed);
    CK_PROPERTY(_CountDirection);
    CK_PROPERTY(_RunState);
};

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
struct CKTIMER_API FCk_Request_Timer_Jump : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Timer_Jump);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Timer_Jump);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _JumpDuration;

    // Relative (default): _JumpDuration is a delta applied to the current elapsed. Absolute: _JumpDuration is the
    // TARGET elapsed and the handler moves the chrono by the direction-dependent gap. Rides the request struct
    // (addon-as-parameter) — no new UFUNCTION overload.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_RelativeAbsolute _JumpMode = ECk_RelativeAbsolute::Relative;

public:
    CK_PROPERTY_GET(_JumpDuration);
    CK_PROPERTY(_JumpMode);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Timer_Jump, _JumpDuration);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTIMER_API FCk_Request_Timer_Consume : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Timer_Consume);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Timer_Consume);

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
struct CKTIMER_API FCk_Request_Timer_Manipulate : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Timer_Manipulate);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Timer_Manipulate);

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
struct CKTIMER_API FCk_Request_Timer_ChangeDirection : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Timer_ChangeDirection);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Timer_ChangeDirection);

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

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_Timer,
    FCk_Handle_Timer, InHandle,
    FCk_Chrono, InChrono,
    FCk_Time, InDeltaT);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_FiveParams(
    FCk_Delegate_Timer_Jump,
    FCk_Handle_Timer, InHandle,
    FCk_Chrono, InChrono,
    FCk_Time, InDeltaT,
    ECk_Timer_JumpDirection, InJumpDirection,
    FCk_Time, InJumpAmount);

// --------------------------------------------------------------------------------------------------------------------