#include "CkChrono.h"

#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

const FCk_Chrono FCk_Chrono::Zero = FCk_Chrono{ FCk_Time::ZeroSecond() };

// --------------------------------------------------------------------------------------------------------------------

FCk_Chrono::
    FCk_Chrono(TimeType InGoalValue)
    : _GoalValue(InGoalValue)
{
}

auto
    FCk_Chrono::
    operator==(
        const ThisType& InOther) const
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
    { return TickStateType::Done; }

    const auto& NonClampedNewValue = _CurrentValue + InDeltaT;

    _CurrentValue = FMath::Clamp(NonClampedNewValue, TimeType::ZeroSecond(), _GoalValue);

    return Get_IsDone() ? TickStateType::Done : TickStateType::Ticking;
}

auto
    FCk_Chrono::
    Consume(
        const TimeType& InDeltaT)
    -> ConsumeStateType
{
    const auto PreviousValue = _CurrentValue;
    const auto& NonClampedNewValue = _CurrentValue - InDeltaT;

    _CurrentValue = FMath::Clamp(NonClampedNewValue, TimeType::ZeroSecond(), _GoalValue);

    if (_CurrentValue == PreviousValue)
    { return ConsumeStateType::CouldNotConsume; }

    if (NOT Get_HasStarted())
    { return ConsumeStateType::FullyConsumed; }

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
    _CurrentValue = TimeType::ZeroSecond();

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
    Get_IsDepleted() const
    -> bool
{
    return _CurrentValue <= TimeType::ZeroSecond();
}

auto
    FCk_Chrono::
    Get_HasStarted() const
    -> bool
{
    return _CurrentValue > TimeType::ZeroSecond();
}

auto
    FCk_Chrono::
    Get_TimeRemaining(
        NormalizationPolicyType InNormalizationPolicy) const
    -> FCk_Time
{
    const auto RemainingSeconds = [&]() -> double
    {
        const auto& Goal      = _GoalValue.Get_Seconds();
        const auto& Elapsed   = _CurrentValue.Get_Seconds();
        const auto& Remaining = Goal - Elapsed;

        switch(InNormalizationPolicy)
        {
            case NormalizationPolicyType::None:
            {
                return Remaining;
            }
            case NormalizationPolicyType::ZeroToOne:
            {
                return FMath::GetMappedRangeValueClamped
                (
                    FVector2D{ 0, Goal },
                    FVector2D{ 0, 1 },
                    Remaining
                );
            }
            case NormalizationPolicyType::MinusOneToOne:
            {
                return FMath::GetMappedRangeValueClamped
                (
                    FVector2D{ 0, Goal },
                    FVector2D{ -1, 1 },
                    Remaining
                );
            }
            default:
            {
                CK_INVALID_ENUM(InNormalizationPolicy);
                return 0.0;
            }
        }
    }();

    return FCk_Time{ RemainingSeconds };
}

auto
    FCk_Chrono::
    Get_TimeElapsed(
        NormalizationPolicyType InNormalizationPolicy) const
    -> FCk_Time
{
    const auto ElapsedSeconds = [&]() -> double
    {
        const auto& Goal    = _GoalValue.Get_Seconds();
        const auto& Elapsed = _CurrentValue.Get_Seconds();

        switch(InNormalizationPolicy)
        {
            case NormalizationPolicyType::None:
            {
                return Elapsed;
            }
            case NormalizationPolicyType::ZeroToOne:
            {
                return FMath::GetMappedRangeValueClamped
                (
                    FVector2D{ 0, Goal },
                    FVector2D{ 0, 1 },
                    Elapsed
                );
            }
            case NormalizationPolicyType::MinusOneToOne:
            {
                return FMath::GetMappedRangeValueClamped
                (
                    FVector2D{ 0, Goal },
                    FVector2D{ -1, 1 },
                    Elapsed
                );
            }
            default:
            {
                CK_INVALID_ENUM(InNormalizationPolicy);
                return 0.0;
            }
        }
    }();

    return FCk_Time{ ElapsedSeconds };
}

// --------------------------------------------------------------------------------------------------------------------

