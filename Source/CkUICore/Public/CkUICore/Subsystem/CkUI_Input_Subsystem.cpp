// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI_Input_Subsystem.h"

#include "CommonInputSubsystem.h"
#include "Framework/Application/SlateApplication.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Handle_InputSuspension::
    Create(
        uint32 InId,
        FName InReason,
        FName InToken,
        const ULocalPlayer* InLocalPlayer)
    -> FCk_Handle_InputSuspension
{
    FCk_Handle_InputSuspension Handle;
    Handle._Id = InId;
    Handle._Reason = InReason;
    Handle._Token = InToken;
    Handle._LocalPlayer = InLocalPlayer;
    return Handle;
}

auto
    FCk_Handle_InputSuspension::
    IsValid() const
    -> bool
{
    return _Id != 0
        && _Token != NAME_None
        && _LocalPlayer.IsValid();
}

auto
    FCk_Handle_InputSuspension::
    Resume()
    -> void
{
    if (NOT IsValid())
    {
        return;
    }

    auto* LocalPlayer = const_cast<ULocalPlayer*>(_LocalPlayer.Get());

    if (ck::Is_NOT_Valid(LocalPlayer))
    {
        return;
    }

    auto* Subsystem = LocalPlayer->GetSubsystem<UCk_UI_Input_Subsystem_UE>();

    if (ck::Is_NOT_Valid(Subsystem))
    {
        return;
    }

    Subsystem->ResumeInput(*this);
}

auto
    FCk_Handle_InputSuspension::
    Get_LocalPlayer() const
    -> const ULocalPlayer*
{
    return _LocalPlayer.Get();
}

auto
    FCk_Handle_InputSuspension::
    DoMarkInvalid()
    -> void
{
    _Id = 0;
    _Token = NAME_None;
    _LocalPlayer.Reset();
}

auto
    FCk_Handle_InputSuspension::
    operator==(const ThisType& Other) const
    -> bool
{
    return _Id == Other._Id && _Token == Other._Token;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Input_Subsystem_UE::
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
    UCk_UI_Input_Subsystem_UE::
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

auto
    UCk_UI_Input_Subsystem_UE::
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
    UCk_UI_Input_Subsystem_UE::
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
    UCk_UI_Input_Subsystem_UE::
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
    UCk_UI_Input_Subsystem_UE::
    IsInputSuspended() const
    -> bool
{
    return NOT _ActiveSuspensions.IsEmpty();
}

auto
    UCk_UI_Input_Subsystem_UE::
    Get_ActiveSuspensionCount() const
    -> int32
{
    return _ActiveSuspensions.Num();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_UI_Input_Subsystem_UE::
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
    UCk_UI_Input_Subsystem_UE::
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
    UCk_UI_Input_Subsystem_UE::
    DoHandleModalLoopTick(
        float InDeltaTime)
    -> void
{
    // OnModalLoopTickEvent fires every tick while a modal is open and simply STOPS when it closes — there
    // is no close event, so every tick re-arms a next-tick poll that notices the close and restores.

    if (NOT _IsInModalLoop)
    {
        DoSuspendFiltersForModal();
        _IsInModalLoop = true;

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
    UCk_UI_Input_Subsystem_UE::
    DoCheckAndRestoreFiltersAfterModal()
    -> void
{
    if (NOT _IsInModalLoop)
    { return; }

    if (FSlateApplication::IsInitialized())
    {
        const auto& SlateApp = FSlateApplication::Get();
        const auto ActiveModal = SlateApp.GetActiveModalWindow();

        if (ck::IsValid(ActiveModal))
        {
            return;
        }
    }

    DoRestoreFiltersAfterModal();
}

auto
    UCk_UI_Input_Subsystem_UE::
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
    UCk_UI_Input_Subsystem_UE::
    DoRestoreFiltersAfterModal()
    -> void
{
    for (const auto& Token : _SuspendedTokensDuringModal)
    {
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
