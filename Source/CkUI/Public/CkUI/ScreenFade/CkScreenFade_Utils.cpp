// Inspired by https://github.com/suramaru517/ScreenFade

#include "CkScreenFade_Utils.h"

#include "CkUI/Subsystem/CkUI_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ScreenFade_Utils::
    Request_ScreenFade(
        const APlayerController* InOwningPlayer,
        const FCk_ScreenFade_Params& InFadeParams,
        const int32 ZOrder)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOwningPlayer),
        TEXT("Invalid PlayerController supplied, cannot perform Screen Fade"))
    { return; }

    const auto& LocalPlayer = InOwningPlayer->GetLocalPlayer();

    if (ck::Is_NOT_Valid(LocalPlayer))
    { return; }

    const auto& UISubsystem = LocalPlayer->GetSubsystem<UCk_UI_Subsystem_UE>();

    if (ck::Is_NOT_Valid(UISubsystem))
    { return; }

    UISubsystem->Request_AddScreenFadeWidget(InFadeParams, InOwningPlayer, ZOrder);
}

auto
    UCk_ScreenFade_Utils::
    Request_ScreenFadeIn(
        const APlayerController* InOwningPlayer,
        float InFadeTime,
        FLinearColor InFromColor,
        FCk_Delegate_OnScreenFadeFinished_Dynamic OnFinished,
        bool InFadeAudio,
        bool InFadeWhenPaused,
        int32 InZOrder)
    -> void
{
    const auto FadeParams = FCk_ScreenFade_Params{InFadeTime}
                            .Set_FromColor(InFromColor)
                            .Set_ToColor(FLinearColor::Transparent)
                            .Set_OnFinishedDynamic(OnFinished)
                            .Set_FadeAudio(InFadeAudio)
                            .Set_FadeWhenPaused(InFadeWhenPaused);

    Request_ScreenFade(InOwningPlayer, FadeParams, InZOrder);
}

auto
    UCk_ScreenFade_Utils::
    Request_ScreenFadeOut(
        const APlayerController* InOwningPlayer,
        float InFadeTime,
        FLinearColor ToColor,
        FCk_Delegate_OnScreenFadeFinished_Dynamic OnFinished,
        bool InFadeAudio,
        bool InFadeWhenPaused,
        int32 InZOrder)
    -> void
{
    const auto FadeParams = FCk_ScreenFade_Params{InFadeTime}
                            .Set_FromColor(FLinearColor::Transparent)
                            .Set_ToColor(ToColor)
                            .Set_OnFinishedDynamic(OnFinished)
                            .Set_FadeAudio(InFadeAudio)
                            .Set_FadeWhenPaused(InFadeWhenPaused);

    Request_ScreenFade(InOwningPlayer, FadeParams, InZOrder);
}

// --------------------------------------------------------------------------------------------------------------------
