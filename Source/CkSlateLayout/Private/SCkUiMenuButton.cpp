#include "CkSlateLayout/SCkUiMenuButton.h"

#include "Styling/AppStyle.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"

void SCkUiMenuButton::Construct(const FArguments& InArgs)
{
    _Session = MakeShared<FCkUiMenuSession>();
    const TWeakPtr<SCkUiMenuButton> WeakOwner = StaticCastSharedRef<SCkUiMenuButton>(AsShared());
    SComboButton::Construct(SComboButton::FArguments()
        .ComboButtonStyle(&FAppStyle::Get().GetWidgetStyle<FComboButtonStyle>(TEXT("ComboButton")))
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
    FSlateFontInfo InFont)
{
    if (!_Session.IsValid()) { _Session = MakeShared<FCkUiMenuSession>(); }
    _Session->Configure(MoveTemp(InEntries), MoveTemp(InEnabled), MoveTemp(InCanDispatchEvents));
    _Label = MoveTemp(InLabel);
    _Font = MoveTemp(InFont);

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
