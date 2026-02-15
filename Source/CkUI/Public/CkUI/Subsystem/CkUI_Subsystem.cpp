#include "CkUI_Subsystem.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkUI/ScreenFade/CkScreenFade_Widget.h"
#include "CkUI/WidgetLayerHandler/CkWidgetLayerHandler_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

    _SubsystemEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(_Registry);
    UCk_Utils_WidgetLayerHandler_UE::Add(_SubsystemEntity, FCk_Fragment_WidgetLayerHandler_ParamsData{});
}

auto
    UCk_UI_Subsystem_UE::
    Deinitialize()
    -> void
{
    Super::Deinitialize();
}

auto
    UCk_UI_Subsystem_UE::
    Get_WidgetLayerHandler() const
    -> FCk_Handle_WidgetLayerHandler
{
    return UCk_Utils_WidgetLayerHandler_UE::CastChecked(_SubsystemEntity);
}

auto
    UCk_UI_Subsystem_UE::
    Request_AddScreenFadeWidget(
        const FCk_ScreenFade_Params& InFadeParams,
        const APlayerController* InOwningPlayer,
        int32 InZOrder)
    -> void
{
    const auto& ControllerID = DoGet_PlayerControllerID(InOwningPlayer);

    if (_FadeWidgetsForID.Contains(ControllerID))
    {
        DoRemoveScreenFadeWidget(InOwningPlayer, ControllerID);
    }

    auto OnFadeFinished = FCk_Delegate_OnScreenFadeFinished{};

    if (InFadeParams.Get_ToColor().A <= 0.0f)
    {
        OnFadeFinished.BindUObject(this, &ThisType::DoRemoveScreenFadeWidget, ControllerID);
    }

    TSharedRef<SScreenFade_Widget> FadeWidget = SNew(SScreenFade_Widget)._FadeParams(InFadeParams)._OnFadeFinished(OnFadeFinished);

    if (auto* GameViewport = GetWorld()->GetGameViewport();
        ck::IsValid(GameViewport))
    {
        if (ck::IsValid(InOwningPlayer))
        {
            GameViewport->AddViewportWidgetForPlayer(InOwningPlayer->GetLocalPlayer(), FadeWidget, InZOrder);
        }
        else
        {
            GameViewport->AddViewportWidgetContent(FadeWidget, InZOrder + 10);
        }
    }

    _FadeWidgetsForID.Emplace(ControllerID, FadeWidget);
    FadeWidget->StartFade();
}

auto
    UCk_UI_Subsystem_UE::
    DoRemoveScreenFadeWidget(
        const APlayerController* InOwningPlayer,
        int32 InControllerID)
    -> void
{
    const auto FadeWidget = _FadeWidgetsForID[InControllerID].Pin().ToSharedRef();
    _FadeWidgetsForID.Remove(InControllerID);

    if (auto* GameViewport = GetWorld()->GetGameViewport();
        ck::IsValid(GameViewport))
    {
        if (ck::IsValid(InOwningPlayer))
        {
            GameViewport->RemoveViewportWidgetForPlayer(InOwningPlayer->GetLocalPlayer(), FadeWidget);
        }
        else
        {
            GameViewport->RemoveViewportWidgetContent(FadeWidget);
        }
    }
}

auto
    UCk_UI_Subsystem_UE::
    DoRemoveScreenFadeWidget(
        int32 InControllerID)
    -> void
{
    DoRemoveScreenFadeWidget(DoGet_PlayerControllerFromID(InControllerID), InControllerID);
}

auto
    UCk_UI_Subsystem_UE::
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
    UCk_UI_Subsystem_UE::
    DoGet_PlayerControllerFromID(
        const int32 ControllerID) const
    -> APlayerController*
{
    if (ControllerID == _InvalidPlayerControllerID)
    { return {}; }

    for (auto Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        if (auto* PlayerController = Iterator->Get();
            DoGet_PlayerControllerID(PlayerController) == ControllerID)
        {
            return PlayerController;
        }
    }

    return {};
}

// --------------------------------------------------------------------------------------------------------------------
