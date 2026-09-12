#include "CkSlateLayout/SCkUiMenuButton.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/AppStyle.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

namespace ck_ui_menu_button
{
    const FLinearColor DefaultBackground(0x1A / 255.0f, 0x2A / 255.0f, 0x38 / 255.0f, 1.0f);
    const FLinearColor DefaultBorder(0x31 / 255.0f, 0x49 / 255.0f, 0x5B / 255.0f, 1.0f);
    const FLinearColor DefaultHoverBackground(0x22 / 255.0f, 0x37 / 255.0f, 0x46 / 255.0f, 1.0f);
    const FLinearColor DefaultHoverBorder(0x48 / 255.0f, 0x70 / 255.0f, 0x84 / 255.0f, 1.0f);
    const FLinearColor DefaultPressedBackground(0x1C / 255.0f, 0x30 / 255.0f, 0x3E / 255.0f, 1.0f);
    const FLinearColor DefaultPressedBorder(0x3D / 255.0f, 0x63 / 255.0f, 0x76 / 255.0f, 1.0f);

    auto ReadColor(const TOptional<FLinearColor>& InValue, const FLinearColor& InDefault) -> FLinearColor { return InValue.Get(InDefault); }
    auto MakeDisabledColor(const FLinearColor& InColor) -> FLinearColor { auto Result = InColor; Result.A *= 0.55f; return Result; }
    auto HasButtonPresentation(const FCkUiMenuButtonVisualStyle& InVisualStyle) -> bool
    {
        return InVisualStyle.Background.IsSet() || InVisualStyle.BorderColor.IsSet() || InVisualStyle.HoverBackground.IsSet() || InVisualStyle.HoverBorderColor.IsSet()
            || InVisualStyle.PressedBackground.IsSet() || InVisualStyle.PressedBorderColor.IsSet() || InVisualStyle.DisabledBackground.IsSet() || InVisualStyle.DisabledBorderColor.IsSet()
            || InVisualStyle.Radius.IsSet() || InVisualStyle.OutlineWidth.IsSet() || InVisualStyle.HasContentPadding;
    }
    auto MakeButtonStyle(const FCkUiMenuButtonVisualStyle& InVisualStyle) -> FButtonStyle
    {
        const FLinearColor Background = ReadColor(InVisualStyle.Background, DefaultBackground);
        const FLinearColor Border = ReadColor(InVisualStyle.BorderColor, DefaultBorder);
        const FLinearColor HoverBackground = ReadColor(InVisualStyle.HoverBackground, DefaultHoverBackground);
        const FLinearColor HoverBorder = ReadColor(InVisualStyle.HoverBorderColor, DefaultHoverBorder);
        const FLinearColor PressedBackground = ReadColor(InVisualStyle.PressedBackground, DefaultPressedBackground);
        const FLinearColor PressedBorder = ReadColor(InVisualStyle.PressedBorderColor, DefaultPressedBorder);
        const FLinearColor DisabledBackground = ReadColor(InVisualStyle.DisabledBackground, MakeDisabledColor(Background));
        const FLinearColor DisabledBorder = ReadColor(InVisualStyle.DisabledBorderColor, MakeDisabledColor(Border));
        const float Radius = InVisualStyle.Radius.Get(4.0f);
        const float OutlineWidth = InVisualStyle.OutlineWidth.Get(1.0f);
        return FButtonStyle().SetNormal(FSlateRoundedBoxBrush(Background, Radius, Border, OutlineWidth))
            .SetHovered(FSlateRoundedBoxBrush(HoverBackground, Radius, HoverBorder, OutlineWidth))
            .SetPressed(FSlateRoundedBoxBrush(PressedBackground, Radius, PressedBorder, OutlineWidth))
            .SetDisabled(FSlateRoundedBoxBrush(DisabledBackground, Radius, DisabledBorder, OutlineWidth))
            .SetNormalPadding(FMargin(0.0f)).SetPressedPadding(FMargin(0.0f));
    }
    auto MakeComboButtonStyle(const FCkUiMenuButtonVisualStyle& InVisualStyle) -> FComboButtonStyle
    {
        FComboButtonStyle Result = FAppStyle::Get().GetWidgetStyle<FComboButtonStyle>(TEXT("ComboButton"));
        if (HasButtonPresentation(InVisualStyle))
        {
            Result.SetButtonStyle(MakeButtonStyle(InVisualStyle));
            Result.SetContentPadding(InVisualStyle.ContentPadding);
        }
        return Result;
    }
}

void SCkUiMenuButton::Construct(const FArguments& InArgs)
{
    _Session = MakeShared<FCkUiMenuSession>();
    _ComboButtonStyle = FAppStyle::Get().GetWidgetStyle<FComboButtonStyle>(TEXT("ComboButton"));
    const TWeakPtr<SCkUiMenuButton> WeakOwner = StaticCastSharedRef<SCkUiMenuButton>(AsShared());
    SComboButton::Construct(SComboButton::FArguments()
        .ComboButtonStyle(&_ComboButtonStyle)
        .OnGetMenuContent_Lambda([WeakOwner]() -> TSharedRef<SWidget>
        {
            const TSharedPtr<SCkUiMenuButton> Owner = WeakOwner.Pin();
            return Owner.IsValid() && Owner->_Session.IsValid() ? Owner->_Session->BuildMenu() : SNullWidget::NullWidget;
        })
        .IsEnabled_Lambda([WeakOwner]()
        {
            const TSharedPtr<SCkUiMenuButton> Owner = WeakOwner.Pin();
            return Owner.IsValid() && Owner->_Session.IsValid() && Owner->_Session->IsEnabled();
        })
        .ButtonContent()
        [
            SNew(STextBlock)
            .Text_Lambda([WeakOwner]()
            {
                const TSharedPtr<SCkUiMenuButton> Owner = WeakOwner.Pin();
                return Owner.IsValid() ? Owner->_Label.Get(FText::GetEmpty()) : FText::GetEmpty();
            })
            .Font_Lambda([WeakOwner]()
            {
                const TSharedPtr<SCkUiMenuButton> Owner = WeakOwner.Pin();
                return Owner.IsValid() ? Owner->_Font : FSlateFontInfo{};
            })
        ]);
}

void SCkUiMenuButton::SetConfiguration(
    TArray<FEntry> InEntries,
    TAttribute<FText> InLabel,
    TAttribute<bool> InEnabled,
    TAttribute<bool> InCanDispatchEvents,
    FSlateFontInfo InFont,
    FCkUiMenuButtonVisualStyle InVisualStyle)
{
    if (!_Session.IsValid()) { _Session = MakeShared<FCkUiMenuSession>(); }
    _Session->Configure(MoveTemp(InEntries), MoveTemp(InEnabled), MoveTemp(InCanDispatchEvents));
    _Label = MoveTemp(InLabel);
    _Font = MoveTemp(InFont);
    _VisualStyle = MoveTemp(InVisualStyle);
    _ComboButtonStyle = ck_ui_menu_button::MakeComboButtonStyle(_VisualStyle);
    SetButtonContentPadding(_ComboButtonStyle.ContentPadding);
    _HasDownArrow = _VisualStyle.HasDownArrow.Get(true);
    SetHasDownArrow(_HasDownArrow);
    Invalidate(EInvalidateWidgetReason::Layout | EInvalidateWidgetReason::Paint);

    // Publication must remain non-eventful. A currently hosted menu holds the old immutable snapshot,
    // so defer its release until Slate's next tick after the caller completes the reload.
    _PendingPopupRelease |= IsOpen();
}

void SCkUiMenuButton::ReleasePopup()
{
    _PendingPopupRelease = false;
    if (_Session.IsValid()) { _Session->ClearOwnedFocus(); }
    if (IsOpen()) { SetIsOpen(false); }
}

void SCkUiMenuButton::Deactivate()
{
    _PendingPopupRelease = false;
    if (_Session.IsValid()) { _Session->Deactivate(); }
    if (IsOpen()) { SetIsOpen(false); }
}

auto SCkUiMenuButton::GetFocusTransferTarget() const -> TSharedPtr<SWidget>
{
    return IsOpen() ? MenuContent : nullptr;
}

void SCkUiMenuButton::Tick(const FGeometry& InAllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    SComboButton::Tick(InAllottedGeometry, InCurrentTime, InDeltaTime);
    if (_PendingPopupRelease)
    {
        ReleasePopup();
    }
    else if (IsOpen() && (!_Session.IsValid() || !_Session->HasVisibleEntries()))
    {
        ReleasePopup();
    }
}
