#include "CkSmTask_Delay.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmTask_Delay::
    OnStateEnter()
    -> void
{
    _ElapsedSeconds = 0.0f;
}

auto
    UCk_SmTask_Delay::
    Tick(
        float InDeltaSeconds)
    -> ECk_SmTaskResult
{
    _ElapsedSeconds += InDeltaSeconds;

    if (_ElapsedSeconds >= _DelaySeconds)
    {
        return ECk_SmTaskResult::Succeeded;
    }

    return ECk_SmTaskResult::Running;
}

// --------------------------------------------------------------------------------------------------------------------
