#include "CkWatermarkStat_OsVersion_Widget.h"

#include <HAL/PlatformMisc.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkWatermarkStat_OsVersion_UWidget_UE::
    NativeGetStatName() const
    -> FText
{
    return FText::FromString(TEXT("OS"));
}

auto
    UCkWatermarkStat_OsVersion_UWidget_UE::
    NativeGetStatValue() const
    -> FText
{
    static const FString Version = FPlatformMisc::GetOSVersion();
    return FText::FromString(Version.IsEmpty() ? TEXT("---") : Version);
}

// --------------------------------------------------------------------------------------------------------------------
