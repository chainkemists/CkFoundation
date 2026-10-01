#include "CkPso_DrainTracker.h"

#include <Math/UnrealMathUtility.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Pso_DrainTracker::
    Request_Begin(
        const FCk_Pso_DrainTracker_Params& InParams)
    -> void
{
    *this = FCk_Pso_DrainTracker{};

    _Params = InParams;
    _State = ECk_Pso_DrainState::Draining;
}

auto
    FCk_Pso_DrainTracker::
    Request_Reset()
    -> void
{
    *this = FCk_Pso_DrainTracker{};
}

auto
    FCk_Pso_DrainTracker::
    Update(
        uint32 InNumRemaining,
        FCk_Time InDeltaT)
    -> void
{
    if (_State == ECk_Pso_DrainState::Idle || _State == ECk_Pso_DrainState::TimedOut)
    { return; }

    const auto PreviousState = _State;

    if (PreviousState == ECk_Pso_DrainState::Complete && InNumRemaining == 0)
    { return; }

    if (Get_IsHolding())
    { _Elapsed += InDeltaT; }

    const auto CountChanged = InNumRemaining != _NumRemaining;

    _NumRemaining = InNumRemaining;
    _NumPeak = FMath::Max(_NumPeak, InNumRemaining);

    if (CountChanged || InNumRemaining == 0)
    { _TimeSinceLastChange = FCk_Time::ZeroSecond(); }
    else if (PreviousState == ECk_Pso_DrainState::Draining)
    { _TimeSinceLastChange += InDeltaT; }

    if (InNumRemaining > 0)
    {
        _State = ECk_Pso_DrainState::Draining;
        _QuietElapsed = FCk_Time::ZeroSecond();
    }
    else if (PreviousState == ECk_Pso_DrainState::Settling)
    {
        _QuietElapsed += InDeltaT;
    }
    else
    {
        _State = ECk_Pso_DrainState::Settling;
        _QuietElapsed = FCk_Time::ZeroSecond();
    }

    DoUpdate_ProgressRatio();

    if (_State == ECk_Pso_DrainState::Settling && _QuietElapsed >= _Params.Get_QuietPeriod())
    {
        _State = ECk_Pso_DrainState::Complete;
        _ProgressRatio = 1.0f;
        return;
    }

    DoTry_TimeOut();
}

auto
    FCk_Pso_DrainTracker::
    Get_IsHolding() const
    -> bool
{
    return _State == ECk_Pso_DrainState::Draining || _State == ECk_Pso_DrainState::Settling;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Pso_DrainTracker::
    DoUpdate_ProgressRatio()
    -> void
{
    const auto RawRatio = _NumPeak == 0
        ? 1.0f
        : 1.0f - static_cast<float>(_NumRemaining) / static_cast<float>(_NumPeak);

    _ProgressRatio = FMath::Max(_ProgressRatio, RawRatio);
}

auto
    FCk_Pso_DrainTracker::
    DoTry_TimeOut()
    -> void
{
    const auto MaxWaitIsEnabled = _Params.Get_MaxWait() > FCk_Time::ZeroSecond();
    if (MaxWaitIsEnabled && _Elapsed >= _Params.Get_MaxWait())
    {
        DoTimeOut(ECk_Pso_DrainTimeoutReason::MaxWait);
        return;
    }

    const auto StallTimeoutIsEnabled = _Params.Get_StallTimeout() > FCk_Time::ZeroSecond();
    if (StallTimeoutIsEnabled &&
        _State == ECk_Pso_DrainState::Draining &&
        _TimeSinceLastChange >= _Params.Get_StallTimeout())
    {
        DoTimeOut(ECk_Pso_DrainTimeoutReason::Stalled);
    }
}

auto
    FCk_Pso_DrainTracker::
    DoTimeOut(
        ECk_Pso_DrainTimeoutReason InReason)
    -> void
{
    _State = ECk_Pso_DrainState::TimedOut;
    _TimeoutReason = InReason;
}

// --------------------------------------------------------------------------------------------------------------------
