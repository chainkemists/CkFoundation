#include "CkUI_Subsystem.h"

#include "Blueprint/UserWidget.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Game/CkGame_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkUI/CkUI_Log.h"
#include "CkUI/CustomWidgets/Watermark/CkWatermark_Widget.h"

#include "CkUI/Settings/CkUI_Settings.h"
#include "CkUI/WidgetLayerHandler/CkWidgetLayerHandler_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_ui
{
    namespace cvar
    {
        static int WatermarkDisplayPolicy = static_cast<int32>(ECk_Watermark_DisplayPolicy::Regular);
        static FAutoConsoleVariableRef CVar_WatermarkDisplayPolicy(
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

                CK_ENSURE_IF_NOT(ck::IsValid(UISubsystem), TEXT("Could not retrive UI Subsystem"))
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

    _WatermarkWidget->AddToViewport();

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
