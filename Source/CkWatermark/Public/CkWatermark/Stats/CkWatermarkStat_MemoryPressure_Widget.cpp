#include "CkWatermarkStat_MemoryPressure_Widget.h"

#include <HAL/PlatformMemory.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkWatermarkStat_MemoryPressure_UWidget_UE::
    NativeGetStatName() const
    -> FText
{
    return FText::FromString(TEXT("Mem Pressure"));
}

auto
    UCkWatermarkStat_MemoryPressure_UWidget_UE::
    NativeGetStatValue() const
    -> FText
{
    switch (FPlatformMemory::GetStats().GetMemoryPressureStatus())
    {
        case FGenericPlatformMemoryStats::EMemoryPressureStatus::Nominal:  return FText::FromString(TEXT("Nominal"));
        case FGenericPlatformMemoryStats::EMemoryPressureStatus::Warning:  return FText::FromString(TEXT("Warning"));
        case FGenericPlatformMemoryStats::EMemoryPressureStatus::Critical: return FText::FromString(TEXT("Critical"));
        default:                              return FText::FromString(TEXT("Unknown"));
    }
}

auto
    UCkWatermarkStat_MemoryPressure_UWidget_UE::
    NativeGetStatColor() const
    -> FLinearColor
{
    switch (FPlatformMemory::GetStats().GetMemoryPressureStatus())
    {
        case FGenericPlatformMemoryStats::EMemoryPressureStatus::Nominal:  return FLinearColor(0.2f, 1.0f, 0.3f, 1.f);  // green
        case FGenericPlatformMemoryStats::EMemoryPressureStatus::Warning:  return FLinearColor(1.0f, 0.85f, 0.0f, 1.f); // yellow
        case FGenericPlatformMemoryStats::EMemoryPressureStatus::Critical: return FLinearColor(1.0f, 0.15f, 0.1f, 1.f); // red
        default: return FLinearColor(0.55f, 0.55f, 0.55f, 1.f); // gray
    }
}

// --------------------------------------------------------------------------------------------------------------------
