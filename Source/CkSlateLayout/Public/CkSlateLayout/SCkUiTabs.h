#pragma once

#include "CkSlateLayout/CkUiWidgetRegistry.h"

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "Styling/SlateTypes.h"
#include "Widgets/SCompoundWidget.h"

struct FKeyEvent;

/**
 * Retained authored tabs.  Keys, rather than positions, own native header and
 * panel identity; the string model remains the sole authority for selection.
 */
class CKSLATELAYOUT_API SCkUiTabs final : public SCompoundWidget
{
public:
    struct FPanel
    {
        FString Key;
        TAttribute<FText> Label;
        TAttribute<bool> Enabled;
        TAttribute<bool> Visible;
        TSharedPtr<SWidget> Content;
        TFunction<void()> OnDeactivate;
    };

    SLATE_BEGIN_ARGS(SCkUiTabs) {}
    SLATE_END_ARGS()

    SCkUiTabs();
    void Construct(const FArguments& InArgs);
    ~SCkUiTabs() override;

    /**
     * Publication-only configuration update.  It mounts owned staged ancestry
     * immediately, but defers selection visibility and focus reconciliation to Tick.
     */
    void SetConfiguration(TArray<FPanel> InPanels, TAttribute<FString> InValue,
        FCkUiOnStringChanged InChanged, TAttribute<bool> InCanDispatchEvents, FSlateFontInfo InFont,
        FCkUiTabsVisualStyle InVisualStyle);

    /** Makes held widgets inert after their view has released this container. */
    void Deactivate();

    /** True only for this container's keyed native header widgets. */
    bool IsRetainedHeader(const TSharedPtr<SWidget>& InWidget) const;

    virtual void Tick(const FGeometry& InAllottedGeometry, double InCurrentTime, float InDeltaTime) override;
    virtual FReply OnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InEvent) override;

private:
    void ActivateKey(const FString& InKey);
    void MoveHeaderFocus(const FString& InKey, int32 InDirection, bool bToBoundary, int32 InUserIndex);
    bool IsKeyEnabled(const FString& InKey) const;
    bool IsKeyVisible(const FString& InKey) const;
    bool IsKeySelected(const FString& InKey) const;
    void UpdateHeaderAppearance();
    void Reconcile();
    void ReconcileSelectionAndOwnedFocus();
    void ReconcileDisabledHeaderFocus();

    struct FEntry;
    TSharedPtr<class SWrapBox> _Headers;
    TSharedPtr<class SVerticalBox> _Panels;
    TArray<FPanel> _PendingPanels;
    TArray<FEntry> _Entries;
    TAttribute<FString> _Value;
    TAttribute<bool> _CanDispatchEvents;
    FCkUiOnStringChanged _Changed;
    FSlateFontInfo _Font;
    FCkUiTabsVisualStyle _VisualStyle;
    FButtonStyle _CoreHeaderStyle;
    FButtonStyle _FlatHeaderStyle;
    FString _AppliedSelectedKey;
    bool _ReconcilePending = false;
    bool _Active = true;
    bool _Dispatching = false;
    bool _ChangingPresentation = false;
    uint64 _ConfigurationRevision = 0;
};
