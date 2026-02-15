#include "CkWatermarkStat_RAM_Widget.h"

#include "CkMemory/CkMemory_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkWatermarkStat_RAM_UWidget_UE::
    NativeGetStatName() const
    -> FText
{
    return FText::FromString(TEXT("RAM"));
}

auto
    UCkWatermarkStat_RAM_UWidget_UE::
    NativeGetStatValue() const
    -> FText
{
    const float UsedGB = UCk_Stats_Subsystem_UE::Get_MemoryCountSnapshot(this).Get_PhysicalMemoryUsed();

    FNumberFormattingOptions Opts;
    Opts.MinimumFractionalDigits = 1;
    Opts.MaximumFractionalDigits = 1;
    Opts.UseGrouping = false;

    return FText::Format(FText::FromString(TEXT("{0} GB")),
                         FText::AsNumber(UsedGB, &Opts));
}

// --------------------------------------------------------------------------------------------------------------------
