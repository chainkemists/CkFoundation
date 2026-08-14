// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkScreenFade_Subsystem.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkUICore/ScreenFade/CkScreenFade_Slate.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ScreenFade_Subsystem_UE::
    Request_AddScreenFadeWidget(
        const FCk_ScreenFade_Params& InFadeParams,
        const APlayerController* InOwningPlayer,
        int32 InZOrder)
    -> void
{
    const auto ControllerID = DoGet_PlayerControllerID(InOwningPlayer);

    if (_FadeWidgetsForID.Contains(ControllerID))
    {
        DoRemoveScreenFadeWidget(InOwningPlayer, ControllerID);
    }

    auto OnFadeFinished = FCk_Delegate_OnScreenFadeFinished{};

    if (InFadeParams.Get_ToColor().A <= 0.0f)
    {
        OnFadeFinished.BindUObject(this, &ThisType::DoRemoveScreenFadeWidget, ControllerID);
    }

    TSharedRef<SCk_ScreenFade> FadeWidget = SNew(SCk_ScreenFade)
        .FadeParams(InFadeParams)
        .OnFadeFinished(OnFadeFinished);

    auto* GameViewport = GetWorld()->GetGameViewport();

    if (ck::Is_NOT_Valid(GameViewport))
    { return; }

    if (ck::IsValid(InOwningPlayer))
    {
        GameViewport->AddViewportWidgetForPlayer(InOwningPlayer->GetLocalPlayer(), FadeWidget, InZOrder);
    }
    else
    {
        GameViewport->AddViewportWidgetContent(FadeWidget, InZOrder + 10);
    }

    _FadeWidgetsForID.Emplace(ControllerID, FadeWidget);
    FadeWidget->StartFade();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ScreenFade_Subsystem_UE::
    DoRemoveScreenFadeWidget(
        const APlayerController* InOwningPlayer,
        int32 InControllerID)
    -> void
{
    const auto FadeWidget = _FadeWidgetsForID[InControllerID].Pin().ToSharedRef();
    _FadeWidgetsForID.Remove(InControllerID);

    auto* GameViewport = GetWorld()->GetGameViewport();

    if (ck::Is_NOT_Valid(GameViewport))
    { return; }

    if (ck::IsValid(InOwningPlayer))
    {
        GameViewport->RemoveViewportWidgetForPlayer(InOwningPlayer->GetLocalPlayer(), FadeWidget);
    }
    else
    {
        GameViewport->RemoveViewportWidgetContent(FadeWidget);
    }
}

auto
    UCk_ScreenFade_Subsystem_UE::
    DoRemoveScreenFadeWidget(
        int32 InControllerID)
    -> void
{
    DoRemoveScreenFadeWidget(DoGet_PlayerControllerFromID(InControllerID), InControllerID);
}

auto
    UCk_ScreenFade_Subsystem_UE::
    DoGet_PlayerControllerID(
        const APlayerController* PlayerController) const
    -> int32
{
    if (ck::IsValid(PlayerController))
    {
        if (const auto* LocalPlayer = PlayerController->GetLocalPlayer();
            ck::IsValid(LocalPlayer))
        {
            return LocalPlayer->GetControllerId();
        }
    }

    return _InvalidPlayerControllerID;
}

auto
    UCk_ScreenFade_Subsystem_UE::
    DoGet_PlayerControllerFromID(
        const int32 ControllerID) const
    -> APlayerController*
{
    if (ControllerID == _InvalidPlayerControllerID)
    { return nullptr; }

    for (auto Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        if (auto* PlayerController = Iterator->Get();
            DoGet_PlayerControllerID(PlayerController) == ControllerID)
        {
            return PlayerController;
        }
    }

    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
