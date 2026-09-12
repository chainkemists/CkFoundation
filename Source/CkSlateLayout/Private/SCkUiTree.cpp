#include "CkSlateLayout/SCkUiTree.h"

#include "CkUiSelection.h"
#include "CkSlateLayout/CkUiContextMenu.h"
#include "CkSlateLayout/SCkUiSurface.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

namespace ck_ui_tree
{
    using FNode = SCkUiTree::FNode;

    struct FConfiguration
    {
        FCkUiNode CellRoot;
        FString ProjectionField;
        bool Selectable = true;
        bool ExpandOnRowClick = false;
        float RowHeight = 24.0f;
        TAttribute<FText> Filter;
        SCkUiTree::FCellFactory Factory;
        FOnCkUiTreeSelectionChanged OnSelectionChanged;
        FOnCkUiContextMenuOpening OnAuthoredContextMenu;
        FCkUiTreeVisualStyle VisualStyle;
    };

    class FTree final : public STreeView<SCkUiTree::FNode>
    {
    public:
        TFunction<void(SCkUiTree::FNode, const FPointerEvent&)> OnItemRightClicked;
        TFunction<bool(const FKeyEvent&)> OnContextMenuKey;
        FOnKeyDown OnHostKeyDown;

        virtual void Private_OnItemRightClicked(SCkUiTree::FNode InNode, const FPointerEvent& InMouseEvent) override
        {
            if (OnItemRightClicked) { OnItemRightClicked(MoveTemp(InNode), InMouseEvent); }
            else { OnRightMouseButtonUp(InMouseEvent); }
        }

        virtual FReply OnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override
        {
            if (OnContextMenuKey && OnContextMenuKey(InKeyEvent)) { return FReply::Handled(); }
            if (OnHostKeyDown.IsBound())
            {
                const FReply HostReply = OnHostKeyDown.Execute(InGeometry, InKeyEvent);
                if (HostReply.IsEventHandled()) { return HostReply; }
            }
            return STreeView<SCkUiTree::FNode>::OnKeyDown(InGeometry, InKeyEvent);
        }

        void OpenLegacyContextMenu(const FPointerEvent& InMouseEvent) { OnRightMouseButtonUp(InMouseEvent); }
    };
}

struct SCkUiTree::FImpl final : public TSharedFromThis<FImpl>
{
    class FRow;
    class FPreparedUpdate;

    TSharedPtr<FCkUiTreeCollection> Collection;
    FSlateFontInfo BaseFont;
    FTableRowStyle RowStyle;
    TSharedPtr<ck_ui_tree::FTree> Tree;
    TSharedPtr<FCkUiContextMenuHost> ContextMenu = MakeShared<FCkUiContextMenuHost>();
    TArray<FNode> FilteredRoots;
    TMap<FString, TArray<FNode>> FilteredChildren;
    TSet<FString> FilteredKeys;
    TSet<FString> UserExpandedKeys;
    TArray<TWeakPtr<FRow>> LiveRows;
    TOptional<FString> SelectedKey;
    TOptional<FString> ContextMenuKey;
    ck_ui_tree::FConfiguration Configuration;
    FDelegateHandle CollectionChangedHandle;
    FString LastCellError;
    FString AppliedFilter;
    bool bProjectionDirty = true;
    bool bRefreshing = false;
    bool bPreparing = false;
    bool bNotifying = false;
    bool bApplyingEffectiveExpansion = false;

    ~FImpl()
    {
        ReleaseContextMenu();
        if (Collection.IsValid() && CollectionChangedHandle.IsValid())
        { Collection->OnChanged().Remove(CollectionChangedHandle); }
    }

    void OnCollectionChanged() { bProjectionDirty = true; }

    auto IsCurrentNode(const FNode& InNode) const -> bool
    {
        return InNode.IsValid() && Collection.IsValid() && Collection->FindNode(InNode->GetKey()) == InNode;
    }

    auto IsVisibleNode(const FNode& InNode) const -> bool
    {
        return IsCurrentNode(InNode) && FilteredKeys.Contains(InNode->GetKey());
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
        const FNode Node = Collection.IsValid() ? Collection->FindNode(InKey) : FNode{};
        if (!IsVisibleNode(Node) || !Tree.IsValid())
        {
            ReleaseContextMenu();
            return false;
        }
        const FOnCkUiContextMenuOpening Callback = Configuration.OnAuthoredContextMenu;
        const TSharedPtr<FCkUiMenuSession> Session = Callback.Execute(InKey);
        if (!Session.IsValid()) { return false; }
        const TSharedRef<SWidget> Parent = InParent.IsValid() ? InParent.ToSharedRef() : Tree.ToSharedRef();
        if (!ContextMenu->Open(Parent, InOwnerPath, InScreenPosition, Session, InFocusUserIndex)) { return false; }
        ContextMenuKey = InKey;
        return true;
    }

    void OnItemRightClicked(FNode InNode, const FPointerEvent& InMouseEvent)
    {
        if (!Configuration.OnAuthoredContextMenu.IsBound())
        {
            if (Tree.IsValid()) { Tree->OpenLegacyContextMenu(InMouseEvent); }
            return;
        }
        const FWidgetPath OwnerPath = InMouseEvent.GetEventPath() != nullptr ? *InMouseEvent.GetEventPath() : FWidgetPath{};
        if (InNode.IsValid()) { OpenAuthoredContextMenu(InNode->GetKey(), OwnerPath, InMouseEvent.GetScreenSpacePosition(), InMouseEvent.GetUserIndex()); }
    }

    auto OnContextMenuKey(const FKeyEvent& InKeyEvent) -> bool
    {
        // This engine's InputCore key table does not expose the Windows Context/Menu key.
        const bool bContextKey = InKeyEvent.GetKey() == EKeys::F10 && InKeyEvent.IsShiftDown();
        if (!bContextKey || !Configuration.OnAuthoredContextMenu.IsBound()) { return false; }
        if (!SelectedKey.IsSet()) { return true; }
        const FNode Node = Collection.IsValid() ? Collection->FindNode(SelectedKey.GetValue()) : FNode{};
        if (!IsVisibleNode(Node)) { return true; }
        FWidgetPath OwnerPath;
        TSharedPtr<SWidget> Parent = Tree;
        FVector2f Position = Tree->GetCachedGeometry().GetLayoutBoundingRect().GetBottomLeft();
        if (const TSharedPtr<ITableRow> Row = Tree->WidgetFromItem(Node); Row.IsValid())
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
        return true;
    }

    auto ErrorCell(const FString& InMessage) -> TSharedRef<SWidget>
    {
        LastCellError = InMessage;
        return SNew(STextBlock).Text(FText::FromString(InMessage));
    }

    auto MakeCell(const ck_ui_tree::FConfiguration& InConfiguration, const FNode& InNode,
        TSharedPtr<FCkUiView>& OutCell, FString& OutFailure) const -> bool
    {
        if (!IsCurrentNode(InNode)) { OutFailure = TEXT("tree row node is no longer current"); return false; }
        OutCell = InConfiguration.Factory(InConfiguration.CellRoot, TWeakPtr<const FCkUiTreeNode>(InNode), OutFailure);
        if (!OutCell.IsValid() || !OutFailure.IsEmpty())
        {
            if (OutFailure.IsEmpty()) { OutFailure = TEXT("tree cell factory returned no view"); }
            return false;
        }
        return true;
    }

    auto GetCell(const FNode& InNode, TSharedPtr<FCkUiView>& InOutCell) -> TSharedRef<SWidget>
    {
        const TSharedPtr<FImpl> KeepAlive = AsShared();
        if (!IsCurrentNode(InNode)) { return ErrorCell(TEXT("tree row node is no longer current")); }
        if (InOutCell.IsValid()) { return InOutCell->GetRegion(TEXT("cell")); }
        FString Failure;
        TSharedPtr<FCkUiView> Candidate;
        const int64 Revision = Collection->GetRevision();
        TGuardValue<bool> PreparingGuard(bPreparing, true);
        if (!MakeCell(Configuration, InNode, Candidate, Failure)) { return ErrorCell(Failure); }
        if (Collection->GetRevision() != Revision || !IsCurrentNode(InNode))
        { return ErrorCell(TEXT("tree collection changed while generating row")); }
        InOutCell = MoveTemp(Candidate);
        return InOutCell->GetRegion(TEXT("cell"));
    }

    auto PassesFilter(const FNode& InNode, const FString& InFilter) const -> bool
    {
        if (InFilter.IsEmpty()) { return true; }
        for (const FCkUiFieldSchema& Field : Collection->GetSchema())
        {
            if (Field.Kind != ECkUiFieldKind::Text) { continue; }
            const FCkUiFieldValue* Value = InNode->FindField(Field.Name);
            if (Value != nullptr && Value->Text.ToString().Contains(InFilter, ESearchCase::IgnoreCase)) { return true; }
        }
        return false;
    }

    auto PassesProjection(const FNode& InNode) const -> bool
    {
        if (Configuration.ProjectionField.IsEmpty()) { return true; }
        const FCkUiFieldValue* Value = InNode->FindField(Configuration.ProjectionField);
        return Value != nullptr && Value->Kind == ECkUiFieldKind::Bool && Value->Bool;
    }

    auto CanToggleExpansionFromRow(const FNode& InNode, const FPointerEvent& InMouseEvent) const -> bool
    {
        return Configuration.ExpandOnRowClick && IsVisibleNode(InNode)
            && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
            && !InMouseEvent.IsControlDown() && !InMouseEvent.IsShiftDown()
            && !InMouseEvent.IsAltDown() && !InMouseEvent.IsCommandDown()
            && Collection->GetChildren(InNode->GetKey()).Num() > 0;
    }

    void ToggleExpansionFromRow(const FNode& InNode)
    {
        if (!Tree.IsValid()) { return; }
        const bool bExpand = !Tree->IsItemExpanded(InNode);
        if (bExpand) { UserExpandedKeys.Add(InNode->GetKey()); }
        else { UserExpandedKeys.Remove(InNode->GetKey()); }
        ApplyExpansion(InNode, bExpand);
    }

    void Notify(TOptional<FString> InKey, const ESelectInfo::Type InInfo)
    {
        if (!Configuration.OnSelectionChanged.IsBound() || bNotifying) { return; }
        const TSharedPtr<FImpl> KeepAlive = AsShared();
        TGuardValue<bool> Guard(bNotifying, true);
        Configuration.OnSelectionChanged.Execute(MoveTemp(InKey), InInfo);
    }

    void ApplyExpansion(const FNode& InNode, const bool bExpand)
    {
        if (Tree.IsValid())
        {
            TGuardValue<bool> Guard(bApplyingEffectiveExpansion, true);
            Tree->SetItemExpansion(InNode, bExpand);
        }
    }

    void RebuildProjection()
    {
        if (bRefreshing || !Collection.IsValid() || !Tree.IsValid()) { return; }
        const TSharedPtr<FImpl> KeepAlive = AsShared();
        TGuardValue<bool> Guard(bRefreshing, true);
        AppliedFilter = Configuration.Filter.Get(FText::GetEmpty()).ToString();

        auto NextKeys = TSet<FString>{};
        if (!Configuration.ProjectionField.IsEmpty())
        {
            for (const FNode& Node : Collection->GetNodes())
            {
                if (!Node.IsValid() || !PassesProjection(Node)) { continue; }
                bool bAncestorsPass = true;
                for (FNode Current = Node; Current.IsValid(); )
                {
                    if (!PassesProjection(Current)) { bAncestorsPass = false; break; }
                    const TOptional<FString>& Parent = Current->GetParentKey();
                    Current = Parent.IsSet() ? Collection->FindNode(Parent.GetValue()) : FNode{};
                }
                if (bAncestorsPass) { NextKeys.Add(Node->GetKey()); }
            }
        }
        else if (AppliedFilter.IsEmpty())
        {
            for (const FNode& Node : Collection->GetNodes()) { if (Node.IsValid()) { NextKeys.Add(Node->GetKey()); } }
        }
        else
        {
            for (const FNode& Node : Collection->GetNodes())
            {
                if (!Node.IsValid() || !PassesFilter(Node, AppliedFilter)) { continue; }
                for (FNode Current = Node; Current.IsValid(); )
                {
                    NextKeys.Add(Current->GetKey());
                    const TOptional<FString>& Parent = Current->GetParentKey();
                    Current = Parent.IsSet() ? Collection->FindNode(Parent.GetValue()) : FNode{};
                }
            }
        }

        auto NextRoots = TArray<FNode>{};
        auto NextChildren = TMap<FString, TArray<FNode>>{};
        for (const FNode& Node : Collection->GetNodes())
        {
            if (!Node.IsValid() || !NextKeys.Contains(Node->GetKey())) { continue; }
            const TOptional<FString>& Parent = Node->GetParentKey();
            if (!Parent.IsSet()) { NextRoots.Add(Node); }
            else if (NextKeys.Contains(Parent.GetValue())) { NextChildren.FindOrAdd(Parent.GetValue()).Add(Node); }
        }
        FilteredKeys = MoveTemp(NextKeys);
        FilteredRoots = MoveTemp(NextRoots);
        FilteredChildren = MoveTemp(NextChildren);
        auto RetainedExpandedKeys = TSet<FString>{};
        for (const FString& Key : UserExpandedKeys)
        { if (Collection->FindNode(Key).IsValid()) { RetainedExpandedKeys.Add(Key); } }
        UserExpandedKeys = MoveTemp(RetainedExpandedKeys);
        bProjectionDirty = false;

        const bool bFiltering = !AppliedFilter.IsEmpty();
        for (const FNode& Node : Collection->GetNodes())
        {
            if (!Node.IsValid()) { continue; }
            const TArray<FNode>* Children = FilteredChildren.Find(Node->GetKey());
            ApplyExpansion(Node, bFiltering ? Children != nullptr && !Children->IsEmpty() : UserExpandedKeys.Contains(Node->GetKey()));
        }
        Tree->RequestTreeRefresh();

        if (SelectedKey.IsSet())
        {
            const FNode Selected = Collection->FindNode(SelectedKey.GetValue());
            if (!Selected.IsValid() || !FilteredKeys.Contains(SelectedKey.GetValue()))
            {
                SelectedKey.Reset();
                Tree->ClearSelection();
                Notify({}, ESelectInfo::Direct);
            }
            else { ck_ui_selection::SelectOnly(*Tree, Selected); }
        }
    }

    void OnGetChildren(FNode InNode, TArray<FNode>& OutChildren) const
    {
        if (!IsCurrentNode(InNode)) { return; }
        if (const TArray<FNode>* Children = FilteredChildren.Find(InNode->GetKey())) { OutChildren = *Children; }
    }

    void OnSelectionChanged(FNode InNode, const ESelectInfo::Type InInfo)
    {
        const TSharedPtr<FImpl> KeepAlive = AsShared();
        if (!Configuration.Selectable) { return; }
        if (InInfo == ESelectInfo::Direct) { return; }
        if (!InNode.IsValid())
        {
            if (!SelectedKey.IsSet()) { return; }
            SelectedKey.Reset();
            Notify({}, InInfo);
            return;
        }
        if (!IsCurrentNode(InNode) || !FilteredKeys.Contains(InNode->GetKey())) { return; }
        if (SelectedKey.IsSet() && SelectedKey.GetValue() == InNode->GetKey()) { return; }
        SelectedKey = InNode->GetKey();
        Notify(SelectedKey, InInfo);
    }

    void OnExpansionChanged(FNode InNode, const bool bExpanded)
    {
        if (bApplyingEffectiveExpansion || bRefreshing || !IsCurrentNode(InNode)) { return; }
        if (bExpanded) { UserExpandedKeys.Add(InNode->GetKey()); }
        else { UserExpandedKeys.Remove(InNode->GetKey()); }
    }

    void PruneLiveRows() { LiveRows.RemoveAll([](const TWeakPtr<FRow>& Row) { return !Row.IsValid(); }); }
    void Commit(ck_ui_tree::FConfiguration&& InConfiguration, TArray<TPair<TSharedPtr<FRow>, TSharedPtr<FCkUiView>>>&& InCells);
};

class SCkUiTree::FImpl::FRow final : public STableRow<SCkUiTree::FNode>
{
public:
    SLATE_BEGIN_ARGS(FRow) {}
        SLATE_ARGUMENT(SCkUiTree::FNode, Node)
        SLATE_ARGUMENT(TWeakPtr<FImpl>, Impl)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwner)
    {
        _Node = InArgs._Node;
        _Impl = InArgs._Impl;
        SAssignNew(_CellBox, SBox).Clipping(EWidgetClipping::ClipToBounds);
        auto Arguments = STableRow<SCkUiTree::FNode>::FArguments().Padding(FMargin(0.0f)).ShowSelection(true);
        if (const TSharedPtr<FImpl> Impl = _Impl.Pin()) { Arguments.Style(&Impl->RowStyle); }
        STableRow<SCkUiTree::FNode>::Construct(Arguments[_CellBox.ToSharedRef()], InOwner);
        SignalSelectionMode = ETableRowSignalSelectionMode::Instantaneous;
        if (const TSharedPtr<FImpl> Impl = _Impl.Pin()) { Impl->LiveRows.Add(StaticCastSharedRef<FRow>(AsShared())); }
        if (const TSharedPtr<FImpl> Impl = _Impl.Pin())
        {
            _CellBox->SetHeightOverride(Impl->Configuration.RowHeight);
            _CellBox->SetContent(Impl->GetCell(_Node, _Cell));
        }
    }

    virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
        const bool bParentEnabled) const override
    {
        const int32 PaintedLayer = STableRow<SCkUiTree::FNode>::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
        const TSharedPtr<FImpl> Impl = _Impl.Pin();
        if (!Impl.IsValid() || !IsItemSelected() || !Impl->Configuration.VisualStyle.SelectedAccentColor.IsSet() || !Impl->Configuration.VisualStyle.SelectedAccentWidth.IsSet()) { return PaintedLayer; }
        const float Width = Impl->Configuration.VisualStyle.SelectedAccentWidth.GetValue();
        if (Width <= 0.0f) { return PaintedLayer; }
        FSlateDrawElement::MakeBox(OutDrawElements, PaintedLayer + 1,
            AllottedGeometry.ToPaintGeometry(FVector2f{Width, AllottedGeometry.GetLocalSize().Y}, FSlateLayoutTransform{}),
            FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")), ESlateDrawEffect::None,
            Impl->Configuration.VisualStyle.SelectedAccentColor.GetValue());
        return PaintedLayer + 1;
    }

    virtual FReply OnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override
    {
        if (const TSharedPtr<FImpl> Impl = _Impl.Pin(); Impl.IsValid() && Impl->CanToggleExpansionFromRow(_Node, InMouseEvent))
        {
            Impl->ToggleExpansionFromRow(_Node);
            return FReply::Handled();
        }
        return STableRow<SCkUiTree::FNode>::OnMouseButtonDown(InGeometry, InMouseEvent);
    }

    auto GetNode() const -> SCkUiTree::FNode { return _Node; }
    auto ReplaceCell(TSharedPtr<FCkUiView>&& InCell, const float InRowHeight) -> TSharedPtr<FCkUiView>
    {
        TSharedPtr<FCkUiView> Previous = MoveTemp(_Cell);
        _Cell = MoveTemp(InCell);
        if (_CellBox.IsValid())
        {
            _CellBox->SetHeightOverride(InRowHeight);
            _CellBox->SetContent(_Cell.IsValid() ? _Cell->GetRegion(TEXT("cell")) : SNullWidget::NullWidget);
        }
        return Previous;
    }

private:
    SCkUiTree::FNode _Node;
    TWeakPtr<FImpl> _Impl;
    TSharedPtr<FCkUiView> _Cell;
    TSharedPtr<SBox> _CellBox;
};

void SCkUiTree::FImpl::Commit(ck_ui_tree::FConfiguration&& InConfiguration,
    TArray<TPair<TSharedPtr<FRow>, TSharedPtr<FCkUiView>>>&& InCells)
{
    const TSharedPtr<FImpl> KeepAlive = AsShared();
    Configuration = MoveTemp(InConfiguration);
    RowStyle = FCoreStyle::Get().GetWidgetStyle<FTableRowStyle>(TEXT("TableView.Row"));
    const FCkUiTreeVisualStyle& VisualStyle = Configuration.VisualStyle;
    if (VisualStyle.RowBackground.IsSet())
    {
        const FSlateRoundedBoxBrush Brush{VisualStyle.RowBackground.GetValue(), 0.0f};
        RowStyle.SetEvenRowBackgroundBrush(Brush).SetOddRowBackgroundBrush(Brush);
    }
    if (VisualStyle.RowHoverBackground.IsSet())
    {
        const FSlateRoundedBoxBrush Brush{VisualStyle.RowHoverBackground.GetValue(), 0.0f};
        RowStyle.SetEvenRowBackgroundHoveredBrush(Brush).SetOddRowBackgroundHoveredBrush(Brush);
    }
    if (VisualStyle.RowSelectedBackground.IsSet())
    {
        const FSlateRoundedBoxBrush Brush{VisualStyle.RowSelectedBackground.GetValue(), 0.0f};
        RowStyle.SetActiveBrush(Brush).SetActiveHoveredBrush(Brush).SetInactiveBrush(Brush).SetInactiveHoveredBrush(Brush);
    }
    if (!Configuration.Selectable)
    {
        SelectedKey.Reset();
        // Native ClearSelection returns early in None mode, so clear before changing modes.
        Tree->ClearSelection();
    }
    Tree->SetSelectionMode(Configuration.Selectable ? ESelectionMode::Single : ESelectionMode::None);
    auto PreviousCells = TArray<TSharedPtr<FCkUiView>>{};
    for (TPair<TSharedPtr<FRow>, TSharedPtr<FCkUiView>>& Pair : InCells)
    { if (Pair.Key.IsValid()) { PreviousCells.Add(Pair.Key->ReplaceCell(MoveTemp(Pair.Value), Configuration.RowHeight)); } }
    bProjectionDirty = true;
    if (Tree.IsValid())
    {
        Tree->RequestTreeRefresh();
        Tree->Invalidate(EInvalidateWidgetReason::PaintAndVolatility);
    }
}

class SCkUiTree::FImpl::FPreparedUpdate final : public ICkUiPreparedWidgetUpdate
{
public:
    FPreparedUpdate(TWeakPtr<FImpl> InImpl, ck_ui_tree::FConfiguration&& InConfiguration,
        TArray<TPair<TSharedPtr<FRow>, TSharedPtr<FCkUiView>>>&& InCells)
        : _Impl(MoveTemp(InImpl)), _Configuration(MoveTemp(InConfiguration)), _Cells(MoveTemp(InCells)) {}

    virtual void Commit() noexcept override
    { if (const TSharedPtr<FImpl> Impl = _Impl.Pin()) { Impl->Commit(MoveTemp(_Configuration), MoveTemp(_Cells)); } }

private:
    TWeakPtr<FImpl> _Impl;
    ck_ui_tree::FConfiguration _Configuration;
    TArray<TPair<TSharedPtr<FRow>, TSharedPtr<FCkUiView>>> _Cells;
};

void SCkUiTree::Construct(const FArguments& InArgs)
{
    _Impl = MakeShared<FImpl>();
    _Impl->Collection = InArgs._Collection;
    _Impl->BaseFont = InArgs._BaseFont.Size > 0.0f ? InArgs._BaseFont : FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10);
    _Impl->RowStyle = FCoreStyle::Get().GetWidgetStyle<FTableRowStyle>(TEXT("TableView.Row"));
    if (!_Impl->Collection.IsValid())
    {
        _Impl->LastCellError = TEXT("tree requires a collection");
        ChildSlot[SNew(STextBlock).Text(FText::FromString(_Impl->LastCellError))];
        return;
    }
    SAssignNew(_Impl->Tree, ck_ui_tree::FTree)
        .TreeItemsSource(&_Impl->FilteredRoots)
        .SelectionMode(ESelectionMode::Single)
        .ClearSelectionOnClick(true)
        .OnGenerateRow_Lambda([WeakImpl = TWeakPtr<FImpl>(_Impl)](FNode InNode, const TSharedRef<STableViewBase>& InOwner) -> TSharedRef<ITableRow>
        {
            if (const TSharedPtr<FImpl> Impl = WeakImpl.Pin()) { return SNew(FImpl::FRow, InOwner).Node(MoveTemp(InNode)).Impl(Impl); }
            return SNew(STableRow<FNode>, InOwner)[SNew(STextBlock).Text(FText::FromString(TEXT("tree owner released")))];
        })
        .OnGetChildren_Lambda([WeakImpl = TWeakPtr<FImpl>(_Impl)](FNode InNode, TArray<FNode>& OutChildren)
        { if (const TSharedPtr<FImpl> Impl = WeakImpl.Pin()) { Impl->OnGetChildren(MoveTemp(InNode), OutChildren); } })
        .OnSelectionChanged_Lambda([WeakImpl = TWeakPtr<FImpl>(_Impl)](FNode InNode, ESelectInfo::Type InInfo)
        { if (const TSharedPtr<FImpl> Impl = WeakImpl.Pin()) { Impl->OnSelectionChanged(MoveTemp(InNode), InInfo); } })
        .OnExpansionChanged_Lambda([WeakImpl = TWeakPtr<FImpl>(_Impl)](FNode InNode, bool bExpanded)
        { if (const TSharedPtr<FImpl> Impl = WeakImpl.Pin()) { Impl->OnExpansionChanged(MoveTemp(InNode), bExpanded); } });
    _Impl->Tree->OnItemRightClicked = [WeakImpl = TWeakPtr<FImpl>(_Impl)](FNode InNode, const FPointerEvent& InMouseEvent)
    {
        if (const TSharedPtr<FImpl> Impl = WeakImpl.Pin()) { Impl->OnItemRightClicked(MoveTemp(InNode), InMouseEvent); }
    };
    _Impl->Tree->OnContextMenuKey = [WeakImpl = TWeakPtr<FImpl>(_Impl)](const FKeyEvent& InKeyEvent)
    {
        const TSharedPtr<FImpl> Impl = WeakImpl.Pin();
        return Impl.IsValid() && Impl->OnContextMenuKey(InKeyEvent);
    };
    _Impl->CollectionChangedHandle = _Impl->Collection->OnChanged().AddSP(_Impl.ToSharedRef(), &FImpl::OnCollectionChanged);
    ChildSlot[_Impl->Tree.ToSharedRef()];
}

SCkUiTree::~SCkUiTree() = default;

void SCkUiTree::SetHostKeyDownHandler(FOnKeyDown InHandler)
{
    if (_Impl.IsValid() && _Impl->Tree.IsValid()) { _Impl->Tree->OnHostKeyDown = MoveTemp(InHandler); }
}

void SCkUiTree::ReleaseContextMenu()
{
    if (_Impl.IsValid()) { _Impl->ReleaseContextMenu(); }
}

void SCkUiTree::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
    if (!_Impl.IsValid()) { return; }
    if (_Impl->Configuration.Filter.Get(FText::GetEmpty()).ToString() != _Impl->AppliedFilter) { _Impl->bProjectionDirty = true; }
    if (_Impl->bProjectionDirty) { _Impl->RebuildProjection(); }
    if (_Impl->ContextMenu.IsValid()) { _Impl->ContextMenu->Tick(); }
    if (_Impl->ContextMenuKey.IsSet())
    {
        const FNode Node = _Impl->Collection.IsValid() ? _Impl->Collection->FindNode(_Impl->ContextMenuKey.GetValue()) : FNode{};
        if (!_Impl->IsVisibleNode(Node)) { _Impl->ReleaseContextMenu(); }
    }
    _Impl->PruneLiveRows();
}

auto SCkUiTree::Prepare(const FCkUiNode& InDefinition, FCellFactory InFactory, TAttribute<FText> InFilter,
    FOnCkUiTreeSelectionChanged InSelectionChanged, FString& OutFailure,
    FOnCkUiContextMenuOpening InAuthoredContextMenu) -> TUniquePtr<ICkUiPreparedWidgetUpdate>
{
    OutFailure.Reset();
    if (!_Impl.IsValid() || !_Impl->Collection.IsValid()) { OutFailure = TEXT("tree has no collection"); return {}; }
    if (_Impl->bPreparing || _Impl->bRefreshing || _Impl->bNotifying) { OutFailure = TEXT("tree configuration cannot be prepared during tree activity"); return {}; }
    if (!InFactory) { OutFailure = TEXT("tree cell factory is required"); return {}; }
    if (InDefinition.Kind != ECkUiNodeKind::Tree || InDefinition.Children.Num() != 1 || !FMath::IsFinite(InDefinition.RowHeight) || InDefinition.RowHeight < 1.0f || InDefinition.RowHeight > 1024.0f)
    { OutFailure = TEXT("tree definition or row height is invalid"); return {}; }

    const TSharedPtr<FImpl> KeepAlive = _Impl;
    TGuardValue<bool> Guard(_Impl->bPreparing, true);
    auto Configuration = ck_ui_tree::FConfiguration{};
    Configuration.CellRoot = InDefinition.Children[0];
    Configuration.ProjectionField = InDefinition.ProjectionField;
    Configuration.Selectable = InDefinition.TableSelectable;
    Configuration.ExpandOnRowClick = InDefinition.TreeExpandOnRowClick;
    Configuration.RowHeight = InDefinition.RowHeight;
    Configuration.Filter = MoveTemp(InFilter);
    Configuration.Factory = MoveTemp(InFactory);
    Configuration.OnSelectionChanged = MoveTemp(InSelectionChanged);
    Configuration.OnAuthoredContextMenu = MoveTemp(InAuthoredContextMenu);
    Configuration.VisualStyle = InDefinition.TreeVisualStyle;
    if (!Configuration.ProjectionField.IsEmpty())
    {
        const FCkUiFieldSchema* Projection = _Impl->Collection->GetSchema().FindByPredicate([&Configuration](const FCkUiFieldSchema& Field)
        { return Field.Name == Configuration.ProjectionField; });
        if (Projection == nullptr || Projection->Kind != ECkUiFieldKind::Bool || !Projection->Required)
        { OutFailure = TEXT("tree projection-field must name a required Bool schema field"); return {}; }
    }
    const int64 PreparationRevision = _Impl->Collection->GetRevision();
    auto Cells = TArray<TPair<TSharedPtr<FImpl::FRow>, TSharedPtr<FCkUiView>>>{};
    for (const TWeakPtr<FImpl::FRow>& WeakRow : _Impl->LiveRows)
    {
        const TSharedPtr<FImpl::FRow> Row = WeakRow.Pin();
        if (!Row.IsValid() || !_Impl->IsCurrentNode(Row->GetNode())) { continue; }
        if (_Impl->Collection->GetRevision() != PreparationRevision)
        {
            OutFailure = TEXT("tree collection changed while preparing cells");
            return {};
        }
        TSharedPtr<FCkUiView> Cell;
        if (!_Impl->MakeCell(Configuration, Row->GetNode(), Cell, OutFailure)) { return {}; }
        if (_Impl->Collection->GetRevision() != PreparationRevision)
        {
            OutFailure = TEXT("tree collection changed while preparing cells");
            return {};
        }
        Cells.Emplace(Row, MoveTemp(Cell));
    }
    if (_Impl->Collection->GetRevision() != PreparationRevision)
    {
        OutFailure = TEXT("tree collection changed while preparing cells");
        return {};
    }
    return MakeUnique<FImpl::FPreparedUpdate>(_Impl, MoveTemp(Configuration), MoveTemp(Cells));
}

auto SCkUiTree::TrySelectKey(TOptional<FString> InKey, const bool InNotify) -> bool
{
    if (!_Impl.IsValid() || !_Impl->Tree.IsValid() || _Impl->bPreparing || _Impl->bRefreshing || _Impl->bNotifying) { return false; }
    if (!_Impl->Configuration.Selectable) { return false; }
    if (!InKey.IsSet())
    {
        const bool bHadSelection = _Impl->SelectedKey.IsSet();
        _Impl->SelectedKey.Reset();
        _Impl->Tree->ClearSelection();
        if (bHadSelection && InNotify) { _Impl->Notify({}, ESelectInfo::Direct); }
        return true;
    }
    const FNode Node = _Impl->Collection->FindNode(InKey.GetValue());
    if (!Node.IsValid() || !_Impl->FilteredKeys.Contains(InKey.GetValue())) { return false; }
    const bool bChanged = !_Impl->SelectedKey.IsSet() || _Impl->SelectedKey.GetValue() != InKey.GetValue();
    _Impl->SelectedKey = InKey;
    ck_ui_selection::SelectOnly(*_Impl->Tree, Node);
    if (bChanged && InNotify) { _Impl->Notify(InKey, ESelectInfo::Direct); }
    return true;
}

auto SCkUiTree::TrySetExpanded(const FString& InKey, const bool bInExpanded) -> bool
{
    if (!_Impl.IsValid() || !_Impl->Tree.IsValid() || _Impl->bPreparing || _Impl->bRefreshing || _Impl->bNotifying) { return false; }
    const FNode Node = _Impl->Collection->FindNode(InKey);
    if (!Node.IsValid()) { return false; }
    if (bInExpanded) { _Impl->UserExpandedKeys.Add(InKey); }
    else { _Impl->UserExpandedKeys.Remove(InKey); }
    if (_Impl->AppliedFilter.IsEmpty()) { _Impl->ApplyExpansion(Node, bInExpanded); }
    return true;
}

auto SCkUiTree::GetSelectedKey() const -> TOptional<FString> { return _Impl.IsValid() ? _Impl->SelectedKey : TOptional<FString>{}; }
auto SCkUiTree::GetExpandedKeys() const -> TSet<FString> { return _Impl.IsValid() ? _Impl->UserExpandedKeys : TSet<FString>{}; }
auto SCkUiTree::GetVisibleNodeCount() const -> int32 { return _Impl.IsValid() ? _Impl->FilteredKeys.Num() : 0; }
auto SCkUiTree::GetTree() const -> TSharedPtr<STreeView<FNode>>
{
    return _Impl.IsValid() ? StaticCastSharedPtr<STreeView<FNode>>(_Impl->Tree) : TSharedPtr<STreeView<FNode>>{};
}

int32 SCkUiTree::GetLiveRowCount() const
{
    if (!_Impl.IsValid()) { return 0; }
    int32 Count = 0;
    for (const TWeakPtr<FImpl::FRow>& Row : _Impl->LiveRows) if (Row.IsValid()) { ++Count; }
    return Count;
}

const FString& SCkUiTree::GetLastCellError() const
{
    static const FString Empty;
    return _Impl.IsValid() ? _Impl->LastCellError : Empty;
}
