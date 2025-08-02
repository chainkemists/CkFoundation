// Inspired by https://github.com/suramaru517/ScreenFade

#include "CkScreenFade_Widget.h"

#include <AudioDevice.h>
#include <Engine/GameViewportClient.h>
#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    SScreenFade_Widget::
    Construct(
        const FArguments& InArgs)
    -> void
{
    _FadeParams = InArgs.__FadeParams;
    _OnFadeFinished = InArgs.__OnFadeFinished;
}

auto
    SScreenFade_Widget::
    Tick(
        const FGeometry& AllottedGeometry,
        const double InCurrentTime,
        const float InDeltaTime)
    -> void
{
    if (NOT _FadeParams.Get_FadeWhenPaused() && Get_IsGamePaused())
    { return; }

    _TimeRemaining = FMath::Max(_TimeRemaining - InDeltaTime, 0.0f);

    if (_TimeRemaining <= 0.0f)
    {
        FinishFade();
        return;
    }

    const auto NextColor = _FadeParams.Get_ToColor() - (_FadeParams.Get_ToColor() - _FadeParams.Get_FromColor()) * _TimeRemaining / _FadeParams.Get_FadeTime();
    ApplyFade(NextColor);
}

auto
    SScreenFade_Widget::
    StartFade()
    -> void
{
    SetImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")));

    if (_FadeParams.Get_FadeTime() <= 0.0f)
    {
        FinishFade();
        return;
    }

    _TimeRemaining = _FadeParams.Get_FadeTime();
    ApplyFade(_FadeParams.Get_FromColor());

    SetCanTick(true);
}

auto
    SScreenFade_Widget::
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
    SScreenFade_Widget::
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
    SScreenFade_Widget::
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
    SScreenFade_Widget::
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
    SScreenFade_Widget::
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
