#include "CkUI_Subsystem.h"

#include "Blueprint/UserWidget.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Game/CkGame_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkGameSession/Subsystem/CkGameSession_Subsystem.h"
#include "CkSignal/Public/CkSignal/CkSignal_Fragment_Data.h"

#include "CkUI/CustomWidgets/Watermark/CkWatermark_Widget.h"

#include "CkUI/Settings/CkUI_Settings.h"

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

                const auto& UISubsystem = GameInstance->GetSubsystem<UCk_UI_Subsystem_UE>();

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

    const auto& GameSessionSubsystem = InCollection.InitializeDependency<UCk_GameSession_Subsystem_UE>();

    CK_ENSURE_IF_NOT(ck::IsValid(GameSessionSubsystem), TEXT("Failed to retrieve the GameSession Subsystem!"))
    { return; }

    _PostFireUnbind_Connection = ck::UUtils_Signal_OnLoginEvent::Bind<&UCk_UI_Subsystem_UE::OnPlayerControllerReady>
    (
        this,
        GameSessionSubsystem->Get_SignalHandle(),
        ECk_Signal_BindingPolicy::FireIfPayloadInFlight,
        ECk_Signal_PostFireBehavior::DoNothing
    );
}

auto
    UCk_UI_Subsystem_UE::
    Deinitialize()
    -> void
{
    _PostFireUnbind_Connection.release();

    if (ck::IsValid(_WatermarkWidget))
    {
        _WatermarkWidget->RemoveFromParent();
        _WatermarkWidget = nullptr;
    }

    Super::Deinitialize();
}

auto
    UCk_UI_Subsystem_UE::
    ShouldCreateSubsystem(
        UObject* InOuter) const
    -> bool
{
    const auto& GameInstance = CastChecked<UGameInstance>(InOuter);
    const auto& IsServerWorld = GameInstance->IsDedicatedServerInstance();

    return NOT IsServerWorld;
}

auto
    UCk_UI_Subsystem_UE::
    OnPlayerControllerReady(
        TWeakObjectPtr<APlayerController> InNewPlayerController,
        TArray<TWeakObjectPtr<APlayerController>> InAllPlayerControllers)
    -> void
{
    if (ck::Is_NOT_Valid(_WatermarkWidget))
    {
        DoCreateAndSetWatermarkWidget();
    }

    if (ck::Is_NOT_Valid(_WatermarkWidget))
    { return; }

    const auto& GameInstance = UCk_Utils_Game_UE::Get_GameInstance(nullptr);

    if (ck::Is_NOT_Valid(GameInstance))
    { return; }

    _WatermarkWidget->AddToViewport();

    auto* ClientGameViewport = GameInstance->GetGameViewportClient();

    if (ck::Is_NOT_Valid(ClientGameViewport))
    { return; }

    ClientGameViewport->AddViewportWidgetContent(_WatermarkWidget->TakeWidget(), UCk_Utils_UI_ProjectSettings_UE::Get_WatermarkWidget_ZOrder());
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
    DoCreateAndSetWatermarkWidget()
    -> void
{
    if (ck::IsValid(_WatermarkWidget))
    { return; }

    const auto& WatermarkWidgetClass = UCk_Utils_UI_ProjectSettings_UE::Get_WatermarkWidgetClass();

    CK_ENSURE_IF_NOT(ck::IsValid(WatermarkWidgetClass), TEXT("Invalid Watermark Widget setup in the Project Settings!"))
    { return; }

    const auto& GameInstance = UCk_Utils_Game_UE::Get_GameInstance(this);
    _WatermarkWidget = Cast<UCk_Watermark_UserWidget_UE>(CreateWidget(GameInstance, WatermarkWidgetClass));

    CK_ENSURE_IF_NOT(ck::IsValid(_WatermarkWidget), TEXT("Failed to create the Watermark Widget!"))
    { return; }

    _WatermarkWidget->Request_SetDisplayPolicy(static_cast<ECk_Watermark_DisplayPolicy>(ck_ui::cvar::WatermarkDisplayPolicy));
}

// --------------------------------------------------------------------------------------------------------------------
