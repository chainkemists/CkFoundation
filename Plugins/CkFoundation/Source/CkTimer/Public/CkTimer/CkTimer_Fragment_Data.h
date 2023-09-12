#pragma once

#include "CkCore/Chrono/CkChrono.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkFormat/CkFormat.h"

#include "CkTimer_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Timer_Manipulate : uint8
{
    Reset,
    Stop,
    Pause,
    Resume
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Timer_Manipulate);

UENUM(BlueprintType)
enum class ECk_Timer_Behavior : uint8
{
    StopOnDone,
    ResetOnDone,
    PauseOnDone
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Timer_Behavior);

UENUM(BlueprintType)
enum class ECk_Timer_State : uint8
{
    Paused,
    Running
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTIMER_API FCk_Fragment_Timer_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Timer_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Time   _Duration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Timer_Behavior _Behavior = ECk_Timer_Behavior::PauseOnDone;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Timer_State _StartingState = ECk_Timer_State::Paused;

public:
    CK_PROPERTY_GET(_Duration);
    CK_PROPERTY(_Behavior);
    CK_PROPERTY(_StartingState);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Timer_ParamsData, _Duration);
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
    ECk_Timer_Manipulate _Manipulate;

public:
    CK_PROPERTY_GET(_Manipulate);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Timer_Manipulate, _Manipulate);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Timer,
    FCk_Handle, InHandle,
    FCk_Chrono, InChrono);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_Timer_MC,
    FCk_Handle, InHandle,
    FCk_Chrono, InChrono);

// --------------------------------------------------------------------------------------------------------------------

