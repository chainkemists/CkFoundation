// Inspired by https://github.com/suramaru517/ScreenFade

#pragma once

#include "CkUICore/ScreenFade/CkScreenFade_Utils.h"

#include <CoreMinimal.h>
#include <Widgets/Images/SImage.h>
#include <Widgets/SWidget.h>

// --------------------------------------------------------------------------------------------------------------------

class SCk_ScreenFade : public SImage
{
public:
    SLATE_BEGIN_ARGS(SCk_ScreenFade)
    {
    }
        SLATE_ARGUMENT(FCk_ScreenFade_Params, FadeParams)

        SLATE_EVENT(FCk_Delegate_OnScreenFadeFinished, OnFadeFinished)
    SLATE_END_ARGS()

    auto Construct(const FArguments& InArgs) -> void;
    auto Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) -> void override;

    auto StartFade() -> void;

private:
    auto FinishFade() -> void;
    auto ApplyFade(const FLinearColor& NextColor) -> void;

    static auto Get_World() -> UWorld*;
    static auto Get_IsGamePaused() -> bool;
    static auto SetPrimaryVolume(const float Volume) -> void;

private:
    FCk_ScreenFade_Params _FadeParams;
    FCk_Delegate_OnScreenFadeFinished _OnFadeFinished;

    FCk_Time _TimeRemaining;
};

// --------------------------------------------------------------------------------------------------------------------
