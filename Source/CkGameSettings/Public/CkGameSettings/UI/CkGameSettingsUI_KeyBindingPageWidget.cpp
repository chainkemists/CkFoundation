#include "CkGameSettingsUI_KeyBindingPageWidget.h"

#include "CkGameSettings/CkGameSettings_Log.h"
#include "CkGameSettings/UI/CkGameSettingsUI_RowWidgets.h"

#include "CkInput/CkKeyIcon_Utils.h"

#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"

#include <CommonButtonBase.h>
#include <CommonInputSubsystem.h>
#include <CommonInputBaseTypes.h>
#include <Framework/Application/IInputProcessor.h>
#include <Framework/Application/SlateApplication.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_game_settings_ui_key_binding_page_widget
{
    constexpr auto AnalogCaptureThreshold = 0.5f;
}

// --------------------------------------------------------------------------------------------------------------------

/**
 * Captures the NEXT input during a rebind window, regardless of Slate keyboard focus — an
 * activatable settings screen routinely owns focus, and a widget-focus capture also cannot see
 * mouse buttons or the wheel at all (legacy rows bind LMB/RMB/scroll). Registered by the page
 * for exactly the capture window; every handled event is swallowed so the press cannot leak
 * into the UI underneath.
 */
class FCk_GameSettingsUI_KeyCaptureProcessor : public IInputProcessor
{
public:
    explicit FCk_GameSettingsUI_KeyCaptureProcessor(
        UCk_GameSettingsUI_KeyBindingPageWidget* InPage)
        : _Page(InPage)
    {}

    auto Tick(const float InDeltaTime, FSlateApplication& InSlateApp, TSharedRef<ICursor> InCursor) -> void override
    {}

    auto HandleKeyDownEvent(FSlateApplication& InSlateApp, const FKeyEvent& InKeyEvent) -> bool override
    {
        DoDeliver(InKeyEvent.GetKey());
        return true;
    }

    auto HandleMouseButtonDownEvent(FSlateApplication& InSlateApp, const FPointerEvent& InPointerEvent) -> bool override
    {
        DoDeliver(InPointerEvent.GetEffectingButton());
        return true;
    }

    auto HandleMouseWheelOrGestureEvent(FSlateApplication& InSlateApp, const FPointerEvent& InWheelEvent, const FPointerEvent* InGestureEvent) -> bool override
    {
        if (InWheelEvent.GetWheelDelta() == 0.0f)
        { return true; }

        DoDeliver(InWheelEvent.GetWheelDelta() > 0.0f ? EKeys::MouseScrollUp : EKeys::MouseScrollDown);
        return true;
    }

    auto HandleAnalogInputEvent(FSlateApplication& InSlateApp, const FAnalogInputEvent& InAnalogEvent) -> bool override
    {
        using namespace ck_game_settings_ui_key_binding_page_widget;

        if (FMath::Abs(InAnalogEvent.GetAnalogValue()) < AnalogCaptureThreshold)
        { return true; }

        DoDeliver(InAnalogEvent.GetKey());
        return true;
    }

private:
    auto DoDeliver(const FKey& InKey) -> void
    {
        if (auto* Page = _Page.Get();
            ck::IsValid(Page))
        { Page->INTERNAL__HandleCapturedKey(InKey); }
    }

private:
    TWeakObjectPtr<UCk_GameSettingsUI_KeyBindingPageWidget> _Page;
};

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSettingsUI_KeyBindingRowWidget::
    InjectMapping(
        APlayerController* InPlayerController,
        FName InMappingName,
        const FText& InDisplayName)
    -> void
{
    if (_ListenerBound && _PlayerController.IsValid())
    {
        UCk_Utils_KeyBinding_UE::UnbindFrom_OnMappingKeyChanged(_PlayerController.Get(), _ChangeListener);
        _ListenerBound = false;
    }

    _PlayerController = InPlayerController;
    _MappingName = InMappingName;

    if (ck::IsValid(_NameText))
    { _NameText->SetText(InDisplayName); }

    ck::game_settings_ui::BindClick(_KeyButton, this, &UCk_GameSettingsUI_KeyBindingRowWidget::HandleKeyClicked);

    ck::game_settings_ui::BindClick(_ResetButton, this, &UCk_GameSettingsUI_KeyBindingRowWidget::HandleResetClicked);

    const auto PlayerControllerIsUsable = ck::IsValid(InPlayerController);
    CK_ENSURE_IF_NOT(PlayerControllerIsUsable, TEXT("Invalid PlayerController passed to KeyBinding row [{}] for mapping [{}]"), this, InMappingName)
    {}
    if (NOT PlayerControllerIsUsable)
    { return; }

    if (auto* InputSubsystem = UCommonInputSubsystem::Get(InPlayerController->GetLocalPlayer());
        ck::IsValid(InputSubsystem) && NOT _InputMethodChangedHandle.IsValid())
    {
        _InputSubsystem = InputSubsystem;
        _InputMethodChangedHandle = InputSubsystem->OnInputMethodChangedNative.AddUObject(
            this, &UCk_GameSettingsUI_KeyBindingRowWidget::HandleInputMethodChanged);
    }

    DoRefreshKeyDisplay();
}

auto
    UCk_GameSettingsUI_KeyBindingRowWidget::
    NativeDestruct()
    -> void
{
    if (_ListenerBound && _PlayerController.IsValid())
    {
        UCk_Utils_KeyBinding_UE::UnbindFrom_OnMappingKeyChanged(_PlayerController.Get(), _ChangeListener);
        _ListenerBound = false;
    }

    if (_InputMethodChangedHandle.IsValid() && _InputSubsystem.IsValid())
    {
        _InputSubsystem->OnInputMethodChangedNative.Remove(_InputMethodChangedHandle);
        _InputMethodChangedHandle.Reset();
    }

    Super::NativeDestruct();
}

auto
    UCk_GameSettingsUI_KeyBindingRowWidget::
    HandleKeyClicked()
    -> void
{
    OnRebindRequested.Broadcast(_MappingName, _ResolvedSlot);
}

auto
    UCk_GameSettingsUI_KeyBindingRowWidget::
    HandleResetClicked()
    -> void
{
    if (NOT _PlayerController.IsValid())
    { return; }

    UCk_Utils_KeyBinding_UE::ResetMappingToDefault(_PlayerController.Get(), _MappingName);
}

auto
    UCk_GameSettingsUI_KeyBindingRowWidget::
    HandleMappingKeyChanged(
        FName InMappingName,
        FKey InOldKey,
        FKey InNewKey)
    -> void
{
    DoRefreshKeyDisplay();
}

auto
    UCk_GameSettingsUI_KeyBindingRowWidget::
    HandleInputMethodChanged(
        ECommonInputType InNewInputType)
    -> void
{
    DoRefreshKeyDisplay();
}

auto
    UCk_GameSettingsUI_KeyBindingRowWidget::
    Request_SetCaptureDisplay(
        bool InCapturing)
    -> void
{
    if (NOT InCapturing)
    {
        DoRefreshKeyDisplay();
        return;
    }

    if (ck::IsValid(_KeyIconImage))
    { _KeyIconImage->SetVisibility(ESlateVisibility::Collapsed); }

    if (ck::IsValid(_KeyText))
    {
        _KeyText->SetVisibility(ESlateVisibility::HitTestInvisible);
        _KeyText->SetText(NSLOCTEXT("CkGameSettings", "KeyCapturePrompt", "..."));
    }
}

auto
    UCk_GameSettingsUI_KeyBindingRowWidget::
    DoResolveDisplayedSlot()
    -> void
{
    if (NOT _PlayerController.IsValid())
    { return; }

    const auto WantsGamepad = _InputSubsystem.IsValid() &&
        _InputSubsystem->GetCurrentInputType() == ECommonInputType::Gamepad;

    // The DEFAULT key classifies the binding's device — the current key loses that identity the
    // moment the binding is unbound.
    const auto Get_IsGamepadBinding = [](const FPlayerKeyMapping& InMapping)
    {
        const auto& ClassifyingKey = InMapping.GetDefaultKey().IsValid()
            ? InMapping.GetDefaultKey()
            : InMapping.GetCurrentKey();
        return ClassifyingKey.IsGamepadKey();
    };

    auto MatchedSlot = TOptional<EPlayerMappableKeySlot>{};
    auto MatchedUnboundSlot = TOptional<EPlayerMappableKeySlot>{};
    auto FallbackSlot = TOptional<EPlayerMappableKeySlot>{};

    for (const auto& Mapping : UCk_Utils_KeyBinding_UE::Get_AllRemappableKeys(_PlayerController.Get()))
    {
        if (Mapping.GetMappingName() != _MappingName)
        { continue; }

        if (NOT FallbackSlot.IsSet())
        { FallbackSlot = Mapping.GetSlot(); }

        if (Get_IsGamepadBinding(Mapping) != WantsGamepad)
        { continue; }

        if (Mapping.GetCurrentKey().IsValid() && NOT MatchedSlot.IsSet())
        { MatchedSlot = Mapping.GetSlot(); }

        if (NOT MatchedUnboundSlot.IsSet())
        { MatchedUnboundSlot = Mapping.GetSlot(); }
    }

    const auto ResolvedSlot = MatchedSlot.IsSet() ? MatchedSlot
        : MatchedUnboundSlot.IsSet() ? MatchedUnboundSlot
        : FallbackSlot;

    if (NOT ResolvedSlot.IsSet())
    { return; }

    if (ResolvedSlot.GetValue() == _ResolvedSlot && _ListenerBound)
    { return; }

    _ResolvedSlot = ResolvedSlot.GetValue();

    // The change listener follows the displayed binding, so a rebind of THIS slot refreshes the row.
    if (_ListenerBound)
    { UCk_Utils_KeyBinding_UE::UnbindFrom_OnMappingKeyChanged(_PlayerController.Get(), _ChangeListener); }

    auto OnChanged = FCk_OnMappingKeyChanged{};
    OnChanged.BindDynamic(this, &UCk_GameSettingsUI_KeyBindingRowWidget::HandleMappingKeyChanged);

    _ChangeListener = UCk_Utils_KeyBinding_UE::BindTo_OnMappingKeyChanged(_PlayerController.Get(), _MappingName, _ResolvedSlot, OnChanged);
    _ListenerBound = true;
}

auto
    UCk_GameSettingsUI_KeyBindingRowWidget::
    DoRefreshKeyDisplay()
    -> void
{
    if (NOT _PlayerController.IsValid())
    { return; }

    DoResolveDisplayedSlot();

    const auto CurrentKey = UCk_Utils_KeyBinding_UE::Get_KeyForMapping(_PlayerController.Get(), _MappingName, _ResolvedSlot);
    const auto KeyBrush = UCk_Utils_KeyIcon_UE::Get_BrushForKey(_PlayerController.Get(), CurrentKey);
    const auto BrushHasIcon = ck::IsValid(KeyBrush.GetResourceObject(), ck::IsValid_Policy_NullptrOnly{});

    // The glyph wins only when the WBP gave it somewhere to render — a text-only row (no icon
    // slot bound) keeps its text even for keys the platform has a glyph for, or every bound key
    // would collapse the text and display NOTHING.
    const auto ShowIcon = BrushHasIcon && ck::IsValid(_KeyIconImage);

    if (ck::IsValid(_KeyIconImage))
    {
        _KeyIconImage->SetVisibility(ShowIcon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

        if (ShowIcon)
        { _KeyIconImage->SetBrush(KeyBrush); }
    }

    if (ck::IsValid(_KeyText))
    {
        _KeyText->SetVisibility(ShowIcon ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
        _KeyText->SetText(CurrentKey.GetDisplayName());
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    Request_RebuildRows()
    -> void
{
    _Rows.Reset();

    if (ck::IsValid(_RowContainer))
    { _RowContainer->ClearChildren(); }

    const auto PlayerController = GetOwningPlayer();

    if (ck::Is_NOT_Valid(PlayerController))
    { return; }

    const auto RowClassIsUsable = ck::IsValid(_RowWidgetClass.Get());
    CK_ENSURE_IF_NOT(RowClassIsUsable, TEXT("KeyBinding page [{}] has no _RowWidgetClass"), this)
    {}
    if (NOT RowClassIsUsable)
    { return; }

    // ONE row per mapping NAME: the profile stores a mapping per (name, slot) — keyboard and
    // gamepad defaults, plus unbound placeholder slots — and a row per entry renders as duplicate
    // and empty rows. The first entry carries the display metadata; the row itself resolves which
    // slot to show per input method.
    const auto AllMappings = UCk_Utils_KeyBinding_UE::Get_AllRemappableKeys(PlayerController);

    auto NameOrder = TArray<FName>{};
    auto FirstMappingByName = TMap<FName, FPlayerKeyMapping>{};

    for (const auto& Mapping : AllMappings)
    {
        const auto MappingName = Mapping.GetMappingName();

        if (FirstMappingByName.Contains(MappingName))
        { continue; }

        NameOrder.Emplace(MappingName);
        FirstMappingByName.Emplace(MappingName, Mapping);
    }

    auto NamesToShow = TArray<FName>{};

    if (const auto CuratedNames = Get_CuratedMappingNames();
        NOT CuratedNames.IsEmpty())
    {
        for (const auto& CuratedName : CuratedNames)
        {
            CK_ENSURE_IF_NOT(FirstMappingByName.Contains(CuratedName),
                TEXT("KeyBinding page [{}] curates mapping [{}], which the key profile does not know. It was skipped."),
                this, CuratedName)
            { continue; }

            NamesToShow.Emplace(CuratedName);
        }
    }
    else
    {
        // Uncurated: every mapping, grouped by display category in first-seen order
        auto CategoryOrder = TArray<FString>{};
        auto NamesByCategory = TMap<FString, TArray<FName>>{};

        for (const auto& MappingName : NameOrder)
        {
            const auto Category = FirstMappingByName[MappingName].GetDisplayCategory().ToString();

            if (NOT NamesByCategory.Contains(Category))
            {
                CategoryOrder.Emplace(Category);
                NamesByCategory.Emplace(Category);
            }

            NamesByCategory[Category].Emplace(MappingName);
        }

        for (const auto& Category : CategoryOrder)
        { NamesToShow.Append(NamesByCategory[Category]); }
    }

    auto PreviousCategory = TOptional<FString>{};

    for (const auto& MappingName : NamesToShow)
    {
        const auto& Mapping = FirstMappingByName[MappingName];

        if (const auto Category = Mapping.GetDisplayCategory().ToString();
            NOT PreviousCategory.IsSet() || PreviousCategory.GetValue() != Category)
        {
            PreviousCategory = Category;
            OnCategoryHeaderNeeded(Mapping.GetDisplayCategory());
        }

        const auto Row = CreateWidget<UCk_GameSettingsUI_KeyBindingRowWidget>(this, _RowWidgetClass);

        if (ck::Is_NOT_Valid(Row))
        { continue; }

        Row->OnRebindRequested.AddUniqueDynamic(this, &UCk_GameSettingsUI_KeyBindingPageWidget::HandleRebindRequested);

        if (ck::IsValid(_RowContainer))
        { _RowContainer->AddChild(Row); }

        _Rows.Emplace(Row);

        Row->InjectMapping(PlayerController, MappingName, Mapping.GetDisplayName());

        OnRowCreated(Row, Mapping.GetDisplayCategory());
    }
}

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    Get_CuratedMappingNames_Implementation()
    -> TArray<FName>
{
    return {};
}

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    Get_GeneratedRowCount() const
    -> int32
{
    return _Rows.Num();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    NativeConstruct()
    -> void
{
    Super::NativeConstruct();

    SetIsFocusable(true);

    ck::game_settings_ui::BindClick(_ResetAllButton, this, &UCk_GameSettingsUI_KeyBindingPageWidget::HandleResetAllClicked);

    ck::game_settings_ui::BindClick(_SwapButton, this, &UCk_GameSettingsUI_KeyBindingPageWidget::HandleSwapClicked);

    ck::game_settings_ui::BindClick(_OverwriteButton, this, &UCk_GameSettingsUI_KeyBindingPageWidget::HandleOverwriteClicked);

    ck::game_settings_ui::BindClick(_ConflictCancelButton, this, &UCk_GameSettingsUI_KeyBindingPageWidget::HandleConflictCancelClicked);

    if (ck::IsValid(_CaptureOverlay))
    { _CaptureOverlay->SetVisibility(ESlateVisibility::Collapsed); }

    if (ck::IsValid(_ConflictOverlay))
    { _ConflictOverlay->SetVisibility(ESlateVisibility::Collapsed); }

    Request_RebuildRows();
}

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    NativeDestruct()
    -> void
{
    if (_CaptureProcessor.IsValid() && FSlateApplication::IsInitialized())
    { FSlateApplication::Get().UnregisterInputPreProcessor(_CaptureProcessor); }

    if (const auto PlayerController = GetOwningPlayer();
        ck::IsValid(PlayerController))
    {
        UCk_Utils_KeyBinding_UE::SaveKeyBindings(PlayerController);
    }

    Super::NativeDestruct();
}

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    NativeOnKeyDown(
        const FGeometry& InGeometry,
        const FKeyEvent& InKeyEvent)
    -> FReply
{
    if (NOT _CaptureActive)
    { return Super::NativeOnKeyDown(InGeometry, InKeyEvent); }

    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        DoCancelCapture();
        return FReply::Handled();
    }

    DoAttemptRebind(InKeyEvent.GetKey());
    return FReply::Handled();
}

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    NativeOnAnalogValueChanged(
        const FGeometry& InGeometry,
        const FAnalogInputEvent& InAnalogEvent)
    -> FReply
{
    using namespace ck_game_settings_ui_key_binding_page_widget;

    if (NOT _CaptureActive)
    { return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent); }

    if (FMath::Abs(InAnalogEvent.GetAnalogValue()) < AnalogCaptureThreshold)
    { return FReply::Handled(); }

    DoAttemptRebind(InAnalogEvent.GetKey());
    return FReply::Handled();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    HandleRebindRequested(
        FName InMappingName,
        EPlayerMappableKeySlot InSlot)
    -> void
{
    DoBeginCapture(InMappingName, InSlot);
}

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    HandleResetAllClicked()
    -> void
{
    if (_ConfirmResetAll)
    {
        _ResetAllPending = true;
        OnResetAllRequested();
        return;
    }

    DoResetAllNow();
}

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    Request_ConfirmResetAll()
    -> void
{
    CK_ENSURE_IF_NOT(_ResetAllPending, TEXT("Request_ConfirmResetAll on [{}] with no pending reset"), this)
    { return; }

    _ResetAllPending = false;
    DoResetAllNow();
}

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    Request_CancelResetAll()
    -> void
{
    const auto ResetIsPending = _ResetAllPending;
    CK_ENSURE_IF_NOT(ResetIsPending, TEXT("Request_CancelResetAll on [{}] with no pending reset"), this)
    {}
    if (NOT ResetIsPending)
    { return; }

    _ResetAllPending = false;
}

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    DoResetAllNow()
    -> void
{
    const auto PlayerController = GetOwningPlayer();

    if (ck::Is_NOT_Valid(PlayerController))
    { return; }

    UCk_Utils_KeyBinding_UE::ResetAllToDefaults(PlayerController);
    DoSetStatus(FText::FromString(TEXT("All key bindings reset to defaults")));
}

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    Request_ResolveConflict(
        ECk_GameSettingsUI_ConflictResolution InResolution)
    -> void
{
    const auto ConflictIsPending = _ConflictPending;
    CK_ENSURE_IF_NOT(ConflictIsPending, TEXT("Request_ResolveConflict on [{}] with no pending conflict"), this)
    {}
    if (NOT ConflictIsPending)
    { return; }

    switch (InResolution)
    {
        case ECk_GameSettingsUI_ConflictResolution::Swap:      { HandleSwapClicked(); return; }
        case ECk_GameSettingsUI_ConflictResolution::Overwrite: { HandleOverwriteClicked(); return; }
        case ECk_GameSettingsUI_ConflictResolution::Cancel:    { HandleConflictCancelClicked(); return; }
    }
}

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    HandleSwapClicked()
    -> void
{
    _ConflictPending = false;

    const auto PlayerController = GetOwningPlayer();

    if (ck::Is_NOT_Valid(PlayerController))
    { return; }

    auto FailureReason = FGameplayTagContainer{};
    const auto Succeeded = UCk_Utils_KeyBinding_UE::SwapKeys(PlayerController, _PendingMappingName, _PendingSlot, _PendingKey, FailureReason);

    DoSetConflictOverlayVisible(false);
    DoFinishRebind(Succeeded, FailureReason);
}

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    HandleOverwriteClicked()
    -> void
{
    _ConflictPending = false;

    const auto PlayerController = GetOwningPlayer();

    if (ck::Is_NOT_Valid(PlayerController))
    { return; }

    auto FailureReason = FGameplayTagContainer{};
    const auto Succeeded = UCk_Utils_KeyBinding_UE::UnbindConflictAndRemap(PlayerController, _PendingMappingName, _PendingSlot, _PendingKey, FailureReason);

    DoSetConflictOverlayVisible(false);
    DoFinishRebind(Succeeded, FailureReason);
}

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    HandleConflictCancelClicked()
    -> void
{
    _ConflictPending = false;

    DoSetConflictOverlayVisible(false);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    DoBeginCapture(
        FName InMappingName,
        EPlayerMappableKeySlot InSlot)
    -> void
{
    _PendingMappingName = InMappingName;
    _PendingSlot = InSlot;
    _CaptureActive = true;

    if (ck::IsValid(_CapturePromptText))
    {
        _CapturePromptText->SetText(FText::Format(
            NSLOCTEXT("CkGameSettings", "KeyCapturePromptFormat", "Press a key for {0} — Esc cancels"),
            DoGet_MappingDisplayName(InMappingName)));
    }

    if (ck::IsValid(_CaptureOverlay))
    { _CaptureOverlay->SetVisibility(ESlateVisibility::Visible); }

    DoSetPendingRowCaptureDisplay(true);

    if (FSlateApplication::IsInitialized())
    {
        if (NOT _CaptureProcessor.IsValid())
        { _CaptureProcessor = MakeShared<FCk_GameSettingsUI_KeyCaptureProcessor>(this); }

        FSlateApplication::Get().RegisterInputPreProcessor(_CaptureProcessor);
    }

    SetKeyboardFocus();
}

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    INTERNAL__HandleCapturedKey(
        const FKey& InKey)
    -> void
{
    if (NOT _CaptureActive)
    { return; }

    if (InKey == EKeys::Escape)
    {
        DoCancelCapture();
        return;
    }

    DoAttemptRebind(InKey);
}

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    DoCancelCapture()
    -> void
{
    _CaptureActive = false;

    if (_CaptureProcessor.IsValid() && FSlateApplication::IsInitialized())
    { FSlateApplication::Get().UnregisterInputPreProcessor(_CaptureProcessor); }

    DoSetPendingRowCaptureDisplay(false);

    if (ck::IsValid(_CaptureOverlay))
    { _CaptureOverlay->SetVisibility(ESlateVisibility::Collapsed); }
}

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    DoSetPendingRowCaptureDisplay(
        bool InCapturing)
    -> void
{
    for (const auto& Row : _Rows)
    {
        if (ck::IsValid(Row) && Row->Get_MappingName() == _PendingMappingName)
        { Row->Request_SetCaptureDisplay(InCapturing); }
    }
}

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    DoGet_MappingDisplayName(
        FName InMappingName) const
    -> FText
{
    if (const auto PlayerController = GetOwningPlayer();
        ck::IsValid(PlayerController))
    {
        for (const auto& Mapping : UCk_Utils_KeyBinding_UE::Get_AllRemappableKeys(PlayerController))
        {
            if (Mapping.GetMappingName() == InMappingName && NOT Mapping.GetDisplayName().IsEmpty())
            { return Mapping.GetDisplayName(); }
        }
    }

    return FText::FromName(InMappingName);
}

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    DoAttemptRebind(
        const FKey& InNewKey)
    -> void
{
    DoCancelCapture();

    const auto PlayerController = GetOwningPlayer();

    if (ck::Is_NOT_Valid(PlayerController))
    { return; }

    _PendingKey = InNewKey;

    auto ExcludedNames = TArray<FName>{_PendingMappingName};
    ExcludedNames.Append(Get_LinkedMappingNames(_PendingMappingName));

    auto Conflicts = TArray<FCk_KeyBinding_ConflictInfo>{};

    if (UCk_Utils_KeyBinding_UE::Get_HasKeyConflicts(PlayerController, InNewKey, ExcludedNames, Conflicts, _ConflictScope))
    {
        const auto Description = Conflicts.Num() > 0
            ? FText::FromString(FString::Printf(TEXT("[%s] is already bound to [%s]"),
                *InNewKey.GetDisplayName().ToString(),
                *Conflicts[0].Get_DisplayName().ToString()))
            : FText::GetEmpty();

        if (ck::IsValid(_ConflictText))
        { _ConflictText->SetText(Description); }

        _ConflictPending = true;

        if (ck::IsValid(_ConflictOverlay))
        { DoSetConflictOverlayVisible(true); }
        else
        { OnConflictDetected(Description); }

        return;
    }

    auto FailureReason = FGameplayTagContainer{};
    const auto Succeeded = UCk_Utils_KeyBinding_UE::RemapKey(PlayerController, _PendingMappingName, _PendingSlot, InNewKey, FailureReason);

    DoFinishRebind(Succeeded, FailureReason);
}

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    DoFinishRebind(
        bool InSucceeded,
        const FGameplayTagContainer& InFailureReason)
    -> void
{
    if (InSucceeded)
    {
        // Linked followers ride along — covers the plain, swap and overwrite paths alike.
        if (const auto PlayerController = GetOwningPlayer();
            ck::IsValid(PlayerController))
        {
            for (const auto& LinkedName : Get_LinkedMappingNames(_PendingMappingName))
            {
                auto LinkedFailure = FGameplayTagContainer{};
                UCk_Utils_KeyBinding_UE::RemapKey(PlayerController, LinkedName, _PendingSlot, _PendingKey, LinkedFailure);
            }
        }

        DoSetStatus(FText::GetEmpty());
        return;
    }

    DoSetStatus(FText::FromString(FString::Printf(TEXT("Rebind failed: %s"), *InFailureReason.ToStringSimple())));
}

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    Get_LinkedMappingNames_Implementation(
        FName InMappingName)
    -> TArray<FName>
{
    return {};
}

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    DoSetStatus(
        const FText& InStatus)
    -> void
{
    if (ck::IsValid(_StatusText))
    { _StatusText->SetText(InStatus); }
}

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    DoSetConflictOverlayVisible(
        bool InVisible)
    -> void
{
    if (ck::IsValid(_ConflictOverlay))
    { _ConflictOverlay->SetVisibility(InVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed); }
}

// --------------------------------------------------------------------------------------------------------------------
