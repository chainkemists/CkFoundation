#include "CkInputAction_Widget.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkInput/CkInput_Utils.h"
#include "CkInput/CkKeyBinding_Utils.h"
#include "CkInput/CkKeyIcon_Utils.h"

#include "CkUI/Types/CkUI_Types.h"

#include <CommonInputSubsystem.h>
#include <CommonUITypes.h>
#include <EnhancedInputSubsystems.h>
#include <GameFramework/PlayerController.h>
#include <InputAction.h>
#include <Styling/StyleDefaults.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_input_action_widget
{
    // A miss out of TryGetInputBrush comes back as a default-constructed FSlateBrush, which is an
    // Image brush with no resource — NOT the NoDrawType brush the parent tests for. Treating that
    // as resolved would draw nothing while claiming the action is bound.
    auto Get_IsDrawableBrush(
        const FSlateBrush& InBrush) -> bool
    {
        if (InBrush.GetDrawType() == ESlateBrushDrawType::NoDrawType)
        { return false; }

        return ck::IsValid(InBrush.GetResourceObject()) ||
               InBrush.GetResourceName() != NAME_None;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InputActionWidget_UE::
    Get_ResolvedKey() const
    -> FKey
{
    return _ResolvedKey.Get(FKey{EKeys::Invalid});
}

auto
    UCk_InputActionWidget_UE::
    Get_ResolvedKeyDisplayName() const
    -> FText
{
    const auto Key = Get_ResolvedKey();

    if (NOT Key.IsValid())
    { return FText::GetEmpty(); }

    return Key.GetDisplayName();
}

auto
    UCk_InputActionWidget_UE::
    Get_IsBound() const
    -> bool
{
    return Get_ResolvedKey().IsValid();
}

auto
    UCk_InputActionWidget_UE::
    Request_SetSlot(
        EPlayerMappableKeySlot InSlot)
    -> void
{
    if (_Slot == InSlot)
    { return; }

    _Slot = InSlot;
    UpdateActionWidget();
}

auto
    UCk_InputActionWidget_UE::
    Request_SetUnboundPolicy(
        ECk_InputPrompt_UnboundPolicy InUnboundPolicy)
    -> void
{
    if (_UnboundPolicy == InUnboundPolicy)
    { return; }

    _UnboundPolicy = InUnboundPolicy;
    UpdateActionWidget();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InputActionWidget_UE::
    GetIcon() const
    -> FSlateBrush
{
    // DesignTimeKey preview lives entirely in the parent and has no profile to read.
    if (IsDesignTime())
    { return Super::GetIcon(); }

    if (const auto Key = DoResolveKey_FromKeyProfile();
        Key.IsValid())
    {
        if (auto Brush = UCk_Utils_KeyIcon_UE::Get_BrushForKey(GetOwningPlayer(), Key);
            ck_input_action_widget::Get_IsDrawableBrush(Brush))
        { return Brush; }
    }

    if (auto ParentBrush = Super::GetIcon();
        ck_input_action_widget::Get_IsDrawableBrush(ParentBrush))
    { return ParentBrush; }

    if (_UnboundPolicy == ECk_InputPrompt_UnboundPolicy::ShowUnboundBrush &&
        ck_input_action_widget::Get_IsDrawableBrush(_UnboundBrush))
    { return _UnboundBrush; }

    return *FStyleDefaults::GetNoBrush();
}

auto
    UCk_InputActionWidget_UE::
    UpdateActionWidget()
    -> void
{
    Super::UpdateActionWidget();

    if (NOT IsDesignTime() &&
        _UnboundPolicy == ECk_InputPrompt_UnboundPolicy::KeepVisible &&
        GetVisibility() == ESlateVisibility::Collapsed)
    {
        SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }

    DoRefreshResolvedKey();
}

auto
    UCk_InputActionWidget_UE::
    OnWidgetRebuilt()
    -> void
{
    Super::OnWidgetRebuilt();

    DoListenToKeyProfileChanged(true);
}

auto
    UCk_InputActionWidget_UE::
    ReleaseSlateResources(
        bool bReleaseChildren)
    -> void
{
    DoListenToKeyProfileChanged(false);

    Super::ReleaseSlateResources(bReleaseChildren);
}

#if WITH_EDITOR
auto
    UCk_InputActionWidget_UE::
    GetPaletteCategory()
    -> const FText
{
    return ck::widget_palette_categories::Default;
}
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_InputActionWidget_UE::
    DoResolveKey() const
    -> FKey
{
    if (const auto ProfileKey = DoResolveKey_FromKeyProfile();
        ProfileKey.IsValid())
    { return ProfileKey; }

    const auto* InputAction = GetEnhancedInputAction();

    if (ck::Is_NOT_Valid(InputAction) || NOT CommonUI::IsEnhancedInputSupportEnabled())
    { return FKey{EKeys::Invalid}; }

    const auto* InputSubsystem = GetInputSubsystem();

    if (ck::Is_NOT_Valid(InputSubsystem))
    { return FKey{EKeys::Invalid}; }

    // Deliberately the same call the parent's icon path uses, so the reported key can never
    // disagree with the glyph on screen when the device changes which binding is shown.
    return CommonUI::GetFirstKeyForInputType(
        GetOwningLocalPlayer(),
        InputSubsystem->GetCurrentInputType(),
        InputAction);
}

auto
    UCk_InputActionWidget_UE::
    DoResolveKey_FromKeyProfile() const
    -> FKey
{
    if (_Resolution != ECk_InputPrompt_Resolution::KeyProfileThenAppliedContexts)
    { return FKey{EKeys::Invalid}; }

    const auto* PlayerController = GetOwningPlayer();

    if (ck::Is_NOT_Valid(PlayerController))
    { return FKey{EKeys::Invalid}; }

    const auto* InputAction = GetEnhancedInputAction();

    if (ck::Is_NOT_Valid(InputAction))
    { return FKey{EKeys::Invalid}; }

    return UCk_Utils_KeyBinding_UE::Get_KeyForInputAction(PlayerController, InputAction, _Slot);
}

auto
    UCk_InputActionWidget_UE::
    DoRefreshResolvedKey()
    -> void
{
    const auto NewKey = IsDesignTime() ? FKey{EKeys::Invalid} : DoResolveKey();

    if (_ResolvedKey.IsSet() && _ResolvedKey.GetValue() == NewKey)
    { return; }

    _ResolvedKey = NewKey;
    OnResolvedKeyChanged.Broadcast(NewKey);
}

auto
    UCk_InputActionWidget_UE::
    DoListenToKeyProfileChanged(
        bool InListen)
    -> void
{
    if (const auto BoundSettings = _BoundUserSettings.Get();
        ck::IsValid(BoundSettings))
    {
        BoundSettings->OnSettingsChanged.RemoveDynamic(this, &ThisClass::HandleInputUserSettingsChanged);
    }

    _BoundUserSettings.Reset();

    if (NOT InListen || IsDesignTime())
    { return; }

    const auto* PlayerController = GetOwningPlayer();

    if (ck::Is_NOT_Valid(PlayerController))
    { return; }

    const auto* Subsystem = UCk_Utils_Input_UE::Get_EnhancedInputLocalPlayerSubsystem(PlayerController);

    if (ck::Is_NOT_Valid(Subsystem))
    { return; }

    // The parent already re-resolves on ControlMappingsRebuilt, which covers a rebind while the
    // action's context is applied. This covers the case this widget exists for: a profile change
    // with NO applied context to rebuild — every row of a rebinding screen.
    auto* Settings = Subsystem->GetUserSettings();

    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->OnSettingsChanged.AddUniqueDynamic(this, &ThisClass::HandleInputUserSettingsChanged);
    _BoundUserSettings = Settings;
}

auto
    UCk_InputActionWidget_UE::
    HandleInputUserSettingsChanged(
        UEnhancedInputUserSettings* InSettings)
    -> void
{
    UpdateActionWidget();
}

// --------------------------------------------------------------------------------------------------------------------
