#pragma once

#include "CkSlateLayout/CkUiCollection.h"
#include "CkSlateLayout/CkUiContextMenu.h"
#include "CkSlateLayout/CkUiWidgetRegistry.h"
#include "Fonts/SlateFontInfo.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class FCkUiView;

DECLARE_DELEGATE_TwoParams(FOnCkUiTableSelectionChanged, TOptional<FString>, ESelectInfo::Type);

/** Virtualized native table whose read-only cells use FCkUiView. */
class CKSLATELAYOUT_API SCkUiTable final : public SCompoundWidget
{
public:
    using FRecord = TSharedPtr<const FCkUiRecord>;
    using FCellFactory = TFunction<TSharedPtr<FCkUiView>(const FCkUiNode&, TWeakPtr<const FCkUiRecord>, FString&)>;

    SLATE_BEGIN_ARGS(SCkUiTable) {}
        SLATE_ARGUMENT(TSharedPtr<FCkUiCollection>, Collection)
        SLATE_ARGUMENT(FSlateFontInfo, BaseFont)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    virtual ~SCkUiTable() override;
    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

    /** Prepare detached cell views; Commit publishes only after the outer document succeeds. */
    auto Prepare(const FCkUiNode& InDefinition, FCellFactory InFactory, TAttribute<FText> InFilter,
        FOnCkUiTableSelectionChanged InSelectionChanged, FString& OutFailure, FOnContextMenuOpening InContextMenu = {},
        FOnCkUiContextMenuOpening InAuthoredContextMenu = {},
        const TMap<FString, TAttribute<FText>>& InTextBindings = {}) -> TUniquePtr<ICkUiPreparedWidgetUpdate>;
    void ReleaseContextMenu();
    auto TrySelectKey(TOptional<FString> InKey, bool InNotify = false) -> bool;
    auto TryRefresh() -> bool;
    auto GetSelectedKey() const -> TOptional<FString>;
    auto GetVisibleRecordCount() const -> int32;
    auto GetLiveRowCount() const -> int32;
    auto GetList() const -> TSharedPtr<SListView<FRecord>>;
    const FString& GetLastCellError() const;

private:
    struct FImpl;
    TSharedPtr<FImpl> _Impl;
};
