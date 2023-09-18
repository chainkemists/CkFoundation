#include "CkChrono.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

const FCk_Chrono FCk_Chrono::Zero = FCk_Chrono{ FCk_Time::Zero };

// --------------------------------------------------------------------------------------------------------------------

FCk_Chrono::
    FCk_Chrono(TimeType InGoalValue)
    : _GoalValue(InGoalValue)
{
}

auto
    FCk_Chrono::
    operator==(const ThisType& InOther) const
    -> bool
{
    return Get_GoalValue() == InOther.Get_GoalValue() && _CurrentValue == InOther._CurrentValue;
}

auto
    FCk_Chrono::
    Tick(
        const TimeType& InDeltaT)
    -> TickStateType
{
    if (Get_IsDone())
    {
        return TickStateType::Done;
    }

    const auto& nonClampedNewValue = _CurrentValue + InDeltaT;

    _CurrentValue = FMath::Clamp(nonClampedNewValue, TimeType::Zero, _GoalValue);

    return Get_IsDone() ? TickStateType::Done : TickStateType::Ticking;
}

auto
    FCk_Chrono::
    Consume(
        const TimeType& InDeltaT)
    -> ConsumeStateType
{
    const auto previousValue = _CurrentValue;
    const auto& nonClampedNewValue = _CurrentValue - InDeltaT;

    _CurrentValue = FMath::Clamp(nonClampedNewValue, TimeType::Zero, _GoalValue);

    if (_CurrentValue == previousValue)
    {
        return ConsumeStateType::CouldNotConsume;
    }

    if (NOT Get_HasStarted())
    {
        return ConsumeStateType::FullyConsumed;
    }

    return ConsumeStateType::Consumed;
}

auto
    FCk_Chrono::
    Complete()
    -> ThisType&
{
    _CurrentValue = _GoalValue;

    return *this;
}

auto
    FCk_Chrono::
    Reset()
    -> ThisType&
{
    _CurrentValue = TimeType::Zero;

    return *this;
}

auto
    FCk_Chrono::
    Get_IsDone() const
    -> bool
{
    return _CurrentValue >= _GoalValue;
}

auto
    FCk_Chrono::
    Get_HasStarted() const
    -> bool
{
    return _CurrentValue > TimeType::Zero;
}

auto
    FCk_Chrono::
    Get_TimeRemaining(
        NormalizationPolicyType InNormalizationPolicy) const
    -> FCk_Time
{
    const auto remainingSeconds = [&]() -> float
    {
        const auto& goal      = _GoalValue.Get_Seconds();
        const auto& elapsed   = _CurrentValue.Get_Seconds();
        const auto& remaining = goal - elapsed;

        switch(InNormalizationPolicy)
        {
            case NormalizationPolicyType::None:
            {
                return remaining;
            }
            case NormalizationPolicyType::ZeroToOne:
            {
                return FMath::GetMappedRangeValueClamped
                (
                    FVector2D{ 0, goal },
                    FVector2D{ 0, 1 },
                    remaining
                );
            }
            case NormalizationPolicyType::MinusOneToOne:
            {
                return FMath::GetMappedRangeValueClamped
                (
                    FVector2D{ 0, goal },
                    FVector2D{ -1, 1 },
                    remaining
                );
            }
            default:
            {
                CK_INVALID_ENUM(InNormalizationPolicy);
                return 0.0f;
            }
        }
    }();

    return FCk_Time{ remainingSeconds };
}

auto
    FCk_Chrono::
    Get_TimeElapsed(
        NormalizationPolicyType InNormalizationPolicy) const
    -> FCk_Time
{
    const auto elapsedSeconds = [&]() -> float
    {
        const auto& goal    = _GoalValue.Get_Seconds();
        const auto& elapsed = _CurrentValue.Get_Seconds();

        switch(InNormalizationPolicy)
        {
            case NormalizationPolicyType::None:
            {
                return elapsed;
            }
            case NormalizationPolicyType::ZeroToOne:
            {
                return FMath::GetMappedRangeValueClamped
                (
                    FVector2D{ 0, goal },
                    FVector2D{ 0, 1 },
                    elapsed
                );
            }
            case NormalizationPolicyType::MinusOneToOne:
            {
                return FMath::GetMappedRangeValueClamped
                (
                    FVector2D{ 0, goal },
                    FVector2D{ -1, 1 },
                    elapsed
                );
            }
            default:
            {
                CK_INVALID_ENUM(InNormalizationPolicy);
                return 0.0f;
            }
        }
    }();

    return FCk_Time{ elapsedSeconds };
}

// --------------------------------------------------------------------------------------------------------------------

