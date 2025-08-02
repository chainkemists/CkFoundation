// Inspired by https://github.com/suramaru517/ScreenFade

#pragma once

#include "CkUI/ScreenFade/CkScreenFade_Utils.h"

#include <CoreMinimal.h>
#include <Widgets/Images/SImage.h>
#include "Widgets/SWidget.h"

// --------------------------------------------------------------------------------------------------------------------

class SScreenFade_Widget : public SImage
{
public:
	SLATE_BEGIN_ARGS(SScreenFade_Widget)
	{
	}
	SLATE_ARGUMENT(FCk_ScreenFade_Params, _FadeParams)
	SLATE_EVENT(FCk_Delegate_OnScreenFadeFinished, _OnFadeFinished)
	SLATE_END_ARGS()

	auto Construct(const FArguments& InArgs) -> void;
	auto Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) -> void override;

	auto StartFade() -> void;

private:
	void FinishFade();
	void ApplyFade(const FLinearColor& NextColor);

	static auto Get_World() -> UWorld*;
	static auto Get_IsGamePaused() -> bool;
	static auto SetPrimaryVolume(const float Volume) -> void;

	FCk_ScreenFade_Params _FadeParams;
	FCk_Delegate_OnScreenFadeFinished _OnFadeFinished;

	float _TimeRemaining = 0.0f;
};

// --------------------------------------------------------------------------------------------------------------------
