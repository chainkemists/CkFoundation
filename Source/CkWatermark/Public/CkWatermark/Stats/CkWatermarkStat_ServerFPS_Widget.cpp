#include "CkWatermarkStat_ServerFPS_Widget.h"

#include "CkCore/Engine/CkGameState.h"
#include "CkWatermark/Settings/CkWatermark_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkWatermarkStat_ServerFPS_UWidget_UE::
    NativeGetStatName() const
    -> FText
{
    return FText::FromString(TEXT("Server FPS"));
}

auto
    UCkWatermarkStat_ServerFPS_UWidget_UE::
    NativeGetStatValue() const
    -> FText
{
    if (const UWorld* World = GetWorld())
    {
        if (const ACk_GameState_UE* GS = World->GetGameState<ACk_GameState_UE>())
        {
            return FText::AsNumber(static_cast<int32>(GS->Get_ServerFPS()),
                                   &FNumberFormattingOptions::DefaultNoGrouping());
        }
    }
    return FText::FromString(TEXT("---"));
}

auto
    UCkWatermarkStat_ServerFPS_UWidget_UE::
    NativeGetStatColor() const
    -> FLinearColor
{
    if (const UWorld* World = GetWorld())
    {
        if (const ACk_GameState_UE* GS = World->GetGameState<ACk_GameState_UE>())
        {
            return UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_ServerFPS_ColorBands()
                .GetColorForValue(GS->Get_ServerFPS());
        }
    }
    return FLinearColor::White;
}

// --------------------------------------------------------------------------------------------------------------------
