// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI_Subsystem.h"

#include "Blueprint/UserWidget.h"
#include "CommonInputSubsystem.h"
#include "Framework/Application/SlateApplication.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkCore/Engine/CkGameInstance.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Game/CkGame_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkUI/CkUI_Log.h"
#include "CkUI/CustomWidgets/Watermark/CkWatermark_Widget.h"
#include "CkUI/ScreenFade/CkScreenFade_Widget.h"
#include "CkUI/Settings/CkUI_Settings.h"

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
                if (ck::Is_NOT_Valid(UCk_GameInstance_UE::Get_Instance()))
                { return; }

                const auto& GameInstance = UCk_Utils_Game_UE::Get_GameInstance(nullptr);

                if (ck::Is_NOT_Valid(GameInstance))
                { return; }

                const auto& LocalPlayer = GameInstance->FindLocalPlayerFromControllerId(0);

                if (ck::Is_NOT_Valid(LocalPlayer))
                { return; }

                const auto& UISubsystem = LocalPlayer->GetSubsystem<UCk_UI_Subsystem_UE>();

                CK_ENSURE_IF_NOT(ck::IsValid(UISubsystem), TEXT("Could not retrieve UI Subsystem"))
                { return; }

                UISubsystem->Request_UpdateWatermarkDisplayPolicy(
                    static_cast<ECk_Watermark_DisplayPolicy>(WatermarkDisplayPolicy));
            }));
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Lifecycle
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

#if WITH_EDITOR
    if (FSlateApplication::IsInitialized())
    {
        _ModalDialogTickHandle = FSlateApplication::Get().GetOnModalLoopTickEvent().AddUObject(
            this,
            &ThisClass::DoHandleModalLoopTick);
    }
#endif
}

auto
    UCk_UI_Subsystem_UE::
    Deinitialize()
    -> void
{
    ResumeAllInput();

    if (ck::IsValid(_WatermarkWidget))
    {
        _WatermarkWidget->RemoveFromParent();
        _WatermarkWidget = nullptr;
    }

#if WITH_EDITOR
    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().GetOnModalLoopTickEvent().Remove(_ModalDialogTickHandle);
    }
#endif

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

    const auto* LocalPlayer = InNewPlayerController->GetLocalPlayer();

    if (ck::Is_NOT_Valid(LocalPlayer))
    { return; }

    _WatermarkWidget->AddToViewport(UCk_Utils_UI_Settings_UE::Get_WatermarkWidget_ZOrder());

    auto* ClientGameViewport = LocalPlayer->ViewportClient.Get();

    if (ck::Is_NOT_Valid(ClientGameViewport))
    { return; }

    ClientGameViewport->AddViewportWidgetContent(
        _WatermarkWidget->TakeWidget(),
        UCk_Utils_UI_Settings_UE::Get_WatermarkWidget_ZOrder());
}

// --------------------------------------------------------------------------------------------------------------------
// Input Suspension
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Subsystem_UE::
    SuspendInput(
        FName InReason)
    -> FCk_UI_InputSuspensionToken
{
    const auto* LocalPlayer = GetLocalPlayer();

    if (ck::Is_NOT_Valid(LocalPlayer))
    { return {}; }

    _SuspensionIdCounter++;
    const auto SuspendTokenName = DoGenerateSuspendTokenName(InReason);

    DoApplyInputFilter(SuspendTokenName, true);

    const auto& SuspendToken = FCk_UI_InputSuspensionToken::Create(
        _SuspensionIdCounter,
        InReason,
        SuspendTokenName,
        LocalPlayer);

    _ActiveSuspensions.Add(_SuspensionIdCounter, SuspendToken);

    return SuspendToken;
}

auto
    UCk_UI_Subsystem_UE::
    ResumeInput(
        FCk_UI_InputSuspensionToken& InSuspendToken)
    -> void
{
    if (NOT InSuspendToken.IsValid())
    { return; }

    const auto TokenId = InSuspendToken.Get_Id();

    if (NOT _ActiveSuspensions.Contains(TokenId))
    {
        InSuspendToken.DoMarkInvalid();
        return;
    }

    DoApplyInputFilter(InSuspendToken.Get_Token(), false);

    _ActiveSuspensions.Remove(TokenId);
    InSuspendToken.DoMarkInvalid();
}

auto
    UCk_UI_Subsystem_UE::
    ResumeAllInput()
    -> void
{
    for (auto& [Id, Handle] : _ActiveSuspensions)
    {
        DoApplyInputFilter(Handle.Get_Token(), false);
        Handle.DoMarkInvalid();
    }

    _ActiveSuspensions.Empty();
}

auto
    UCk_UI_Subsystem_UE::
    IsInputSuspended() const
    -> bool
{
    return NOT _ActiveSuspensions.IsEmpty();
}

auto
    UCk_UI_Subsystem_UE::
    Get_ActiveSuspensionCount() const
    -> int32
{
    return _ActiveSuspensions.Num();
}

// --------------------------------------------------------------------------------------------------------------------
// Watermark
// --------------------------------------------------------------------------------------------------------------------

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
// Screen Fade
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Subsystem_UE::
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

    TSharedRef<SScreenFade_Widget> FadeWidget = SNew(SScreenFade_Widget)
        ._FadeParams(InFadeParams)
        ._OnFadeFinished(OnFadeFinished);

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
// Internal - Watermark
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Subsystem_UE::
    DoCreateAndSetWatermarkWidget(
        APlayerController* InPlayerController)
    -> void
{
    if (ck::IsValid(_WatermarkWidget))
    { return; }

    const auto WatermarkWidgetClass = UCk_Utils_UI_Settings_UE::Get_WatermarkWidgetClass();

    CK_LOG_ERROR_NOTIFY_IF_NOT(ck::ui, ck::IsValid(WatermarkWidgetClass),
        TEXT("Invalid Watermark Widget setup in the Project Settings!"))
    { return; }

    _WatermarkWidget = Cast<UCk_Watermark_UserWidget_UE>(
        CreateWidget(InPlayerController, WatermarkWidgetClass));

    CK_ENSURE_IF_NOT(ck::IsValid(_WatermarkWidget), TEXT("Failed to create the Watermark Widget!"))
    { return; }

    _WatermarkWidget->Request_SetDisplayPolicy(
        static_cast<ECk_Watermark_DisplayPolicy>(ck_ui::cvar::WatermarkDisplayPolicy));
}

// --------------------------------------------------------------------------------------------------------------------
// Internal - Screen Fade
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Subsystem_UE::
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
// Internal - Input Suspension
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Subsystem_UE::
    DoApplyInputFilter(
        FName InToken,
        bool InShouldFilter) const
    -> void
{
    const auto* LocalPlayer = GetLocalPlayer();

    if (ck::Is_NOT_Valid(LocalPlayer))
    { return; }

    auto* CommonInputSubsystem = UCommonInputSubsystem::Get(LocalPlayer);

    if (ck::Is_NOT_Valid(CommonInputSubsystem))
    { return; }

    CommonInputSubsystem->SetInputTypeFilter(ECommonInputType::MouseAndKeyboard, InToken, InShouldFilter);
    CommonInputSubsystem->SetInputTypeFilter(ECommonInputType::Gamepad, InToken, InShouldFilter);
    CommonInputSubsystem->SetInputTypeFilter(ECommonInputType::Touch, InToken, InShouldFilter);
}

auto
    UCk_UI_Subsystem_UE::
    DoGenerateSuspendTokenName(
        FName InReason) const
    -> FName
{
    auto Token = InReason;
    Token.SetNumber(_SuspensionIdCounter);
    return Token;
}

#if WITH_EDITOR
auto
    UCk_UI_Subsystem_UE::
    DoHandleModalLoopTick(
        float InDeltaTime)
    -> void
{
    // This tick fires continuously while a modal dialog is open.
    // We use it to detect modal entry and schedule restoration on close.

    if (NOT _IsInModalLoop)
    {
        DoSuspendFiltersForModal();
        _IsInModalLoop = true;

        // Schedule a check for when the modal closes.
        // OnModalLoopTickEvent stops firing when modal closes,
        // so we use a deferred callback to restore filters.
        if (auto* World = GetWorld(); ck::IsValid(World))
        {
            World->GetTimerManager().SetTimerForNextTick(
                FTimerDelegate::CreateWeakLambda(this, [this]()
            {
                DoCheckAndRestoreFiltersAfterModal();
            }));
        }
    }
    else
    {
        // Still in modal loop - reschedule the check
        if (auto* World = GetWorld(); ck::IsValid(World))
        {
            World->GetTimerManager().SetTimerForNextTick(
                FTimerDelegate::CreateWeakLambda(this, [this]()
            {
                DoCheckAndRestoreFiltersAfterModal();
            }));
        }
    }
}

auto
    UCk_UI_Subsystem_UE::
    DoCheckAndRestoreFiltersAfterModal()
    -> void
{
    if (NOT _IsInModalLoop)
    { return; }

    // Check if we're still in a modal loop
    if (FSlateApplication::IsInitialized())
    {
        const auto& SlateApp = FSlateApplication::Get();
        const auto ActiveModal = SlateApp.GetActiveModalWindow();

        if (ck::IsValid(ActiveModal))
        {
            // Still have a modal window - stay suspended
            return;
        }
    }

    // Modal is closed - restore filters
    DoRestoreFiltersAfterModal();
}

auto
    UCk_UI_Subsystem_UE::
    DoSuspendFiltersForModal()
    -> void
{
    if (_ActiveSuspensions.IsEmpty())
    { return; }

    _SuspendedTokensDuringModal.Empty();

    for (const auto& [Id, Handle] : _ActiveSuspensions)
    {
        const auto Token = Handle.Get_Token();
        _SuspendedTokensDuringModal.Add(Token);
        DoApplyInputFilter(Token, false);
    }
}

auto
    UCk_UI_Subsystem_UE::
    DoRestoreFiltersAfterModal()
    -> void
{
    for (const auto& Token : _SuspendedTokensDuringModal)
    {
        // Only restore if the suspension is still active
        const auto StillActive = ck::algo::AnyOf(_ActiveSuspensions,
            [&Token](const auto& Pair) { return Pair.Value.Get_Token() == Token; });

        if (StillActive)
        {
            DoApplyInputFilter(Token, true);
        }
    }

    _SuspendedTokensDuringModal.Empty();
    _IsInModalLoop = false;
}
#endif

// --------------------------------------------------------------------------------------------------------------------