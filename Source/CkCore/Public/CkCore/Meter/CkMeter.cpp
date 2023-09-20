#include "CkMeter.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Meter::
    FCk_Meter(
        FCk_Meter_Params InParams)
    : _Params(InParams)
    , _InternalChrono(ChronoType{ ChronoType::TimeType{ InParams.Get_Capacity().Get_MaxCapacity() } })
{
    const auto& startingValue = InParams.Get_StartingValue().Get_Value();
    const auto& meterCapacity = InParams.Get_Capacity();
    const auto& meterMax      = meterCapacity.Get_MaxCapacity();

    _InternalChrono.Tick(ChronoType::TimeType{ startingValue * meterMax });
}

auto
    FCk_Meter::operator==(
        const ThisType& InOther) const -> bool
{
    return _InternalChrono == InOther._InternalChrono;
}

auto
    FCk_Meter::
    operator+(
        const ThisType& InOther) const
    -> ThisType
{
    const auto& ParamsA = Get_Params();
    const auto& ParamsB = InOther.Get_Params();

    const auto& CapacityA = ParamsA.Get_Capacity();
    const auto& CapacityB = ParamsB.Get_Capacity();

    const auto NewCapacity =
        FCk_Meter_Capacity{CapacityA.Get_MaxCapacity() + CapacityB.Get_MaxCapacity()}
        .Set_MinCapacity(CapacityA.Get_MinCapacity() + CapacityB.Get_MinCapacity());

    return FCk_Meter
    {
        FCk_Meter_Params{NewCapacity}
        .Set_StartingValue(FCk_FloatRange_0to1{Get_Used().Get_AmountUsed() + InOther.Get_Used().Get_AmountUsed()})
    };
}

auto
    FCk_Meter::
    operator-(
        const ThisType& InOther) const
    -> ThisType
{
    const auto& ParamsA = Get_Params();
    const auto& ParamsB = InOther.Get_Params();

    const auto& CapacityA = ParamsA.Get_Capacity();
    const auto& CapacityB = ParamsB.Get_Capacity();

    const auto NewCapacity =
        FCk_Meter_Capacity{CapacityA.Get_MaxCapacity() - CapacityB.Get_MaxCapacity()}
        .Set_MinCapacity(CapacityA.Get_MinCapacity() - CapacityB.Get_MinCapacity());

    return FCk_Meter
    {
        FCk_Meter_Params{NewCapacity}
        .Set_StartingValue(FCk_FloatRange_0to1{Get_Used().Get_AmountUsed() - InOther.Get_Used().Get_AmountUsed()})
    };
}

auto
    FCk_Meter::
    operator*(
        const ThisType& InOther) const
    -> ThisType
{
    const auto& ParamsA = Get_Params();
    const auto& ParamsB = InOther.Get_Params();

    const auto& CapacityA = ParamsA.Get_Capacity();
    const auto& CapacityB = ParamsB.Get_Capacity();

    const auto NewCapacity =
        FCk_Meter_Capacity{CapacityA.Get_MaxCapacity() * CapacityB.Get_MaxCapacity()}
        .Set_MinCapacity(CapacityA.Get_MinCapacity() * CapacityB.Get_MinCapacity());

    return FCk_Meter
    {
        FCk_Meter_Params{NewCapacity}
        .Set_StartingValue(FCk_FloatRange_0to1{Get_Used().Get_AmountUsed() * InOther.Get_Used().Get_AmountUsed()})
    };
}

auto
    FCk_Meter::
    operator/(
        const ThisType& InOther) const
    -> ThisType
{
    const auto& ParamsA = Get_Params();
    const auto& ParamsB = InOther.Get_Params();

    const auto& CapacityA = ParamsA.Get_Capacity();
    const auto& CapacityB = ParamsB.Get_Capacity();

    const auto NewCapacity =
        FCk_Meter_Capacity{CapacityA.Get_MaxCapacity() / CapacityB.Get_MaxCapacity()}
        .Set_MinCapacity(CapacityA.Get_MinCapacity() / CapacityB.Get_MinCapacity());

    return FCk_Meter
    {
        FCk_Meter_Params{NewCapacity}
        .Set_StartingValue(FCk_FloatRange_0to1{Get_Used().Get_AmountUsed() / InOther.Get_Used().Get_AmountUsed()})
    };
}

auto
    FCk_Meter::
    Consume(
        const FCk_Meter_Consume& InConsume)
    -> ThisType&
{
    _InternalChrono.Consume(ChronoType::TimeType{ InConsume.Get_ConsumeAmount() });

    return *this;
}

auto
    FCk_Meter::
    Replenish(
        const FCk_Meter_Replenish& InReplenish)
    -> ThisType&
{
    _InternalChrono.Tick(ChronoType::TimeType{ InReplenish.Get_ReplenishAmount() });

    return *this;
}

auto
    FCk_Meter::
    Get_IsFull() const
    -> bool
{
    return _InternalChrono.Get_IsDone();
}

auto
    FCk_Meter::
    Get_IsEmpty() const
    -> bool
{
    return NOT _InternalChrono.Get_HasStarted();
}

auto
    FCk_Meter::
    Get_Remaining() const
    -> FCk_Meter_Remaining
{
    const auto& meterSize = Get_Size().Get_Size();

    return FCk_Meter_Remaining{ meterSize - Get_Used().Get_AmountUsed() };
}

auto
    FCk_Meter::
    Get_Used() const
    -> FCk_Meter_Used
{
    const auto& percentageUsed = Get_PercentageUsed().Get_Value();
    const auto& meterSize      = Get_Size().Get_Size();

    return FCk_Meter_Used{ (meterSize * percentageUsed) };
}

auto
    FCk_Meter::
    Get_Value() const
    -> FCk_Meter_Current
{
    const auto& meterCapacity  = Get_Params().Get_Capacity();
    const auto& meterMin       = meterCapacity.Get_MinCapacity();

    return FCk_Meter_Current{ meterMin + Get_Used().Get_AmountUsed() };
}

auto
    FCk_Meter::
    Get_PercentageUsed() const
    -> FCk_FloatRange_0to1
{
    return FCk_FloatRange_0to1
    {
        _InternalChrono.Get_TimeElapsed(ECk_NormalizationPolicy::ZeroToOne).Get_Seconds()
    };
}

auto
    FCk_Meter::
    Get_PercentageRemaining() const
    -> FCk_FloatRange_0to1
{
    return FCk_FloatRange_0to1
    {
        _InternalChrono.Get_TimeRemaining(ECk_NormalizationPolicy::ZeroToOne).Get_Seconds()
    };
}

auto
    FCk_Meter::
    Get_Size() const
    -> FCk_Meter_Size
{
    const auto& meterCapacity = Get_Params().Get_Capacity();
    const auto& meterMin      = meterCapacity.Get_MinCapacity();
    const auto& meterMax      = meterCapacity.Get_MaxCapacity();

    return FCk_Meter_Size{ meterMax - meterMin };
}

// --------------------------------------------------------------------------------------------------------------------
