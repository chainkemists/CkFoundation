#include "CkWatermarkStat_CpuCores_Widget.h"

#include <HAL/PlatformMisc.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkWatermarkStat_CpuCores_UWidget_UE::
    NativeGetStatName() const
    -> FText
{
    return FText::FromString(TEXT("Cores"));
}

auto
    UCkWatermarkStat_CpuCores_UWidget_UE::
    NativeGetStatValue() const
    -> FText
{
    static const int32 Physical = FPlatformMisc::NumberOfCores();
    static const int32 Logical  = FPlatformMisc::NumberOfCoresIncludingHyperthreads();
    return FText::FromString(FString::Printf(TEXT("%dc / %dt"), Physical, Logical));
}

// --------------------------------------------------------------------------------------------------------------------
