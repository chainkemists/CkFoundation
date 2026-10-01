#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkPso_DrainTracker.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Pso_DrainState : uint8
{
    Idle,
    Draining,
    Settling,
    Complete,
    TimedOut
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Pso_DrainState);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Pso_DrainTimeoutReason : uint8
{
    None,
    MaxWait,
    Stalled
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Pso_DrainTimeoutReason);

// --------------------------------------------------------------------------------------------------------------------

/**
 * A zero _MaxWait or _StallTimeout disables that check. A zero _QuietPeriod completes on the first update that
 * observes no remaining work.
 */
struct CKPSO_API FCk_Pso_DrainTracker_Params
{
public:
    CK_GENERATED_BODY(FCk_Pso_DrainTracker_Params);

private:
    FCk_Time _QuietPeriod;
    FCk_Time _MaxWait;
    FCk_Time _StallTimeout;

public:
    CK_PROPERTY_GET(_QuietPeriod);
    CK_PROPERTY_GET(_MaxWait);
    CK_PROPERTY_GET(_StallTimeout);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Pso_DrainTracker_Params, _QuietPeriod, _MaxWait, _StallTimeout);
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Engine-free state machine over a pending-work count, fed (remaining, DeltaT) once per tick.
 *
 * The count must sit at zero for _QuietPeriod before the window completes, because requests keep trickling in as
 * components register. A DeltaT is charged to the state the tracker was in BEFORE the update, since the count only
 * reports where the interval ended: the zero-work path therefore costs one tick plus the quiet period.
 *
 * Work reappearing after Complete re-opens the SAME window (elapsed, MaxWait budget and peak carry over), so a
 * screen still up after completion keeps covering late requests without granting them a fresh budget.
 *
 * TimedOut is terminal until Request_Begin / Request_Reset: a fail-open window must never re-hold the screen.
 *
 * A stall is a count that has not CHANGED for _StallTimeout. A rising count is work still arriving - during a level
 * load requests routinely outpace compiles - and only _MaxWait bounds that.
 *
 * The progress ratio never decreases within a window - a raised peak or re-opened window holds it where it was.
 */
struct CKPSO_API FCk_Pso_DrainTracker
{
public:
    CK_GENERATED_BODY(FCk_Pso_DrainTracker);

public:
    auto
    Request_Begin(
        const FCk_Pso_DrainTracker_Params& InParams) -> void;

    auto
    Request_Reset() -> void;

    auto
    Update(
        uint32 InNumRemaining,
        FCk_Time InDeltaT) -> void;

    auto
    Get_IsHolding() const -> bool;

private:
    auto
    DoUpdate_ProgressRatio() -> void;

    auto
    DoTry_TimeOut() -> void;

    auto
    DoTimeOut(
        ECk_Pso_DrainTimeoutReason InReason) -> void;

private:
    FCk_Pso_DrainTracker_Params _Params;

    ECk_Pso_DrainState _State = ECk_Pso_DrainState::Idle;
    ECk_Pso_DrainTimeoutReason _TimeoutReason = ECk_Pso_DrainTimeoutReason::None;

    uint32 _NumRemaining = 0;
    uint32 _NumPeak = 0;
    float _ProgressRatio = 0.0f;

    FCk_Time _Elapsed;
    FCk_Time _QuietElapsed;
    FCk_Time _TimeSinceLastChange;

public:
    CK_PROPERTY_GET(_State);
    CK_PROPERTY_GET(_TimeoutReason);
    CK_PROPERTY_GET(_NumRemaining);
    CK_PROPERTY_GET(_NumPeak);
    CK_PROPERTY_GET(_ProgressRatio);
    CK_PROPERTY_GET(_Elapsed);
};

// --------------------------------------------------------------------------------------------------------------------
