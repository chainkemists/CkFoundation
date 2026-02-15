#include "CkWatermarkStat_Ping_Widget.h"

#include "CkWatermark/Settings/CkWatermark_Settings.h"

#include <GameFramework/PlayerController.h>
#include <GameFramework/PlayerState.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkWatermarkStat_Ping_UWidget_UE::
    GetCurrentPingMs() const
    -> float
{
    if (const UWorld* World = GetWorld())
    {
        if (const APlayerController* PC = World->GetFirstPlayerController())
        {
            if (PC->IsLocalController())
            {
                if (const APlayerState* PS = PC->GetPlayerState<APlayerState>())
                {
                    return PS->GetPingInMilliseconds();
                }
            }
        }
    }
    return -1.f;
}

auto
    UCkWatermarkStat_Ping_UWidget_UE::
    NativeGetStatName() const
    -> FText
{
    return FText::FromString(TEXT("Ping"));
}

auto
    UCkWatermarkStat_Ping_UWidget_UE::
    NativeGetStatValue() const
    -> FText
{
    const float Ping = GetCurrentPingMs();
    if (Ping < 0.f)
    {
        return FText::FromString(TEXT("---"));
    }
    return FText::Format(FText::FromString(TEXT("{0} ms")),
                         FText::AsNumber(static_cast<int32>(Ping),
                                         &FNumberFormattingOptions::DefaultNoGrouping()));
}

auto
    UCkWatermarkStat_Ping_UWidget_UE::
    NativeGetStatColor() const
    -> FLinearColor
{
    const float Ping = GetCurrentPingMs();
    if (Ping < 0.f)
    {
        return FLinearColor::White;
    }
    return UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Ping_ColorBands()
        .GetColorForValue(Ping);
}

// --------------------------------------------------------------------------------------------------------------------
