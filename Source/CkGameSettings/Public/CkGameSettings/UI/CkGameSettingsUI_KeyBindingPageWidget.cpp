#include "CkGameSettingsUI_KeyBindingPageWidget.h"

#include "CkGameSettings/CkGameSettings_Log.h"
#include "CkGameSettings/UI/CkGameSettingsUI_RowWidgets.h"

#include "CkInput/CkKeyIcon_Utils.h"

#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"

#include <CommonButtonBase.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_game_settings_ui_key_binding_page_widget
{
    constexpr auto AnalogCaptureThreshold = 0.5f;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSettingsUI_KeyBindingRowWidget::
    InjectMapping(
        APlayerController* InPlayerController,
        FName InMappingName,
        EPlayerMappableKeySlot InSlot,
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
    _Slot = InSlot;

    if (ck::IsValid(_NameText))
    { _NameText->SetText(InDisplayName); }

    ck::game_settings_ui::BindClick(_KeyButton, this, &UCk_GameSettingsUI_KeyBindingRowWidget::HandleKeyClicked);

    ck::game_settings_ui::BindClick(_ResetButton, this, &UCk_GameSettingsUI_KeyBindingRowWidget::HandleResetClicked);

    const auto PlayerControllerIsUsable = ck::IsValid(InPlayerController);
    CK_ENSURE_IF_NOT(PlayerControllerIsUsable, TEXT("Invalid PlayerController passed to KeyBinding row [{}] for mapping [{}]"), this, InMappingName)
    {}
    if (NOT PlayerControllerIsUsable)
    { return; }

    auto OnChanged = FCk_OnMappingKeyChanged{};
    OnChanged.BindDynamic(this, &UCk_GameSettingsUI_KeyBindingRowWidget::HandleMappingKeyChanged);

    _ChangeListener = UCk_Utils_KeyBinding_UE::BindTo_OnMappingKeyChanged(InPlayerController, InMappingName, InSlot, OnChanged);
    _ListenerBound = true;

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

    Super::NativeDestruct();
}

auto
    UCk_GameSettingsUI_KeyBindingRowWidget::
    HandleKeyClicked()
    -> void
{
    OnRebindRequested.Broadcast(_MappingName, _Slot);
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
    DoRefreshKeyDisplay()
    -> void
{
    if (NOT _PlayerController.IsValid())
    { return; }

    const auto CurrentKey = UCk_Utils_KeyBinding_UE::Get_KeyForMapping(_PlayerController.Get(), _MappingName, _Slot);
    const auto KeyBrush = UCk_Utils_KeyIcon_UE::Get_BrushForKey(_PlayerController.Get(), CurrentKey);
    const auto BrushHasIcon = ck::IsValid(KeyBrush.GetResourceObject(), ck::IsValid_Policy_NullptrOnly{});

    if (ck::IsValid(_KeyIconImage))
    {
        _KeyIconImage->SetVisibility(BrushHasIcon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

        if (BrushHasIcon)
        { _KeyIconImage->SetBrush(KeyBrush); }
    }

    if (ck::IsValid(_KeyText))
    {
        _KeyText->SetVisibility(BrushHasIcon ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
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

    const auto AllMappings = UCk_Utils_KeyBinding_UE::Get_AllRemappableKeys(PlayerController);

    auto CategoryOrder = TArray<FString>{};
    auto MappingsByCategory = TMap<FString, TArray<FPlayerKeyMapping>>{};

    for (const auto& Mapping : AllMappings)
    {
        const auto Category = Mapping.GetDisplayCategory().ToString();

        if (NOT MappingsByCategory.Contains(Category))
        {
            CategoryOrder.Emplace(Category);
            MappingsByCategory.Emplace(Category);
        }

        MappingsByCategory[Category].Emplace(Mapping);
    }

    for (const auto& Category : CategoryOrder)
    {
        for (const auto& Mapping : MappingsByCategory[Category])
        {
            const auto Row = CreateWidget<UCk_GameSettingsUI_KeyBindingRowWidget>(this, _RowWidgetClass);

            if (ck::Is_NOT_Valid(Row))
            { continue; }

            Row->OnRebindRequested.AddUniqueDynamic(this, &UCk_GameSettingsUI_KeyBindingPageWidget::HandleRebindRequested);

            if (ck::IsValid(_RowContainer))
            { _RowContainer->AddChild(Row); }

            _Rows.Emplace(Row);

            Row->InjectMapping(PlayerController, Mapping.GetMappingName(), Mapping.GetSlot(), Mapping.GetDisplayName());

            OnRowCreated(Row, Mapping.GetDisplayCategory());
        }
    }
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
    const auto PlayerController = GetOwningPlayer();

    if (ck::Is_NOT_Valid(PlayerController))
    { return; }

    UCk_Utils_KeyBinding_UE::ResetAllToDefaults(PlayerController);
    DoSetStatus(FText::FromString(TEXT("All key bindings reset to defaults")));
}

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    HandleSwapClicked()
    -> void
{
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
        _CapturePromptText->SetText(FText::FromString(
            FString::Printf(TEXT("Press a key for [%s] — Esc cancels"), *InMappingName.ToString())));
    }

    if (ck::IsValid(_CaptureOverlay))
    { _CaptureOverlay->SetVisibility(ESlateVisibility::Visible); }

    SetKeyboardFocus();
}

auto
    UCk_GameSettingsUI_KeyBindingPageWidget::
    DoCancelCapture()
    -> void
{
    _CaptureActive = false;

    if (ck::IsValid(_CaptureOverlay))
    { _CaptureOverlay->SetVisibility(ESlateVisibility::Collapsed); }
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

    auto Conflicts = TArray<FCk_KeyBinding_ConflictInfo>{};

    if (UCk_Utils_KeyBinding_UE::Get_HasKeyConflicts(PlayerController, InNewKey, {_PendingMappingName}, Conflicts))
    {
        if (ck::IsValid(_ConflictText) && Conflicts.Num() > 0)
        {
            _ConflictText->SetText(FText::FromString(FString::Printf(TEXT("[%s] is already bound to [%s]"),
                *InNewKey.GetDisplayName().ToString(),
                *Conflicts[0].Get_DisplayName().ToString())));
        }

        DoSetConflictOverlayVisible(true);
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
        DoSetStatus(FText::GetEmpty());
        return;
    }

    DoSetStatus(FText::FromString(FString::Printf(TEXT("Rebind failed: %s"), *InFailureReason.ToStringSimple())));
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
