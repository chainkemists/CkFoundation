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

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    const auto& WatermarkWidgetClass = UCk_Utils_UI_ProjectSettings_UE::Get_WatermarkWidgetClass();

    CK_ENSURE_IF_NOT(ck::IsValid(WatermarkWidgetClass), TEXT("Invalid Watermark Widget setup in the Project Settings!"))
    { return; }

    const auto& GameInstance = UCk_Utils_Game_UE::Get_GameInstance(this);
    _WatermarkWidget = Cast<UCk_Watermark_UserWidget_UE>(CreateWidget(GameInstance, WatermarkWidgetClass));

    CK_ENSURE_IF_NOT(ck::IsValid(_WatermarkWidget), TEXT("Failed to create the Watermark Widget!"))
    { return; }

    _WatermarkWidget->Request_SetDisplayPolicy(static_cast<ECk_Watermark_DisplayPolicy>(ck_ui::cvar::WatermarkDisplayPolicy));

    const auto& GameSessionSubsystem = GameInstance->GetSubsystem<UCk_GameSession_Subsystem_UE>();

    CK_ENSURE_IF_NOT(ck::IsValid(GameSessionSubsystem), TEXT("Failed to retrive the GameSession Subsystem!"))
    { return; }

    ck::UUtils_Signal_OnLoginEvent_PostFireUnbind::Bind<&UCk_UI_Subsystem_UE::OnPlayerControllerReady>
    (
        this,
        GameSessionSubsystem->Get_SignalHandle(),
        ECk_Signal_BindingPolicy::FireIfPayloadInFlight,
        ECk_Signal_PostFireBehavior::Unbind
    );
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
    OnPlayerControllerReady(
        TWeakObjectPtr<APlayerController> InNewPlayerController,
        TArray<TWeakObjectPtr<APlayerController>> InAllPlayerControllers) const
    -> void
{
    if (ck::Is_NOT_Valid(_WatermarkWidget))
    { return; }

    _WatermarkWidget->AddToViewport(INT32_MAX);
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

// --------------------------------------------------------------------------------------------------------------------
