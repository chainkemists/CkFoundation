// Inspired by https://github.com/suramaru517/ScreenFade

#include "CkScreenFade_Slate.h"

#include <AudioDevice.h>
#include <Engine/GameViewportClient.h>
#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    SCk_ScreenFade::
    Construct(
        const FArguments& InArgs)
    -> void
{
    _FadeParams = InArgs._FadeParams;
    _OnFadeFinished = InArgs._OnFadeFinished;
}

auto
    SCk_ScreenFade::
    Tick(
        const FGeometry& AllottedGeometry,
        const double InCurrentTime,
        const float InDeltaTime)
    -> void
{
    if (NOT _FadeParams.Get_FadeWhenPaused() && Get_IsGamePaused())
    { return; }

    _TimeRemaining = FCk_Time{FMath::Max(_TimeRemaining.Get_Seconds() - InDeltaTime, 0.0)};

    if (_TimeRemaining <= FCk_Time::ZeroSecond())
    {
        FinishFade();
        return;
    }

    const auto FadeAlpha = static_cast<float>(_TimeRemaining / _FadeParams.Get_FadeTime());
    const auto NextColor = _FadeParams.Get_ToColor() - (_FadeParams.Get_ToColor() - _FadeParams.Get_FromColor()) * FadeAlpha;
    ApplyFade(NextColor);
}

auto
    SCk_ScreenFade::
    StartFade()
    -> void
{
    SetImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")));

    if (_FadeParams.Get_FadeTime() <= FCk_Time::ZeroSecond())
    {
        FinishFade();
        return;
    }

    _TimeRemaining = _FadeParams.Get_FadeTime();
    ApplyFade(_FadeParams.Get_FromColor());

    SetCanTick(true);
}

auto
    SCk_ScreenFade::
    FinishFade()
    -> void
{
    ApplyFade(_FadeParams.Get_ToColor());

    _OnFadeFinished.ExecuteIfBound();
    _FadeParams.Get_OnFinished().ExecuteIfBound();
    _FadeParams.Get_OnFinishedDynamic().ExecuteIfBound();

    SetCanTick(false);
}

auto
    SCk_ScreenFade::
    ApplyFade(
        const FLinearColor& NextColor)
    -> void
{
    SetColorAndOpacity(NextColor);

    if (_FadeParams.Get_FadeAudio())
    {
        SetPrimaryVolume(1.0f - NextColor.A);
    }
}

auto
    SCk_ScreenFade::
    Get_World()
    -> UWorld*
{
    if (ck::IsValid(GEngine))
    {
        if (const auto* GameViewport = GEngine->GameViewport.Get();
            ck::IsValid(GameViewport))
        {
            return GameViewport->GetWorld();
        }
    }

    return {};
}

auto
    SCk_ScreenFade::
    Get_IsGamePaused()
    -> bool
{
    if (const auto* World = Get_World();
        ck::IsValid(World))
    {
        return World->IsPaused();
    }

    return {};
}

auto
    SCk_ScreenFade::
    SetPrimaryVolume(
        const float Volume)
    -> void
{
    if (const auto* World = Get_World();
        ck::IsValid(World))
    {
        if (auto* AudioDevice = World->GetAudioDeviceRaw();
            ck::IsValid(AudioDevice, ck::IsValid_Policy_NullptrOnly{}))
        {
            AudioDevice->SetTransientPrimaryVolume(Volume);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
