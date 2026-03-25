#include "CkSmCondition_Timer.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_SmCondition_Timer::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    _StartTimeSeconds = FPlatformTime::Seconds();

    return Super::Construct(InHandle, InSpawnParams);
}

auto
    UCk_SmCondition_Timer::
    Evaluate() const
    -> bool
{
    auto Elapsed = FPlatformTime::Seconds() - _StartTimeSeconds;
    return Elapsed >= static_cast<double>(_DurationSeconds);
}

// --------------------------------------------------------------------------------------------------------------------
