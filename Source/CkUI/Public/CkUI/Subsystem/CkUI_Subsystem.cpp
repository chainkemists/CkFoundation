// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI_Subsystem.h"

#include "CommonInputSubsystem.h"
#include "Framework/Application/SlateApplication.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkUI/ScreenFade/CkScreenFade_Widget.h"

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
#if WITH_EDITOR
    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().GetOnModalLoopTickEvent().Remove(_ModalDialogTickHandle);
    }
#endif

    Super::Deinitialize();
}


// --------------------------------------------------------------------------------------------------------------------
// Input Suspension
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Subsystem_UE::
    SuspendInput(
        FName InReason)
    -> FCk_Handle_InputSuspension
{
    const auto* LocalPlayer = GetLocalPlayer();

    if (ck::Is_NOT_Valid(LocalPlayer))
    { return {}; }

    _SuspensionIdCounter++;
    const auto SuspensionName = DoGenerateSuspensionHandleName(InReason);

    DoApplyInputFilter(SuspensionName, true);

    const auto& SuspensionHandle = FCk_Handle_InputSuspension::Create(
        _SuspensionIdCounter,
        InReason,
        SuspensionName,
        LocalPlayer);

    _ActiveSuspensions.Add(_SuspensionIdCounter, SuspensionHandle);

    return SuspensionHandle;
}

auto
    UCk_UI_Subsystem_UE::
    ResumeInput(
        FCk_Handle_InputSuspension& InSuspensionHandle)
    -> void
{
    if (NOT InSuspensionHandle.IsValid())
    { return; }

    const auto TokenId = InSuspensionHandle.Get_Id();

    if (NOT _ActiveSuspensions.Contains(TokenId))
    {
        InSuspensionHandle.DoMarkInvalid();
        return;
    }

    DoApplyInputFilter(InSuspensionHandle.Get_Token(), false);

    _ActiveSuspensions.Remove(TokenId);
    InSuspensionHandle.DoMarkInvalid();
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
    DoGenerateSuspensionHandleName(
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