#include "CkWatermarkStat_FrameCount_Widget.h"

// GFrameCounter is the global frame number incremented each engine tick.
extern ENGINE_API uint64 GFrameCounter;

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkWatermarkStat_FrameCount_UWidget_UE::
    NativeGetStatName() const
    -> FText
{
    return FText::FromString(TEXT("Frame"));
}

auto
    UCkWatermarkStat_FrameCount_UWidget_UE::
    NativeGetStatValue() const
    -> FText
{
    return FText::AsNumber(static_cast<int64>(GFrameCounter),
                           &FNumberFormattingOptions::DefaultNoGrouping());
}

// --------------------------------------------------------------------------------------------------------------------
