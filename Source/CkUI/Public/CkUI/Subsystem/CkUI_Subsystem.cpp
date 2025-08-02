#include "CkUI_Subsystem.h"

#include "Blueprint/UserWidget.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Game/CkGame_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkUI/CkUI_Log.h"
#include "CkUI/CustomWidgets/Watermark/CkWatermark_Widget.h"
#include "CkUI/ScreenFade/CkScreenFade_Widget.h"

#include "CkUI/Settings/CkUI_Settings.h"
#include "CkUI/WidgetLayerHandler/CkWidgetLayerHandler_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_ui
{
    namespace cvar
    {
        static auto WatermarkDisplayPolicy = static_cast<int32>(ECk_Watermark_DisplayPolicy::Regular);
        static auto CVar_WatermarkDisplayPolicy = FAutoConsoleVariableRef(
            TEXT("ck.UI.WatermarkDisplayPolicy"),
            WatermarkDisplayPolicy,
            TEXT("Set the Watermark Widget Display Policy (Hidden, Regular, Detailed)"),
            FConsoleVariableDelegate::CreateLambda([](IConsoleVariable* CVar)
            {
                const auto& GameInstance = UCk_Utils_Game_UE::Get_GameInstance(nullptr);

                if (ck::Is_NOT_Valid(GameInstance))
                { return; }

                const auto& LocalPlayer = GameInstance->FindLocalPlayerFromControllerId(0);

                if (ck::Is_NOT_Valid(LocalPlayer))
                { return; }

                const auto& UISubsystem = LocalPlayer->GetSubsystem<UCk_UI_Subsystem_UE>();

                CK_ENSURE_IF_NOT(ck::IsValid(UISubsystem), TEXT("Could not retrieve UI Subsystem"))
                { return; }

                UISubsystem->Request_UpdateWatermarkDisplayPolicy(static_cast<ECk_Watermark_DisplayPolicy>(WatermarkDisplayPolicy));
            }));
    }
}

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
    if (ck::IsValid(_WatermarkWidget))
    {
        _WatermarkWidget->RemoveFromParent();
        _WatermarkWidget = nullptr;
    }

    Super::Deinitialize();
}

auto
    UCk_UI_Subsystem_UE::
    PlayerControllerChanged(
        APlayerController* InNewPlayerController)
    -> void
{
    if (ck::Is_NOT_Valid(InNewPlayerController))
    { return; }

    if (ck::Is_NOT_Valid(_WatermarkWidget))
    {
        DoCreateAndSetWatermarkWidget(InNewPlayerController);
    }

    if (ck::Is_NOT_Valid(_WatermarkWidget))
    { return; }

    const auto& LocalPlayer = InNewPlayerController->GetLocalPlayer();

    if (ck::Is_NOT_Valid(LocalPlayer))
    { return; }

    _WatermarkWidget->AddToViewport(UCk_Utils_UI_Settings_UE::Get_WatermarkWidget_ZOrder());

    const auto& ClientGameViewport = LocalPlayer->ViewportClient;

    if (ck::Is_NOT_Valid(ClientGameViewport))
    { return; }

    ClientGameViewport->AddViewportWidgetContent(_WatermarkWidget->TakeWidget(), UCk_Utils_UI_Settings_UE::Get_WatermarkWidget_ZOrder());
}

auto
    UCk_UI_Subsystem_UE::
    Request_UpdateWatermarkDisplayPolicy(
        ECk_Watermark_DisplayPolicy InDisplayPolicy) const
    -> void
{
    if (ck::Is_NOT_Valid(_WatermarkWidget))
    { return; }

    _WatermarkWidget->Request_SetDisplayPolicy(InDisplayPolicy);
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

auto
    UCk_UI_Subsystem_UE::
    DoCreateAndSetWatermarkWidget(
        APlayerController* InPlayerController)
    -> void
{
    if (ck::IsValid(_WatermarkWidget))
    { return; }

    const auto& WatermarkWidgetClass = UCk_Utils_UI_Settings_UE::Get_WatermarkWidgetClass();

    CK_LOG_ERROR_NOTIFY_IF_NOT(ck::ui, ck::IsValid(WatermarkWidgetClass), TEXT("Invalid Watermark Widget setup in the Project Settings!"))
    { return; }

    _WatermarkWidget = Cast<UCk_Watermark_UserWidget_UE>(CreateWidget(InPlayerController, WatermarkWidgetClass));

    CK_ENSURE_IF_NOT(ck::IsValid(_WatermarkWidget), TEXT("Failed to create the Watermark Widget!"))
    { return; }

    _WatermarkWidget->Request_SetDisplayPolicy(static_cast<ECk_Watermark_DisplayPolicy>(ck_ui::cvar::WatermarkDisplayPolicy));
}

// --------------------------------------------------------------------------------------------------------------------
