#include "CkWatermarkStat_EnsureCount_Widget.h"

#include "CkCore/Ensure/CkEnsure_Subsystem.h"
#include "CkWatermark/Settings/CkWatermark_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkWatermarkStat_EnsureCount_UWidget_UE::
    NativeGetStatName() const
    -> FText
{
    return FText::FromString(TEXT("Ensures"));
}

auto
    UCkWatermarkStat_EnsureCount_UWidget_UE::
    NativeGetStatValue() const
    -> FText
{
    if (const UCk_Ensure_Subsystem_UE* Subsystem = GEngine
            ? GEngine->GetEngineSubsystem<UCk_Ensure_Subsystem_UE>()
            : nullptr)
    {
        return FText::AsNumber(Subsystem->Get_EnsureCount(),
                               &FNumberFormattingOptions::DefaultNoGrouping());
    }
    return FText::FromString(TEXT("---"));
}

auto
    UCkWatermarkStat_EnsureCount_UWidget_UE::
    NativeGetStatColor() const
    -> FLinearColor
{
    if (const UCk_Ensure_Subsystem_UE* Subsystem = GEngine
            ? GEngine->GetEngineSubsystem<UCk_Ensure_Subsystem_UE>()
            : nullptr)
    {
        return UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_EnsureCount_ColorBands()
            .GetColorForValue(static_cast<float>(Subsystem->Get_EnsureCount()));
    }
    return FLinearColor::White;
}

// --------------------------------------------------------------------------------------------------------------------
