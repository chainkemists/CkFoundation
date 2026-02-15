#include "CkWatermarkStat_FPS_Widget.h"

#include "CkWatermark/Settings/CkWatermark_Settings.h"

// GAverageFPS is a smoothed frame-rate value maintained by the engine each frame.
ENGINE_API extern float GAverageFPS;

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkWatermarkStat_FPS_UWidget_UE::
    NativeGetStatName() const
    -> FText
{
    return FText::FromString(TEXT("FPS"));
}

auto
    UCkWatermarkStat_FPS_UWidget_UE::
    NativeGetStatValue() const
    -> FText
{
    return FText::AsNumber(static_cast<int32>(GAverageFPS),
                           &FNumberFormattingOptions::DefaultNoGrouping());
}

auto
    UCkWatermarkStat_FPS_UWidget_UE::
    NativeGetStatColor() const
    -> FLinearColor
{
    return UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_FPS_ColorBands()
        .GetColorForValue(GAverageFPS);
}

// --------------------------------------------------------------------------------------------------------------------
