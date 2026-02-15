#include "CkWatermarkStat_CpuBrand_Widget.h"

#include <HAL/PlatformMisc.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkWatermarkStat_CpuBrand_UWidget_UE::
    NativeGetStatName() const
    -> FText
{
    return FText::FromString(TEXT("CPU"));
}

auto
    UCkWatermarkStat_CpuBrand_UWidget_UE::
    NativeGetStatValue() const
    -> FText
{
    static const FString Brand = FPlatformMisc::GetCPUBrand().TrimStartAndEnd();
    return FText::FromString(Brand.IsEmpty() ? TEXT("---") : Brand);
}

// --------------------------------------------------------------------------------------------------------------------
