#include "CkWatermarkStat_VRAM_Widget.h"

#include "CkMemory/CkMemory_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkWatermarkStat_VRAM_UWidget_UE::
    NativeGetStatName() const
    -> FText
{
    return FText::FromString(TEXT("VRAM"));
}

auto
    UCkWatermarkStat_VRAM_UWidget_UE::
    NativeGetStatValue() const
    -> FText
{
    const float UsedGB = UCk_Stats_Subsystem_UE::Get_MemoryCountSnapshot(this).Get_RHIMemoryUsed();

    FNumberFormattingOptions Opts;
    Opts.MinimumFractionalDigits = 1;
    Opts.MaximumFractionalDigits = 1;
    Opts.UseGrouping = false;

    return FText::Format(FText::FromString(TEXT("{0} GB")),
                         FText::AsNumber(UsedGB, &Opts));
}

// --------------------------------------------------------------------------------------------------------------------
