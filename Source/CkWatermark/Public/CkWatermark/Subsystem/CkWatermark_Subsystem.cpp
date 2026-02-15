#include "CkWatermark_Subsystem.h"

#include "CkCore/Engine/CkGameInstance.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Game/CkGame_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkWatermark/Settings/CkWatermark_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_watermark
{
    namespace cvar
    {
#if CK_BUILD_SHIPPING
        static auto WatermarkDisplayPolicy = static_cast<int32>(ECk_Watermark_DisplayPolicy::Hidden);
#else
        static auto WatermarkDisplayPolicy = static_cast<int32>(ECk_Watermark_DisplayPolicy::Regular);
#endif
        static auto CVar_WatermarkDisplayPolicy = FAutoConsoleVariableRef(
            TEXT("ck.UI.WatermarkDisplayPolicy"),
            WatermarkDisplayPolicy,
            TEXT("Set the Watermark Widget Display Policy (Hidden, Regular, Detailed)"),
            FConsoleVariableDelegate::CreateLambda([](IConsoleVariable* CVar)
            {
                if (ck::Is_NOT_Valid(UCk_GameInstance_UE::Get_Instance()))
                { return; }

                const auto& GameInstance = UCk_Utils_Game_UE::Get_GameInstance(nullptr);

                if (ck::Is_NOT_Valid(GameInstance))
                { return; }

                const auto& LocalPlayer = GameInstance->FindLocalPlayerFromControllerId(0);

                if (ck::Is_NOT_Valid(LocalPlayer))
                { return; }

                const auto& WatermarkSubsystem = LocalPlayer->GetSubsystem<UCk_Watermark_Subsystem_UE>();

                CK_ENSURE_IF_NOT(ck::IsValid(WatermarkSubsystem), TEXT("Could not retrieve Watermark Subsystem"))
                { return; }

                WatermarkSubsystem->Request_UpdateWatermarkDisplayPolicy(static_cast<ECk_Watermark_DisplayPolicy>(WatermarkDisplayPolicy));
            }));
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Watermark_Subsystem_UE::
    Deinitialize()
    -> void
{
    if (ck::IsValid(_WatermarkWidget))
    {
        if (const auto CachedSlateWidget = _WatermarkWidget->GetCachedWidget(); CachedSlateWidget.IsValid())
        {
            if (const auto* LocalPlayer = GetLocalPlayer();
                ck::IsValid(LocalPlayer) && ck::IsValid(LocalPlayer->ViewportClient))
            {
                LocalPlayer->ViewportClient->RemoveViewportWidgetContent(CachedSlateWidget.ToSharedRef());
            }
        }

        _WatermarkWidget = nullptr;
    }

    Super::Deinitialize();
}

auto
    UCk_Watermark_Subsystem_UE::
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

    const auto& ClientGameViewport = LocalPlayer->ViewportClient;

    if (ck::Is_NOT_Valid(ClientGameViewport))
    { return; }

    ClientGameViewport->AddViewportWidgetContent(_WatermarkWidget->TakeWidget(), UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_Widget_ZOrder());
}

auto
    UCk_Watermark_Subsystem_UE::
    Request_UpdateWatermarkDisplayPolicy(
        ECk_Watermark_DisplayPolicy InDisplayPolicy) const
    -> void
{
    if (ck::Is_NOT_Valid(_WatermarkWidget))
    { return; }

    _WatermarkWidget->Request_SetDisplayPolicy(InDisplayPolicy);
}

auto
    UCk_Watermark_Subsystem_UE::
    DoCreateAndSetWatermarkWidget(
        APlayerController* InPlayerController)
    -> void
{
    if (ck::IsValid(_WatermarkWidget))
    { return; }

    _WatermarkWidget = NewObject<UCkWatermark_Panel_UWidget_UE>(InPlayerController);

    CK_ENSURE_IF_NOT(ck::IsValid(_WatermarkWidget), TEXT("Failed to create the Watermark Panel Widget!"))
    { return; }

    _WatermarkWidget->Request_SetDisplayPolicy(static_cast<ECk_Watermark_DisplayPolicy>(ck_watermark::cvar::WatermarkDisplayPolicy));
}

// --------------------------------------------------------------------------------------------------------------------
