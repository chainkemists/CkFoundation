#include "CkWatermarkStat_Time_Widget.h"

#include <Misc/DateTime.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkWatermarkStat_Time_UWidget_UE::
    NativeGetStatName() const
    -> FText
{
    return FText::FromString(TEXT("Time"));
}

auto
    UCkWatermarkStat_Time_UWidget_UE::
    NativeGetStatValue() const
    -> FText
{
    return FText::FromString(FDateTime::Now().ToString(TEXT("%H:%M:%S")));
}

// --------------------------------------------------------------------------------------------------------------------
