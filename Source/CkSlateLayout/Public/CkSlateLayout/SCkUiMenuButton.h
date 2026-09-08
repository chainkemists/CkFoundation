#pragma once

#include "CoreMinimal.h"
#include "CkSlateLayout/CkUiMenuSession.h"
#include "Widgets/Input/SComboButton.h"

/** Shared native Slate host for authored button menus. */
class CKSLATELAYOUT_API SCkUiMenuButton final : public SComboButton
{
public:
    using FEntry = FCkUiMenuSession::FEntry;

    SLATE_BEGIN_ARGS(SCkUiMenuButton) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    /** Publishes a new menu definition without opening, closing, or moving focus during the caller's reload. */
    void SetConfiguration(
        TArray<FEntry> InEntries,
        TAttribute<FText> InLabel,
        TAttribute<bool> InEnabled,
        TAttribute<bool> InCanDispatchEvents,
        FSlateFontInfo InFont);

    /** Closes the owned popup after clearing only focus paths within its menu content. */
    void ReleasePopup();
    void Deactivate();
    auto GetFocusTransferTarget() const -> TSharedPtr<SWidget>;

    virtual void Tick(const FGeometry& InAllottedGeometry, double InCurrentTime, float InDeltaTime) override;

private:
    TSharedPtr<FCkUiMenuSession> _Session;
    TAttribute<FText> _Label;
    FSlateFontInfo _Font;
    bool _PendingPopupRelease = false;
};
