#include "CkMeter_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Meter_UE::
    Make_Meter(
        const FCk_Meter_Params& InMeterParams)
    -> FCk_Meter
{
    return FCk_Meter{InMeterParams};
}

auto
    UCk_Utils_Meter_UE::
    Break_Meter(
        const FCk_Meter&     InMeter,
        FCk_Meter_Capacity&  OutMeterCapacity,
        FCk_Meter_Current&   OutMeterCurrentValue,
        FCk_Meter_Remaining& OutRemaining,
        FCk_Meter_Used&      OutUsed)
    -> void
{
    OutMeterCapacity     = InMeter.Get_Params().Get_Capacity();
    OutMeterCurrentValue = InMeter.Get_Value();
    OutRemaining         = InMeter.Get_Remaining();
    OutUsed              = InMeter.Get_Used();
}

auto
    UCk_Utils_Meter_UE::
    Consume_Meter(
        FCk_Meter& InMeter,
        const FCk_Meter_Consume& InConsume)
    -> FCk_Meter
{
    return InMeter.Consume(InConsume);
}

auto
    UCk_Utils_Meter_UE::
    Replenish_Meter(
        FCk_Meter& InMeter,
        const FCk_Meter_Replenish& InReplenish)
    -> FCk_Meter
{
    return InMeter.Replenish(InReplenish);
}

auto
    UCk_Utils_Meter_UE::
    Get_MeterSize(
        const FCk_Meter& InMeter)
    -> FCk_Meter_Size
{
    return InMeter.Get_Size();
}

auto
    UCk_Utils_Meter_UE::
    Get_MeterPercentageUsed(
        const FCk_Meter& InMeter)
    -> FCk_FloatRange_0to1
{
    return InMeter.Get_PercentageUsed();
}

auto
    UCk_Utils_Meter_UE::
    Get_MeterPercentageRemaining(
        const FCk_Meter& InMeter)
    -> FCk_FloatRange_0to1
{
    return InMeter.Get_PercentageRemaining();
}

auto
    UCk_Utils_Meter_UE::
    Get_IsMeterFull(
        const FCk_Meter& InMeter)
    -> bool
{
    return InMeter.Get_IsFull();
}

auto
    UCk_Utils_Meter_UE::
    Get_IsMeterEmpty(
        const FCk_Meter& InMeter)
    -> bool
{
    return InMeter.Get_IsEmpty();
}

// --------------------------------------------------------------------------------------------------------------------

