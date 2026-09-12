#include "CkSlateLayout/SCkUiTable.h"

#include "CkUiSelection.h"
#include "CkSlateLayout/CkUiContextMenu.h"
#include "CkSlateLayout/SCkUiSurface.h"
#include "Framework/Application/SlateApplication.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateNoResource.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Views/STableRow.h"

namespace ck_ui_table
{
    using FRecord = SCkUiTable::FRecord;
    using FCellViews = TMap<FName, TSharedPtr<FCkUiView>>;

    struct FColumn
    {
        FName Id;
        TAttribute<FText> Header;
        FString SortField;
        FCkUiNode CellRoot;
        FCkUiStyle HeaderStyle;
        float Width = 160.0f;
        float MinWidth = 0.0f;
        float Grow = 0.0f;
    };

    struct FConfiguration
    {
        TArray<FColumn> Columns;
        bool Selectable = true;
        float RowHeight = 24.0f;
        FCkUiTableVisualStyle VisualStyle;
        TAttribute<FText> Filter;
        SCkUiTable::FCellFactory Factory;
        FOnCkUiTableSelectionChanged OnSelectionChanged;
        FOnContextMenuOpening OnContextMenu;
        FOnCkUiContextMenuOpening OnAuthoredContextMenu;
    };

    class FList final : public SListView<SCkUiTable::FRecord>
    {
    public:
        TFunction<void(SCkUiTable::FRecord, const FPointerEvent&)> OnItemRightClicked;
        TFunction<TOptional<FReply>(const FGeometry&, const FKeyEvent&)> OnContextMenuKey;

        virtual void Private_OnItemRightClicked(SCkUiTable::FRecord InRecord, const FPointerEvent& InMouseEvent) override
        {
            if (OnItemRightClicked) { OnItemRightClicked(MoveTemp(InRecord), InMouseEvent); }
            else { OnRightMouseButtonUp(InMouseEvent); }
        }

        virtual FReply OnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override
        {
            if (OnContextMenuKey)
            {
                const TOptional<FReply> Reply = OnContextMenuKey(InGeometry, InKeyEvent);
                if (Reply.IsSet()) { return Reply.GetValue(); }
            }
            return SListView<SCkUiTable::FRecord>::OnKeyDown(InGeometry, InKeyEvent);
        }

        void OpenLegacyContextMenu(const FPointerEvent& InMouseEvent) { OnRightMouseButtonUp(InMouseEvent); }
    };
}

struct SCkUiTable::FImpl final : public TSharedFromThis<FImpl>
{
    class FRow;
    class FPreparedUpdate;

    struct FCellScope final
    {
        bool Active = true;
    };

    struct FConfigurationToken final
    {
        bool Active = true;
    };

    TSharedPtr<FCkUiCollection> Collection;
    FSlateFontInfo BaseFont;
    TSharedPtr<ck_ui_table::FList> List;
    TSharedPtr<FCkUiContextMenuHost> ContextMenu = MakeShared<FCkUiContextMenuHost>();
    TSharedPtr<SHeaderRow> Header;
    FHeaderRowStyle HeaderStyle;
    FTableRowStyle RowStyle;
    TArray<FRecord> VisibleRows;
    TArray<TWeakPtr<FRow>> LiveRows;
    TOptional<FString> SelectedKey;
    TOptional<FString> ContextMenuKey;
    TOptional<FName> SortColumn;
    EColumnSortMode::Type SortMode = EColumnSortMode::None;
    ck_ui_table::FConfiguration Configuration;
    TSharedPtr<FConfigurationToken> ConfigurationToken;
    FDelegateHandle CollectionChangedHandle;
    FString LastCellError;
    FString AppliedFilter;
    bool bProjectionDirty = true;
    bool bRefreshing = false;
    bool bPreparing = false;
    bool bNotifying = false;

    ~FImpl()
    {
        if (ConfigurationToken.IsValid()) { ConfigurationToken->Active = false; }
        ReleaseContextMenu();
        if (Collection.IsValid() && CollectionChangedHandle.IsValid())
        { Collection->OnChanged().Remove(CollectionChangedHandle); }
    }

    void OnCollectionChanged() { bProjectionDirty = true; }

    auto IsCurrentRecord(const FRecord& InRecord) const -> bool
    {
        return InRecord.IsValid() && Collection.IsValid() && Collection->FindRecord(InRecord->GetKey()) == InRecord;
    }

    auto IsVisibleRecord(const FRecord& InRecord) const -> bool
    {
        return IsCurrentRecord(InRecord) && VisibleRows.Contains(InRecord);
    }

    auto MakeCellDispatchGate(const FRecord& InRecord, const TSharedRef<FCellScope>& InScope,
        const TSharedPtr<FConfigurationToken>& InConfigurationToken) const -> TAttribute<bool>
    {
        if (!InRecord.IsValid() || !InConfigurationToken.IsValid()) { return TAttribute<bool>(false); }
        const TWeakPtr<const FImpl> WeakImpl = AsShared();
        const TWeakPtr<const FCkUiRecord> WeakRecord = InRecord;
        const TWeakPtr<FCellScope> WeakScope = InScope;
        const TWeakPtr<FConfigurationToken> WeakConfigurationToken = InConfigurationToken;
        // A collection update retains this record object for its stable key. Its revision is field data, not row identity.
        const FString ExpectedKey = InRecord->GetKey();
        return TAttribute<bool>::CreateLambda([WeakImpl, WeakRecord, WeakScope, WeakConfigurationToken, ExpectedKey]()
        {
            const TSharedPtr<const FImpl> Impl = WeakImpl.Pin();
            const TSharedPtr<const FCkUiRecord> Record = WeakRecord.Pin();
            const TSharedPtr<FCellScope> Scope = WeakScope.Pin();
            const TSharedPtr<FConfigurationToken> Token = WeakConfigurationToken.Pin();
            return Impl.IsValid() && Record.IsValid() && Scope.IsValid() && Scope->Active && Token.IsValid() && Token->Active
                && Impl->ConfigurationToken.Get() == Token.Get()
                && Record->GetKey() == ExpectedKey
                && Impl->IsVisibleRecord(Record);
        });
    }

    void ReleaseContextMenu()
    {
        ContextMenuKey.Reset();
        if (ContextMenu.IsValid()) { ContextMenu->Release(); }
    }

    auto OpenAuthoredContextMenu(const FString& InKey, const FWidgetPath& InOwnerPath, const FVector2f& InScreenPosition,
        const int32 InFocusUserIndex = INDEX_NONE, TSharedPtr<SWidget> InParent = {}) -> bool
    {
        if (!Configuration.OnAuthoredContextMenu.IsBound() || !ContextMenu.IsValid()) { return false; }
        const FRecord Record = Collection.IsValid() ? Collection->FindRecord(InKey) : FRecord{};
        if (!IsVisibleRecord(Record) || !List.IsValid())
        {
            ReleaseContextMenu();
            return false;
        }
        const FOnCkUiContextMenuOpening Callback = Configuration.OnAuthoredContextMenu;
        const TSharedPtr<FCkUiMenuSession> Session = Callback.Execute(InKey);
        if (!Session.IsValid()) { return false; }
        const TSharedRef<SWidget> Parent = InParent.IsValid() ? InParent.ToSharedRef() : List.ToSharedRef();
        if (!ContextMenu->Open(Parent, InOwnerPath, InScreenPosition, Session, InFocusUserIndex)) { return false; }
        ContextMenuKey = InKey;
        return true;
    }

    void OnItemRightClicked(FRecord InRecord, const FPointerEvent& InMouseEvent)
    {
        if (!Configuration.OnAuthoredContextMenu.IsBound())
        {
            if (!IsVisibleRecord(InRecord) || !List.IsValid()) { ContextMenuKey.Reset(); return; }
            ContextMenuKey = InRecord->GetKey();
            List->OpenLegacyContextMenu(InMouseEvent);
            ContextMenuKey.Reset();
            return;
        }
        const FWidgetPath OwnerPath = InMouseEvent.GetEventPath() != nullptr ? *InMouseEvent.GetEventPath() : FWidgetPath{};
        if (InRecord.IsValid()) { OpenAuthoredContextMenu(InRecord->GetKey(), OwnerPath, InMouseEvent.GetScreenSpacePosition(), InMouseEvent.GetUserIndex()); }
    }

    auto OnContextMenuKey(const FGeometry&, const FKeyEvent& InKeyEvent) -> TOptional<FReply>
    {
        // This engine's InputCore key table does not expose the Windows Context/Menu key.
        const bool bContextKey = InKeyEvent.GetKey() == EKeys::F10 && InKeyEvent.IsShiftDown();
        if (!bContextKey) { return {}; }
        if (!SelectedKey.IsSet()) { return FReply::Handled(); }
        const FRecord Record = Collection.IsValid() ? Collection->FindRecord(SelectedKey.GetValue()) : FRecord{};
        if (!IsVisibleRecord(Record)) { return FReply::Handled(); }
        if (!Configuration.OnAuthoredContextMenu.IsBound())
        {
            ContextMenuKey.Reset();
            if (!List.IsValid() || !Configuration.OnContextMenu.IsBound()) { return FReply::Handled(); }
            TSharedPtr<SWidget> Parent = List;
            FVector2f Position = List->GetCachedGeometry().GetLayoutBoundingRect().GetBottomLeft();
            if (const TSharedPtr<ITableRow> Row = List->WidgetFromItem(Record); Row.IsValid())
            {
                Parent = Row->AsWidget();
                Position = Parent->GetCachedGeometry().GetLayoutBoundingRect().GetBottomLeft();
            }
            if (!Parent.IsValid()) { return FReply::Handled(); }

            FWidgetPath OwnerPath;
            FSlateApplication& Slate = FSlateApplication::Get();
            if (!Slate.GeneratePathToWidgetUnchecked(Parent.ToSharedRef(), OwnerPath)) { return FReply::Handled(); }

            const FOnContextMenuOpening Callback = Configuration.OnContextMenu;
            ContextMenuKey = Record->GetKey();
            const TSharedPtr<SWidget> Menu = Callback.Execute();
            ContextMenuKey.Reset();
            if (Menu.IsValid())
            {
                Slate.PushMenu(Parent.ToSharedRef(), OwnerPath, Menu.ToSharedRef(), Position,
                    FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu), true, FVector2f::ZeroVector,
                    TOptional<EPopupMethod>{}, true, InKeyEvent.GetUserIndex());
            }
            return FReply::Handled();
        }
        FWidgetPath OwnerPath;
        TSharedPtr<SWidget> Parent = List;
        FVector2f Position = List->GetCachedGeometry().GetLayoutBoundingRect().GetBottomLeft();
        if (const TSharedPtr<ITableRow> Row = List->WidgetFromItem(Record); Row.IsValid())
        {
            Parent = Row->AsWidget();
            Position = Parent->GetCachedGeometry().GetLayoutBoundingRect().GetBottomLeft();
        }
        FSlateApplication& Slate = FSlateApplication::Get();
        if (const TSharedPtr<SWidget> Focused = Slate.GetUserFocusedWidget(InKeyEvent.GetUserIndex()); Focused.IsValid())
        {
            Parent = Focused;
            Slate.GeneratePathToWidgetUnchecked(Focused.ToSharedRef(), OwnerPath);
        }
        else if (Parent.IsValid()) { Slate.GeneratePathToWidgetUnchecked(Parent.ToSharedRef(), OwnerPath); }
        OpenAuthoredContextMenu(SelectedKey.GetValue(), OwnerPath, Position, InKeyEvent.GetUserIndex(), Parent);
        return FReply::Handled();
    }

    auto FindColumn(const FName InId) const -> const ck_ui_table::FColumn*
    {
        for (const ck_ui_table::FColumn& Column : Configuration.Columns)
        { if (Column.Id == InId) { return &Column; } }
        return nullptr;
    }

    auto ErrorCell(const FString& InMessage) -> TSharedRef<SWidget>
    {
        LastCellError = InMessage;
        return SNew(STextBlock).Text(FText::FromString(InMessage));
    }

    auto MakeCells(const ck_ui_table::FConfiguration& InConfiguration, const FRecord& InRecord,
        const TAttribute<bool>& InDispatchGate, ck_ui_table::FCellViews& OutCells, FString& OutFailure) const -> bool
    {
        if (!IsCurrentRecord(InRecord)) { OutFailure = TEXT("table row record is no longer current"); return false; }
        for (const ck_ui_table::FColumn& Column : InConfiguration.Columns)
        {
            FString Failure;
            const TSharedPtr<FCkUiView> View = InConfiguration.Factory(Column.CellRoot, TWeakPtr<const FCkUiRecord>(InRecord), InDispatchGate, Failure);
            if (!View.IsValid() || !Failure.IsEmpty())
            {
                OutFailure = Failure.IsEmpty() ? FString::Printf(TEXT("table cell factory returned no view for column '%s'"), *Column.Id.ToString()) : Failure;
                return false;
            }
            OutCells.Add(Column.Id, View);
        }
        return true;
    }

    auto GetCell(const FRecord& InRecord, const FName InColumn, const TAttribute<bool>& InDispatchGate,
        ck_ui_table::FCellViews& InOutCells) -> TSharedRef<SWidget>
    {
        const TSharedPtr<FImpl> KeepAlive = AsShared();
        if (!IsCurrentRecord(InRecord)) { return ErrorCell(TEXT("table row record is no longer current")); }
        if (const TSharedPtr<FCkUiView>* Existing = InOutCells.Find(InColumn); Existing != nullptr && Existing->IsValid())
        { return (*Existing)->GetRegion(TEXT("cell")); }

        const ck_ui_table::FColumn* Column = FindColumn(InColumn);
        if (Column == nullptr) { return ErrorCell(TEXT("table column is no longer current")); }
        FString Failure;
        const TSharedPtr<FCkUiView> View = Configuration.Factory(Column->CellRoot, TWeakPtr<const FCkUiRecord>(InRecord), InDispatchGate, Failure);
        if (!View.IsValid() || !Failure.IsEmpty())
        { return ErrorCell(Failure.IsEmpty() ? FString::Printf(TEXT("table cell factory returned no view for column '%s'"), *InColumn.ToString()) : Failure); }
        InOutCells.Add(InColumn, View);
        return View->GetRegion(TEXT("cell"));
    }

    auto PassesFilter(const FRecord& InRecord, const FString& InFilter) const -> bool
    {
        if (InFilter.IsEmpty()) { return true; }
        for (const FCkUiFieldSchema& Field : Collection->GetSchema())
        {
            if (Field.Kind != ECkUiFieldKind::Text) { continue; }
            if (const FCkUiFieldValue* Value = InRecord->FindField(Field.Name);
                Value != nullptr && Value->Text.ToString().Contains(InFilter, ESearchCase::IgnoreCase)) { return true; }
        }
        return false;
    }

    auto Less(const FRecord& A, const FRecord& B) const -> bool
    {
        const ck_ui_table::FColumn* Column = SortColumn.IsSet() ? FindColumn(SortColumn.GetValue()) : nullptr;
        if (Column == nullptr || Column->SortField.IsEmpty()) { return A->GetKey() < B->GetKey(); }
        const FCkUiFieldValue* Left = A->FindField(Column->SortField);
        const FCkUiFieldValue* Right = B->FindField(Column->SortField);
        if (Left == nullptr || Right == nullptr || Left->Kind != Right->Kind) { return A->GetKey() < B->GetKey(); }
        bool bLess = false;
        bool bGreater = false;
        switch (Left->Kind)
        {
        case ECkUiFieldKind::Text: bLess = Left->Text.ToString() < Right->Text.ToString(); bGreater = Right->Text.ToString() < Left->Text.ToString(); break;
        case ECkUiFieldKind::Number: bLess = Left->Number < Right->Number; bGreater = Right->Number < Left->Number; break;
        case ECkUiFieldKind::Integer: bLess = Left->Integer < Right->Integer; bGreater = Right->Integer < Left->Integer; break;
        case ECkUiFieldKind::Bool: bLess = !Left->Bool && Right->Bool; bGreater = Left->Bool && !Right->Bool; break;
        default: return A->GetKey() < B->GetKey();
        }
        if (!bLess && !bGreater) { return A->GetKey() < B->GetKey(); }
        return SortMode == EColumnSortMode::Ascending ? bLess : bGreater;
    }

    void Notify(TOptional<FString> InKey, const ESelectInfo::Type InInfo)
    {
        if (!Configuration.OnSelectionChanged.IsBound() || bNotifying) { return; }
        const TSharedPtr<FImpl> KeepAlive = AsShared();
        TGuardValue<bool> Guard(bNotifying, true);
        Configuration.OnSelectionChanged.Execute(MoveTemp(InKey), InInfo);
    }

    void RebuildProjection()
    {
        if (bRefreshing || !Collection.IsValid()) { return; }
        const TSharedPtr<FImpl> KeepAlive = AsShared();
        TGuardValue<bool> Guard(bRefreshing, true);
        AppliedFilter = Configuration.Filter.Get(FText::GetEmpty()).ToString();
        auto Next = TArray<FRecord>{};
        Next.Reserve(Collection->GetRecords().Num());
        for (const FRecord& Record : Collection->GetRecords())
        { if (Record.IsValid() && PassesFilter(Record, AppliedFilter)) { Next.Add(Record); } }
        if (SortColumn.IsSet() && SortMode != EColumnSortMode::None)
        { Next.Sort([this](const FRecord& A, const FRecord& B) { return Less(A, B); }); }

        bool bChanged = VisibleRows.Num() != Next.Num();
        if (!bChanged) for (int32 Index = 0; Index < VisibleRows.Num(); ++Index)
        { if (VisibleRows[Index] != Next[Index]) { bChanged = true; break; } }
        VisibleRows = MoveTemp(Next);
        bProjectionDirty = false;
        if (bChanged && List.IsValid()) { List->RequestListRefresh(); }

        if (SelectedKey.IsSet())
        {
            const FString PreviousKey = SelectedKey.GetValue();
            const FRecord* Existing = VisibleRows.FindByPredicate([&PreviousKey](const FRecord& Record)
            { return Record.IsValid() && Record->GetKey() == PreviousKey; });
            if (Existing == nullptr)
            {
                SelectedKey.Reset();
                if (List.IsValid()) { List->ClearSelection(); }
                Notify({}, ESelectInfo::Direct);
            }
            else if (List.IsValid()) { ck_ui_selection::SelectOnly(*List, *Existing); }
        }
    }

    void OnListSelectionChanged(FRecord InRecord, const ESelectInfo::Type InInfo)
    {
        const TSharedPtr<FImpl> KeepAlive = AsShared();
        if (InInfo == ESelectInfo::Direct || !Configuration.Selectable) { return; }
        if (!InRecord.IsValid())
        {
            if (!SelectedKey.IsSet()) { return; }
            SelectedKey.Reset();
            Notify({}, InInfo);
            return;
        }
        if (!IsCurrentRecord(InRecord)) { return; }
        const FString Key = InRecord->GetKey();
        if (SelectedKey.IsSet() && SelectedKey.GetValue() == Key) { return; }
        SelectedKey = Key;
        Notify(Key, InInfo);
    }

    void OnHeaderSort(EColumnSortPriority::Type, const FName& InColumn, const EColumnSortMode::Type InMode)
    {
        if (bPreparing || bRefreshing) { return; }
        const ck_ui_table::FColumn* Column = FindColumn(InColumn);
        if (Column == nullptr || Column->SortField.IsEmpty()) { return; }
        SortColumn = InMode == EColumnSortMode::None ? TOptional<FName>{} : TOptional<FName>{InColumn};
        SortMode = InMode;
        bProjectionDirty = true;
        if (Header.IsValid()) { Header->Invalidate(EInvalidateWidgetReason::Paint); }
    }

    auto GetSortIndicatorText(const FName InColumn) const -> FText
    {
        if (!SortColumn.IsSet() || SortColumn.GetValue() != InColumn || SortMode == EColumnSortMode::None)
        { return FText::FromString(TEXT("↕")); }
        return SortMode == EColumnSortMode::Ascending ? FText::FromString(TEXT("↑")) : FText::FromString(TEXT("↓"));
    }

    void AddHeaderColumn(const ck_ui_table::FColumn& InColumn)
    {
        auto Arguments = SHeaderRow::Column(InColumn.Id);
        Arguments.DefaultLabel(InColumn.Header).MinSize(InColumn.MinWidth);
        auto Font = BaseFont;
        if (InColumn.HeaderStyle.FontSize.IsSet()) { Font.Size = InColumn.HeaderStyle.FontSize.GetValue(); }
        if (InColumn.HeaderStyle.LetterSpacing.IsSet()) { Font.LetterSpacing = InColumn.HeaderStyle.LetterSpacing.GetValue(); }
        if (InColumn.HeaderStyle.Bold) { Font.TypefaceFontName = TEXT("Bold"); }
        TSharedRef<SWidget> HeaderContent = SNew(STextBlock).Text(InColumn.Header).Font(Font)
            .ColorAndOpacity(InColumn.HeaderStyle.Color.IsSet()
                ? FSlateColor(InColumn.HeaderStyle.Color.GetValue()) : FSlateColor::UseForeground());
        if (!InColumn.SortField.IsEmpty() && Configuration.VisualStyle.SortIndicatorColor.IsSet())
        {
            const FName ColumnId = InColumn.Id;
            const TWeakPtr<FImpl> WeakImpl = AsShared();
            HeaderContent = SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth()[HeaderContent]
                + SHorizontalBox::Slot().AutoWidth().Padding(FMargin(4.0f, 0.0f, 0.0f, 0.0f))
                [SNew(STextBlock).Text(TAttribute<FText>::CreateLambda([WeakImpl, ColumnId]()
                {
                    const TSharedPtr<FImpl> Impl = WeakImpl.Pin();
                    return Impl.IsValid() ? Impl->GetSortIndicatorText(ColumnId) : FText::GetEmpty();
                })).Font(Font).ColorAndOpacity(FSlateColor(Configuration.VisualStyle.SortIndicatorColor.GetValue()))];
        }
        Arguments.HAlignHeader(InColumn.HeaderStyle.HAlign).VAlignHeader(InColumn.HeaderStyle.VAlign)
            .HeaderContentPadding(Configuration.VisualStyle.HasHeaderPadding
                ? TOptional<FMargin>(Configuration.VisualStyle.HeaderPadding) : TOptional<FMargin>())
            [HeaderContent];
        if (!InColumn.SortField.IsEmpty())
        {
            const FName ColumnId = InColumn.Id;
            const TWeakPtr<FImpl> WeakImpl = AsShared();
            Arguments.SortMode(TAttribute<EColumnSortMode::Type>::CreateLambda([WeakImpl, ColumnId]()
            {
                const TSharedPtr<FImpl> Impl = WeakImpl.Pin();
                return Impl.IsValid() && Impl->SortColumn.IsSet() && Impl->SortColumn.GetValue() == ColumnId
                    ? Impl->SortMode : EColumnSortMode::None;
            })).OnSort(FOnSortModeChanged::CreateSP(AsShared(), &FImpl::OnHeaderSort));
        }
        else { Arguments.CanManuallySort(false); }
        if (InColumn.Grow > 0.0f) { Arguments.FillWidth(InColumn.Grow); }
        else { Arguments.FixedWidth(InColumn.Width); }
        Header->AddColumn(Arguments);
    }

    void PruneLiveRows() { LiveRows.RemoveAll([](const TWeakPtr<FRow>& Row) { return !Row.IsValid(); }); }
    void Commit(ck_ui_table::FConfiguration&& InConfiguration, TSharedPtr<FConfigurationToken> InConfigurationToken,
        TArray<TPair<TSharedPtr<FRow>, ck_ui_table::FCellViews>>&& InCells);
};

class SCkUiTable::FImpl::FRow final : public SMultiColumnTableRow<SCkUiTable::FRecord>
{
public:
    SLATE_BEGIN_ARGS(FRow) {}
        SLATE_ARGUMENT(SCkUiTable::FRecord, Record)
        SLATE_ARGUMENT(TWeakPtr<FImpl>, Impl)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwner)
    {
        _Record = InArgs._Record;
        _Impl = InArgs._Impl;
        _CellScope = MakeShared<FCellScope>();
        const TSharedPtr<FImpl> Impl = _Impl.Pin();
        auto Arguments = FSuperRowType::FArguments().Padding(FMargin(0.0f)).ShowSelection(true);
        if (Impl.IsValid()) { Arguments.Style(&Impl->RowStyle); }
        SMultiColumnTableRow<SCkUiTable::FRecord>::Construct(Arguments, InOwner);
        SignalSelectionMode = ETableRowSignalSelectionMode::Instantaneous;
        if (Impl.IsValid()) { Impl->LiveRows.Add(StaticCastSharedRef<FRow>(AsShared())); }
    }

    ~FRow() { _CellScope->Active = false; }

    auto GetRecord() const -> SCkUiTable::FRecord { return _Record; }
    auto ReplaceCells(ck_ui_table::FCellViews&& InCells) -> ck_ui_table::FCellViews
    {
        auto Previous = MoveTemp(_Cells);
        _Cells = MoveTemp(InCells);
        return Previous;
    }

    auto GetCellDispatchGate(const TSharedPtr<FConfigurationToken>& InConfigurationToken = {}) const -> TAttribute<bool>
    {
        const TSharedPtr<FImpl> Impl = _Impl.Pin();
        const TSharedPtr<FConfigurationToken> Token = InConfigurationToken.IsValid()
            ? InConfigurationToken : (Impl.IsValid() ? Impl->ConfigurationToken : TSharedPtr<FConfigurationToken>{});
        return Impl.IsValid() ? Impl->MakeCellDispatchGate(_Record, _CellScope, Token) : TAttribute<bool>(false);
    }

    virtual auto GenerateWidgetForColumn(const FName& InColumn) -> TSharedRef<SWidget> override
    {
        const TSharedPtr<FImpl> Impl = _Impl.Pin();
        if (!Impl.IsValid()) { return SNew(STextBlock).Text(FText::FromString(TEXT("table owner released"))); }
        // Header changes may revisit a retired row before Slate consumes its pending list refresh.
        if (!Impl->IsCurrentRecord(_Record)) { return SNullWidget::NullWidget; }
        if (Impl->FindColumn(InColumn) == nullptr) { return SNew(STextBlock).Text(FText::FromString(TEXT("table column is unavailable"))); }
        return SNew(SBox).HeightOverride(Impl->Configuration.RowHeight).Clipping(EWidgetClipping::ClipToBounds)
            [Impl->GetCell(_Record, InColumn, GetCellDispatchGate(), _Cells)];
    }

private:
    SCkUiTable::FRecord _Record;
    TWeakPtr<FImpl> _Impl;
    TSharedRef<FCellScope> _CellScope = MakeShared<FCellScope>();
    ck_ui_table::FCellViews _Cells;
};

void SCkUiTable::FImpl::Commit(ck_ui_table::FConfiguration&& InConfiguration, TSharedPtr<FConfigurationToken> InConfigurationToken,
    TArray<TPair<TSharedPtr<FRow>, ck_ui_table::FCellViews>>&& InCells)
{
    const TSharedPtr<FImpl> KeepAlive = AsShared();
    ReleaseContextMenu();
    if (ConfigurationToken.IsValid()) { ConfigurationToken->Active = false; }
    ConfigurationToken = MoveTemp(InConfigurationToken);
    Configuration = MoveTemp(InConfiguration);
    HeaderStyle = FAppStyle::Get().GetWidgetStyle<FHeaderRowStyle>(TEXT("TableView.Header"));
    RowStyle = FCoreStyle::Get().GetWidgetStyle<FTableRowStyle>(TEXT("TableView.Row"));
    if (Configuration.VisualStyle.Enabled)
    {
        const FCkUiTableVisualStyle& Visual = Configuration.VisualStyle;
        if (Visual.HeaderBackground.IsSet())
        { HeaderStyle.SetBackgroundBrush(FSlateColorBrush(Visual.HeaderBackground.GetValue())); }
        if (Visual.SortIndicatorColor.IsSet())
        {
            const FSlateNoResource NoResource;
            HeaderStyle.ColumnStyle.SetSortPrimaryAscendingImage(NoResource).SetSortPrimaryDescendingImage(NoResource)
                .SetSortSecondaryAscendingImage(NoResource).SetSortSecondaryDescendingImage(NoResource);
            HeaderStyle.LastColumnStyle.SetSortPrimaryAscendingImage(NoResource).SetSortPrimaryDescendingImage(NoResource)
                .SetSortSecondaryAscendingImage(NoResource).SetSortSecondaryDescendingImage(NoResource);
        }
        const FLinearColor Separator = Visual.RowSeparatorColor.Get(FLinearColor::Transparent);
        const float SeparatorWidth = Visual.RowSeparatorWidth.Get(Visual.RowSeparatorColor.IsSet() ? 1.0f : 0.0f);
        const auto RowBrush = [&Separator, SeparatorWidth](const FLinearColor& InBackground)
        { return FSlateRoundedBoxBrush(InBackground, 0.0f, Separator, SeparatorWidth); };
        if (Visual.RowBackground.IsSet() || Visual.RowSeparatorColor.IsSet())
        {
            const FSlateRoundedBoxBrush Brush = RowBrush(Visual.RowBackground.Get(FLinearColor::Transparent));
            RowStyle.SetEvenRowBackgroundBrush(Brush).SetOddRowBackgroundBrush(Brush);
        }
        if (Visual.RowHoverBackground.IsSet())
        {
            const FSlateRoundedBoxBrush Brush = RowBrush(Visual.RowHoverBackground.GetValue());
            RowStyle.SetEvenRowBackgroundHoveredBrush(Brush).SetOddRowBackgroundHoveredBrush(Brush);
        }
        if (Visual.RowSelectedBackground.IsSet())
        {
            const FSlateRoundedBoxBrush Brush = RowBrush(Visual.RowSelectedBackground.GetValue());
            RowStyle.SetActiveBrush(Brush).SetActiveHoveredBrush(Brush)
                .SetInactiveBrush(Brush).SetInactiveHoveredBrush(Brush);
        }
    }
    if (!Configuration.Selectable)
    {
        SelectedKey.Reset();
        // Native ClearSelection returns early in None mode, so clear before changing modes.
        List->ClearSelection();
    }
    List->SetSelectionMode(Configuration.Selectable ? ESelectionMode::Single : ESelectionMode::None);
    if (SortColumn.IsSet())
    {
        const ck_ui_table::FColumn* Sort = FindColumn(SortColumn.GetValue());
        if (Sort == nullptr || Sort->SortField.IsEmpty())
        {
            SortColumn.Reset();
            SortMode = EColumnSortMode::None;
        }
    }
    auto PreviousCells = TArray<ck_ui_table::FCellViews>{};
    PreviousCells.Reserve(InCells.Num());
    for (TPair<TSharedPtr<FRow>, ck_ui_table::FCellViews>& Pair : InCells)
    { if (Pair.Key.IsValid()) { PreviousCells.Add(Pair.Key->ReplaceCells(MoveTemp(Pair.Value))); } }
    Header->ClearColumns();
    for (const ck_ui_table::FColumn& Column : Configuration.Columns) { AddHeaderColumn(Column); }
    Header->Invalidate(EInvalidateWidgetReason::PaintAndVolatility);
    List->Invalidate(EInvalidateWidgetReason::PaintAndVolatility);
    bProjectionDirty = true;
}

class SCkUiTable::FImpl::FPreparedUpdate final : public ICkUiPreparedWidgetUpdate
{
public:
    FPreparedUpdate(TWeakPtr<FImpl> InImpl, ck_ui_table::FConfiguration&& InConfiguration, TSharedPtr<FConfigurationToken> InConfigurationToken,
        TArray<TPair<TSharedPtr<FRow>, ck_ui_table::FCellViews>>&& InCells)
        : _Impl(MoveTemp(InImpl)), _Configuration(MoveTemp(InConfiguration)), _ConfigurationToken(MoveTemp(InConfigurationToken)), _Cells(MoveTemp(InCells)) {}

    virtual void Commit() noexcept override
    {
        if (const TSharedPtr<FImpl> Impl = _Impl.Pin()) { Impl->Commit(MoveTemp(_Configuration), MoveTemp(_ConfigurationToken), MoveTemp(_Cells)); }
    }

private:
    TWeakPtr<FImpl> _Impl;
    ck_ui_table::FConfiguration _Configuration;
    TSharedPtr<FConfigurationToken> _ConfigurationToken;
    TArray<TPair<TSharedPtr<FRow>, ck_ui_table::FCellViews>> _Cells;
};

void SCkUiTable::Construct(const FArguments& InArgs)
{
    _Impl = MakeShared<FImpl>();
    _Impl->Collection = InArgs._Collection;
    _Impl->BaseFont = InArgs._BaseFont.Size > 0.0f ? InArgs._BaseFont : FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10);
    if (!_Impl->Collection.IsValid())
    {
        _Impl->LastCellError = TEXT("table requires a collection");
        ChildSlot[SNew(STextBlock).Text(FText::FromString(_Impl->LastCellError))];
        return;
    }
    _Impl->HeaderStyle = FAppStyle::Get().GetWidgetStyle<FHeaderRowStyle>(TEXT("TableView.Header"));
    _Impl->RowStyle = FCoreStyle::Get().GetWidgetStyle<FTableRowStyle>(TEXT("TableView.Row"));
    SAssignNew(_Impl->Header, SHeaderRow).Style(&_Impl->HeaderStyle);
    SAssignNew(_Impl->List, ck_ui_table::FList)
        .ListItemsSource(&_Impl->VisibleRows)
        .SelectionMode(ESelectionMode::Single)
        .ClearSelectionOnClick(true)
        .OnGenerateRow_Lambda([WeakImpl = TWeakPtr<FImpl>(_Impl)](FRecord InRecord, const TSharedRef<STableViewBase>& InOwner) -> TSharedRef<ITableRow>
        {
            if (const TSharedPtr<FImpl> Impl = WeakImpl.Pin()) { return SNew(FImpl::FRow, InOwner).Record(MoveTemp(InRecord)).Impl(Impl); }
            return SNew(STableRow<FRecord>, InOwner)[SNew(STextBlock).Text(FText::FromString(TEXT("table owner released")))];
        })
        .OnSelectionChanged_Lambda([WeakImpl = TWeakPtr<FImpl>(_Impl)](FRecord InRecord, ESelectInfo::Type InInfo)
        { if (const TSharedPtr<FImpl> Impl = WeakImpl.Pin()) { Impl->OnListSelectionChanged(MoveTemp(InRecord), InInfo); } })
        .HeaderRow(_Impl->Header)
        .OnContextMenuOpening(FOnContextMenuOpening::CreateLambda([WeakImpl = TWeakPtr<FImpl>(_Impl)]()
        {
            const TSharedPtr<FImpl> Impl = WeakImpl.Pin();
            const FOnContextMenuOpening Callback = Impl.IsValid() && !Impl->Configuration.OnAuthoredContextMenu.IsBound()
                ? Impl->Configuration.OnContextMenu : FOnContextMenuOpening{};
            const TSharedPtr<SWidget> Menu = Callback.IsBound() ? Callback.Execute() : TSharedPtr<SWidget>{};
            if (Impl.IsValid()) { Impl->ContextMenuKey.Reset(); }
            return Menu;
        }));
    _Impl->List->OnItemRightClicked = [WeakImpl = TWeakPtr<FImpl>(_Impl)](FRecord InRecord, const FPointerEvent& InMouseEvent)
    {
        if (const TSharedPtr<FImpl> Impl = WeakImpl.Pin()) { Impl->OnItemRightClicked(MoveTemp(InRecord), InMouseEvent); }
    };
    _Impl->List->OnContextMenuKey = [WeakImpl = TWeakPtr<FImpl>(_Impl)](const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) -> TOptional<FReply>
    {
        const TSharedPtr<FImpl> Impl = WeakImpl.Pin();
        return Impl.IsValid() ? Impl->OnContextMenuKey(InGeometry, InKeyEvent) : TOptional<FReply>{};
    };
    _Impl->CollectionChangedHandle = _Impl->Collection->OnChanged().AddSP(_Impl.ToSharedRef(), &FImpl::OnCollectionChanged);
    ChildSlot[_Impl->List.ToSharedRef()];
}

SCkUiTable::~SCkUiTable() = default;

void SCkUiTable::ReleaseContextMenu()
{
    if (_Impl.IsValid()) { _Impl->ReleaseContextMenu(); }
}

void SCkUiTable::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
    if (!_Impl.IsValid()) { return; }
    if (_Impl->Configuration.Filter.Get(FText::GetEmpty()).ToString() != _Impl->AppliedFilter) { _Impl->bProjectionDirty = true; }
    if (_Impl->bProjectionDirty) { _Impl->RebuildProjection(); }
    if (_Impl->ContextMenu.IsValid()) { _Impl->ContextMenu->Tick(); }
    if (_Impl->ContextMenuKey.IsSet())
    {
        const FRecord Record = _Impl->Collection.IsValid() ? _Impl->Collection->FindRecord(_Impl->ContextMenuKey.GetValue()) : FRecord{};
        if (!_Impl->IsVisibleRecord(Record)) { _Impl->ReleaseContextMenu(); }
    }
    _Impl->PruneLiveRows();
}

auto SCkUiTable::Prepare(const FCkUiNode& InDefinition, FCellFactory InFactory, TAttribute<FText> InFilter,
    FOnCkUiTableSelectionChanged InSelectionChanged, FString& OutFailure, FOnContextMenuOpening InContextMenu,
    FOnCkUiContextMenuOpening InAuthoredContextMenu, const TMap<FString, TAttribute<FText>>& InTextBindings) -> TUniquePtr<ICkUiPreparedWidgetUpdate>
{
    OutFailure.Reset();
    if (!_Impl.IsValid() || !_Impl->Collection.IsValid()) { OutFailure = TEXT("table has no collection"); return {}; }
    if (_Impl->bPreparing || _Impl->bRefreshing || _Impl->bNotifying) { OutFailure = TEXT("table configuration cannot be prepared during table activity"); return {}; }
    if (!InFactory) { OutFailure = TEXT("table cell factory is required"); return {}; }
    if (InDefinition.Kind != ECkUiNodeKind::Table || !FMath::IsFinite(InDefinition.RowHeight)
        || InDefinition.RowHeight < 1.0f || InDefinition.RowHeight > 1024.0f)
    { OutFailure = TEXT("table definition or row height is invalid"); return {}; }
    if (InDefinition.Children.IsEmpty() || InDefinition.Children.Num() > 128)
    { OutFailure = TEXT("table requires 1..128 columns"); return {}; }

    const TSharedPtr<FImpl> KeepAlive = _Impl;
    TGuardValue<bool> Guard(_Impl->bPreparing, true);
    const TSharedPtr<FImpl::FConfigurationToken> ConfigurationToken = MakeShared<FImpl::FConfigurationToken>();
    auto Configuration = ck_ui_table::FConfiguration{};
    Configuration.Selectable = InDefinition.TableSelectable;
    Configuration.VisualStyle = InDefinition.TableVisualStyle;
    Configuration.RowHeight = InDefinition.RowHeight;
    Configuration.Filter = MoveTemp(InFilter);
    Configuration.Factory = MoveTemp(InFactory);
    Configuration.OnSelectionChanged = MoveTemp(InSelectionChanged);
    Configuration.OnContextMenu = MoveTemp(InContextMenu);
    Configuration.OnAuthoredContextMenu = MoveTemp(InAuthoredContextMenu);
    auto ColumnIds = TSet<FName>{};
    for (const FCkUiNode& Node : InDefinition.Children)
    {
        if (Node.Kind != ECkUiNodeKind::TableColumn || Node.Id.IsEmpty() || Node.Children.Num() != 1)
        { OutFailure = TEXT("prepared table definition contains an invalid column"); return {}; }
        ck_ui_table::FColumn Column;
        Column.Id = FName(*Node.Id);
        if (Column.Id.IsNone() || ColumnIds.Contains(Column.Id))
        { OutFailure = TEXT("table column IDs must remain unique after FName conversion"); return {}; }
        ColumnIds.Add(Column.Id);
        Column.Header = Node.HeaderBinding.IsEmpty()
            ? TAttribute<FText>(FText::FromString(Node.Header)) : InTextBindings.FindRef(Node.HeaderBinding);
        if (!Column.Header.IsSet())
        { OutFailure = TEXT("table column header binding is invalid"); return {}; }
        Column.SortField = Node.SortField;
        Column.CellRoot = Node.Children[0];
        Column.HeaderStyle = Node.Style;
        Column.MinWidth = Node.Style.MinWidth;
        Column.Grow = Node.Style.Grow;
        Column.Width = Node.Style.MaxWidth.Get(Node.Style.MinWidth > 0.0f ? Node.Style.MinWidth : 160.0f);
        if (!FMath::IsFinite(Column.MinWidth) || Column.MinWidth < 0.0f || !FMath::IsFinite(Column.Grow) || Column.Grow < 0.0f
            || !FMath::IsFinite(Column.Width) || Column.Width <= 0.0f
            || (Node.Style.MaxWidth.IsSet() && Node.Style.MaxWidth.GetValue() < Column.MinWidth))
        { OutFailure = TEXT("table column width is invalid"); return {}; }
        Configuration.Columns.Add(MoveTemp(Column));
    }
    if (Configuration.Columns.IsEmpty()) { OutFailure = TEXT("table requires at least one column"); return {}; }

    auto Cells = TArray<TPair<TSharedPtr<FImpl::FRow>, ck_ui_table::FCellViews>>{};
    Cells.Reserve(_Impl->LiveRows.Num());
    for (const TWeakPtr<FImpl::FRow>& WeakRow : _Impl->LiveRows)
    {
        const TSharedPtr<FImpl::FRow> Row = WeakRow.Pin();
        if (!Row.IsValid()) { continue; }
        const FRecord Record = Row->GetRecord();
        // Collection publication precedes Slate's deferred destruction of removed rows.
        if (!_Impl->IsCurrentRecord(Record)) { continue; }
        ck_ui_table::FCellViews RowCells;
        if (!_Impl->MakeCells(Configuration, Record, Row->GetCellDispatchGate(ConfigurationToken), RowCells, OutFailure) || !OutFailure.IsEmpty()) { return {}; }
        Cells.Emplace(Row, MoveTemp(RowCells));
    }
    return MakeUnique<FImpl::FPreparedUpdate>(_Impl, MoveTemp(Configuration), ConfigurationToken, MoveTemp(Cells));
}

auto SCkUiTable::TrySelectKey(TOptional<FString> InKey, const bool InNotify) -> bool
{
    if (!_Impl.IsValid() || _Impl->bPreparing || _Impl->bRefreshing || _Impl->bNotifying) { return false; }
    if (InKey.IsSet() && !_Impl->Configuration.Selectable) { return false; }
    if (!InKey.IsSet())
    {
        const bool bHadSelection = _Impl->SelectedKey.IsSet();
        _Impl->SelectedKey.Reset();
        if (_Impl->List.IsValid()) { _Impl->List->ClearSelection(); }
        if (bHadSelection && InNotify) { _Impl->Notify({}, ESelectInfo::Direct); }
        return true;
    }
    for (const FRecord& Record : _Impl->VisibleRows)
    {
        if (Record.IsValid() && Record->GetKey() == InKey.GetValue())
        {
            if (!_Impl->IsCurrentRecord(Record)) { return false; }
            const bool bChanged = !_Impl->SelectedKey.IsSet() || _Impl->SelectedKey.GetValue() != InKey.GetValue();
            _Impl->SelectedKey = InKey;
            ck_ui_selection::SelectOnly(*_Impl->List, Record);
            if (bChanged && InNotify) { _Impl->Notify(InKey, ESelectInfo::Direct); }
            return true;
        }
    }
    return false;
}

auto SCkUiTable::TryRefresh() -> bool
{
    if (!_Impl.IsValid() || !_Impl->Collection.IsValid() || !_Impl->List.IsValid()
        || _Impl->bPreparing || _Impl->bRefreshing || _Impl->bNotifying) { return false; }
    _Impl->bProjectionDirty = true;
    _Impl->RebuildProjection();
    return true;
}

auto SCkUiTable::GetSelectedKey() const -> TOptional<FString> { return _Impl.IsValid() ? _Impl->SelectedKey : TOptional<FString>{}; }
auto SCkUiTable::GetContextMenuKey() const -> TOptional<FString> { return _Impl.IsValid() ? _Impl->ContextMenuKey : TOptional<FString>{}; }
auto SCkUiTable::GetVisibleRecordCount() const -> int32 { return _Impl.IsValid() ? _Impl->VisibleRows.Num() : 0; }
auto SCkUiTable::GetList() const -> TSharedPtr<SListView<FRecord>>
{
    return _Impl.IsValid() ? StaticCastSharedPtr<SListView<FRecord>>(_Impl->List) : TSharedPtr<SListView<FRecord>>{};
}

int32 SCkUiTable::GetLiveRowCount() const
{
    if (!_Impl.IsValid()) { return 0; }
    int32 Count = 0;
    for (const TWeakPtr<FImpl::FRow>& Row : _Impl->LiveRows) if (Row.IsValid()) { ++Count; }
    return Count;
}

const FString& SCkUiTable::GetLastCellError() const
{
    static const FString Empty;
    return _Impl.IsValid() ? _Impl->LastCellError : Empty;
}
