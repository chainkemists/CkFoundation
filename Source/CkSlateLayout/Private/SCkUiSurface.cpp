#include "CkSlateLayout/SCkUiSurface.h"
#include "CkUiWidgetRegistry.h"

#include "CkSlateLayout/CkFlexBox.h"
#include "CkSlateLayout/CkFlexText.h"
#include "CkSlateLayout/SCkUiSplitter.h"
#include "CkSlateLayout/SCkUiTabs.h"
#include "CkSlateLayout/SCkUiMenuButton.h"
#include "CkSlateLayout/SCkUiRepeat.h"
#include "CkSlateLayout/SCkUiScrollBox.h"
#include "Brushes/SlateRoundedBoxBrush.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "HAL/FileManager.h"
#include "Layout/WidgetPath.h"
#include "Layout/Children.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopeExit.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace ck_ui_surface
{
    // Native Slate parents measure bottom-up without a width constraint. Retain the
    // allotted width so their next prepass receives the authored content's wrapped height.
    class SCkUiRegion final : public SBox
    {
    public:
        SCkUiRegion() { SetCanTick(true); }

        void Tick(const FGeometry& Geometry, const double CurrentTime, const float DeltaTime) override
        {
            SBox::Tick(Geometry, CurrentTime, DeltaTime);
            const float Width = Geometry.GetLocalSize().X;
            const float Scale = Geometry.GetAccumulatedLayoutTransform().GetScale();
            if (FMath::IsFinite(Width) && Width >= 0.0f && FMath::IsFinite(Scale) && Scale > 0.0f
                && (!_Width.IsSet() || _Width.GetValue() != Width || _Scale != Scale))
            {
                _Width = Width;
                _Scale = Scale;
                Invalidate(EInvalidateWidgetReason::Layout);
            }
        }

    protected:
        auto ComputeDesiredSize(const float Scale) const -> FVector2D override
        {
            if (_Width.IsSet() && ChildSlot.GetWidget()->GetVisibility() != EVisibility::Collapsed)
            {
                const auto Measure = ChildSlot.GetWidget()->GetMetaData<FCkFlexMeasureMetaData>();
                if (Measure.IsValid())
                {
                    return Measure->Measure(FCkFlexMeasureArgs{
                        .AvailableWidth = _Width.GetValue(), .WidthMode = YGMeasureModeExactly,
                        .LayoutScale = Scale});
                }
            }
            return SBox::ComputeDesiredSize(Scale);
        }

    private:
        TOptional<float> _Width;
        float _Scale = 1.0f;
    };

    // Decorative layers occupy the primary child's bounds without affecting its desired size.
    class SCkUiOverlay final : public SOverlay
    {
    protected:
        auto ComputeDesiredSize(float) const -> FVector2D override
        {
            if (Children.Num() == 0 || Children[0].GetWidget()->GetVisibility() == EVisibility::Collapsed)
            { return FVector2D::ZeroVector; }
            return Children[0].GetWidget()->GetDesiredSize() + Children[0].GetPadding().GetDesiredSize();
        }
    };

    auto BackgroundBrush() -> const FSlateBrush*
    {
        // Region widgets can outlive the view that created them.
        static const auto Brush = FSlateRoundedBoxBrush{FLinearColor::White, 6.0f};
        return &Brush;
    }

    auto Error(const FString& InSource, const FString& InMessage) -> FString
    {
        return FString::Printf(TEXT("%s: %s"), *InSource, *InMessage);
    }

    auto IsFiniteNonNegative(const float InValue) -> bool
    {
        return FMath::IsFinite(InValue) && InValue >= 0.0f;
    }

    auto FieldKind(const ECkUiCustomPropertyKind InKind) -> TOptional<ECkUiFieldKind>
    {
        switch (InKind)
        {
        case ECkUiCustomPropertyKind::TextBinding: return ECkUiFieldKind::Text;
        case ECkUiCustomPropertyKind::NumberBinding: return ECkUiFieldKind::Number;
        case ECkUiCustomPropertyKind::BoolBinding: return ECkUiFieldKind::Bool;
        case ECkUiCustomPropertyKind::ColorBinding: return ECkUiFieldKind::Color;
        case ECkUiCustomPropertyKind::ImageBinding: return ECkUiFieldKind::Image;
        default: return {};
        }
    }

    auto IsStyleValid(const FCkUiStyle& InStyle) -> bool
    {
        const auto Padding = InStyle.Padding;
        const auto PaddingIsValid = IsFiniteNonNegative(Padding.Left) && IsFiniteNonNegative(Padding.Top)
            && IsFiniteNonNegative(Padding.Right) && IsFiniteNonNegative(Padding.Bottom);
        const auto OptionalIsValid = [](const TOptional<float>& InValue) { return !InValue.IsSet() || IsFiniteNonNegative(InValue.GetValue()); };
        const auto BoundsAreValid = (!InStyle.MaxWidth.IsSet() || InStyle.MaxWidth.GetValue() >= InStyle.MinWidth)
            && (!InStyle.MaxHeight.IsSet() || InStyle.MaxHeight.GetValue() >= InStyle.MinHeight);
        const bool FlexWrapIsValid = InStyle.FlexWrap == ECkFlexWrap::NoWrap || InStyle.FlexWrap == ECkFlexWrap::Wrap || InStyle.FlexWrap == ECkFlexWrap::WrapReverse;
        return FlexWrapIsValid && PaddingIsValid && IsFiniteNonNegative(InStyle.Gap) && IsFiniteNonNegative(InStyle.Grow)
            && IsFiniteNonNegative(InStyle.Shrink) && IsFiniteNonNegative(InStyle.MinWidth)
            && IsFiniteNonNegative(InStyle.MinHeight) && OptionalIsValid(InStyle.MaxWidth)
            && OptionalIsValid(InStyle.MaxHeight) && OptionalIsValid(InStyle.FontSize) && BoundsAreValid;
    }

    auto MakeSlotArguments(const FCkUiStyle& InStyle, const TSharedRef<SWidget>& InWidget) -> SCkFlexBox::FSlot::FSlotArguments
    {
        auto Result = SCkFlexBox::Slot();
        Result.Grow(InStyle.Grow).Shrink(InStyle.Shrink).MinWidth(InStyle.MinWidth).MinHeight(InStyle.MinHeight)
            .HAlign(InStyle.HAlign).VAlign(InStyle.VAlign)[InWidget];
        if (InStyle.MaxWidth.IsSet()) { Result.MaxWidth(InStyle.MaxWidth.GetValue()); }
        if (InStyle.MaxHeight.IsSet()) { Result.MaxHeight(InStyle.MaxHeight.GetValue()); }
        return Result;
    }

    auto HasWidget(const FWidgetPath& InPath, const TSharedPtr<SWidget>& InWidget) -> bool
    {
        if (!InWidget.IsValid()) { return false; }
        for (int32 Index = 0; Index < InPath.Widgets.Num(); ++Index)
        {
            const FArrangedWidget& Arranged = InPath.Widgets[Index];
            if (Arranged.Widget == InWidget) { return true; }
        }
        return false;
    }

    auto MakeMeasuredPort(const TSharedRef<SWidget>& InWidget, const FString& InId) -> TSharedRef<SBox>
    {
        const auto Port = SNew(SBox).Tag(FName(*InId));
        if (const TSharedPtr<FCkFlexMeasureMetaData> Measure = InWidget->GetMetaData<FCkFlexMeasureMetaData>(); Measure.IsValid())
        {
            Port->AddMetadata(MakeShared<FCkFlexMeasureMetaData>(
                [WeakWidget = TWeakPtr<SWidget>(InWidget), Measure](const FCkFlexMeasureArgs& Args)
                {
                    const TSharedPtr<SWidget> Widget = WeakWidget.Pin();
                    return Widget.IsValid() ? Measure->Measure(Args) : FVector2D::ZeroVector;
                },
                [WeakWidget = TWeakPtr<SWidget>(InWidget), Measure](const float Width, const float Height)
                {
                    const TSharedPtr<SWidget> Widget = WeakWidget.Pin();
                    if (Widget.IsValid()) { Measure->NotifyArranged(Width, Height); }
                }));
        }
        return Port;
    }

    auto CollectWidgetPointers(const TSharedRef<SWidget>& InRoot, TSet<const SWidget*>& OutWidgets, const TSet<const SWidget*>* InStops = nullptr) -> bool
    {
        auto Pending = TArray<TSharedRef<SWidget>>{InRoot};
        while (!Pending.IsEmpty())
        {
            const TSharedRef<SWidget> Current = Pending.Pop(EAllowShrinking::No);
            if (&Current.Get() == &SNullWidget::NullWidget.Get() || (InStops != nullptr && InStops->Contains(&Current.Get()))) { continue; }
            if (OutWidgets.Contains(&Current.Get())) { return false; }
            OutWidgets.Add(&Current.Get());
            FChildren* Children = Current->GetChildren();
            if (Children == nullptr) { continue; }
            for (int32 Index = 0; Index < Children->Num(); ++Index)
            { Pending.Add(Children->GetChildAt(Index)); }
        }
        return true;
    }

}

struct FCkUiView::FRepeatScope
{
    bool Active = false;
};

struct FCkUiView::FRepeatState
{
    struct FItem
    {
        TSharedPtr<const FCkUiRecord> Record;
        TSharedPtr<FCkUiView> View;
        TSharedPtr<FRepeatScope> Scope;
    };
    FCkUiNode Definition;
    TSharedPtr<FCkUiCollection> Collection;
    TSharedPtr<SCkUiRepeat> Widget;
    TArray<FItem> Items;
};

struct FCkUiView::FCustomSlot
{
    TSharedPtr<SBox> Mount;
    TSharedPtr<FCkUiView> View;
    TSharedPtr<FRepeatScope> Scope;
};

struct FCkUiView::FCustomSlotSet
{
    TMap<FString, TSharedPtr<FCustomSlot>> Slots;
};

struct FCkUiView::FStagedDocument
{
    struct FScrollContent
    {
        TSharedPtr<SScrollBox> Scroll;
        TSharedPtr<SWidget> Child;
        FMargin Padding;
        EOrientation Direction = Orient_Vertical;
    };
    TMap<FString, TSharedPtr<SWidget>> Regions;
    TArray<FRetainedRecord> Retained;
    TArray<TUniquePtr<ICkUiPreparedWidgetUpdate>> PreparedUpdates;
    TMap<FString, TAttribute<FText>> SearchHints;
    TArray<TSharedPtr<SWidget>> CustomWidgets;
    TArray<TSharedPtr<SWidget>> OwnedWidgets;
    TArray<FScrollContent> ScrollContents;
    TArray<TFunction<void()>> TabsUpdates;
    TMap<FString, FCkUiMenu> Menus;
    TArray<TFunction<void()>> MenuUpdates;
    TMap<FString, TSharedPtr<FRepeatState>> Repeats;
    TMap<FString, TSharedPtr<FCustomSlotSet>> CustomSlots;
    TArray<TSharedPtr<FNestedUpdate>> Nested;
};

struct FCkUiView::FCommitState
{
    struct FFocusedUser
    {
        int32 UserIndex = INDEX_NONE;
        TSharedPtr<SWidget> Widget;
        bool RetainedSurvives = false;
        TSharedPtr<ICkUiRetainedWidget> Component;
    };

    TMap<FString, FRetainedRecord> PreviousRetained;
    TMap<FString, FRetainedRecord> NextRetained;
    TArray<FCapturedPointer> CapturedPointers;
    TArray<FFocusedUser> FocusedUsers;
    TMap<FString, TSharedPtr<FRepeatState>> PreviousRepeats;
    TMap<FString, TSharedPtr<FCustomSlotSet>> PreviousCustomSlots;
    TArray<TPair<TSharedPtr<FCkUiView>, TSharedPtr<FCommitState>>> RetiredChildren;
};

struct FCkUiView::FNestedUpdate
{
    TSharedPtr<FCkUiView> View;
    TSharedPtr<FStagedDocument> Staged;
    FCommitState State;
    ~FNestedUpdate()
    {
        Staged.Reset();
        State = {};
        if (View.IsValid()) { View->_IsReloading = false; }
    }
};

auto FCkUiView::Create(FNativeBindings InBindings, FActions InActions, FTokens InTokens, FSlateFontInfo InBaseFont, FDataBindings InData, TSharedPtr<const FCkUiWidgetRegistrySnapshot> InCustomRegistry) -> TSharedRef<FCkUiView>
{
    return MakeShareable(new FCkUiView(MoveTemp(InBindings), MoveTemp(InActions), MoveTemp(InTokens), MoveTemp(InBaseFont), MoveTemp(InData), MoveTemp(InCustomRegistry)));
}

FCkUiView::FCkUiView(FNativeBindings InBindings, FActions InActions, FTokens InTokens, FSlateFontInfo InBaseFont, FDataBindings InData, TSharedPtr<const FCkUiWidgetRegistrySnapshot> InCustomRegistry)
    : _Bindings(MoveTemp(InBindings))
    , _Actions(MoveTemp(InActions))
    , _Tokens(MoveTemp(InTokens))
    , _BaseFont(MoveTemp(InBaseFont))
    , _Data(MoveTemp(InData))
    , _CustomRegistry(MoveTemp(InCustomRegistry))
{
    if (_BaseFont.Size <= 0.0f)
    { _BaseFont = FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10); }
    _LastResult.Succeeded = true;
}

auto FCkUiView::GetRegion(const FString& InName) -> TSharedRef<SWidget>
{
    if (const TSharedPtr<SBox>* Existing = _RegionMounts.Find(InName); Existing != nullptr && Existing->IsValid())
    {
        return Existing->ToSharedRef();
    }
    if (_Revision > 0)
    {
        ensureMsgf(false, TEXT("Named UI regions must be registered before the first committed document."));
        return SNullWidget::NullWidget;
    }

    const auto Mount = SNew(ck_ui_surface::SCkUiRegion).Tag(FName(*InName));
    _RegionMounts.Add(InName, Mount);
    return Mount;
}

auto FCkUiView::FindRetained(const FString& InId) const -> const FRetainedRecord*
{
    if (const FRetainedRecord* Local = _CommittedRetained.Find(InId)) { return Local; }
    FString ContainerId;
    for (const auto& Pair : _CustomSlots)
    {
        if (Pair.Key.Len() > ContainerId.Len() && InId.StartsWith(Pair.Key + TEXT("/"), ESearchCase::CaseSensitive))
        { ContainerId = Pair.Key; }
    }
    if (ContainerId.IsEmpty()) { return nullptr; }
    int32 Separator = INDEX_NONE;
    const FString Remainder = InId.Mid(ContainerId.Len() + 1);
    if (!Remainder.FindChar(TEXT('/'), Separator)) { return nullptr; }
    const FString SlotName = Remainder.Left(Separator);
    const FString ChildId = Remainder.Mid(Separator + 1);
    if (SlotName.IsEmpty() || ChildId.IsEmpty()) { return nullptr; }
    const TSharedPtr<FCustomSlotSet>* Set = _CustomSlots.Find(ContainerId);
    if (Set == nullptr || !Set->IsValid()) { return nullptr; }
    const TSharedPtr<FCustomSlot>* Slot = (*Set)->Slots.Find(SlotName);
    if (Slot == nullptr || !Slot->IsValid() || !(*Slot)->View.IsValid()
        || !(*Slot)->Scope.IsValid() || !(*Slot)->Scope->Active) { return nullptr; }
    return (*Slot)->View->FindRetained(ChildId);
}

auto FCkUiView::GetTable(const FString& InId) const -> TSharedPtr<SCkUiTable>
{
    const FRetainedRecord* Record = FindRetained(InId);
    return Record != nullptr && Record->Kind == ERetainedKind::Table ? Record->Table : nullptr;
}

auto FCkUiView::GetTree(const FString& InId) const -> TSharedPtr<SCkUiTree>
{
    const FRetainedRecord* Record = FindRetained(InId);
    return Record != nullptr && Record->Kind == ERetainedKind::Tree ? StaticCastSharedPtr<SCkUiTree>(Record->Widget) : nullptr;
}

auto FCkUiView::GetScroll(const FString& InId) const -> TSharedPtr<SScrollBox>
{
    const FRetainedRecord* Record = FindRetained(InId);
    return Record != nullptr && Record->Kind == ERetainedKind::Scroll ? StaticCastSharedPtr<SScrollBox>(Record->Widget) : nullptr;
}

auto FCkUiView::GetSplitter(const FString& InId) const -> TSharedPtr<SCkUiSplitter>
{
    const FRetainedRecord* Record = FindRetained(InId);
    return Record != nullptr && Record->Kind == ERetainedKind::Splitter ? StaticCastSharedPtr<SCkUiSplitter>(Record->Widget) : nullptr;
}

auto FCkUiView::GetTabs(const FString& InId) const -> TSharedPtr<SCkUiTabs>
{
    const FRetainedRecord* Record = FindRetained(InId);
    return Record != nullptr && Record->Kind == ERetainedKind::Tabs ? StaticCastSharedPtr<SCkUiTabs>(Record->Widget) : nullptr;
}

auto FCkUiView::GetMenuButton(const FString& InId) const -> TSharedPtr<SCkUiMenuButton>
{
    const FRetainedRecord* Record = FindRetained(InId);
    return Record != nullptr && Record->Kind == ERetainedKind::MenuButton ? StaticCastSharedPtr<SCkUiMenuButton>(Record->Widget) : nullptr;
}

auto FCkUiView::GetRepeat(const FString& InId) const -> TSharedPtr<SCkUiRepeat>
{
    const FRetainedRecord* Record = FindRetained(InId);
    return Record != nullptr && Record->Kind == ERetainedKind::Repeat ? StaticCastSharedPtr<SCkUiRepeat>(Record->Widget) : nullptr;
}

auto FCkUiView::CanDispatchEvents() const -> bool
{
    return !_IsReloading && (!_Data.CanDispatchEvents.IsSet() || _Data.CanDispatchEvents.Get(true));
}

auto FCkUiView::BuildMenuEntries(const TMap<FString, FCkUiMenu>& InMenus, const FString& InMenuId,
    TOptional<FString> InContextKey) const -> TArray<FCkUiMenuSession::FEntry>
{
    const TWeakPtr<FCkUiView> WeakOwner = const_cast<FCkUiView*>(this)->AsShared();
    const auto MakeText = [WeakOwner](const FString& Literal, const FString& Binding) -> TAttribute<FText>
    {
        if (Binding.IsEmpty()) { return FText::FromString(Literal); }
        return TAttribute<FText>::CreateLambda([WeakOwner, Binding]()
        {
            const TSharedPtr<FCkUiView> Owner = WeakOwner.Pin();
            return Owner.IsValid() ? Owner->_Data.Text.FindRef(Binding).Get(FText::GetEmpty()) : FText::GetEmpty();
        });
    };
    const auto MakeBool = [WeakOwner](const FString& Binding) -> TAttribute<bool>
    {
        if (Binding.IsEmpty()) { return true; }
        return TAttribute<bool>::CreateLambda([WeakOwner, Binding]()
        {
            const TSharedPtr<FCkUiView> Owner = WeakOwner.Pin();
            return Owner.IsValid() && Owner->_Data.Visibility.FindRef(Binding).Get(false);
        });
    };
    TArray<FCkUiMenuSession::FEntry> Entries;
    const FCkUiMenu* Menu = InMenus.Find(InMenuId);
    if (Menu == nullptr) { return Entries; }
    for (const FCkUiMenuEntry& Definition : Menu->Entries)
    {
        FCkUiMenuSession::FEntry Entry;
        Entry.Key = Definition.Key;
        Entry.Label = MakeText(Definition.Label, Definition.LabelBinding);
        Entry.Tooltip = MakeText(Definition.Tooltip, Definition.TooltipBinding);
        Entry.Enabled = MakeBool(Definition.EnabledBinding);
        Entry.Visible = MakeBool(Definition.VisibilityBinding);
        Entry.Separator = Definition.Kind == ECkUiMenuEntryKind::Separator;
        if (Definition.Kind == ECkUiMenuEntryKind::Submenu)
        { Entry.Children = BuildMenuEntries(InMenus, Definition.MenuReference, InContextKey); }
        if (Definition.Kind == ECkUiMenuEntryKind::Item)
        {
            Entry.Action = FSimpleDelegate::CreateLambda([WeakOwner, ActionName = Definition.Action, InContextKey]()
            {
                const TSharedPtr<FCkUiView> Owner = WeakOwner.Pin();
                if (!Owner.IsValid() || !Owner->CanDispatchEvents()) { return; }
                if (InContextKey.IsSet())
                {
                    const FOnCkUiContextAction Action = Owner->_Data.ContextActions.FindRef(ActionName);
                    Action.ExecuteIfBound(InContextKey.GetValue());
                }
                else
                {
                    const FSimpleDelegate Action = Owner->_Actions.FindRef(ActionName);
                    Action.ExecuteIfBound();
                }
            });
        }
        Entries.Add(MoveTemp(Entry));
    }
    return Entries;
}

auto FCkUiView::MakeContextMenuBinding(const FCkUiNode& InNode, const TMap<FString, FCkUiMenu>& InMenus) const -> FOnCkUiContextMenuOpening
{
    if (InNode.ContextMenuReference.IsEmpty()) { return {}; }
    const TWeakPtr<FCkUiView> WeakOwner = const_cast<FCkUiView*>(this)->AsShared();
    const TSharedRef<const TMap<FString, FCkUiMenu>> Menus = MakeShared<TMap<FString, FCkUiMenu>>(InMenus);
    const TWeakPtr<FCkUiCollection> WeakCollection = _Data.Collections.FindRef(InNode.Binding);
    const TWeakPtr<FCkUiTreeCollection> WeakTree = _Data.Trees.FindRef(InNode.Binding);
    return FOnCkUiContextMenuOpening::CreateLambda([WeakOwner, Menus, WeakCollection, WeakTree,
        MenuId = InNode.ContextMenuReference, IsTree = InNode.Kind == ECkUiNodeKind::Tree](const FString& Key) -> TSharedPtr<FCkUiMenuSession>
    {
        const TSharedPtr<FCkUiView> Owner = WeakOwner.Pin();
        if (!Owner.IsValid() || !Owner->CanDispatchEvents()) { return {}; }
        const TSharedPtr<FCkUiCollection> Collection = WeakCollection.Pin();
        const TSharedPtr<FCkUiTreeCollection> Tree = WeakTree.Pin();
        const TWeakPtr<const FCkUiRecord> Record = Collection.IsValid() ? Collection->FindRecord(Key) : nullptr;
        const TWeakPtr<const FCkUiTreeNode> Node = Tree.IsValid() ? Tree->FindNode(Key) : nullptr;
        if (IsTree ? !Node.IsValid() : !Record.IsValid()) { return {}; }
        const int64 Revision = Owner->_Revision;
        const TAttribute<bool> CanDispatch = TAttribute<bool>::CreateLambda([WeakOwner, WeakCollection, WeakTree, Record, Node, Key, IsTree, Revision]()
        {
            const TSharedPtr<FCkUiView> CurrentOwner = WeakOwner.Pin();
            if (!CurrentOwner.IsValid() || !CurrentOwner->CanDispatchEvents() || CurrentOwner->_Revision != Revision) { return false; }
            if (IsTree)
            {
                const TSharedPtr<FCkUiTreeCollection> CurrentTree = WeakTree.Pin();
                return Node.IsValid() && CurrentTree.IsValid() && CurrentTree->FindNode(Key) == Node.Pin();
            }
            const TSharedPtr<FCkUiCollection> CurrentCollection = WeakCollection.Pin();
            return Record.IsValid() && CurrentCollection.IsValid() && CurrentCollection->FindRecord(Key) == Record.Pin();
        });
        const TSharedRef<FCkUiMenuSession> Session = MakeShared<FCkUiMenuSession>();
        Session->Configure(Owner->BuildMenuEntries(*Menus, MenuId, Key), true, CanDispatch);
        return Session;
    });
}

auto FCkUiView::ValidateDocument(const FCkUiDocument& InDocument, const FString& InSource, TArray<FString>& OutErrors) const -> bool
{
    if (_Data.SlateUserIndex < INDEX_NONE)
    {
        OutErrors.Add(ck_ui_surface::Error(InSource, TEXT("Slate user context must be INDEX_NONE or a nonnegative user index.")));
        return false;
    }
    for (const auto& [Id, Menu] : InDocument.Menus)
    {
        for (const FCkUiMenuEntry& Entry : Menu.Entries)
        {
            for (const FString& Binding : {Entry.LabelBinding, Entry.TooltipBinding})
            {
                if (!Binding.IsEmpty() && !_Data.Text.FindRef(Binding).IsSet())
                { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Menu '%s' references missing text binding '%s'."), *Id, *Binding))); }
            }
            for (const FString& Binding : {Entry.EnabledBinding, Entry.VisibilityBinding})
            {
                if (!Binding.IsEmpty() && !_Data.Visibility.FindRef(Binding).IsSet())
                { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Menu '%s' references missing boolean binding '%s'."), *Id, *Binding))); }
            }
            if (Entry.Kind == ECkUiMenuEntryKind::Item && !_Actions.FindRef(Entry.Action).IsBound() && !_Data.ContextActions.FindRef(Entry.Action).IsBound())
            { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Menu '%s' references missing action '%s'."), *Id, *Entry.Action))); }
        }
    }
    if (InDocument.Regions.Num() != _RegionMounts.Num())
    {
        OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Document defines %d regions but %d retained mounts were requested."), InDocument.Regions.Num(), _RegionMounts.Num())));
    }
    for (const auto& [Name, Mount] : _RegionMounts)
    {
        if (!InDocument.Regions.Contains(Name)) { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Document is missing requested region '%s'."), *Name))); }
        if (!Mount.IsValid()) { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Retained mount '%s' is invalid."), *Name))); }
    }
    for (const auto& [Name, Node] : InDocument.Regions)
    {
        if (!_RegionMounts.Contains(Name)) { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Document declares unknown region '%s'."), *Name))); }
        const auto& Style = Node.Style;
        if (Style.Grow != 0.0f || Style.Shrink != 0.0f || Style.MinWidth != 0.0f || Style.MinHeight != 0.0f
            || Style.MaxWidth.IsSet() || Style.MaxHeight.IsSet() || Style.HAlign != HAlign_Fill || Style.VAlign != VAlign_Fill)
        {
            OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(
                TEXT("Region root '%s' is sized by its native mount; sizing, growth and alignment belong on a child node."), *Node.Id)));
        }
    }

    auto SeenIds = TSet<FString>{};
    auto SeenBindings = TSet<FString>{};
    TMap<FString, FRetainedRecord> CandidateRetained;
    TFunction<void(const FCkUiNode&, const FString&)> ValidateNode;
    ValidateNode = [this, &InDocument, &InSource, &OutErrors, &SeenIds, &SeenBindings, &CandidateRetained, &ValidateNode](const FCkUiNode& Node, const FString& Location)
    {
        if (Node.Id.IsEmpty()) { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("%s has no node id."), *Location))); }
        else if (SeenIds.Contains(Node.Id)) { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Node id '%s' is not unique."), *Node.Id))); }
        else { SeenIds.Add(Node.Id); }
        if (!ck_ui_surface::IsStyleValid(Node.Style)) { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Node '%s' has invalid style values."), *Node.Id))); }
        if (Node.Kind == ECkUiNodeKind::Repeat)
        {
            const auto Collection = _Data.Collections.FindRef(Node.Binding);
            if (!Collection.IsValid() || Node.Children.Num() != 1)
            { OutErrors.Add(ck_ui_surface::Error(InSource, TEXT("Repeat requires a collection and exactly one item root."))); return; }
            if (!Node.VisibilityBinding.IsEmpty() && !_Data.Visibility.FindRef(Node.VisibilityBinding).IsSet())
            { OutErrors.Add(ck_ui_surface::Error(InSource, TEXT("Repeat visibility requires its boolean binding."))); return; }
            FCkUiDocument ItemDocument;
            MakeRepeatItem(Node.Children[0], Collection, {}, MakeShared<FRepeatScope>(), {}, ItemDocument, OutErrors);
            CandidateRetained.Add(Node.Id, FRetainedRecord{Node.Id, ERetainedKind::Repeat, Node.Binding});
            return;
        }
        if (!Node.ContextMenuReference.IsEmpty() && (Node.Kind != ECkUiNodeKind::Table && Node.Kind != ECkUiNodeKind::Tree))
        { OutErrors.Add(ck_ui_surface::Error(InSource, TEXT("Only tables and trees support context-menu."))); }
        if (!Node.ContextMenuReference.IsEmpty() && !Node.ContextMenuAction.IsEmpty())
        { OutErrors.Add(ck_ui_surface::Error(InSource, TEXT("context-menu cannot be combined with context-menu-action."))); }
        if (Node.Kind == ECkUiNodeKind::MenuButton || !Node.ContextMenuReference.IsEmpty())
        {
            const bool IsContext = Node.Kind != ECkUiNodeKind::MenuButton;
            TArray<FString> Pending{IsContext ? Node.ContextMenuReference : Node.MenuReference};
            TSet<FString> Visited;
            while (!Pending.IsEmpty())
            {
                const FString MenuId = Pending.Pop();
                if (Visited.Contains(MenuId)) { continue; }
                Visited.Add(MenuId);
                const FCkUiMenu* Menu = InDocument.Menus.Find(MenuId);
                if (Menu == nullptr)
                { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Node '%s' references unknown menu '%s'."), *Node.Id, *MenuId))); continue; }
                for (const FCkUiMenuEntry& Entry : Menu->Entries)
                {
                    if (Entry.Kind == ECkUiMenuEntryKind::Submenu) { Pending.Add(Entry.MenuReference); }
                    if (Entry.Kind == ECkUiMenuEntryKind::Item
                        && !(IsContext ? _Data.ContextActions.FindRef(Entry.Action).IsBound() : _Actions.FindRef(Entry.Action).IsBound()))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Node '%s' requires a %s action '%s'."), *Node.Id, IsContext ? TEXT("context") : TEXT("command"), *Entry.Action))); }
                }
            }
        }
        if (Node.Style.FlexWrap != ECkFlexWrap::NoWrap && Node.Kind != ECkUiNodeKind::Row && Node.Kind != ECkUiNodeKind::Column && Node.Kind != ECkUiNodeKind::Repeat)
        { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Node '%s': flex-wrap is only valid on row, column and repeat nodes."), *Node.Id))); }
        if (Node.Kind == ECkUiNodeKind::Repeat && Node.RepeatDirection != Orient_Horizontal && Node.RepeatDirection != Orient_Vertical)
        { OutErrors.Add(TEXT("Repeat direction must be horizontal or vertical.")); }

        const FCkUiBuiltinWidgetSchema* Schema = ck_ui_widget_registry::FindByKind(Node.Kind);
        if (Schema == nullptr && Node.Kind != ECkUiNodeKind::Custom)
        {
            OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Node '%s' has an unsupported node kind."), *Node.Id)));
        }
        else if (Schema != nullptr)
        {
            switch (Schema->BindingKind)
            {
            case ECkUiBuiltinBindingKind::Native:
                if (Node.Binding.IsEmpty()) { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Native node '%s' has no binding."), *Node.Id))); }
                else if (!_Bindings.Contains(Node.Binding) || !_Bindings.FindRef(Node.Binding).IsValid()) { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Native node '%s' references unknown binding '%s'."), *Node.Id, *Node.Binding))); }
                else if (SeenBindings.Contains(Node.Binding)) { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Native binding '%s' is used more than once."), *Node.Binding))); }
                else { SeenBindings.Add(Node.Binding); CandidateRetained.Add(Node.Id, FRetainedRecord{Node.Id, ERetainedKind::Native, Node.Binding}); }
                break;
            case ECkUiBuiltinBindingKind::OptionalText:
                if (Node.FieldBindings.Contains(TEXT("bind"))) { break; }
                if (!Node.Binding.IsEmpty() && (!_Data.Text.Contains(Node.Binding) || !_Data.Text.FindRef(Node.Binding).IsSet()))
                { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Text node '%s' references missing text binding '%s'."), *Node.Id, *Node.Binding))); }
                if (!Node.Binding.IsEmpty() && !Node.Text.IsEmpty())
                { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Text node '%s' cannot combine literal text with a binding."), *Node.Id))); }
                break;
            case ECkUiBuiltinBindingKind::Image:
                if (Node.FieldBindings.Contains(TEXT("bind"))) { break; }
                if (!_Data.Images.Contains(Node.Binding) || !_Data.Images.FindRef(Node.Binding).IsSet())
                { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Image node '%s' references missing image binding '%s'."), *Node.Id, *Node.Binding))); }
                break;
            case ECkUiBuiltinBindingKind::SearchText:
                if (!_Data.Text.Contains(Node.Binding) || !_Data.Text.FindRef(Node.Binding).IsSet()
                    || !_Data.TextChanged.Contains(Node.Binding) || !_Data.TextChanged.FindRef(Node.Binding).IsBound())
                { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Search node '%s' requires both text and text-changed bindings named '%s'."), *Node.Id, *Node.Binding))); }
                else { CandidateRetained.Add(Node.Id, FRetainedRecord{Node.Id, ERetainedKind::Search, Node.Binding}); }
                break;
            case ECkUiBuiltinBindingKind::Collection:
                if (Node.Binding.IsEmpty() || !_Data.Collections.Contains(Node.Binding) || !_Data.Collections.FindRef(Node.Binding).IsValid())
                { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Table node '%s' references missing collection '%s'."), *Node.Id, *Node.Binding))); }
                else
                {
                    CandidateRetained.Add(Node.Id, FRetainedRecord{Node.Id, ERetainedKind::Table, Node.Binding});
                    if (!Node.FilterBinding.IsEmpty() && (!_Data.Text.Contains(Node.FilterBinding) || !_Data.Text.FindRef(Node.FilterBinding).IsSet()))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Table node '%s' references missing filter text '%s'."), *Node.Id, *Node.FilterBinding))); }
                    if (!Node.SelectionAction.IsEmpty() && (!_Data.TableSelectionChanged.Contains(Node.SelectionAction) || !_Data.TableSelectionChanged.FindRef(Node.SelectionAction).IsBound()))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Table node '%s' references missing selection callback '%s'."), *Node.Id, *Node.SelectionAction))); }
                    if (!Node.ContextMenuAction.IsEmpty() && (!_Data.TableContextMenus.Contains(Node.ContextMenuAction) || !_Data.TableContextMenus.FindRef(Node.ContextMenuAction).IsBound()))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Table node '%s' references missing context menu callback '%s'."), *Node.Id, *Node.ContextMenuAction))); }
                }
                break;
            case ECkUiBuiltinBindingKind::TreeCollection:
                if (!_Data.Trees.FindRef(Node.Binding).IsValid())
                { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Tree node '%s' references missing tree '%s'."), *Node.Id, *Node.Binding))); }
                else
                {
                    CandidateRetained.Add(Node.Id, FRetainedRecord{Node.Id, ERetainedKind::Tree, Node.Binding});
                    if (!Node.FilterBinding.IsEmpty() && (!_Data.Text.Contains(Node.FilterBinding) || !_Data.Text.FindRef(Node.FilterBinding).IsSet()))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Tree node '%s' references missing filter text '%s'."), *Node.Id, *Node.FilterBinding))); }
                    if (!Node.SelectionAction.IsEmpty() && (!_Data.TreeSelectionChanged.Contains(Node.SelectionAction) || !_Data.TreeSelectionChanged.FindRef(Node.SelectionAction).IsBound()))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Tree node '%s' references missing selection callback '%s'."), *Node.Id, *Node.SelectionAction))); }
                    if (!Node.ContextMenuAction.IsEmpty())
                    { OutErrors.Add(ck_ui_surface::Error(InSource, TEXT("Tree context menus are not yet supported."))); }
                }
                break;
            case ECkUiBuiltinBindingKind::None:
                if (!Node.Binding.IsEmpty() && Node.Kind != ECkUiNodeKind::Tabs && Node.Kind != ECkUiNodeKind::MenuButton)
                { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Non-native node '%s' declares binding '%s'."), *Node.Id, *Node.Binding))); }
                break;
            }
        }

        if (Node.Kind == ECkUiNodeKind::Text || Node.Kind == ECkUiNodeKind::Button)
        {
            if (!Node.ColorBinding.IsEmpty() && (!_Data.Color.Contains(Node.ColorBinding) || !_Data.Color.FindRef(Node.ColorBinding).IsSet()))
            { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Text '%s' references missing color binding '%s'."), *Node.Id, *Node.ColorBinding))); }
            if (!Node.TooltipBinding.IsEmpty() && (!_Data.Text.Contains(Node.TooltipBinding) || !_Data.Text.FindRef(Node.TooltipBinding).IsSet()))
            { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Text '%s' references missing tooltip binding '%s'."), *Node.Id, *Node.TooltipBinding))); }
        }
        if (Node.Kind == ECkUiNodeKind::Search && !Node.PlaceholderBinding.IsEmpty()
            && (!_Data.Text.Contains(Node.PlaceholderBinding) || !_Data.Text.FindRef(Node.PlaceholderBinding).IsSet()))
        { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Search node '%s' references missing placeholder text '%s'."), *Node.Id, *Node.PlaceholderBinding))); }
        if (Node.Kind == ECkUiNodeKind::TableColumn && !Node.HeaderBinding.IsEmpty()
            && (!_Data.Text.Contains(Node.HeaderBinding) || !_Data.Text.FindRef(Node.HeaderBinding).IsSet()))
        { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Table column '%s' references missing header text '%s'."), *Node.Id, *Node.HeaderBinding))); }
        if (Node.Kind == ECkUiNodeKind::Custom)
        {
            const FCkUiCustomWidgetRegistration* Registration = _CustomRegistry.IsValid() ? _CustomRegistry->Find(Node.CustomTag) : nullptr;
            if (Registration == nullptr)
            { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Custom node '%s' references unregistered tag '%s'."), *Node.Id, *Node.CustomTag))); }
            else
            {
                auto KnownProperties = TSet<FString>{};
                for (const FCkUiCustomPropertySchema& Property : Registration->Schema.Properties)
                {
                    KnownProperties.Add(Property.Name);
                    if (Node.FieldBindings.Contains(Property.Name)) { continue; }
                    const FCkUiCustomPropertyValue* Value = Node.CustomProperties.Find(Property.Name);
                    if (Value == nullptr)
                    {
                        if (Property.bRequired) { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Custom node '%s' is missing property '%s'."), *Node.Id, *Property.Name))); }
                        continue;
                    }
                    if (Value->Kind != Property.Kind)
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Custom node '%s' has invalid property type for '%s'."), *Node.Id, *Property.Name))); continue; }
                    if (Property.Kind == ECkUiCustomPropertyKind::Number && !FMath::IsFinite(Value->Number))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Custom node '%s' has non-finite number property '%s'."), *Node.Id, *Property.Name))); }
                    else if (Property.Kind == ECkUiCustomPropertyKind::Color && (!FMath::IsFinite(Value->Color.R) || !FMath::IsFinite(Value->Color.G) || !FMath::IsFinite(Value->Color.B) || !FMath::IsFinite(Value->Color.A)))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Custom node '%s' has non-finite color property '%s'."), *Node.Id, *Property.Name))); }
                    if (Property.Kind == ECkUiCustomPropertyKind::TextBinding && (!_Data.Text.Contains(Value->Name) || !_Data.Text.FindRef(Value->Name).IsSet()))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Custom node '%s' references missing text binding '%s'."), *Node.Id, *Value->Name))); }
                    else if (Property.Kind == ECkUiCustomPropertyKind::StringBinding && (!_Data.String.Contains(Value->Name) || !_Data.String.FindRef(Value->Name).IsSet()))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Custom node '%s' references missing string binding '%s'."), *Node.Id, *Value->Name))); }
                    else if (Property.Kind == ECkUiCustomPropertyKind::CollectionBinding && (!_Data.Collections.Contains(Value->Name) || !_Data.Collections.FindRef(Value->Name).IsValid()))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Custom node '%s' references missing collection binding '%s'."), *Node.Id, *Value->Name))); }
                    else if (Property.Kind == ECkUiCustomPropertyKind::StringChanged && (!_Data.StringChanged.Contains(Value->Name) || !_Data.StringChanged.FindRef(Value->Name).IsBound()))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Custom node '%s' references missing string-changed callback '%s'."), *Node.Id, *Value->Name))); }
                    else if (Property.Kind == ECkUiCustomPropertyKind::ImageBinding && (!_Data.Images.Contains(Value->Name) || !_Data.Images.FindRef(Value->Name).IsSet()))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Custom node '%s' references missing image binding '%s'."), *Node.Id, *Value->Name))); }
                    else if (Property.Kind == ECkUiCustomPropertyKind::NumberBinding && (!_Data.Number.Contains(Value->Name) || !_Data.Number.FindRef(Value->Name).IsSet()))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Custom node '%s' references missing number binding '%s'."), *Node.Id, *Value->Name))); }
                    else if (Property.Kind == ECkUiCustomPropertyKind::BoolBinding && (!_Data.Visibility.Contains(Value->Name) || !_Data.Visibility.FindRef(Value->Name).IsSet()))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Custom node '%s' references missing bool binding '%s'."), *Node.Id, *Value->Name))); }
                    else if (Property.Kind == ECkUiCustomPropertyKind::ColorBinding && (!_Data.Color.Contains(Value->Name) || !_Data.Color.FindRef(Value->Name).IsSet()))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Custom node '%s' references missing color binding '%s'."), *Node.Id, *Value->Name))); }
                    else if (Property.Kind == ECkUiCustomPropertyKind::Action && (!_Actions.Contains(Value->Name) || !_Actions.FindRef(Value->Name).IsBound()))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Custom node '%s' references missing action '%s'."), *Node.Id, *Value->Name))); }
                    else if (Property.Kind == ECkUiCustomPropertyKind::TextChanged && (!_Data.TextChanged.Contains(Value->Name) || !_Data.TextChanged.FindRef(Value->Name).IsBound()))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Custom node '%s' references missing text-changed callback '%s'."), *Node.Id, *Value->Name))); }
                    else if (Property.Kind == ECkUiCustomPropertyKind::TextCommitted && (!_Data.TextCommitted.Contains(Value->Name) || !_Data.TextCommitted.FindRef(Value->Name).IsBound()))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Custom node '%s' references missing text-committed callback '%s'."), *Node.Id, *Value->Name))); }
                    else if (Property.Kind == ECkUiCustomPropertyKind::BoolChanged && (!_Data.BoolChanged.Contains(Value->Name) || !_Data.BoolChanged.FindRef(Value->Name).IsBound()))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Custom node '%s' references missing bool-changed callback '%s'."), *Node.Id, *Value->Name))); }
                    else if (Property.Kind == ECkUiCustomPropertyKind::NumberChanged && (!_Data.NumberChanged.Contains(Value->Name) || !_Data.NumberChanged.FindRef(Value->Name).IsBound()))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Custom node '%s' references missing number-changed callback '%s'."), *Node.Id, *Value->Name))); }
                    else if (Property.Kind == ECkUiCustomPropertyKind::NumberCommitted && (!_Data.NumberCommitted.Contains(Value->Name) || !_Data.NumberCommitted.FindRef(Value->Name).IsBound()))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Custom node '%s' references missing number-committed callback '%s'."), *Node.Id, *Value->Name))); }
                    else if (Property.Kind == ECkUiCustomPropertyKind::NumberInteraction && (!_Data.NumberInteraction.Contains(Value->Name) || !_Data.NumberInteraction.FindRef(Value->Name).IsBound()))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Custom node '%s' references missing number-interaction callback '%s'."), *Node.Id, *Value->Name))); }
                }
                for (const auto& [Name, Value] : Node.CustomProperties)
                {
                    if (!KnownProperties.Contains(Name))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Custom node '%s' declares unknown property '%s'."), *Node.Id, *Name))); }
                }
                if (Registration->RetainedFactory)
                {
                    FString StateKey;
                    if (!Registration->Schema.StateKeyProperty.IsEmpty())
                    {
                        const FCkUiCustomPropertyValue* Value = Node.CustomProperties.Find(Registration->Schema.StateKeyProperty);
                        if (Value == nullptr || Value->Kind != ECkUiCustomPropertyKind::Text || Value->Text.IsEmpty())
                        { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Custom node '%s' requires a non-empty state key '%s'."), *Node.Id, *Registration->Schema.StateKeyProperty))); }
                        else { StateKey = Value->Text.ToString(); }
                    }
                    CandidateRetained.Add(Node.Id, FRetainedRecord{Node.Id, ERetainedKind::Custom, StateKey, Node.CustomTag});
                }
            }
            TSet<FString> SuppliedSlots;
            if (Registration != nullptr)
            {
                const auto Previous = _CustomSlots.FindRef(Node.Id);
                for (const auto& Child : Node.Children)
                {
                    if (Child.CustomSlotName.IsEmpty() || SuppliedSlots.Contains(Child.CustomSlotName)
                        || !Registration->Schema.Slots.ContainsByPredicate([&Child](const auto& Slot) { return Slot.Name == Child.CustomSlotName; }))
                    { OutErrors.Add(TEXT("Custom child has an unknown or duplicate slot.")); continue; }
                    SuppliedSlots.Add(Child.CustomSlotName);
                    const auto ExistingSlot = Previous.IsValid() ? Previous->Slots.FindRef(Child.CustomSlotName) : nullptr;
                    const auto Slot = ExistingSlot.IsValid() && ExistingSlot->View.IsValid() ? ExistingSlot : MakeShared<FCustomSlot>();
                    if (!Slot->Scope.IsValid()) { Slot->Scope = MakeShared<FRepeatScope>(); }
                    FCkUiDocument ChildDocument;
                    MakeCustomSlotView(Child, Slot, InDocument.Menus, ChildDocument, OutErrors);
                }
                for (const auto& Slot : Registration->Schema.Slots)
                { if (Slot.bRequired && !SuppliedSlots.Contains(Slot.Name)) { OutErrors.Add(TEXT("Custom node is missing a required slot.")); } }
            }
        }

        if (Node.Kind == ECkUiNodeKind::Scroll)
        {
            if (Node.ScrollDirection != Orient_Horizontal && Node.ScrollDirection != Orient_Vertical)
            { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Scroll '%s' has invalid direction."), *Node.Id))); }
            CandidateRetained.Add(Node.Id, FRetainedRecord{Node.Id, ERetainedKind::Scroll,
                Node.ScrollDirection == Orient_Horizontal ? TEXT("horizontal") : TEXT("vertical")});
        }

        if (Node.Kind == ECkUiNodeKind::MenuButton)
        {
            if (!InDocument.Menus.Contains(Node.MenuReference))
            { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Menu button '%s' references unknown menu '%s'."), *Node.Id, *Node.MenuReference))); }
            if (!Node.Binding.IsEmpty() && !_Data.Text.FindRef(Node.Binding).IsSet())
            { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Menu button '%s' requires its label binding."), *Node.Id))); }
            if (!Node.TabEnabledBinding.IsEmpty() && !_Data.Visibility.FindRef(Node.TabEnabledBinding).IsSet())
            { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Menu button '%s' requires its enabled binding."), *Node.Id))); }
            CandidateRetained.Add(Node.Id, FRetainedRecord{Node.Id, ERetainedKind::MenuButton, TEXT("menu-button")});
        }

        if (Node.Kind == ECkUiNodeKind::Button && !Node.ButtonEnabledBinding.IsEmpty()
            && (!_Data.Visibility.Contains(Node.ButtonEnabledBinding) || !_Data.Visibility.FindRef(Node.ButtonEnabledBinding).IsSet()))
        { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Button '%s' references missing enabled binding '%s'."), *Node.Id, *Node.ButtonEnabledBinding))); }

        if (Node.Kind == ECkUiNodeKind::Tabs)
        {
            if (Node.Binding.IsEmpty() || !_Data.String.FindRef(Node.Binding).IsSet()
                || Node.Action.IsEmpty() || !_Data.StringChanged.FindRef(Node.Action).IsBound())
            { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Tabs '%s' requires string value and changed bindings."), *Node.Id))); }
            CandidateRetained.Add(Node.Id, FRetainedRecord{Node.Id, ERetainedKind::Tabs, Node.Binding});
        }
        if (Node.Kind == ECkUiNodeKind::Tab)
        {
            if (!Node.TabLabelBinding.IsEmpty() && !_Data.Text.FindRef(Node.TabLabelBinding).IsSet())
            { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Tab '%s' references missing label binding."), *Node.Id))); }
            if (!Node.TabEnabledBinding.IsEmpty() && !_Data.Visibility.FindRef(Node.TabEnabledBinding).IsSet())
            { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Tab '%s' references missing enabled binding."), *Node.Id))); }
        }
        if (Node.Kind == ECkUiNodeKind::Splitter)
        {
            if (Node.SplitterDirection != Orient_Horizontal && Node.SplitterDirection != Orient_Vertical)
            { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Splitter '%s' has invalid direction."), *Node.Id))); }
            CandidateRetained.Add(Node.Id, FRetainedRecord{Node.Id, ERetainedKind::Splitter,
                Node.SplitterDirection == Orient_Horizontal ? TEXT("horizontal") : TEXT("vertical")});
        }

        if (Node.Kind == ECkUiNodeKind::Table || Node.Kind == ECkUiNodeKind::Tree)
        {
            if (!FMath::IsFinite(Node.RowHeight) || Node.RowHeight < 1.0f || Node.RowHeight > 1024.0f)
            { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Collection node '%s' row-height must be finite within 1..1024."), *Node.Id))); }
            const TSharedPtr<FCkUiCollection> Collection = _Data.Collections.FindRef(Node.Binding);
            const TSharedPtr<FCkUiTreeCollection> Tree = _Data.Trees.FindRef(Node.Binding);
            const TArray<FCkUiFieldSchema>* Fields = Node.Kind == ECkUiNodeKind::Table
                ? (Collection.IsValid() ? &Collection->GetSchema() : nullptr)
                : (Tree.IsValid() ? &Tree->GetSchema() : nullptr);
            if (Fields != nullptr)
            {
                const auto FindSchema = [Fields](const FString& InField) -> const FCkUiFieldSchema*
                {
                    for (const FCkUiFieldSchema& Field : *Fields)
                    { if (Field.Name == InField) { return &Field; } }
                    return nullptr;
                };
                TFunction<void(const FCkUiNode&)> ValidateCell;
                ValidateCell = [this, &OutErrors, &InSource, &FindSchema, &ValidateCell](const FCkUiNode& Cell)
                {
                    if (Cell.Kind != ECkUiNodeKind::Row && Cell.Kind != ECkUiNodeKind::Column && Cell.Kind != ECkUiNodeKind::Text && Cell.Kind != ECkUiNodeKind::Image && Cell.Kind != ECkUiNodeKind::Scroll && Cell.Kind != ECkUiNodeKind::Overlay && Cell.Kind != ECkUiNodeKind::Custom)
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Table cell '%s' uses a non-readonly node."), *Cell.Id))); return; }
                    const FCkUiCustomWidgetRegistration* Custom = Cell.Kind == ECkUiNodeKind::Custom && _CustomRegistry.IsValid() ? _CustomRegistry->Find(Cell.CustomTag) : nullptr;
                    if (Cell.Kind == ECkUiNodeKind::Custom && (Custom == nullptr || Custom->RetainedFactory))
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Table cell '%s' requires a registered stateless custom factory."), *Cell.Id))); return; }
                    bool HasEditableEvent = false;
                    if (Custom != nullptr)
                    {
                        for (const auto& [Name, Value] : Cell.CustomProperties)
                        {
                            const FCkUiCustomPropertySchema* Property = Custom->Schema.Properties.FindByPredicate([&Name](const FCkUiCustomPropertySchema& Candidate)
                            { return Candidate.Name == Name; });
                            if (Property != nullptr && (Property->Kind == ECkUiCustomPropertyKind::StringChanged || Property->Kind == ECkUiCustomPropertyKind::TextChanged || Property->Kind == ECkUiCustomPropertyKind::TextCommitted || Property->Kind == ECkUiCustomPropertyKind::BoolChanged || Property->Kind == ECkUiCustomPropertyKind::NumberChanged || Property->Kind == ECkUiCustomPropertyKind::NumberCommitted || Property->Kind == ECkUiCustomPropertyKind::NumberInteraction))
                            { HasEditableEvent = true; break; }
                        }
                    }
                    if (HasEditableEvent)
                    { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Table cell '%s' cannot use editable custom event properties."), *Cell.Id))); return; }
                    for (const auto& [Target, Field] : Cell.FieldBindings)
                    {
                        TOptional<ECkUiFieldKind> Expected;
                        if (Target == TEXT("visible")) { Expected = ECkUiFieldKind::Bool; }
                        else if (Target == TEXT("bind") && Cell.Kind == ECkUiNodeKind::Text) { Expected = ECkUiFieldKind::Text; }
                        else if (Target == TEXT("bind") && Cell.Kind == ECkUiNodeKind::Image) { Expected = ECkUiFieldKind::Image; }
                        else if (Target == TEXT("color") && Cell.Kind == ECkUiNodeKind::Text) { Expected = ECkUiFieldKind::Color; }
                        else if (Target == TEXT("tooltip") && Cell.Kind == ECkUiNodeKind::Text) { Expected = ECkUiFieldKind::Text; }
                        else if (Custom != nullptr)
                        {
                            for (const FCkUiCustomPropertySchema& Property : Custom->Schema.Properties)
                            { if (Property.Name == Target) { Expected = ck_ui_surface::FieldKind(Property.Kind); break; } }
                        }
                        if (!Expected.IsSet())
                        { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Table cell '%s' has unsupported field target '%s'."), *Cell.Id, *Target))); continue; }
                        const FCkUiFieldSchema* Actual = FindSchema(Field);
                        if (Actual == nullptr || Actual->Kind != Expected.GetValue() || !Actual->Required)
                        { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Table cell '%s' field '%s' has the wrong schema type."), *Cell.Id, *Field))); }
                    }
                    for (const FCkUiNode& Child : Cell.Children) { ValidateCell(Child); }
                };
                for (const FCkUiNode& Column : Node.Children)
                {
                    if (Node.Kind == ECkUiNodeKind::Tree)
                    {
                        const int32 PreviousErrors = OutErrors.Num();
                        ValidateCell(Column);
                        if (OutErrors.Num() == PreviousErrors)
                        { MakeCellView<FCkUiTreeNode>(Column, {}, true, OutErrors); }
                        continue;
                    }
                    if (!Column.SortField.IsEmpty())
                    { const FCkUiFieldSchema* Sort = FindSchema(Column.SortField); if (Sort == nullptr || (Sort->Kind != ECkUiFieldKind::Text && Sort->Kind != ECkUiFieldKind::Number && Sort->Kind != ECkUiFieldKind::Bool)) { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Table column '%s' sort field '%s' must be a Text, Number, or Bool schema field."), *Column.Id, *Column.SortField))); } }
                    if (!Column.Children.IsEmpty())
                    {
                        const int32 PreviousErrors = OutErrors.Num();
                        ValidateCell(Column.Children[0]);
                        if (OutErrors.Num() == PreviousErrors)
                        { MakeCellView<FCkUiRecord>(Column.Children[0], {}, true, OutErrors); }
                    }
                }
            }
        }

        if (!Node.VisibilityBinding.IsEmpty() && (Node.Kind != ECkUiNodeKind::Custom && (Schema == nullptr || !Schema->bAllowsVisibility)))
        { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Node '%s' may not declare a visibility binding."), *Node.Id))); }
        else if (!Node.VisibilityBinding.IsEmpty() && !Node.FieldBindings.Contains(TEXT("visible")) && (!_Data.Visibility.Contains(Node.VisibilityBinding) || !_Data.Visibility.FindRef(Node.VisibilityBinding).IsSet()))
        { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Node '%s' references missing visibility binding '%s'."), *Node.Id, *Node.VisibilityBinding))); }

        if (Schema != nullptr && Schema->bRequiresAction)
        {
            if (Node.Action.IsEmpty() || !_Actions.Contains(Node.Action) || !_Actions.FindRef(Node.Action).IsBound())
            { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Button '%s' references missing action '%s'."), *Node.Id, *Node.Action))); }
        }
        else if (!Node.Action.IsEmpty() && Node.Kind != ECkUiNodeKind::Tabs)
        {
            OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Only button node '%s' may declare an action."), *Node.Id)));
        }

        if (Schema != nullptr && (Node.Children.Num() < Schema->MinChildren || (Schema->MaxChildren != INDEX_NONE && Node.Children.Num() > Schema->MaxChildren)))
        {
            if (Schema->Kind == ECkUiNodeKind::Scroll)
            { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Scroll node '%s' must contain exactly one child."), *Node.Id))); }
            else if (Schema->Kind == ECkUiNodeKind::Native)
            { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Native node '%s' cannot have children."), *Node.Id))); }
            else
            { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Leaf node '%s' cannot have children."), *Node.Id))); }
        }
        for (const FCkUiNode& Child : Node.Children) { ValidateNode(Child, Node.Id); }
    };
    for (const auto& [Region, Node] : InDocument.Regions) { ValidateNode(Node, FString::Printf(TEXT("Region '%s'"), *Region)); }

    for (const auto& [Binding, Widget] : _Bindings)
    {
        if (!SeenBindings.Contains(Binding)) { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Native binding '%s' is not present exactly once."), *Binding))); }
    }
    for (const auto& [Id, Candidate] : CandidateRetained)
    {
        if (const FRetainedRecord* Existing = _CommittedRetained.Find(Id); Existing != nullptr)
        {
            if (Existing->Kind != Candidate.Kind || Existing->CustomTag != Candidate.CustomTag)
            { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Retained id '%s' cannot change kind."), *Id))); }
            else if (Existing->CompatibilityKey != Candidate.CompatibilityKey && Existing->Kind != ERetainedKind::Repeat)
            {
                if (Existing->Kind == ERetainedKind::Custom)
                { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Custom id '%s' cannot change state key from '%s' to '%s'."), *Id, *Existing->CompatibilityKey, *Candidate.CompatibilityKey))); }
                else if (Existing->Kind == ERetainedKind::Scroll)
                { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Scroll id '%s' cannot change direction."), *Id))); }
                else if (Existing->Kind == ERetainedKind::Splitter)
                { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Splitter id '%s' cannot change direction."), *Id))); }
                else
                {
                    const TCHAR* Label = Existing->Kind == ERetainedKind::Native ? TEXT("Native") : TEXT("Search");
                    OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("%s id '%s' cannot change binding from '%s' to '%s'."), Label, *Id, *Existing->CompatibilityKey, *Candidate.CompatibilityKey)));
                }
            }
        }
    }
    for (const auto& [Id, Existing] : _CommittedRetained)
    {
        if (Existing.Kind == ERetainedKind::Native && !CandidateRetained.Contains(Id))
        { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Committed native id '%s' cannot change kind or disappear."), *Id))); }
        if (Existing.Kind == ERetainedKind::Search && SeenIds.Contains(Id) && !CandidateRetained.Contains(Id))
        { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Committed search id '%s' cannot change kind."), *Id))); }
        if (Existing.Kind == ERetainedKind::Custom && SeenIds.Contains(Id) && !CandidateRetained.Contains(Id))
        { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Committed custom id '%s' cannot change kind."), *Id))); }
        if (Existing.Kind == ERetainedKind::Scroll && SeenIds.Contains(Id) && !CandidateRetained.Contains(Id))
        { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Committed scroll id '%s' cannot change kind."), *Id))); }
        if (Existing.Kind == ERetainedKind::Tabs && SeenIds.Contains(Id) && !CandidateRetained.Contains(Id))
        { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Committed tabs id '%s' cannot change kind."), *Id))); }
        if (Existing.Kind == ERetainedKind::MenuButton && SeenIds.Contains(Id) && !CandidateRetained.Contains(Id))
        { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Committed menu button id '%s' cannot change kind."), *Id))); }
        if (Existing.Kind == ERetainedKind::Splitter && SeenIds.Contains(Id) && !CandidateRetained.Contains(Id))
        { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Committed splitter id '%s' cannot change kind."), *Id))); }
        if ((Existing.Kind == ERetainedKind::Table || Existing.Kind == ERetainedKind::Tree) && SeenIds.Contains(Id) && !CandidateRetained.Contains(Id))
        { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Committed collection id '%s' cannot change kind."), *Id))); }
    }
    auto BindingNames = TArray<FString>{};
    _Bindings.GetKeys(BindingNames);
    for (int32 FirstIndex = 0; FirstIndex < BindingNames.Num(); ++FirstIndex)
    {
        const FString& FirstName = BindingNames[FirstIndex];
        const TSharedPtr<SWidget> FirstWidget = _Bindings.FindRef(FirstName);
        if (!FirstWidget.IsValid()) { continue; }
        const TSharedPtr<SWidget> FirstParent = FirstWidget->GetParentWidget();
        bool FirstParentIsTracked = false;
        for (const auto& [Id, Record] : _CommittedRetained)
        {
            if (Record.Kind == ERetainedKind::Native && Record.Port == FirstParent)
            { FirstParentIsTracked = true; break; }
        }
        if (FirstParent.IsValid() && !FirstParentIsTracked)
        { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Native binding '%s' is already mounted under an unrelated parent."), *FirstName))); }
        for (int32 SecondIndex = FirstIndex + 1; SecondIndex < BindingNames.Num(); ++SecondIndex)
        {
            const FString& SecondName = BindingNames[SecondIndex];
            const TSharedPtr<SWidget> SecondWidget = _Bindings.FindRef(SecondName);
            if (!SecondWidget.IsValid()) { continue; }
            auto Parent = FirstWidget->GetParentWidget();
            bool Aliased = FirstWidget == SecondWidget;
            while (!Aliased && Parent.IsValid())
            {
                Aliased = Parent == SecondWidget;
                Parent = Parent->GetParentWidget();
            }
            Parent = SecondWidget->GetParentWidget();
            while (!Aliased && Parent.IsValid())
            {
                Aliased = Parent == FirstWidget;
                Parent = Parent->GetParentWidget();
            }
            if (Aliased) { OutErrors.Add(ck_ui_surface::Error(InSource, FString::Printf(TEXT("Native bindings '%s' and '%s' alias the same widget hierarchy."), *FirstName, *SecondName))); }
        }
    }
    return OutErrors.IsEmpty();
}

auto FCkUiView::MakePort(const FCkUiNode& InNode, FStagedDocument& InOutStaged) const -> TSharedRef<SWidget>
{
    const TSharedPtr<SWidget> Native = _Bindings.FindRef(InNode.Binding);
    const auto Port = ck_ui_surface::MakeMeasuredPort(Native.ToSharedRef(), InNode.Id);
    InOutStaged.Retained.Add(FRetainedRecord{InNode.Id, ERetainedKind::Native, InNode.Binding, FString{}, Native, nullptr, Port});
    const auto Styled = ApplyStyle(InNode.Style, Port);
    Styled->SetVisibility(TAttribute<EVisibility>::CreateLambda([WeakNative = TWeakPtr<SWidget>(Native)]()
    {
        const TSharedPtr<SWidget> Pinned = WeakNative.Pin();
        return Pinned.IsValid() ? Pinned->GetVisibility() : EVisibility::Collapsed;
    }));
    return Styled;
}

auto FCkUiView::ApplyStyle(const FCkUiStyle& InStyle, const TSharedRef<SWidget>& InContent) const -> TSharedRef<SWidget>
{
    const bool HasConstraints = InStyle.MinWidth > 0.0f || InStyle.MinHeight > 0.0f || InStyle.MaxWidth.IsSet() || InStyle.MaxHeight.IsSet();
    TSharedRef<SWidget> Result = InContent;
    if (HasConstraints)
    {
        const auto MinWidth = InStyle.MinWidth > 0.0f ? FOptionalSize(InStyle.MinWidth) : FOptionalSize();
        const auto MinHeight = InStyle.MinHeight > 0.0f ? FOptionalSize(InStyle.MinHeight) : FOptionalSize();
        const auto MaxWidth = InStyle.MaxWidth.IsSet() ? FOptionalSize(InStyle.MaxWidth.GetValue()) : FOptionalSize();
        const auto MaxHeight = InStyle.MaxHeight.IsSet() ? FOptionalSize(InStyle.MaxHeight.GetValue()) : FOptionalSize();
        Result = SNew(SBox).MinDesiredWidth(MinWidth).MinDesiredHeight(MinHeight).MaxDesiredWidth(MaxWidth).MaxDesiredHeight(MaxHeight)[Result];
    }
    if (InStyle.Background.IsSet())
    {
        Result = SNew(SBorder).Tag(InContent->GetTag()).BorderImage(ck_ui_surface::BackgroundBrush())
            .BorderBackgroundColor(InStyle.Background.GetValue()).Padding(FMargin(0.0f))[Result];
    }
    if (&Result.Get() != &InContent.Get())
    {
        if (const TSharedPtr<FCkFlexMeasureMetaData> Measure = InContent->GetMetaData<FCkFlexMeasureMetaData>(); Measure.IsValid())
        {
            Result->AddMetadata(MakeShared<FCkFlexMeasureMetaData>(
                [WeakContent = TWeakPtr<SWidget>(InContent), Measure, MinWidth = InStyle.MinWidth, MinHeight = InStyle.MinHeight, MaxWidth = InStyle.MaxWidth, MaxHeight = InStyle.MaxHeight](const FCkFlexMeasureArgs& Args)
                {
                    auto Size = WeakContent.IsValid() ? Measure->Measure(Args) : FVector2D::ZeroVector;
                    Size.X = FMath::Max(Size.X, MinWidth);
                    Size.Y = FMath::Max(Size.Y, MinHeight);
                    if (MaxWidth.IsSet()) { Size.X = FMath::Min(Size.X, MaxWidth.GetValue()); }
                    if (MaxHeight.IsSet()) { Size.Y = FMath::Min(Size.Y, MaxHeight.GetValue()); }
                    return Size;
                },
                [WeakContent = TWeakPtr<SWidget>(InContent), Measure](const float Width, const float Height)
                {
                    if (WeakContent.IsValid()) { Measure->NotifyArranged(Width, Height); }
                }));
        }
    }
    return Result;
}

auto FCkUiView::MakeRepeatItem(const FCkUiNode& InNode, const TSharedPtr<FCkUiCollection>& Collection,
    const TSharedPtr<const FCkUiRecord>& Record, const TSharedPtr<FRepeatScope>& Scope,
    const TSharedPtr<FCkUiView>& Existing, FCkUiDocument& OutDocument, TArray<FString>& OutErrors) const -> TSharedPtr<FCkUiView>
{
    auto Data = _Data;
    Data.Collections.Reset(); Data.Trees.Reset();
    auto Actions = _Actions;
    const TWeakPtr<FCkUiView> WeakOwner = const_cast<FCkUiView*>(this)->AsShared();
    const TWeakPtr<const FCkUiRecord> WeakRecord = Record;
    const TWeakPtr<FCkUiCollection> WeakCollection = Collection;
    const TWeakPtr<FRepeatScope> WeakScope = Scope;
    const auto Eligible = [WeakOwner, WeakRecord, WeakCollection, WeakScope]()
    {
        const auto Owner = WeakOwner.Pin(); const auto Row = WeakRecord.Pin();
        const auto Model = WeakCollection.Pin(); const auto ItemScope = WeakScope.Pin();
        return Owner.IsValid() && Owner->CanDispatchEvents() && ItemScope.IsValid() && ItemScope->Active
            && Row.IsValid() && Model.IsValid() && Model->FindRecord(Row->GetKey()) == Row;
    };
    Data.CanDispatchEvents = TAttribute<bool>::CreateLambda(Eligible);
    for (const auto& [Name, Callback] : _Data.ItemActions)
    {
        if (Actions.Contains(TEXT("@item:") + Name)) { OutErrors.Add(TEXT("Reserved repeat action alias collision.")); return {}; }
        Actions.Add(TEXT("@item:") + Name, FSimpleDelegate::CreateLambda([Eligible, WeakRecord, Callback]()
        { if (Eligible()) { const auto Row = WeakRecord.Pin(); if (Row.IsValid()) { Callback.ExecuteIfBound(Row->GetKey()); } } }));
    }
    for (const FCkUiFieldSchema& Field : Collection->GetSchema())
    {
        const FString Alias = TEXT("@field:") + Field.Name;
        if (Data.Text.Contains(Alias) || Data.Color.Contains(Alias) || Data.Number.Contains(Alias)
            || Data.Visibility.Contains(Alias) || Data.Images.Contains(Alias))
        { OutErrors.Add(TEXT("Reserved repeat field alias collision.")); return {}; }
        const auto Find = [WeakRecord, Name = Field.Name]() -> const FCkUiFieldValue*
        { const auto Row = WeakRecord.Pin(); return Row.IsValid() ? Row->FindField(Name) : nullptr; };
        switch (Field.Kind)
        {
        case ECkUiFieldKind::Text: Data.Text.Add(Alias, TAttribute<FText>::CreateLambda([Find]() { const auto* V = Find(); return V ? V->Text : FText::GetEmpty(); })); break;
        case ECkUiFieldKind::Bool: Data.Visibility.Add(Alias, TAttribute<bool>::CreateLambda([Find]() { const auto* V = Find(); return V && V->Bool; })); break;
        case ECkUiFieldKind::Number: Data.Number.Add(Alias, TAttribute<float>::CreateLambda([Find]() { const auto* V = Find(); return V ? V->Number : 0.0f; })); break;
        case ECkUiFieldKind::Color: Data.Color.Add(Alias, TAttribute<FLinearColor>::CreateLambda([Find]() { const auto* V = Find(); return V ? V->Color : FLinearColor::Transparent; })); break;
        case ECkUiFieldKind::Image: Data.Images.Add(Alias, TAttribute<const FSlateBrush*>::CreateLambda([Find]() { const auto* V = Find(); return V && V->Image.IsValid() ? V->Image.Get() : nullptr; })); break;
        }
    }
    FCkUiNode Bound = InNode;
    TFunction<void(FCkUiNode&)> Rewrite;
    Rewrite = [this, &Collection, &Rewrite, &OutErrors](FCkUiNode& Node)
    {
        if (Node.Kind == ECkUiNodeKind::Repeat || Node.Kind == ECkUiNodeKind::Table || Node.Kind == ECkUiNodeKind::Tree || Node.Kind == ECkUiNodeKind::Native)
        { OutErrors.Add(TEXT("Repeat items cannot contain nested collections or native ports.")); return; }
        if (!Node.ItemAction.IsEmpty())
        {
            if (Node.Kind != ECkUiNodeKind::Button || !_Data.ItemActions.FindRef(Node.ItemAction).IsBound())
            { OutErrors.Add(TEXT("Repeat item requires its declared item action.")); }
            Node.Action = TEXT("@item:") + Node.ItemAction; Node.ItemAction.Reset();
        }
        const auto* Custom = Node.Kind == ECkUiNodeKind::Custom && _CustomRegistry.IsValid() ? _CustomRegistry->Find(Node.CustomTag) : nullptr;
        for (const auto& [Target, Field] : Node.FieldBindings)
        {
            TOptional<ECkUiFieldKind> Expected;
            const FCkUiCustomPropertySchema* Property = Custom ? Custom->Schema.Properties.FindByPredicate([&Target](const auto& P) { return P.Name == Target; }) : nullptr;
            if (Target == TEXT("visible")) { Expected = ECkUiFieldKind::Bool; }
            else if (Target == TEXT("bind") && (Node.Kind == ECkUiNodeKind::Text || Node.Kind == ECkUiNodeKind::Button)) { Expected = ECkUiFieldKind::Text; }
            else if (Target == TEXT("bind") && Node.Kind == ECkUiNodeKind::Image) { Expected = ECkUiFieldKind::Image; }
            else if (Target == TEXT("color")) { Expected = ECkUiFieldKind::Color; }
            else if (Target == TEXT("tooltip")) { Expected = ECkUiFieldKind::Text; }
            else if (Property) { Expected = ck_ui_surface::FieldKind(Property->Kind); }
            const auto* Schema = Collection->GetSchema().FindByPredicate([&Field](const auto& S) { return S.Name == Field; });
            if (!Expected.IsSet() || !Schema || !Schema->Required || Schema->Kind != Expected.GetValue())
            { OutErrors.Add(FString::Printf(TEXT("Repeat field '%s' has an unsupported target or incompatible schema."), *Field)); continue; }
            const FString Alias = TEXT("@field:") + Field;
            if (Target == TEXT("bind")) { Node.Binding = Alias; }
            else if (Target == TEXT("visible")) { Node.VisibilityBinding = Alias; }
            else if (Target == TEXT("color")) { Node.ColorBinding = Alias; }
            else if (Target == TEXT("tooltip")) { Node.TooltipBinding = Alias; }
            else if (Property) { FCkUiCustomPropertyValue Value; Value.Kind = Property->Kind; Value.Name = Alias; Node.CustomProperties.Add(Target, MoveTemp(Value)); }
        }
        Node.FieldBindings.Reset();
        for (auto& Child : Node.Children) { Rewrite(Child); }
    };
    Rewrite(Bound);
    if (!OutErrors.IsEmpty()) { return {}; }
    const auto View = Existing.IsValid() ? Existing : FCkUiView::Create({}, MoveTemp(Actions), {}, _BaseFont, MoveTemp(Data), _CustomRegistry);
    if (!Existing.IsValid())
    {
        const auto Mount = View->GetRegion(TEXT("item"));
        const TWeakPtr<SWidget> WeakMount = Mount;
        Mount->AddMetadata(MakeShared<FCkFlexMeasureMetaData>(
            [WeakMount](const FCkFlexMeasureArgs& Args)
            {
                const auto Pinned = WeakMount.Pin();
                if (!Pinned.IsValid() || Pinned->GetChildren()->Num() == 0) { return FVector2D::ZeroVector; }
                const auto Root = Pinned->GetChildren()->GetChildAt(0);
                const auto Measure = Root->GetMetaData<FCkFlexMeasureMetaData>();
                return Measure.IsValid() ? Measure->Measure(Args) : Root->GetDesiredSize();
            },
            [WeakMount](float Width, float Height)
            {
                const auto Pinned = WeakMount.Pin();
                if (!Pinned.IsValid() || Pinned->GetChildren()->Num() == 0) { return; }
                const auto Measure = Pinned->GetChildren()->GetChildAt(0)->GetMetaData<FCkFlexMeasureMetaData>();
                if (Measure.IsValid()) { Measure->NotifyArranged(Width, Height); }
            }));
    }
    FCkUiNode Root; Root.Id = TEXT("@repeat-root"); Root.Kind = ECkUiNodeKind::Column; Root.Children.Add(MoveTemp(Bound));
    OutDocument.Regions.Add(TEXT("item"), MoveTemp(Root));
    return View->ValidateDocument(OutDocument, TEXT("<repeat-item>"), OutErrors) ? View : nullptr;
}

auto FCkUiView::MakeCustomSlotView(const FCkUiNode& InRoot, const TSharedPtr<FCustomSlot>& InSlot,
    const TMap<FString, FCkUiMenu>& InMenus, FCkUiDocument& OutDocument, TArray<FString>& OutErrors) const -> TSharedPtr<FCkUiView>
{
    auto View = InSlot->View;
    if (!View.IsValid())
    {
        auto Data = _Data;
        const TWeakPtr<FCkUiView> WeakParent = const_cast<FCkUiView*>(this)->AsShared();
        const TWeakPtr<FRepeatScope> WeakScope = InSlot->Scope;
        Data.CanDispatchEvents = TAttribute<bool>::CreateLambda([WeakParent, WeakScope]()
        {
            const auto Parent = WeakParent.Pin(); const auto Scope = WeakScope.Pin();
            return Parent.IsValid() && Scope.IsValid() && Scope->Active && Parent->CanDispatchEvents();
        });
        View = FCkUiView::Create(_Bindings, _Actions, _Tokens, _BaseFont, MoveTemp(Data), _CustomRegistry);
        const auto Mount = View->GetRegion(TEXT("slot"));
        const TWeakPtr<SWidget> WeakMount = Mount;
        Mount->AddMetadata(MakeShared<FCkFlexMeasureMetaData>(
            [WeakMount](const FCkFlexMeasureArgs& Args) -> FVector2D
            {
                const auto Widget = WeakMount.Pin();
                if (!Widget.IsValid() || Widget->GetChildren()->Num() == 0) { return FVector2D::ZeroVector; }
                const auto Child = Widget->GetChildren()->GetChildAt(0);
                const auto Measure = Child->GetMetaData<FCkFlexMeasureMetaData>();
                return Measure.IsValid() ? Measure->Measure(Args) : Child->GetDesiredSize();
            },
            [WeakMount](const float Width, const float Height)
            {
                const auto Widget = WeakMount.Pin();
                if (!Widget.IsValid() || Widget->GetChildren()->Num() == 0) { return; }
                const auto Measure = Widget->GetChildren()->GetChildAt(0)->GetMetaData<FCkFlexMeasureMetaData>();
                if (Measure.IsValid()) { Measure->NotifyArranged(Width, Height); }
            }));
    }
    FCkUiNode Root; Root.Id = TEXT("@slot-root"); Root.Kind = ECkUiNodeKind::Column;
    TSet<FString> Ids;
    const auto CollectIds = [&Ids](const FCkUiNode& Node, auto&& Self) -> void
    { Ids.Add(Node.Id); for (const auto& Child : Node.Children) { Self(Child, Self); } };
    CollectIds(InRoot, CollectIds);
    while (Ids.Contains(Root.Id)) { Root.Id += TEXT("_"); }
    auto Child = InRoot; Child.CustomSlotName.Reset(); Root.Children.Add(MoveTemp(Child));
    OutDocument.Regions.Add(TEXT("slot"), MoveTemp(Root)); OutDocument.Menus = InMenus;
    return View->ValidateDocument(OutDocument, TEXT("<custom-slot>"), OutErrors) ? View : nullptr;
}

auto FCkUiView::PrepareCustomSlots(const FCkUiNode& InNode, const FCkUiCustomWidgetSchema& InSchema,
    FStagedDocument& OutStaged, FCkUiCustomWidgetArguments& OutArguments, TArray<FString>& OutErrors) const -> bool
{
    if (InSchema.Slots.IsEmpty()) { return true; }
    const auto Previous = _CustomSlots.FindRef(InNode.Id);
    const auto Next = MakeShared<FCustomSlotSet>();
    for (const auto& Definition : InSchema.Slots)
    {
        const auto Old = Previous.IsValid() ? Previous->Slots.FindRef(Definition.Name) : nullptr;
        const auto Slot = MakeShared<FCustomSlot>();
        Slot->Mount = Old.IsValid() ? Old->Mount : SNew(SBox);
        const auto* Root = InNode.Children.FindByPredicate([&Definition](const FCkUiNode& Child) { return Child.CustomSlotName == Definition.Name; });
        if (Root != nullptr)
        {
            Slot->View = Old.IsValid() ? Old->View : nullptr;
            Slot->Scope = Slot->View.IsValid() ? Old->Scope : MakeShared<FRepeatScope>();
            FCkUiDocument Document;
            Slot->View = MakeCustomSlotView(*Root, Slot, OutStaged.Menus, Document, OutErrors);
            if (!Slot->View.IsValid()) { return false; }
            if (Slot->View->_IsReloading) { OutErrors.Add(TEXT("Custom slot child is already reloading.")); return false; }
            const auto Child = MakeShared<FNestedUpdate>(); Child->View = Slot->View; Child->Staged = MakeShared<FStagedDocument>();
            Child->View->_IsReloading = true;
            if (!Child->View->StageDocument(Document, *Child->Staged, OutErrors)) { return false; }
            OutStaged.Nested.Add(Child);
        }
        if (!Old.IsValid())
        {
            const TWeakPtr<SBox> WeakMount = Slot->Mount;
            Slot->Mount->AddMetadata(MakeShared<FCkFlexMeasureMetaData>(
                [WeakMount](const FCkFlexMeasureArgs& Args) -> FVector2D
                {
                    const auto Mount = WeakMount.Pin();
                    if (!Mount.IsValid() || Mount->GetChildren()->Num() == 0) { return FVector2D::ZeroVector; }
                    const auto Child = Mount->GetChildren()->GetChildAt(0);
                    const auto Measure = Child->GetMetaData<FCkFlexMeasureMetaData>();
                    return Measure.IsValid() ? Measure->Measure(Args) : Child->GetDesiredSize();
                },
                [WeakMount](const float Width, const float Height)
                {
                    const auto Mount = WeakMount.Pin();
                    if (!Mount.IsValid() || Mount->GetChildren()->Num() == 0) { return; }
                    const auto Measure = Mount->GetChildren()->GetChildAt(0)->GetMetaData<FCkFlexMeasureMetaData>();
                    if (Measure.IsValid()) { Measure->NotifyArranged(Width, Height); }
                }));
        }
        Next->Slots.Add(Definition.Name, Slot); OutArguments.Slots.Add(Definition.Name, Slot->Mount);
    }
    OutStaged.CustomSlots.Add(InNode.Id, Next);
    const TWeakPtr<FCkUiView> WeakOwner = const_cast<FCkUiView*>(this)->AsShared();
    TMap<FString, TWeakPtr<SWidget>> SlotMounts;
    TMap<FString, TWeakPtr<FCkUiView>> SlotViews;
    for (const auto& Entry : Next->Slots)
    { SlotMounts.Add(Entry.Key, Entry.Value->Mount); SlotViews.Add(Entry.Key, Entry.Value->View); }
    const int32 UserIndex = _Data.SlateUserIndex;
    OutArguments.ReleaseSlotPointerCaptures = [WeakOwner, SlotMounts, SlotViews, UserIndex](const FString& SlotName)
    {
        const auto Owner = WeakOwner.Pin();
        if ((Owner.IsValid() && Owner->_IsReloading) || UserIndex < 0 || !FSlateApplication::IsInitialized()) { return; }
        const auto Mount = SlotMounts.FindRef(SlotName).Pin();
        if (!Mount.IsValid()) { return; }
        const auto User = FSlateApplication::Get().GetUser(UserIndex);
        if (!User.IsValid()) { return; }
        TSet<const SWidget*> OwnedWidgets;
        if (!ck_ui_surface::CollectWidgetPointers(Mount.ToSharedRef(), OwnedWidgets)) { return; }
        TSet<uint32> PointerIndices{FSlateApplication::CursorPointerIndex};
        for (const auto& Entry : User->GetWidgetsUnderPointerLastEventByIndex()) { PointerIndices.Add(Entry.Key); }
        const auto CollectReports = [&PointerIndices, UserIndex](const TSharedPtr<FCkUiView>& View, const auto& Self) -> void
        {
            if (!View.IsValid()) { return; }
            for (const auto& Entry : View->_CommittedRetained)
            {
                if (!Entry.Value.Component.IsValid()) { continue; }
                for (const auto& Capture : Entry.Value.Component->GetPointerCaptures())
                { if (Capture.UserIndex == UserIndex) { PointerIndices.Add(Capture.PointerIndex); } }
            }
            for (const auto& Entry : View->_CustomSlots)
            { for (const auto& Child : Entry.Value->Slots) { Self(Child.Value->View, Self); } }
            for (const auto& Entry : View->_RepeatStates)
            { for (const auto& Item : Entry.Value->Items) { Self(Item.View, Self); } }
        };
        CollectReports(SlotViews.FindRef(SlotName).Pin(), CollectReports);
        TMap<uint32, TSharedPtr<SWidget>> Captures;
        for (const uint32 Index : PointerIndices)
        {
            const auto Captor = User->GetPointerCaptor(Index);
            if (Captor.IsValid() && OwnedWidgets.Contains(Captor.Get())) { Captures.Add(Index, Captor); }
        }
        // Capture-loss callbacks may reload or redirect another pointer; release only the snapshot still owned.
        for (const auto& Capture : Captures)
        { if (User->GetPointerCaptor(Capture.Key) == Capture.Value) { User->ReleaseCapture(Capture.Key); } }
    };
    return true;
}
auto FCkUiView::PrepareRepeat(const FCkUiNode& InNode, FStagedDocument& OutStaged, TArray<FString>& OutErrors) const -> TSharedPtr<SWidget>
{
    const auto Collection = _Data.Collections.FindRef(InNode.Binding);
    if (!Collection.IsValid() || InNode.Children.Num() != 1) { OutErrors.Add(TEXT("Invalid repeat definition.")); return {}; }
    const auto Previous = _RepeatStates.FindRef(InNode.Id);
    const auto Next = MakeShared<FRepeatState>();
    Next->Definition = InNode; Next->Collection = Collection;
    Next->Widget = Previous.IsValid() && Previous->Collection == Collection ? Previous->Widget : nullptr;
    if (!Next->Widget.IsValid())
    {
        const TWeakPtr<FCkUiView> WeakOwner = const_cast<FCkUiView*>(this)->AsShared();
        const auto WeakPresenter = MakeShared<TWeakPtr<SCkUiRepeat>>();
        Next->Widget = SNew(SCkUiRepeat).Collection(Collection).OnRefresh(FCkUiRepeatRefresh::CreateLambda([WeakOwner, WeakPresenter, Id = InNode.Id]()
        { const auto Owner = WeakOwner.Pin(); return Owner.IsValid() && Owner->GetRepeat(Id) == WeakPresenter->Pin() && Owner->RefreshRepeat(Id); }));
        *WeakPresenter = Next->Widget;
        Next->Widget->SetTag(FName(*InNode.Id));
    }
    const auto Records = Collection->GetRecords();
    const int64 CollectionRevision = Collection->GetRevision();
    for (const auto& Record : Records)
    {
        const FRepeatState::FItem* Old = Previous.IsValid() ? Previous->Items.FindByPredicate([&Record](const auto& Item) { return Item.Record == Record; }) : nullptr;
        FRepeatState::FItem Item; Item.Record = Record; Item.Scope = Old ? Old->Scope : MakeShared<FRepeatScope>();
        FCkUiDocument Document;
        Item.View = MakeRepeatItem(InNode.Children[0], Collection, Record, Item.Scope, Old ? Old->View : nullptr, Document, OutErrors);
        if (!Item.View.IsValid() || Item.View->_IsReloading) { OutErrors.Add(TEXT("Repeat child could not enter the transaction.")); return {}; }
        const auto Update = MakeShared<FNestedUpdate>(); Update->View = Item.View; Item.View->_IsReloading = true;
        Update->Staged = MakeShared<FStagedDocument>();
        OutStaged.Nested.Add(Update);
        if (!Item.View->StageDocument(Document, *Update->Staged, OutErrors)) { return {}; }
        Next->Items.Add(MoveTemp(Item));
    }
    if (Collection->GetRevision() != CollectionRevision) { OutErrors.Add(TEXT("Repeat collection changed during preparation.")); return {}; }
    OutStaged.Repeats.Add(InNode.Id, Next);
    const auto Port = ck_ui_surface::MakeMeasuredPort(Next->Widget.ToSharedRef(), InNode.Id);
    if (!InNode.VisibilityBinding.IsEmpty())
    {
        const TWeakPtr<FCkUiView> WeakOwner = const_cast<FCkUiView*>(this)->AsShared();
        Port->SetVisibility(TAttribute<EVisibility>::CreateLambda([WeakOwner, Binding = InNode.VisibilityBinding]()
        { const auto Owner = WeakOwner.Pin(); return Owner.IsValid() && Owner->_Data.Visibility.FindRef(Binding).Get(false) ? EVisibility::Visible : EVisibility::Collapsed; }));
    }
    OutStaged.Retained.Add(FRetainedRecord{InNode.Id, ERetainedKind::Repeat, InNode.Binding, {}, Next->Widget, nullptr, Port});
    return ApplyStyle(InNode.Style, Port);
}

auto FCkUiView::RefreshRepeat(const FString& InId) -> bool
{
    if (_IsReloading || !IsInGameThread()) { return false; }
    const auto Previous = _RepeatStates.FindRef(InId);
    if (!Previous.IsValid()) { return false; }
    const auto& Records = Previous->Collection->GetRecords();
    bool Same = Records.Num() == Previous->Items.Num();
    for (int32 Index = 0; Same && Index < Records.Num(); ++Index) { Same = Records[Index] == Previous->Items[Index].Record; }
    if (Same) { Previous->Widget->SetLastFailure({}); return true; }
    const auto KeepAlive = AsShared();
    TGuardValue<bool> Guard(_IsReloading, true);
    FStagedDocument Staged;
    TArray<FString> Errors;
    if (!PrepareRepeat(Previous->Definition, Staged, Errors).IsValid())
    { Previous->Widget->SetLastFailure(FString::Join(Errors, TEXT("\n"))); return false; }
    if (!FlattenNested(Staged, Errors)) { Previous->Widget->SetLastFailure(FString::Join(Errors, TEXT("\n"))); return false; }
    TSet<const SWidget*> NestedMounts;
    CollectChildMounts(NestedMounts);
    for (const auto& Child : Staged.Nested)
    { for (const auto& [Name, Mount] : Child->View->_RegionMounts) { NestedMounts.Add(Mount.Get()); } }
    TSet<const SWidget*> Owned;
    for (const auto& Child : Staged.Nested)
    {
        TSet<const SWidget*> Candidate;
        for (const auto& [Name, Mount] : Child->View->_RegionMounts) { Candidate.Add(Mount.Get()); }
        const auto AddTree = [&Candidate, &NestedMounts](const TSharedPtr<SWidget>& Root)
        {
            if (!Root.IsValid()) { return; }
            TSet<const SWidget*> Tree;
            ck_ui_surface::CollectWidgetPointers(Root.ToSharedRef(), Tree, &NestedMounts);
            Candidate.Append(Tree);
        };
        for (const auto& [Name, Root] : Child->Staged->Regions) { AddTree(Root); }
        for (const auto& Retained : Child->Staged->Retained) { AddTree(Retained.Widget); }
        for (const auto& Root : Child->Staged->OwnedWidgets) { AddTree(Root); }
        for (const auto& Content : Child->Staged->ScrollContents) { AddTree(Content.Child); }
        for (const auto* Widget : Candidate)
        {
            if (Owned.Contains(Widget)) { Previous->Widget->SetLastFailure(TEXT("Repeat children cannot share widget ownership.")); return false; }
            Owned.Add(Widget);
        }
    }
    for (const auto& Child : Staged.Nested) { Child->State = Child->View->CaptureCommit(*Child->Staged); }
    const auto Next = Staged.Repeats.FindRef(InId);
    TArray<TPair<TSharedPtr<FCkUiView>, FCommitState>> RemovedInteractions;
    for (const auto& Old : Previous->Items)
    {
        if (!Next->Items.ContainsByPredicate([&Old](const auto& Item) { return Item.Scope == Old.Scope; }))
        {
            FStagedDocument Empty;
            RemovedInteractions.Emplace(Old.View, Old.View->CaptureCommit(Empty));
            Old.Scope->Active = false;
        }
    }
    _RepeatStates.Add(InId, Next);
    for (const auto& Child : Staged.Nested) { Child->View->PublishConfiguration(MoveTemp(*Child->Staged), Child->State); }
    MountRepeat(*Next);
    for (const auto& Child : Staged.Nested) { Child->View->MountChildren(); }
    for (const auto& Child : Staged.Nested) { Child->View->ReconcileInteractions(Child->State); }
    for (auto& Removed : RemovedInteractions) { Removed.Key->ReconcileInteractions(Removed.Value); }
    for (const auto& Old : Previous->Items)
    { if (!Old.Scope->Active) { Old.View->ReleaseTransientInteractions(Old.View->GetRegion(TEXT("item"))); } }
    return true;
}

auto FCkUiView::MountRepeat(FRepeatState& State) -> void
{
    TArray<SCkUiRepeat::FItem> Items;
    for (const auto& Item : State.Items)
    { Item.Scope->Active = true; Items.Add({Item.Record->GetKey(), Item.Record, Item.View->GetRegion(TEXT("item"))}); }
    State.Widget->SetItems(MoveTemp(Items), State.Definition.Style.Gap, State.Definition.Style.Padding,
        State.Definition.RepeatDirection, State.Definition.Style.FlexWrap);
}

template <typename TRecord>
auto FCkUiView::MakeCellView(const FCkUiNode& InCell, TWeakPtr<const TRecord> WeakRecord,
    const bool InValidateOnly, TArray<FString>& OutErrors) const -> TSharedPtr<FCkUiView>
{
    auto Data = _Data;
    // Table cells inherit ordinary bindings/actions, but cannot recursively own collections or selection callbacks.
    Data.Collections.Reset();
    Data.Trees.Reset();
    Data.TreeSelectionChanged.Reset();
    Data.TableSelectionChanged.Reset();
    Data.TableContextMenus.Reset();
    Data.ContextActions.Reset();
    auto BoundCell = InCell;
    int32 AliasIndex = 0;
    TFunction<void(FCkUiNode&)> BindFields;
    BindFields = [this, WeakRecord, &Data, &BindFields, &AliasIndex](FCkUiNode& Node)
    {
        for (const auto& [Target, Field] : Node.FieldBindings)
        {
            FString Alias;
            do { Alias = FString::Printf(TEXT("_row_field_%d"), AliasIndex++); }
            while (Data.Text.Contains(Alias) || Data.Images.Contains(Alias) || Data.Visibility.Contains(Alias)
                || Data.String.Contains(Alias) || Data.StringChanged.Contains(Alias) || Data.Number.Contains(Alias) || Data.Color.Contains(Alias) || Data.TextChanged.Contains(Alias) || Data.BoolChanged.Contains(Alias) || Data.NumberChanged.Contains(Alias) || Data.NumberCommitted.Contains(Alias) || Data.NumberInteraction.Contains(Alias));
            if (Target == TEXT("bind") && (Node.Kind == ECkUiNodeKind::Text || Node.Kind == ECkUiNodeKind::Button))
            {
                Data.Text.Add(Alias, TAttribute<FText>::CreateLambda([WeakRecord, Field]() { const auto Row = WeakRecord.Pin(); const FCkUiFieldValue* Value = Row.IsValid() ? Row->FindField(Field) : nullptr; return Value != nullptr ? Value->Text : FText::GetEmpty(); }));
                Node.Binding = Alias;
            }
            else if (Target == TEXT("bind") && Node.Kind == ECkUiNodeKind::Image)
            {
                Data.Images.Add(Alias, TAttribute<const FSlateBrush*>::CreateLambda([WeakRecord, Field]() { const auto Row = WeakRecord.Pin(); const FCkUiFieldValue* Value = Row.IsValid() ? Row->FindField(Field) : nullptr; return Value != nullptr && Value->Image.IsValid() ? Value->Image.Get() : nullptr; }));
                Node.Binding = Alias;
            }
            else if (Target == TEXT("visible"))
            {
                Data.Visibility.Add(Alias, TAttribute<bool>::CreateLambda([WeakRecord, Field]() { const auto Row = WeakRecord.Pin(); const FCkUiFieldValue* Value = Row.IsValid() ? Row->FindField(Field) : nullptr; return Value != nullptr && Value->Bool; }));
                Node.VisibilityBinding = Alias;
            }
            else if (Target == TEXT("color") && Node.Kind == ECkUiNodeKind::Text)
            {
                Data.Color.Add(Alias, TAttribute<FLinearColor>::CreateLambda([WeakRecord, Field]()
                { const auto Row = WeakRecord.Pin(); const auto* Value = Row.IsValid() ? Row->FindField(Field) : nullptr; return Value != nullptr ? Value->Color : FLinearColor::Transparent; }));
                Node.ColorBinding = Alias;
            }
            else if (Target == TEXT("tooltip") && Node.Kind == ECkUiNodeKind::Text)
            {
                Data.Text.Add(Alias, TAttribute<FText>::CreateLambda([WeakRecord, Field]()
                { const auto Row = WeakRecord.Pin(); const auto* Value = Row.IsValid() ? Row->FindField(Field) : nullptr; return Value != nullptr ? Value->Text : FText::GetEmpty(); }));
                Node.TooltipBinding = Alias;
            }
            else if (Node.Kind == ECkUiNodeKind::Custom)
            {
                const FCkUiCustomWidgetRegistration* Custom = _CustomRegistry.IsValid() ? _CustomRegistry->Find(Node.CustomTag) : nullptr;
                if (Custom == nullptr) { continue; } // Schema validation rejects this before staging.
                const FCkUiCustomPropertySchema* Property = Custom->Schema.Properties.FindByPredicate(
                    [&Target](const FCkUiCustomPropertySchema& Item) { return Item.Name == Target; });
                if (Property == nullptr) { continue; }
                switch (Property->Kind)
                {
                case ECkUiCustomPropertyKind::TextBinding:
                    Data.Text.Add(Alias, TAttribute<FText>::CreateLambda([WeakRecord, Field]()
                    { const auto Row = WeakRecord.Pin(); const auto* Value = Row.IsValid() ? Row->FindField(Field) : nullptr; return Value != nullptr ? Value->Text : FText::GetEmpty(); }));
                    break;
                case ECkUiCustomPropertyKind::NumberBinding:
                    Data.Number.Add(Alias, TAttribute<float>::CreateLambda([WeakRecord, Field]()
                    { const auto Row = WeakRecord.Pin(); const auto* Value = Row.IsValid() ? Row->FindField(Field) : nullptr; return Value != nullptr ? Value->Number : 0.0f; }));
                    break;
                case ECkUiCustomPropertyKind::BoolBinding:
                    Data.Visibility.Add(Alias, TAttribute<bool>::CreateLambda([WeakRecord, Field]()
                    { const auto Row = WeakRecord.Pin(); const auto* Value = Row.IsValid() ? Row->FindField(Field) : nullptr; return Value != nullptr && Value->Bool; }));
                    break;
                case ECkUiCustomPropertyKind::ColorBinding:
                    Data.Color.Add(Alias, TAttribute<FLinearColor>::CreateLambda([WeakRecord, Field]()
                    { const auto Row = WeakRecord.Pin(); const auto* Value = Row.IsValid() ? Row->FindField(Field) : nullptr; return Value != nullptr ? Value->Color : FLinearColor::Transparent; }));
                    break;
                case ECkUiCustomPropertyKind::ImageBinding:
                    Data.Images.Add(Alias, TAttribute<const FSlateBrush*>::CreateLambda([WeakRecord, Field]()
                    { const auto Row = WeakRecord.Pin(); const auto* Value = Row.IsValid() ? Row->FindField(Field) : nullptr; return Value != nullptr ? Value->Image.Get() : nullptr; }));
                    break;
                default: continue;
                }
                FCkUiCustomPropertyValue Bound;
                Bound.Kind = Property->Kind;
                Bound.Name = Alias;
                Node.CustomProperties.Add(Target, MoveTemp(Bound));
            }
        }
        Node.FieldBindings.Reset();
        for (FCkUiNode& Child : Node.Children) { BindFields(Child); }
    };
    BindFields(BoundCell);
    const TSharedRef<FCkUiView> CellView = FCkUiView::Create({}, {}, {}, _BaseFont, MoveTemp(Data), _CustomRegistry);
    CellView->GetRegion(TEXT("cell"));
    auto Document = FCkUiDocument{};
    auto Root = FCkUiNode{};
    TSet<FString> CellIds;
    TFunction<void(const FCkUiNode&)> CollectIds;
    CollectIds = [&CellIds, &CollectIds](const FCkUiNode& Node)
    { CellIds.Add(Node.Id); for (const FCkUiNode& Child : Node.Children) { CollectIds(Child); } };
    CollectIds(BoundCell);
    Root.Id = TEXT("_cell_root");
    while (CellIds.Contains(Root.Id)) { Root.Id += TEXT("_"); }
    Root.Kind = ECkUiNodeKind::Column; Root.Children.Add(MoveTemp(BoundCell));
    Document.Regions.Add(TEXT("cell"), MoveTemp(Root));
    if (!CellView->ValidateDocument(Document, TEXT("<table-cell>"), OutErrors)) { return nullptr; }
    if (InValidateOnly) { return CellView; }
    FStagedDocument Staged;
    if (!CellView->StageDocument(Document, Staged, OutErrors)) { return nullptr; }
    if (!CellView->Commit(MoveTemp(Staged), OutErrors)) { return nullptr; }
    return CellView;
}

auto FCkUiView::StageNode(const FCkUiNode& InNode, FStagedDocument& InOutStaged, TArray<FString>& OutErrors) const -> TSharedPtr<SWidget>
{
    if (InNode.Kind == ECkUiNodeKind::Repeat) { return PrepareRepeat(InNode, InOutStaged, OutErrors); }
    const auto TextColor = [this](const FCkUiNode& Node) -> TAttribute<FSlateColor>
    {
        if (Node.ColorBinding.IsEmpty())
        { return TAttribute<FSlateColor>(Node.Style.Color.IsSet() ? FSlateColor(Node.Style.Color.GetValue()) : FSlateColor::UseForeground()); }
        const TAttribute<FLinearColor> Color = _Data.Color.FindRef(Node.ColorBinding);
        return TAttribute<FSlateColor>::CreateLambda([Color]() { return FSlateColor(Color.Get(FLinearColor::Transparent)); });
    };
    const auto TooltipText = [this](const FCkUiNode& Node) -> TAttribute<FText>
    {
        if (Node.TooltipBinding.IsEmpty() && Node.Tooltip.IsEmpty()) { return {}; }
        return Node.TooltipBinding.IsEmpty() ? TAttribute<FText>(FText::FromString(Node.Tooltip)) : _Data.Text.FindRef(Node.TooltipBinding);
    };
    const auto ApplyVisibility = [this, &InNode](const TSharedRef<SWidget>& InWidget) -> TSharedRef<SWidget>
    {
        if (!InNode.VisibilityBinding.IsEmpty())
        {
            const TWeakPtr<FCkUiView> WeakView = const_cast<FCkUiView*>(this)->AsShared();
            const FString Binding = InNode.VisibilityBinding;
            if (InNode.Kind == ECkUiNodeKind::Native)
            {
                const TWeakPtr<SWidget> WeakNative = _Bindings.FindRef(InNode.Binding);
                InWidget->SetVisibility(TAttribute<EVisibility>::CreateLambda([WeakView, WeakNative, Binding]()
                {
                    const TSharedPtr<FCkUiView> View = WeakView.Pin();
                    const TAttribute<bool>* Value = View.IsValid() ? View->_Data.Visibility.Find(Binding) : nullptr;
                    const TSharedPtr<SWidget> Native = WeakNative.Pin();
                    if (Value == nullptr || !Value->Get(true)) { return EVisibility::Collapsed; }
                    return Native.IsValid() ? Native->GetVisibility() : EVisibility::Collapsed;
                }));
            }
            else
            {
                InWidget->SetVisibility(TAttribute<EVisibility>::CreateLambda([WeakView, Binding]()
                {
                    const TSharedPtr<FCkUiView> View = WeakView.Pin();
                    const TAttribute<bool>* Value = View.IsValid() ? View->_Data.Visibility.Find(Binding) : nullptr;
                    return Value != nullptr && Value->Get(true) ? EVisibility::Visible : EVisibility::Collapsed;
                }));
            }
        }
        return InWidget;
    };
    const auto MakeFont = [this, &InNode]()
    {
        auto Font = _BaseFont;
        if (InNode.Style.FontSize.IsSet()) { Font.Size = FMath::RoundToInt(InNode.Style.FontSize.GetValue()); }
        if (InNode.Style.Bold) { Font.TypefaceFontName = TEXT("Bold"); }
        return Font;
    };
    const auto MakeTextAttribute = [this, &InNode]() -> TAttribute<FText>
    {
        return InNode.Binding.IsEmpty() ? TAttribute<FText>(FText::FromString(InNode.Text)) : _Data.Text.FindRef(InNode.Binding);
    };

    if (InNode.Kind == ECkUiNodeKind::Native) { return ApplyVisibility(MakePort(InNode, InOutStaged)); }

    if (InNode.Kind == ECkUiNodeKind::Text)
    {
        return ApplyVisibility(ApplyStyle(InNode.Style, SNew(SCkFlexText).Tag(FName(*InNode.Id)).Text(MakeTextAttribute())
            .Font(MakeFont()).AllowWrapping(InNode.Style.AllowWrapping).OverflowPolicy(InNode.Style.OverflowPolicy).WrappingPolicy(InNode.Style.WrappingPolicy)
            .Clipping(InNode.Style.AllowWrapping ? EWidgetClipping::Inherit : EWidgetClipping::ClipToBounds)
            .ColorAndOpacity(TextColor(InNode)).ToolTipText(TooltipText(InNode))));
    }
    if (InNode.Kind == ECkUiNodeKind::MenuButton)
    {
        const TWeakPtr<FCkUiView> WeakOwner = const_cast<FCkUiView*>(this)->AsShared();
        const auto MakeBool = [WeakOwner](const FString& Binding) -> TAttribute<bool>
        {
            if (Binding.IsEmpty()) { return TAttribute<bool>(true); }
            return TAttribute<bool>::CreateLambda([WeakOwner, Binding]()
            {
                const TSharedPtr<FCkUiView> Owner = WeakOwner.Pin();
                return Owner.IsValid() && Owner->_Data.Visibility.FindRef(Binding).Get(false);
            });
        };
        const auto MakeMenuText = [WeakOwner](const FString& Literal, const FString& Binding) -> TAttribute<FText>
        {
            if (Binding.IsEmpty()) { return TAttribute<FText>(FText::FromString(Literal)); }
            return TAttribute<FText>::CreateLambda([WeakOwner, Binding]()
            {
                const TSharedPtr<FCkUiView> Owner = WeakOwner.Pin();
                return Owner.IsValid() ? Owner->_Data.Text.FindRef(Binding).Get(FText::GetEmpty()) : FText::GetEmpty();
            });
        };
        TArray<SCkUiMenuButton::FEntry> Entries = BuildMenuEntries(InOutStaged.Menus, InNode.MenuReference);
        TSharedPtr<SCkUiMenuButton> Menu = GetMenuButton(InNode.Id);
        if (!Menu.IsValid()) { Menu = SNew(SCkUiMenuButton).Tag(FName(*InNode.Id)); }
        const TAttribute<FText> Label = MakeMenuText(InNode.Text, InNode.Binding);
        const TAttribute<bool> Enabled = MakeBool(InNode.TabEnabledBinding);
        const TAttribute<bool> CanDispatch = TAttribute<bool>::CreateLambda([WeakOwner]()
        { const TSharedPtr<FCkUiView> Owner = WeakOwner.Pin(); return Owner.IsValid() && Owner->CanDispatchEvents(); });
        const FSlateFontInfo Font = MakeFont();
        InOutStaged.MenuUpdates.Add([Menu, Entries = MoveTemp(Entries), Label, Enabled, CanDispatch, Font]() mutable
        { Menu->SetConfiguration(MoveTemp(Entries), Label, Enabled, CanDispatch, Font); });
        const TSharedRef<SBox> Port = ck_ui_surface::MakeMeasuredPort(Menu.ToSharedRef(), InNode.Id);
        InOutStaged.Retained.Add(FRetainedRecord{InNode.Id, ERetainedKind::MenuButton, TEXT("menu-button"), FString{}, Menu, nullptr, Port});
        return ApplyVisibility(ApplyStyle(InNode.Style, Port));
    }
    if (InNode.Kind == ECkUiNodeKind::Button)
    {
        const FSimpleDelegate Action = _Actions.FindRef(InNode.Action);
        const TWeakPtr<FCkUiView> WeakOwner = const_cast<FCkUiView*>(this)->AsShared();
        const FString EnabledBinding = InNode.ButtonEnabledBinding;
        const int64 ExpectedRevision = _Revision + 1;
        const auto IsEnabled = [WeakOwner, EnabledBinding, ExpectedRevision]() -> bool
        {
            const TSharedPtr<FCkUiView> Owner = WeakOwner.Pin();
            if (!Owner.IsValid() || !Owner->CanDispatchEvents() || Owner->_Revision != ExpectedRevision) { return false; }
            return EnabledBinding.IsEmpty() || Owner->_Data.Visibility.FindRef(EnabledBinding).Get(false);
        };
        const TAttribute<bool> Enabled = TAttribute<bool>::CreateLambda(IsEnabled);
        const FButtonStyle* ButtonStyle = &FCoreStyle::Get().GetWidgetStyle<FButtonStyle>(TEXT("Button"));
        const FMargin ContentPadding(4.0f, 2.0f);
        const auto Label = SNew(SCkFlexText).Text(MakeTextAttribute()).Font(MakeFont())
            .AllowWrapping(InNode.Style.AllowWrapping).OverflowPolicy(InNode.Style.OverflowPolicy).WrappingPolicy(InNode.Style.WrappingPolicy)
            .Clipping(InNode.Style.AllowWrapping ? EWidgetClipping::Inherit : EWidgetClipping::ClipToBounds)
            .ColorAndOpacity(TextColor(InNode));
        const auto Button = SNew(SButton).Tag(FName(*InNode.Id)).ButtonStyle(ButtonStyle).ContentPadding(ContentPadding).ToolTipText(TooltipText(InNode))
            .IsEnabled(Enabled).OnClicked_Lambda([Action, IsEnabled]() mutable
            { if (IsEnabled()) { Action.ExecuteIfBound(); } return FReply::Handled(); })[Label];
        const TWeakPtr<SButton> WeakButton = Button;
        const TWeakPtr<SCkFlexText> WeakLabel = Label;
        const auto PaddingSize = [WeakButton, ButtonStyle, ContentPadding]() -> FVector2D
        {
            const auto Current = WeakButton.Pin();
            return (ContentPadding + (Current.IsValid() && Current->IsPressed()
                ? ButtonStyle->PressedPadding : ButtonStyle->NormalPadding)).GetDesiredSize();
        };
        Button->AddMetadata(MakeShared<FCkFlexMeasureMetaData>(
            [WeakLabel, PaddingSize](const FCkFlexMeasureArgs& Args) -> FVector2D
            {
                const auto Text = WeakLabel.Pin();
                if (!Text.IsValid() || !IsValid_CkFlexMeasureArgs(Args)) { return FVector2D::ZeroVector; }
                const FVector2D Padding = PaddingSize();
                const float Width = Args.WidthMode == YGMeasureModeUndefined ? YGUndefined : FMath::Max(0.0f, Args.AvailableWidth - float(Padding.X));
                const float Height = Args.HeightMode == YGMeasureModeUndefined ? YGUndefined : FMath::Max(0.0f, Args.AvailableHeight - float(Padding.Y));
                FVector2D Size = Text->Measure(Width, Args.WidthMode, Height, Args.HeightMode, Args.LayoutScale) + Padding;
                if (Args.WidthMode != YGMeasureModeUndefined) { Size.X = Args.WidthMode == YGMeasureModeExactly ? Args.AvailableWidth : FMath::Min<double>(Size.X, Args.AvailableWidth); }
                if (Args.HeightMode != YGMeasureModeUndefined) { Size.Y = Args.HeightMode == YGMeasureModeExactly ? Args.AvailableHeight : FMath::Min<double>(Size.Y, Args.AvailableHeight); }
                return Size;
            },
            [WeakLabel, PaddingSize](const float Width, const float)
            {
                if (const auto Text = WeakLabel.Pin()) { Text->SetArrangedWidth(FMath::Max(0.0f, Width - float(PaddingSize().X))); }
            }));
        return ApplyVisibility(ApplyStyle(InNode.Style, Button));
    }
    if (InNode.Kind == ECkUiNodeKind::Image)
    {
        return ApplyVisibility(ApplyStyle(InNode.Style, SNew(SImage).Tag(FName(*InNode.Id)).Image(_Data.Images.FindRef(InNode.Binding))));
    }
    if (InNode.Kind == ECkUiNodeKind::Search)
    {
        TSharedPtr<SSearchBox> Search;
        if (const FRetainedRecord* Existing = _CommittedRetained.Find(InNode.Id); Existing != nullptr && Existing->Kind == ERetainedKind::Search)
        { Search = StaticCastSharedPtr<SSearchBox>(Existing->Widget); }
        if (!Search.IsValid())
        {
            const TWeakPtr<FCkUiView> WeakView = const_cast<FCkUiView*>(this)->AsShared();
            const FString Id = InNode.Id;
            const FOnTextChanged TextChanged = _Data.TextChanged.FindRef(InNode.Binding);
            const TSharedRef<bool> SuppressInitialWrite = MakeShared<bool>(true);
            Search = SNew(SSearchBox).Tag(FName(*Id))
                .HintText_Lambda([WeakView, Id]()
                {
                    const TSharedPtr<FCkUiView> View = WeakView.Pin();
                    return View.IsValid() ? View->_CommittedSearchHints.FindRef(Id).Get(FText::GetEmpty()) : FText::GetEmpty();
                })
                .InitialText(_Data.Text.FindRef(InNode.Binding))
                .OnTextChanged_Lambda([WeakView, SuppressInitialWrite, TextChanged](const FText& InText)
                {
                    const TSharedPtr<FCkUiView> View = WeakView.Pin();
                    if (!*SuppressInitialWrite && View.IsValid() && View->CanDispatchEvents()) { TextChanged.ExecuteIfBound(InText); }
                });
            // SSearchBox consumes InitialText by value in its Construct implementation. Install the
            // original attribute while the new control is still detached so later model updates stay
            // live without turning the initial binding install into an authored text-change callback.
            Search->SetText(_Data.Text.FindRef(InNode.Binding));
            *SuppressInitialWrite = false;
        }
        InOutStaged.Retained.Add(FRetainedRecord{InNode.Id, ERetainedKind::Search, InNode.Binding, FString{}, Search, nullptr, SNew(SBox).Tag(FName(*InNode.Id))});
        InOutStaged.SearchHints.Add(InNode.Id, InNode.PlaceholderBinding.IsEmpty()
            ? TAttribute<FText>(FText::FromString(InNode.Placeholder)) : _Data.Text.FindRef(InNode.PlaceholderBinding));
        return ApplyVisibility(ApplyStyle(InNode.Style, InOutStaged.Retained.Last().Port.ToSharedRef()));
    }
    if (InNode.Kind == ECkUiNodeKind::Table)
    {
        TSharedPtr<SCkUiTable> Table;
        if (const FRetainedRecord* Existing = _CommittedRetained.Find(InNode.Id); Existing != nullptr && Existing->Kind == ERetainedKind::Table)
        { Table = Existing->Table; }
        if (!Table.IsValid()) { Table = SNew(SCkUiTable).Collection(_Data.Collections.FindRef(InNode.Binding)).BaseFont(_BaseFont); }
        if (!Table.IsValid()) { OutErrors.Add(FString::Printf(TEXT("Table node '%s' could not be created."), *InNode.Id)); return nullptr; }

        const TWeakPtr<FCkUiView> WeakOwner = const_cast<FCkUiView*>(this)->AsShared();
        const auto CellFactory = [WeakOwner](const FCkUiNode& Cell, const TWeakPtr<const FCkUiRecord> WeakRecord, FString& Failure) -> TSharedPtr<FCkUiView>
        {
            const TSharedPtr<FCkUiView> Owner = WeakOwner.Pin();
            if (!Owner.IsValid() || !WeakRecord.IsValid()) { Failure = TEXT("The table cell owner or record expired."); return nullptr; }
            TArray<FString> Errors;
            TSharedPtr<FCkUiView> Result = Owner->MakeCellView(Cell, WeakRecord, false, Errors);
            if (!Result.IsValid()) { Failure = Errors.IsEmpty() ? TEXT("Table cell staging failed.") : Errors[0]; }
            return Result;
        };
        FString Failure;
        const TAttribute<FText> Filter = InNode.FilterBinding.IsEmpty() ? TAttribute<FText>(FText::GetEmpty()) : _Data.Text.FindRef(InNode.FilterBinding);
        const FOnCkUiTableSelectionChanged ConsumerSelection = _Data.TableSelectionChanged.FindRef(InNode.SelectionAction);
        const FOnCkUiTableSelectionChanged Selection = InNode.SelectionAction.IsEmpty() ? FOnCkUiTableSelectionChanged{}
            : FOnCkUiTableSelectionChanged::CreateLambda([WeakOwner, ConsumerSelection](TOptional<FString> Key, ESelectInfo::Type Info)
            { if (const TSharedPtr<FCkUiView> Owner = WeakOwner.Pin(); Owner.IsValid() && Owner->CanDispatchEvents()) { ConsumerSelection.ExecuteIfBound(MoveTemp(Key), Info); } });
        const FOnContextMenuOpening ConsumerContextMenu = _Data.TableContextMenus.FindRef(InNode.ContextMenuAction);
        const FOnContextMenuOpening ContextMenu = InNode.ContextMenuAction.IsEmpty() ? FOnContextMenuOpening{}
            : FOnContextMenuOpening::CreateLambda([WeakOwner, ConsumerContextMenu]() -> TSharedPtr<SWidget>
            { if (const TSharedPtr<FCkUiView> Owner = WeakOwner.Pin(); Owner.IsValid() && Owner->CanDispatchEvents()) { return ConsumerContextMenu.Execute(); } return {}; });
        auto HeaderTextBindings = TMap<FString, TAttribute<FText>>{};
        HeaderTextBindings.Reserve(InNode.Children.Num());
        for (const FCkUiNode& Column : InNode.Children)
        {
            if (Column.HeaderBinding.IsEmpty()) { continue; }
            const FString Name = Column.HeaderBinding;
            HeaderTextBindings.Add(Name, TAttribute<FText>::CreateLambda([WeakOwner, Name]()
            {
                const TSharedPtr<FCkUiView> Owner = WeakOwner.Pin();
                return Owner.IsValid() ? Owner->_Data.Text.FindRef(Name).Get(FText::GetEmpty()) : FText::GetEmpty();
            }));
        }
        TUniquePtr<ICkUiPreparedWidgetUpdate> Prepared = Table->Prepare(InNode, CellFactory, Filter, Selection, Failure, ContextMenu,
            MakeContextMenuBinding(InNode, InOutStaged.Menus), HeaderTextBindings);
        if (!Failure.IsEmpty() || !Prepared.IsValid()) { OutErrors.Add(!Failure.IsEmpty() ? Failure : FString::Printf(TEXT("Table node '%s' returned no prepared update."), *InNode.Id)); return nullptr; }
        InOutStaged.PreparedUpdates.Add(MoveTemp(Prepared));
        const TSharedRef<SBox> Port = ck_ui_surface::MakeMeasuredPort(Table.ToSharedRef(), InNode.Id);
        InOutStaged.Retained.Add(FRetainedRecord{InNode.Id, ERetainedKind::Table, InNode.Binding, FString{}, Table, nullptr, Port, Table});
        const TSharedRef<SWidget> Padded = SNew(SBox).Padding(InNode.Style.Padding)[Port];
        return ApplyVisibility(ApplyStyle(InNode.Style, Padded));
    }
    if (InNode.Kind == ECkUiNodeKind::Tree)
    {
        TSharedPtr<SCkUiTree> Tree;
        if (const FRetainedRecord* Existing = _CommittedRetained.Find(InNode.Id); Existing != nullptr && Existing->Kind == ERetainedKind::Tree)
        { Tree = StaticCastSharedPtr<SCkUiTree>(Existing->Widget); }
        if (!Tree.IsValid()) { Tree = SNew(SCkUiTree).Collection(_Data.Trees.FindRef(InNode.Binding)).BaseFont(_BaseFont); }
        if (!Tree.IsValid()) { OutErrors.Add(FString::Printf(TEXT("Tree node '%s' could not be created."), *InNode.Id)); return nullptr; }

        const TWeakPtr<FCkUiView> WeakOwner = const_cast<FCkUiView*>(this)->AsShared();
        const auto CellFactory = [WeakOwner](const FCkUiNode& Cell, const TWeakPtr<const FCkUiTreeNode> WeakRecord, FString& Failure) -> TSharedPtr<FCkUiView>
        {
            const TSharedPtr<FCkUiView> Owner = WeakOwner.Pin();
            if (!Owner.IsValid() || !WeakRecord.IsValid()) { Failure = TEXT("The tree row owner or record expired."); return nullptr; }
            TArray<FString> Errors;
            TSharedPtr<FCkUiView> Result = Owner->MakeCellView(Cell, WeakRecord, false, Errors);
            if (!Result.IsValid()) { Failure = Errors.IsEmpty() ? TEXT("Tree row staging failed.") : Errors[0]; }
            return Result;
        };
        FString Failure;
        const TAttribute<FText> Filter = InNode.FilterBinding.IsEmpty() ? TAttribute<FText>(FText::GetEmpty()) : _Data.Text.FindRef(InNode.FilterBinding);
        const FOnCkUiTreeSelectionChanged ConsumerSelection = _Data.TreeSelectionChanged.FindRef(InNode.SelectionAction);
        const FOnCkUiTreeSelectionChanged Selection = InNode.SelectionAction.IsEmpty() ? FOnCkUiTreeSelectionChanged{}
            : FOnCkUiTreeSelectionChanged::CreateLambda([WeakOwner, ConsumerSelection](TOptional<FString> Key, ESelectInfo::Type Info)
            { if (const TSharedPtr<FCkUiView> Owner = WeakOwner.Pin(); Owner.IsValid() && Owner->CanDispatchEvents()) { ConsumerSelection.ExecuteIfBound(MoveTemp(Key), Info); } });
        TUniquePtr<ICkUiPreparedWidgetUpdate> Prepared = Tree->Prepare(InNode, CellFactory, Filter, Selection, Failure,
            MakeContextMenuBinding(InNode, InOutStaged.Menus));
        if (!Failure.IsEmpty() || !Prepared.IsValid()) { OutErrors.Add(!Failure.IsEmpty() ? Failure : FString::Printf(TEXT("Tree node '%s' returned no prepared update."), *InNode.Id)); return nullptr; }
        InOutStaged.PreparedUpdates.Add(MoveTemp(Prepared));
        const TSharedRef<SBox> Port = ck_ui_surface::MakeMeasuredPort(Tree.ToSharedRef(), InNode.Id);
        InOutStaged.Retained.Add(FRetainedRecord{InNode.Id, ERetainedKind::Tree, InNode.Binding, FString{}, Tree, nullptr, Port});
        const TSharedRef<SWidget> Padded = SNew(SBox).Padding(InNode.Style.Padding)[Port];
        return ApplyVisibility(ApplyStyle(InNode.Style, Padded));
    }
    if (InNode.Kind == ECkUiNodeKind::Custom)
    {
        const FCkUiCustomWidgetRegistration* Registration = _CustomRegistry.IsValid() ? _CustomRegistry->Find(InNode.CustomTag) : nullptr;
        if (Registration == nullptr)
        { OutErrors.Add(FString::Printf(TEXT("Custom node '%s' is not registered."), *InNode.Id)); return nullptr; }

        const TWeakPtr<FCkUiView> WeakOwner = const_cast<FCkUiView*>(this)->AsShared();
        auto Arguments = FCkUiCustomWidgetArguments{.Id = InNode.Id, .Style = InNode.Style, .CanDispatchEvents = TAttribute<bool>::CreateLambda([WeakOwner]()
        {
            const TSharedPtr<FCkUiView> Owner = WeakOwner.Pin();
            return Owner.IsValid() && Owner->CanDispatchEvents();
        }), .BaseFont = _BaseFont, .SlateUserIndex = _Data.SlateUserIndex};
        for (const auto& [Name, Value] : InNode.CustomProperties)
        {
            switch (Value.Kind)
            {
            case ECkUiCustomPropertyKind::Text: Arguments.TextProperties.Add(Name, Value.Text); break;
            case ECkUiCustomPropertyKind::Number: Arguments.NumberProperties.Add(Name, Value.Number); break;
            case ECkUiCustomPropertyKind::Bool: Arguments.BoolProperties.Add(Name, Value.Bool); break;
            case ECkUiCustomPropertyKind::Color: Arguments.ColorProperties.Add(Name, Value.Color); break;
            case ECkUiCustomPropertyKind::TextBinding: Arguments.TextBindings.Add(Name, _Data.Text.FindRef(Value.Name)); break;
            case ECkUiCustomPropertyKind::StringBinding: Arguments.StringBindings.Add(Name, _Data.String.FindRef(Value.Name)); break;
            case ECkUiCustomPropertyKind::CollectionBinding: Arguments.Collections.Add(Name, _Data.Collections.FindRef(Value.Name)); break;
            case ECkUiCustomPropertyKind::StringChanged:
            {
                const FCkUiOnStringChanged Callback = _Data.StringChanged.FindRef(Value.Name);
                Arguments.StringChanged.Add(Name, FCkUiOnStringChanged::CreateLambda([WeakOwner, Callback](const FString& InValue)
                {
                    const TSharedPtr<FCkUiView> Owner = WeakOwner.Pin();
                    if (Owner.IsValid() && Owner->CanDispatchEvents()) { Callback.ExecuteIfBound(InValue); }
                }));
                break;
            }
            case ECkUiCustomPropertyKind::ImageBinding: Arguments.ImageBindings.Add(Name, _Data.Images.FindRef(Value.Name)); break;
            case ECkUiCustomPropertyKind::NumberBinding: Arguments.NumberBindings.Add(Name, _Data.Number.FindRef(Value.Name)); break;
            case ECkUiCustomPropertyKind::BoolBinding: Arguments.BoolBindings.Add(Name, _Data.Visibility.FindRef(Value.Name)); break;
            case ECkUiCustomPropertyKind::ColorBinding: Arguments.ColorBindings.Add(Name, _Data.Color.FindRef(Value.Name)); break;
            case ECkUiCustomPropertyKind::Action:
            {
                const FSimpleDelegate Callback = _Actions.FindRef(Value.Name);
                Arguments.Actions.Add(Name, FSimpleDelegate::CreateLambda([WeakOwner, Callback]()
                { if (const TSharedPtr<FCkUiView> Owner = WeakOwner.Pin(); Owner.IsValid() && Owner->CanDispatchEvents()) { Callback.ExecuteIfBound(); } }));
                break;
            }
            case ECkUiCustomPropertyKind::TextChanged:
            {
                const FOnTextChanged Callback = _Data.TextChanged.FindRef(Value.Name);
                Arguments.TextChanged.Add(Name, FOnTextChanged::CreateLambda([WeakOwner, Callback](const FText& InText)
                {
                    const TSharedPtr<FCkUiView> Owner = WeakOwner.Pin();
                    if (Owner.IsValid() && Owner->CanDispatchEvents()) { Callback.ExecuteIfBound(InText); }
                }));
                break;
            }
            case ECkUiCustomPropertyKind::TextCommitted:
            {
                const FOnTextCommitted Callback = _Data.TextCommitted.FindRef(Value.Name);
                Arguments.TextCommitted.Add(Name, FOnTextCommitted::CreateLambda([WeakOwner, Callback](const FText& InText, const ETextCommit::Type InCommitType)
                {
                    const TSharedPtr<FCkUiView> Owner = WeakOwner.Pin();
                    if (Owner.IsValid() && Owner->CanDispatchEvents()) { Callback.ExecuteIfBound(InText, InCommitType); }
                }));
                break;
            }
            case ECkUiCustomPropertyKind::BoolChanged:
            {
                const FCkUiOnBoolChanged Callback = _Data.BoolChanged.FindRef(Value.Name);
                Arguments.BoolChanged.Add(Name, FCkUiOnBoolChanged::CreateLambda([WeakOwner, Callback](const bool InValue)
                {
                    const TSharedPtr<FCkUiView> Owner = WeakOwner.Pin();
                    if (Owner.IsValid() && Owner->CanDispatchEvents()) { Callback.ExecuteIfBound(InValue); }
                }));
                break;
            }
            case ECkUiCustomPropertyKind::NumberChanged:
            {
                const FCkUiOnNumberChanged Callback = _Data.NumberChanged.FindRef(Value.Name);
                Arguments.NumberChanged.Add(Name, FCkUiOnNumberChanged::CreateLambda([WeakOwner, Callback](const float InValue)
                {
                    const TSharedPtr<FCkUiView> Owner = WeakOwner.Pin();
                    if (FMath::IsFinite(InValue) && Owner.IsValid() && Owner->CanDispatchEvents()) { Callback.ExecuteIfBound(InValue); }
                }));
                break;
            }
            case ECkUiCustomPropertyKind::NumberCommitted:
            {
                const FCkUiOnNumberCommitted Callback = _Data.NumberCommitted.FindRef(Value.Name);
                Arguments.NumberCommitted.Add(Name, FCkUiOnNumberCommitted::CreateLambda([WeakOwner, Callback](const float InValue, const ETextCommit::Type InCommitType)
                {
                    const TSharedPtr<FCkUiView> Owner = WeakOwner.Pin();
                    if (FMath::IsFinite(InValue) && Owner.IsValid() && Owner->CanDispatchEvents()) { Callback.ExecuteIfBound(InValue, InCommitType); }
                }));
                break;
            }
            case ECkUiCustomPropertyKind::NumberInteraction:
            {
                const FCkUiOnNumberInteraction Callback = _Data.NumberInteraction.FindRef(Value.Name);
                Arguments.NumberInteraction.Add(Name, FCkUiOnNumberInteraction::CreateLambda([WeakOwner, Callback](const FCkUiNumberInteraction& InEvent)
                {
                    const bool ValidPhase = InEvent.Phase == ECkUiInteractionPhase::Begin
                        || InEvent.Phase == ECkUiInteractionPhase::Commit || InEvent.Phase == ECkUiInteractionPhase::Cancel;
                    const bool ValidSource = InEvent.Source == ECkUiInteractionSource::Pointer
                        || InEvent.Source == ECkUiInteractionSource::Keyboard || InEvent.Source == ECkUiInteractionSource::Controller;
                    if (!ValidPhase || !ValidSource || !FMath::IsFinite(InEvent.Value)) { return; }
                    const TSharedPtr<FCkUiView> Owner = WeakOwner.Pin();
                    if (Owner.IsValid() && Owner->CanDispatchEvents()) { Callback.ExecuteIfBound(InEvent); }
                }));
                break;
            }
            default: OutErrors.Add(FString::Printf(TEXT("Custom node '%s' has an invalid property."), *InNode.Id)); return nullptr;
            }
            if (Value.Kind == ECkUiCustomPropertyKind::StringBinding || Value.Kind == ECkUiCustomPropertyKind::StringChanged
                || Value.Kind == ECkUiCustomPropertyKind::CollectionBinding || Value.Kind == ECkUiCustomPropertyKind::TextBinding || Value.Kind == ECkUiCustomPropertyKind::ImageBinding
                || Value.Kind == ECkUiCustomPropertyKind::NumberBinding || Value.Kind == ECkUiCustomPropertyKind::BoolBinding
                || Value.Kind == ECkUiCustomPropertyKind::ColorBinding || Value.Kind == ECkUiCustomPropertyKind::Action
                || Value.Kind == ECkUiCustomPropertyKind::TextChanged || Value.Kind == ECkUiCustomPropertyKind::TextCommitted || Value.Kind == ECkUiCustomPropertyKind::BoolChanged || Value.Kind == ECkUiCustomPropertyKind::NumberChanged || Value.Kind == ECkUiCustomPropertyKind::NumberCommitted || Value.Kind == ECkUiCustomPropertyKind::NumberInteraction)
            { Arguments.BindingNames.Add(Name, Value.Name); }
        }
        if (!PrepareCustomSlots(InNode, Registration->Schema, InOutStaged, Arguments, OutErrors)) { return nullptr; }
        if (Registration->RetainedFactory)
        {
            FString StateKey;
            if (!Registration->Schema.StateKeyProperty.IsEmpty())
            {
                const FCkUiCustomPropertyValue* StateValue = InNode.CustomProperties.Find(Registration->Schema.StateKeyProperty);
                if (StateValue == nullptr || StateValue->Kind != ECkUiCustomPropertyKind::Text || StateValue->Text.IsEmpty())
                { OutErrors.Add(FString::Printf(TEXT("Custom node '%s' has an invalid state key."), *InNode.Id)); return nullptr; }
                StateKey = StateValue->Text.ToString();
            }
            TSharedPtr<ICkUiRetainedWidget> Component;
            const FRetainedRecord* Existing = _CommittedRetained.Find(InNode.Id);
            if (Existing != nullptr && Existing->Kind == ERetainedKind::Custom) { Component = Existing->Component; }
            FString Failure;
            if (!Component.IsValid()) { Component = Registration->RetainedFactory(Arguments, Failure); }
            if (!Failure.IsEmpty() || !Component.IsValid())
            { OutErrors.Add(!Failure.IsEmpty() ? FString::Printf(TEXT("Custom node '%s' (%s): %s"), *InNode.Id, *InNode.CustomTag, *Failure) : FString::Printf(TEXT("Custom node '%s' (%s) returned null."), *InNode.Id, *InNode.CustomTag)); return nullptr; }
            const TSharedRef<SWidget> Widget = Existing != nullptr ? Existing->Widget.ToSharedRef() : Component->GetWidget();
            if (Existing != nullptr && Widget->GetParentWidget() != Existing->Port)
            { OutErrors.Add(FString::Printf(TEXT("Custom node '%s' (%s) cached widget is not mounted in its retained port."), *InNode.Id, *InNode.CustomTag)); return nullptr; }
            if (Widget->GetParentWidget().IsValid() && (Existing == nullptr || Existing->Widget != Widget))
            { OutErrors.Add(FString::Printf(TEXT("Custom component '%s' returned an already mounted widget."), *InNode.CustomTag)); return nullptr; }
            auto CandidateWidgets = TSet<const SWidget*>{};
            if (!ck_ui_surface::CollectWidgetPointers(Widget, CandidateWidgets))
            { OutErrors.Add(FString::Printf(TEXT("Custom component '%s' output reuses a widget within its hierarchy."), *InNode.CustomTag)); return nullptr; }
            // Only declared slot mounts may contain independently owned authored descendants.
            for (const auto& [Name, Mount] : Arguments.Slots)
            {
                if (!Mount.IsValid() || !CandidateWidgets.Contains(Mount.Get()) || Mount == Widget)
                { OutErrors.Add(TEXT("Custom container must mount every declared slot exactly once below its root.")); return nullptr; }
                const auto* Children = Mount->GetChildren();
                for (int32 Index = 0; Children != nullptr && Index < Children->Num(); ++Index)
                {
                    TSet<const SWidget*> Descendants;
                    ck_ui_surface::CollectWidgetPointers(ConstCastSharedRef<SWidget>(Children->GetChildAt(Index)), Descendants);
                    for (const auto& [OtherName, OtherMount] : Arguments.Slots)
                    {
                        if (OtherName != Name && Descendants.Contains(OtherMount.Get()))
                        { OutErrors.Add(TEXT("Custom container declared slot mounts must not contain one another.")); return nullptr; }
                    }
                    for (const auto* Descendant : Descendants) { CandidateWidgets.Remove(Descendant); }
                }
            }
            for (const auto& [Binding, Native] : _Bindings)
            {
                if (Native.IsValid() && CandidateWidgets.Contains(Native.Get()))
                { OutErrors.Add(FString::Printf(TEXT("Custom component '%s' aliases native binding '%s'."), *InNode.CustomTag, *Binding)); return nullptr; }
            }
            auto ExistingWidgets = TSet<const SWidget*>{};
            auto ReusableWidgets = TSet<const SWidget*>{};
            if (Existing != nullptr && Existing->Widget.IsValid())
            { ck_ui_surface::CollectWidgetPointers(Existing->Widget.ToSharedRef(), ReusableWidgets); }
            for (const auto& [Binding, Native] : _Bindings)
            { if (Native.IsValid()) { ck_ui_surface::CollectWidgetPointers(Native.ToSharedRef(), ExistingWidgets); } }
            for (const auto& [Name, Mount] : _RegionMounts)
            { if (Mount.IsValid()) { ck_ui_surface::CollectWidgetPointers(Mount.ToSharedRef(), ExistingWidgets); } }
            for (const auto& [Id, Record] : _CommittedRetained)
            {
                if (Id != InNode.Id && Record.Widget.IsValid()) { ck_ui_surface::CollectWidgetPointers(Record.Widget.ToSharedRef(), ExistingWidgets); }
            }
            for (const FRetainedRecord& Record : InOutStaged.Retained)
            { if (Record.Widget.IsValid()) { ck_ui_surface::CollectWidgetPointers(Record.Widget.ToSharedRef(), ExistingWidgets); } }
            for (const TSharedPtr<SWidget>& ExistingCustom : InOutStaged.CustomWidgets)
            { if (ExistingCustom.IsValid()) { ck_ui_surface::CollectWidgetPointers(ExistingCustom.ToSharedRef(), ExistingWidgets); } }
            for (const SWidget* CandidateWidget : CandidateWidgets)
            {
                if (ExistingWidgets.Contains(CandidateWidget) && !ReusableWidgets.Contains(CandidateWidget))
                { OutErrors.Add(FString::Printf(TEXT("Custom component '%s' aliases an existing retained widget."), *InNode.CustomTag)); return nullptr; }
            }
            FString PrepareFailure;
            TUniquePtr<ICkUiPreparedWidgetUpdate> Prepared = Component->PrepareReload(Arguments, PrepareFailure);
            if (!PrepareFailure.IsEmpty() || !Prepared.IsValid())
            { OutErrors.Add(!PrepareFailure.IsEmpty() ? FString::Printf(TEXT("Custom node '%s' (%s): %s"), *InNode.Id, *InNode.CustomTag, *PrepareFailure) : FString::Printf(TEXT("Custom node '%s' (%s) returned no prepared update."), *InNode.Id, *InNode.CustomTag)); return nullptr; }
            InOutStaged.PreparedUpdates.Add(MoveTemp(Prepared));
            InOutStaged.Retained.Add(FRetainedRecord{InNode.Id, ERetainedKind::Custom, StateKey, InNode.CustomTag, Widget, Component, ck_ui_surface::MakeMeasuredPort(Widget, InNode.Id)});
            InOutStaged.CustomWidgets.Add(Widget);
            return ApplyVisibility(ApplyStyle(InNode.Style, InOutStaged.Retained.Last().Port.ToSharedRef()));
        }
        FString Failure;
        const TSharedPtr<SWidget> Custom = Registration->Factory(Arguments, Failure);
        if (!Failure.IsEmpty())
        { OutErrors.Add(FString::Printf(TEXT("Custom node '%s' (%s): %s"), *InNode.Id, *InNode.CustomTag, *Failure)); return nullptr; }
        if (!Custom.IsValid())
        { OutErrors.Add(FString::Printf(TEXT("Custom node '%s' (%s) returned null."), *InNode.Id, *InNode.CustomTag)); return nullptr; }
        if (Custom->GetParentWidget().IsValid())
        { OutErrors.Add(FString::Printf(TEXT("Custom widget '%s' returned an already mounted widget."), *InNode.CustomTag)); return nullptr; }
        auto CandidateWidgets = TSet<const SWidget*>{};
        if (!ck_ui_surface::CollectWidgetPointers(Custom.ToSharedRef(), CandidateWidgets))
        { OutErrors.Add(FString::Printf(TEXT("Custom widget '%s' output reuses a widget within its hierarchy."), *InNode.CustomTag)); return nullptr; }
        auto ExistingWidgets = TSet<const SWidget*>{};
        for (const auto& [Binding, Native] : _Bindings)
        { if (Native.IsValid()) { ck_ui_surface::CollectWidgetPointers(Native.ToSharedRef(), ExistingWidgets); } }
        for (const auto& [Name, Mount] : _RegionMounts)
        {
            if (Mount.IsValid()) { ck_ui_surface::CollectWidgetPointers(Mount.ToSharedRef(), ExistingWidgets); }
        }
        for (const TSharedPtr<SWidget>& Existing : InOutStaged.CustomWidgets)
        {
            if (Existing.IsValid()) { ck_ui_surface::CollectWidgetPointers(Existing.ToSharedRef(), ExistingWidgets); }
        }
        for (const SWidget* Candidate : CandidateWidgets)
        {
            if (ExistingWidgets.Contains(Candidate))
            { OutErrors.Add(FString::Printf(TEXT("Custom widget '%s' aliases a committed or staged custom widget."), *InNode.CustomTag)); return nullptr; }
        }
        for (const auto& [Binding, Native] : _Bindings)
        {
            if (Native.IsValid() && CandidateWidgets.Contains(Native.Get()))
            { OutErrors.Add(FString::Printf(TEXT("Custom widget '%s' aliases native binding '%s'."), *InNode.CustomTag, *Binding)); return nullptr; }
        }
        for (const auto& [Id, Record] : _CommittedRetained)
        {
            if (Record.Kind == ERetainedKind::Search && Record.Widget.IsValid() && CandidateWidgets.Contains(Record.Widget.Get()))
            { OutErrors.Add(FString::Printf(TEXT("Custom widget '%s' aliases retained search '%s'."), *InNode.CustomTag, *Id)); return nullptr; }
        }
        for (const FRetainedRecord& Record : InOutStaged.Retained)
        {
            if (Record.Kind == ERetainedKind::Search && Record.Widget.IsValid() && CandidateWidgets.Contains(Record.Widget.Get()))
            { OutErrors.Add(FString::Printf(TEXT("Custom widget '%s' aliases staged search '%s'."), *InNode.CustomTag, *Record.Id)); return nullptr; }
        }
        InOutStaged.CustomWidgets.Add(Custom);
        const TSharedRef<SBox> Mount = ck_ui_surface::MakeMeasuredPort(Custom.ToSharedRef(), InNode.Id);
        Mount->SetContent(Custom.ToSharedRef());
        return ApplyVisibility(ApplyStyle(InNode.Style, Mount));
    }
    if (InNode.Kind == ECkUiNodeKind::Tabs)
    {
        TArray<SCkUiTabs::FPanel> Panels;
        for (const FCkUiNode& Tab : InNode.Children)
        {
            const TSharedPtr<SWidget> Content = StageNode(Tab, InOutStaged, OutErrors);
            if (!Content.IsValid()) { return nullptr; }
            const TWeakPtr<FCkUiView> WeakOwner = const_cast<FCkUiView*>(this)->AsShared();
            const TWeakPtr<SWidget> WeakContent = Content;
            TFunction<void()> OnDeactivate = [WeakOwner, WeakContent]()
            {
                const TSharedPtr<FCkUiView> Owner = WeakOwner.Pin();
                const TSharedPtr<SWidget> Root = WeakContent.Pin();
                if (Owner.IsValid() && Root.IsValid()) { Owner->ReleaseTransientInteractions(Root.ToSharedRef()); }
            };
            InOutStaged.OwnedWidgets.Add(Content);
            Panels.Add({Tab.TabKey,
                Tab.TabLabelBinding.IsEmpty() ? TAttribute<FText>(FText::FromString(Tab.Text)) : _Data.Text.FindRef(Tab.TabLabelBinding),
                Tab.TabEnabledBinding.IsEmpty() ? TAttribute<bool>(true) : _Data.Visibility.FindRef(Tab.TabEnabledBinding), Content, MoveTemp(OnDeactivate)});
        }
        TSharedPtr<SCkUiTabs> Tabs = GetTabs(InNode.Id);
        if (!Tabs.IsValid()) { Tabs = SNew(SCkUiTabs).Tag(FName(*InNode.Id)); }
        const TWeakPtr<FCkUiView> WeakView = const_cast<FCkUiView*>(this)->AsShared();
        const FCkUiOnStringChanged Consumer = _Data.StringChanged.FindRef(InNode.Action);
        const FCkUiOnStringChanged Changed = FCkUiOnStringChanged::CreateLambda([WeakView, Consumer](const FString& Key)
        {
            const TSharedPtr<FCkUiView> View = WeakView.Pin();
            if (View.IsValid() && View->CanDispatchEvents()) { Consumer.ExecuteIfBound(Key); }
        });
        const TAttribute<bool> CanDispatch = TAttribute<bool>::CreateLambda([WeakView]()
        {
            const TSharedPtr<FCkUiView> View = WeakView.Pin();
            return View.IsValid() && View->CanDispatchEvents();
        });
        const TAttribute<FString> Value = _Data.String.FindRef(InNode.Binding);
        const FSlateFontInfo Font = MakeFont();
        InOutStaged.TabsUpdates.Add([Tabs, Panels = MoveTemp(Panels), Value, Changed, CanDispatch, Font]() mutable
        { Tabs->SetConfiguration(MoveTemp(Panels), Value, Changed, CanDispatch, Font); });
        const TSharedRef<SBox> Port = ck_ui_surface::MakeMeasuredPort(Tabs.ToSharedRef(), InNode.Id);
        InOutStaged.Retained.Add(FRetainedRecord{InNode.Id, ERetainedKind::Tabs, InNode.Binding, FString{}, Tabs, nullptr, Port});
        return ApplyVisibility(ApplyStyle(InNode.Style, Port));
    }
    if (InNode.Kind == ECkUiNodeKind::Splitter)
    {
        TArray<SCkUiSplitter::FPane> Panes;
        for (const FCkUiNode& ChildNode : InNode.Children)
        {
            const TSharedPtr<SWidget> Child = StageNode(ChildNode, InOutStaged, OutErrors);
            if (!Child.IsValid()) { return nullptr; }
            Panes.Add({ChildNode.Id, Child, ChildNode.Style.Grow > 0.0f ? ChildNode.Style.Grow : 1.0f,
                InNode.SplitterDirection == Orient_Horizontal ? ChildNode.Style.MinWidth : ChildNode.Style.MinHeight});
        }
        TSharedPtr<SCkUiSplitter> Splitter = GetSplitter(InNode.Id);
        if (!Splitter.IsValid()) { Splitter = SNew(SCkUiSplitter).Tag(FName(*InNode.Id)).Orientation(InNode.SplitterDirection); }
        FString Failure;
        TUniquePtr<ICkUiPreparedWidgetUpdate> Prepared = Splitter->Prepare(MoveTemp(Panes), Failure);
        if (!Prepared.IsValid() || !Failure.IsEmpty())
        { OutErrors.Add(FString::Printf(TEXT("Splitter '%s': %s"), *InNode.Id, *Failure)); return nullptr; }
        const TSharedRef<SBox> Port = ck_ui_surface::MakeMeasuredPort(Splitter.ToSharedRef(), InNode.Id);
        Port->SetPadding(InNode.Style.Padding);
        InOutStaged.Retained.Add(FRetainedRecord{InNode.Id, ERetainedKind::Splitter,
            InNode.SplitterDirection == Orient_Horizontal ? TEXT("horizontal") : TEXT("vertical"), FString{}, Splitter, nullptr, Port});
        InOutStaged.PreparedUpdates.Add(MoveTemp(Prepared));
        return ApplyVisibility(ApplyStyle(InNode.Style, Port));
    }
    if (InNode.Kind == ECkUiNodeKind::Scroll)
    {
        const FCkUiNode& ChildNode = InNode.Children[0];
        const TSharedPtr<SWidget> Child = StageNode(ChildNode, InOutStaged, OutErrors);
        if (!Child.IsValid()) { return nullptr; }
        TSharedPtr<SScrollBox> Scroll = GetScroll(InNode.Id);
        if (!Scroll.IsValid())
        {
            if (InNode.ScrollDirection == Orient_Vertical)
            { Scroll = SNew(SCkUiScrollBox).Tag(FName(*InNode.Id)); }
            else
            { Scroll = SNew(SScrollBox).Tag(FName(*InNode.Id)).Orientation(InNode.ScrollDirection)
                .ConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible).AllowOverscroll(EAllowOverscroll::No)
                .AllowContentToShrink(false); }
        }
        const TSharedRef<SBox> Port = ck_ui_surface::MakeMeasuredPort(Scroll.ToSharedRef(), InNode.Id);
        InOutStaged.Retained.Add(FRetainedRecord{InNode.Id, ERetainedKind::Scroll,
            InNode.ScrollDirection == Orient_Horizontal ? TEXT("horizontal") : TEXT("vertical"), FString{}, Scroll, nullptr, Port});
        InOutStaged.ScrollContents.Add({Scroll, Child, InNode.Style.Padding, InNode.ScrollDirection});
        return ApplyVisibility(ApplyStyle(InNode.Style, Port));
    }
    if (InNode.Kind == ECkUiNodeKind::Overlay)
    {
        const TSharedRef<ck_ui_surface::SCkUiOverlay> Overlay = SNew(ck_ui_surface::SCkUiOverlay).Tag(FName(*InNode.Id)).Clipping(EWidgetClipping::ClipToBounds);
        for (const FCkUiNode& ChildNode : InNode.Children)
        {
            const TSharedPtr<SWidget> Child = StageNode(ChildNode, InOutStaged, OutErrors);
            if (!Child.IsValid()) { return nullptr; }
            Overlay->AddSlot().Padding(InNode.Style.Padding).HAlign(ChildNode.Style.HAlign).VAlign(ChildNode.Style.VAlign)[Child.ToSharedRef()];
            if (&ChildNode == &InNode.Children[0])
            {
                if (const TSharedPtr<FCkFlexMeasureMetaData> Measure = Child->GetMetaData<FCkFlexMeasureMetaData>(); Measure.IsValid())
                {
                    const FMargin Padding = InNode.Style.Padding;
                    Overlay->AddMetadata(MakeShared<FCkFlexMeasureMetaData>(
                        [WeakChild = TWeakPtr<SWidget>(Child), Measure, Padding](const FCkFlexMeasureArgs& Args) -> FVector2D
                        {
                            const TSharedPtr<SWidget> Pinned = WeakChild.Pin();
                            if (!Pinned.IsValid() || Pinned->GetVisibility() == EVisibility::Collapsed) { return FVector2D::ZeroVector; }
                            auto Inner = Args;
                            if (Inner.WidthMode != YGMeasureModeUndefined) { Inner.AvailableWidth = FMath::Max(0.0f, Inner.AvailableWidth - Padding.GetTotalSpaceAlong<Orient_Horizontal>()); }
                            if (Inner.HeightMode != YGMeasureModeUndefined) { Inner.AvailableHeight = FMath::Max(0.0f, Inner.AvailableHeight - Padding.GetTotalSpaceAlong<Orient_Vertical>()); }
                            return Measure->Measure(Inner) + Padding.GetDesiredSize();
                        },
                        [WeakChild = TWeakPtr<SWidget>(Child), Measure, Padding](const float Width, const float Height)
                        {
                            if (WeakChild.IsValid())
                            { Measure->NotifyArranged(FMath::Max(0.0f, Width - Padding.GetTotalSpaceAlong<Orient_Horizontal>()), FMath::Max(0.0f, Height - Padding.GetTotalSpaceAlong<Orient_Vertical>())); }
                        }));
                }
            }
        }
        return ApplyVisibility(ApplyStyle(InNode.Style, Overlay));
    }

    auto SlotArguments = TArray<SCkFlexBox::FSlot::FSlotArguments>{};
    SlotArguments.Reserve(InNode.Children.Num());
    for (const FCkUiNode& ChildNode : InNode.Children)
    {
        const TSharedPtr<SWidget> Child = StageNode(ChildNode, InOutStaged, OutErrors);
        if (!Child.IsValid()) { return nullptr; }
        SlotArguments.Add(ck_ui_surface::MakeSlotArguments(ChildNode.Style, Child.ToSharedRef()));
    }
    auto FlexArguments = SCkFlexBox::FArguments{};
    FlexArguments._Direction = InNode.Kind == ECkUiNodeKind::Row ? Orient_Horizontal : Orient_Vertical;
    FlexArguments._Wrap = InNode.Style.FlexWrap;
    FlexArguments._Gap = InNode.Style.Gap;
    FlexArguments._Padding = InNode.Style.Padding;
    FlexArguments._Slots = MoveTemp(SlotArguments);
    const auto Flex = SArgumentNew(FlexArguments, SCkFlexBox);
    Flex->SetTag(FName(*InNode.Id));
    return ApplyVisibility(ApplyStyle(InNode.Style, Flex));
}

auto FCkUiView::StageDocument(const FCkUiDocument& InDocument, FStagedDocument& OutStaged, TArray<FString>& OutErrors) const -> bool
{
    OutStaged.Menus = InDocument.Menus;
    for (const auto& [Name, Root] : InDocument.Regions)
    {
        const TSharedPtr<SWidget> RootWidget = StageNode(Root, OutStaged, OutErrors);
        if (!RootWidget.IsValid())
        {
            OutErrors.Add(FString::Printf(TEXT("Could not stage region '%s'."), *Name));
            return false;
        }
        OutStaged.Regions.Add(Name, RootWidget);
    }
    return OutErrors.IsEmpty();
}

auto FCkUiView::ContainsMountedPath(const FWidgetPath& InPath) const -> bool
{
    for (const auto& [Name, Mount] : _RegionMounts)
    {
        if (Mount.IsValid() && ck_ui_surface::HasWidget(InPath, Mount)) { return true; }
    }
    return false;
}

auto FCkUiView::GetOwnedPointerCaptures(EVisibility InVisibility) const -> TArray<FCapturedPointer>
{
    TArray<FCapturedPointer> CapturedPointers;
    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication& Slate = FSlateApplication::Get();
        for (const auto& [Id, Existing] : _CommittedRetained)
        {
            TArray<FCkUiPointerCapture> Reports;
            if (Existing.Component.IsValid()) { Reports = Existing.Component->GetPointerCaptures(); }
            else if (Existing.Kind == ERetainedKind::Splitter)
            {
                Reports.Add({Slate.GetCursorUser()->GetUserIndex(), FSlateApplication::CursorPointerIndex,
                    StaticCastSharedPtr<SCkUiSplitter>(Existing.Widget)->GetSplitter()});
            }
            for (const FCkUiPointerCapture& Report : Reports)
            {
                const TSharedPtr<FSlateUser> User = Slate.GetUser(Report.UserIndex);
                const TSharedPtr<SWidget> Widget = Report.Widget.Pin();
                if (!User.IsValid() || !Widget.IsValid() || User->GetPointerCaptor(Report.PointerIndex) != Widget) { continue; }
                FWidgetPath Path;
                if (!Slate.GeneratePathToWidgetUnchecked(Widget.ToSharedRef(), Path, InVisibility)
                    || !ContainsMountedPath(Path) || !ck_ui_surface::HasWidget(Path, Existing.Widget)) { continue; }
                const bool Duplicate = CapturedPointers.ContainsByPredicate([&Report](const FCapturedPointer& Item)
                { return Item.Capture.UserIndex == Report.UserIndex && Item.Capture.PointerIndex == Report.PointerIndex; });
                if (!Duplicate) { CapturedPointers.Add({Report, Id}); }
            }
        }
    }
    return CapturedPointers;
}

auto FCkUiView::ReleaseTransientInteractions(const TSharedRef<SWidget>& InRoot) -> void
{
    if (!FSlateApplication::IsInitialized()) { return; }
    FSlateApplication& Slate = FSlateApplication::Get();
    TSet<const SWidget*> OwnedWidgets;
    ck_ui_surface::CollectWidgetPointers(InRoot, OwnedWidgets);
    TArray<TSharedPtr<ICkUiRetainedWidget>> Components;
    TArray<TSharedPtr<SCkUiMenuButton>> Menus;
    TArray<TSharedPtr<SCkUiTable>> Tables;
    TArray<TSharedPtr<SCkUiTree>> Trees;
    for (const auto& [Id, Record] : _CommittedRetained)
    {
        if (!OwnedWidgets.Contains(Record.Widget.Get())) { continue; }
        if (Record.Component.IsValid()) { Components.Add(Record.Component); }
        if (Record.Kind == ERetainedKind::MenuButton) { Menus.Add(StaticCastSharedPtr<SCkUiMenuButton>(Record.Widget)); }
        if (Record.Kind == ERetainedKind::Table) { Tables.Add(StaticCastSharedPtr<SCkUiTable>(Record.Widget)); }
        if (Record.Kind == ERetainedKind::Tree) { Trees.Add(StaticCastSharedPtr<SCkUiTree>(Record.Widget)); }
    }
    TArray<FCkUiPointerCapture> Captures;
    for (const FCapturedPointer& Capture : GetOwnedPointerCaptures(EVisibility::All))
    {
        const TSharedPtr<SWidget> Widget = Capture.Capture.Widget.Pin();
        FWidgetPath Path;
        if (Widget.IsValid() && Slate.GeneratePathToWidgetUnchecked(Widget.ToSharedRef(), Path, EVisibility::All)
            && ck_ui_surface::HasWidget(Path, InRoot)) { Captures.Add(Capture.Capture); }
    }
    // Snapshot ownership before callbacks can reload or remove this view's records.
    for (const FCkUiPointerCapture& Capture : Captures)
    {
        const TSharedPtr<FSlateUser> User = Slate.GetUser(Capture.UserIndex);
        const TSharedPtr<SWidget> Widget = Capture.Widget.Pin();
        if (User.IsValid() && Widget.IsValid() && User->GetPointerCaptor(Capture.PointerIndex) == Widget)
        { User->ReleaseCapture(Capture.PointerIndex); }
    }
    for (const TSharedPtr<ICkUiRetainedWidget>& Component : Components) { Component->ReleaseTransientInteraction(); }
    for (const TSharedPtr<SCkUiMenuButton>& Menu : Menus) { Menu->ReleasePopup(); }
    for (const TSharedPtr<SCkUiTable>& Table : Tables) { Table->ReleaseContextMenu(); }
    for (const TSharedPtr<SCkUiTree>& Tree : Trees) { Tree->ReleaseContextMenu(); }
}

FCkUiView::~FCkUiView()
{
    if (!FSlateApplication::IsInitialized() || !IsInGameThread()) { return; }
    for (const auto& [Id, Record] : _CommittedRetained)
    {
        if (Record.Kind == ERetainedKind::Tabs) { StaticCastSharedPtr<SCkUiTabs>(Record.Widget)->Deactivate(); }
        if (Record.Kind == ERetainedKind::MenuButton) { StaticCastSharedPtr<SCkUiMenuButton>(Record.Widget)->Deactivate(); }
        if (Record.Kind == ERetainedKind::Table) { StaticCastSharedPtr<SCkUiTable>(Record.Widget)->ReleaseContextMenu(); }
        if (Record.Kind == ERetainedKind::Tree) { StaticCastSharedPtr<SCkUiTree>(Record.Widget)->ReleaseContextMenu(); }
    }
    // Mounted widgets can be held by their window after the view owner releases.
    for (const FCapturedPointer& Item : GetOwnedPointerCaptures(EVisibility::All))
    {
        const TSharedPtr<FSlateUser> User = FSlateApplication::Get().GetUser(Item.Capture.UserIndex);
        const TSharedPtr<SWidget> Widget = Item.Capture.Widget.Pin();
        if (User.IsValid() && Widget.IsValid() && User->GetPointerCaptor(Item.Capture.PointerIndex) == Widget)
        { User->ReleaseCapture(Item.Capture.PointerIndex); }
    }
}

auto FCkUiView::CaptureCommit(FStagedDocument& InStaged) -> FCommitState
{
    auto State = FCommitState{};
    auto& NextRetained = State.NextRetained;
    NextRetained.Reserve(InStaged.Retained.Num());
    for (FRetainedRecord& Record : InStaged.Retained)
    {
        const FString Id = Record.Id;
        NextRetained.Add(Id, MoveTemp(Record));
    }

    // Capture retired children while their native paths still belong to the mounted document.
    for (const auto& [Id, Repeat] : _RepeatStates)
    {
        for (const auto& Item : Repeat->Items)
        {
            if (InStaged.Nested.ContainsByPredicate([&Item](const auto& Child) { return Child->View == Item.View; })) { continue; }
            FStagedDocument Empty;
            State.RetiredChildren.Emplace(Item.View, MakeShared<FCommitState>(Item.View->CaptureCommit(Empty)));
        }
    }

    for (const auto& [Id, Container] : _CustomSlots)
    {
        for (const auto& [Name, Slot] : Container->Slots)
        {
            if (!Slot->View.IsValid() || InStaged.Nested.ContainsByPredicate([&Slot](const auto& Child) { return Child->View == Slot->View; })) { continue; }
            FStagedDocument Empty;
            State.RetiredChildren.Emplace(Slot->View, MakeShared<FCommitState>(Slot->View->CaptureCommit(Empty)));
        }
    }
    auto& FocusedUsers = State.FocusedUsers;
    State.CapturedPointers = GetOwnedPointerCaptures();
    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication& Slate = FSlateApplication::Get();
        Slate.ForEachUser([this, &InStaged, &State, &FocusedUsers, &NextRetained, &Slate](FSlateUser& User)
        {
            const TSharedPtr<SWidget> Candidate = User.GetFocusedWidget();
            auto FocusedPath = FWidgetPath{};
            if (!Candidate.IsValid() || !Slate.GeneratePathToWidgetUnchecked(Candidate.ToSharedRef(), FocusedPath)) { return; }
            for (const auto& Child : InStaged.Nested)
            {
                for (const auto& [Name, Mount] : Child->View->_RegionMounts)
                { if (ck_ui_surface::HasWidget(FocusedPath, Mount)) { return; } }
            }
            for (const auto& Child : State.RetiredChildren)
            {
                for (const auto& [Name, Mount] : Child.Key->_RegionMounts)
                { if (ck_ui_surface::HasWidget(FocusedPath, Mount)) { return; } }
            }
            auto bFocusedAuthored = false;
            for (const auto& [Name, Mount] : _RegionMounts)
            {
                for (int32 Index = 0; Mount.IsValid() && Index + 1 < FocusedPath.Widgets.Num(); ++Index)
                {
                    if (FocusedPath.Widgets[Index].Widget == Mount) { bFocusedAuthored = true; break; }
                }
                if (bFocusedAuthored) { break; }
            }
            if (!bFocusedAuthored) { return; }
            auto Focused = FCommitState::FFocusedUser{.UserIndex = User.GetUserIndex(), .Widget = Candidate};
            for (const auto& [Id, Existing] : _CommittedRetained)
            {
                if (!ck_ui_surface::HasWidget(FocusedPath, Existing.Widget)) { continue; }
                if (Existing.Kind == ERetainedKind::Tabs
                    && !StaticCastSharedPtr<SCkUiTabs>(Existing.Widget)->IsRetainedHeader(Candidate)) { continue; }
                // Retained containers do not retain their stateless descendant leaves.
                if ((Existing.Kind == ERetainedKind::Scroll || Existing.Kind == ERetainedKind::Splitter || Existing.Kind == ERetainedKind::Repeat)
                    && Existing.Widget != Focused.Widget) { continue; }
                if (const FRetainedRecord* Next = NextRetained.Find(Id); Next != nullptr
                    && Next->Kind == Existing.Kind && Next->CompatibilityKey == Existing.CompatibilityKey
                    && Next->CustomTag == Existing.CustomTag && Next->Widget == Existing.Widget)
                { Focused.RetainedSurvives = true; Focused.Component = Next->Component; }
                break;
            }
            FocusedUsers.Add(MoveTemp(Focused));
        }, true);
    }

    return State;
}

void FCkUiView::PublishConfiguration(FStagedDocument&& InStaged, FCommitState& InOutState)
{
    auto& PreviousRetained = InOutState.PreviousRetained;
    auto& NextRetained = InOutState.NextRetained;
    PreviousRetained = MoveTemp(_CommittedRetained);
    for (const auto& [Id, Record] : PreviousRetained)
    {
        if (Record.Port.IsValid()) { Record.Port->SetContent(SNullWidget::NullWidget); }
    }
    for (const auto& [Id, Record] : NextRetained)
    {
        if (Record.Port.IsValid() && Record.Widget.IsValid()) { Record.Port->SetContent(Record.Widget.ToSharedRef()); }
    }
    for (const auto& [Name, Root] : InStaged.Regions)
    {
        if (const TSharedPtr<SBox>* Mount = _RegionMounts.Find(Name); Mount != nullptr && Mount->IsValid())
        { (*Mount)->SetContent(Root.ToSharedRef()); }
    }
    _CommittedRetained = MoveTemp(NextRetained);
    _CommittedSearchHints = MoveTemp(InStaged.SearchHints);
    InOutState.PreviousRepeats = MoveTemp(_RepeatStates);
    _RepeatStates = MoveTemp(InStaged.Repeats);
    for (const auto& [Id, Previous] : InOutState.PreviousRepeats)
    {
        const auto Next = _RepeatStates.FindRef(Id);
        for (const auto& Item : Previous->Items)
        {
            if (!Next.IsValid() || !Next->Items.ContainsByPredicate([&Item](const auto& Other) { return Other.Scope == Item.Scope; }))
            { Item.Scope->Active = false; }
        }
    }
    InOutState.PreviousCustomSlots = MoveTemp(_CustomSlots);
    _CustomSlots = MoveTemp(InStaged.CustomSlots);
    for (const auto& [Id, Previous] : InOutState.PreviousCustomSlots)
    {
        const auto Next = _CustomSlots.FindRef(Id);
        for (const auto& [Name, Slot] : Previous->Slots)
        {
            const auto NewSlot = Next.IsValid() ? Next->Slots.FindRef(Name) : nullptr;
            if (Slot->Scope.IsValid() && (!NewSlot.IsValid() || NewSlot->Scope != Slot->Scope)) { Slot->Scope->Active = false; }
        }
    }
    ++_Revision;
    for (const FStagedDocument::FScrollContent& Content : InStaged.ScrollContents)
    {
        // Child trees are detached until whole-document acceptance. Keep the scroll object/offset.
        if (Content.Direction == Orient_Vertical)
        {
            StaticCastSharedPtr<SCkUiScrollBox>(Content.Scroll)->SetAuthoredContent(Content.Child.ToSharedRef(), Content.Padding);
            continue;
        }
        Content.Scroll->ClearChildren();
        Content.Scroll->AddSlot().FillContentSize(1.0f, 0.0f).Padding(Content.Padding)[Content.Child.ToSharedRef()];
    }
    for (TFunction<void()>& Update : InStaged.TabsUpdates) { Update(); }
    for (TFunction<void()>& Update : InStaged.MenuUpdates) { Update(); }
    for (TUniquePtr<ICkUiPreparedWidgetUpdate>& Update : InStaged.PreparedUpdates)
    { Update->Commit(); }

}

void FCkUiView::ReconcileInteractions(FCommitState& InOutState)
{
    // Containers restore their owned focus before retired child views clear it.
    for (const auto& [Id, Previous] : InOutState.PreviousRetained)
    {
        const FRetainedRecord* Next = _CommittedRetained.Find(Id);
        if (Previous.Component.IsValid() && (Next == nullptr || Next->Component != Previous.Component))
        { Previous.Component->ReleaseTransientInteraction(); }
    }
    for (auto& Child : InOutState.RetiredChildren) { Child.Key->ReconcileInteractions(*Child.Value); }
    for (const auto& [Id, Repeat] : InOutState.PreviousRepeats)
    {
        for (const auto& Item : Repeat->Items)
        { if (!Item.Scope->Active) { Item.View->ReleaseTransientInteractions(Item.View->GetRegion(TEXT("item"))); } }
    }
    for (const auto& [Id, Container] : InOutState.PreviousCustomSlots)
    {
        for (const auto& [Name, Slot] : Container->Slots)
        {
            if (Slot->View.IsValid() && Slot->Scope.IsValid() && !Slot->Scope->Active)
            { Slot->View->ReleaseTransientInteractions(Slot->View->GetRegion(TEXT("slot"))); }
        }
    }
    const TMap<FString, FRetainedRecord>& PreviousRetained = InOutState.PreviousRetained;
    const TArray<FCapturedPointer>& CapturedPointers = InOutState.CapturedPointers;
    const TArray<FCommitState::FFocusedUser>& FocusedUsers = InOutState.FocusedUsers;
    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication& Slate = FSlateApplication::Get();
        for (const FCapturedPointer& Item : CapturedPointers)
        {
            const TSharedPtr<FSlateUser> User = Slate.GetUser(Item.Capture.UserIndex);
            const TSharedPtr<SWidget> Widget = Item.Capture.Widget.Pin();
            if (!User.IsValid() || !Widget.IsValid() || User->GetPointerCaptor(Item.Capture.PointerIndex) != Widget) { continue; }
            const FRetainedRecord* Previous = PreviousRetained.Find(Item.RetainedId);
            const FRetainedRecord* Next = _CommittedRetained.Find(Item.RetainedId);
            FWidgetPath NewPath;
            const bool Survives = Previous != nullptr && Next != nullptr && Next->Widget == Previous->Widget
                && Next->Component == Previous->Component && Next->Kind == Previous->Kind
                && Next->CompatibilityKey == Previous->CompatibilityKey && Next->CustomTag == Previous->CustomTag;
            if (Survives && Slate.GeneratePathToWidgetUnchecked(Widget.ToSharedRef(), NewPath)
                && ContainsMountedPath(NewPath) && ck_ui_surface::HasWidget(NewPath, Next->Widget))
            {
                // Slate releases the old capture synchronously before assigning the new path.
                const TSharedPtr<ICkUiRetainedWidget> Component = Next->Component;
                if (Component.IsValid()) { Component->BeginPointerCaptureTransfer(Item.Capture); }
                const bool Restored = User->SetPointerCaptor(Item.Capture.PointerIndex, Widget.ToSharedRef(), NewPath);
                if (Component.IsValid()) { Component->EndPointerCaptureTransfer(Item.Capture, Restored); }
            }
            else { User->ReleaseCapture(Item.Capture.PointerIndex); }
        }
    }

    // Accepted menu reloads dismiss their owned popup before the generic focus repair.
    // Rejected documents never reach publication, so their open menus remain unchanged.
    for (const auto& [Id, Previous] : PreviousRetained)
    {
        if (Previous.Kind == ERetainedKind::Table) { StaticCastSharedPtr<SCkUiTable>(Previous.Widget)->ReleaseContextMenu(); }
        if (Previous.Kind == ERetainedKind::Tree) { StaticCastSharedPtr<SCkUiTree>(Previous.Widget)->ReleaseContextMenu(); }
        if (Previous.Kind != ERetainedKind::MenuButton) { continue; }
        const FRetainedRecord* Next = _CommittedRetained.Find(Id);
        const TSharedPtr<SCkUiMenuButton> Menu = StaticCastSharedPtr<SCkUiMenuButton>(Previous.Widget);
        if (Next == nullptr || Next->Widget != Previous.Widget) { Menu->Deactivate(); }
        else { Menu->ReleasePopup(); }
    }

    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication& Slate = FSlateApplication::Get();
        for (const FCommitState::FFocusedUser& Focused : FocusedUsers)
        {
            if (Slate.GetUserFocusedWidget(Focused.UserIndex) != Focused.Widget) { continue; }
            if (Focused.RetainedSurvives && Focused.Component.IsValid())
            {
                const TSharedPtr<SWidget> Target = Focused.Component->GetFocusTransferTarget();
                FWidgetPath CurrentPath;
                if (Target.IsValid() && Slate.GeneratePathToWidgetUnchecked(Focused.Widget.ToSharedRef(), CurrentPath))
                {
                    int32 TargetIndex = INDEX_NONE;
                    for (int32 Index = 0; Index + 1 < CurrentPath.Widgets.Num(); ++Index)
                    { if (CurrentPath.Widgets[Index].Widget == Target) { TargetIndex = Index; break; } }
                    bool Transferred = false;
                    for (int32 Index = TargetIndex; Index >= 0; --Index)
                    {
                        const TSharedRef<SWidget> Transfer = CurrentPath.Widgets[Index].Widget;
                        if (!Transfer->SupportsKeyboardFocus()) { continue; }
                        Transferred = true;
                        Slate.SetUserFocus(Focused.UserIndex, Transfer, EFocusCause::SetDirectly);
                        if (Slate.GetUserFocusedWidget(Focused.UserIndex) == Transfer)
                        { Slate.SetUserFocus(Focused.UserIndex, Focused.Widget, EFocusCause::SetDirectly); }
                        break;
                    }
                    if (Transferred) { continue; }
                }
            }
            // A pointer-identical retained leaf keeps Slate's old weak ancestry after the region swap.
            // Refresh it through normal focus callbacks; a callback redirect wins over reacquiring this leaf.
            Slate.ClearUserFocus(Focused.UserIndex, EFocusCause::SetDirectly);
            if (Focused.RetainedSurvives && FSlateApplication::IsInitialized() && !Slate.GetUserFocusedWidget(Focused.UserIndex).IsValid())
            { Slate.SetUserFocus(Focused.UserIndex, Focused.Widget, EFocusCause::SetDirectly); }
        }
    }
    for (const auto& [Id, Previous] : PreviousRetained)
    {
        const FRetainedRecord* Next = _CommittedRetained.Find(Id);
        if (Previous.Kind == ERetainedKind::Tabs && (Next == nullptr || Next->Widget != Previous.Widget))
        { StaticCastSharedPtr<SCkUiTabs>(Previous.Widget)->Deactivate(); }
    }
}

auto FCkUiView::FlattenNested(FStagedDocument& InStaged, TArray<FString>& OutErrors) -> bool
{
    TArray<TSharedPtr<FNestedUpdate>> All;
    TSet<const FCkUiView*> Seen;
    const auto Collect = [&All, &Seen, &OutErrors](const TSharedPtr<FNestedUpdate>& Child, auto&& Self) -> bool
    {
        if (!Child.IsValid() || !Child->View.IsValid() || Seen.Contains(Child->View.Get()))
        { OutErrors.Add(TEXT("Nested children cannot share a view.")); return false; }
        Seen.Add(Child->View.Get()); All.Add(Child);
        for (const auto& Descendant : Child->Staged->Nested) { if (!Self(Descendant, Self)) { return false; } }
        return true;
    };
    for (const auto& Child : InStaged.Nested) { if (!Collect(Child, Collect)) { return false; } }
    InStaged.Nested = MoveTemp(All);
    return true;
}

auto FCkUiView::CollectChildMounts(TSet<const SWidget*>& OutMounts) const -> void
{
    const auto Collect = [&OutMounts](const TSharedPtr<FCkUiView>& View)
    {
        if (!View.IsValid()) { return; }
        for (const auto& [Name, Mount] : View->_RegionMounts) { OutMounts.Add(Mount.Get()); }
        View->CollectChildMounts(OutMounts);
    };
    for (const auto& [Id, Repeat] : _RepeatStates)
    { for (const auto& Item : Repeat->Items) { Collect(Item.View); } }
    for (const auto& [Id, Container] : _CustomSlots)
    { for (const auto& [Name, Slot] : Container->Slots) { Collect(Slot->View); } }
}
auto FCkUiView::MountChildren() -> void
{
    for (const auto& [Id, Repeat] : _RepeatStates) { MountRepeat(*Repeat); }
    for (const auto& [Id, Container] : _CustomSlots)
    {
        for (const auto& [Name, Slot] : Container->Slots)
        {
            if (Slot->Scope.IsValid()) { Slot->Scope->Active = true; }
            Slot->Mount->SetContent(Slot->View.IsValid() ? Slot->View->GetRegion(TEXT("slot")) : SNullWidget::NullWidget);
        }
    }
}

auto FCkUiView::Commit(FStagedDocument&& InStaged, TArray<FString>& OutErrors) -> bool
{
    if (!FlattenNested(InStaged, OutErrors)) { return false; }
    FCommitState State = CaptureCommit(InStaged);
    for (const auto& Child : InStaged.Nested) { Child->State = Child->View->CaptureCommit(*Child->Staged); }
    PublishConfiguration(MoveTemp(InStaged), State);
    for (const auto& Child : InStaged.Nested) { Child->View->PublishConfiguration(MoveTemp(*Child->Staged), Child->State); }
    MountChildren();
    for (const auto& Child : InStaged.Nested) { Child->View->MountChildren(); }
    ReconcileInteractions(State);
    for (const auto& Child : InStaged.Nested) { Child->View->ReconcileInteractions(Child->State); }
    return true;
}
auto FCkUiView::TryReload(const FString& InMarkup, const FString& InStylesheet, const FString& InSource) -> FCkUiLoadResult
{
    if (!IsInGameThread())
    { return FCkUiLoadResult{false, {TEXT("UI document reload must run on the game thread.")}}; }
    return TryReloadBatch({FReloadRequest{AsShared(), InMarkup, InStylesheet, InSource}});
}

auto FCkUiView::TryReloadBatch(const TArray<FReloadRequest>& InRequests) -> FCkUiLoadResult
{
    struct FBatchItem
    {
        TSharedPtr<FCkUiView> View;
        FString Markup;
        FString Stylesheet;
        FString Source;
        FCkUiDocument Document;
        FStagedDocument Staged;
        FCommitState CommitState;
    };

    auto Result = FCkUiLoadResult{};
    if (InRequests.IsEmpty())
    {
        Result.Errors.Add(TEXT("UI reload batch requires at least one participant."));
        return Result;
    }
    if (!IsInGameThread())
    {
        Result.Errors.Add(TEXT("UI document reload must run on the game thread."));
        return Result;
    }

    auto Items = TArray<FBatchItem>{};
    Items.Reserve(InRequests.Num());
    auto SeenViews = TSet<const FCkUiView*>{};
    for (const FReloadRequest& Request : InRequests)
    {
        if (!Request.View.IsValid())
        {
            Result.Errors.Add(TEXT("UI reload batch contains a null participant."));
            return Result;
        }
        if (SeenViews.Contains(Request.View.Get()))
        {
            Result.Errors.Add(TEXT("UI reload batch contains the same view more than once."));
            return Result;
        }
        SeenViews.Add(Request.View.Get());
        Items.Add({Request.View, Request.Markup, Request.Stylesheet, Request.Source});
    }

    const auto Fail = [&Items](FCkUiLoadResult InFailure) -> FCkUiLoadResult
    {
        InFailure.Succeeded = false;
        for (const FBatchItem& Item : Items) { Item.View->_LastResult = InFailure; }
        return InFailure;
    };
    for (const FBatchItem& Item : Items)
    {
        if (Item.View->_IsReloading)
        {
            Result.Errors.Add(TEXT("UI document reload was re-entered."));
            return Fail(MoveTemp(Result));
        }
    }

    // Items retain the owners. Clear staged state before these guards release dispatch eligibility.
    auto ReloadingGuards = TArray<TUniquePtr<TGuardValue<bool>>>{};
    ReloadingGuards.Reserve(Items.Num());
    for (FBatchItem& Item : Items)
    { ReloadingGuards.Add(MakeUnique<TGuardValue<bool>>(Item.View->_IsReloading, true)); }
    ON_SCOPE_EXIT
    {
        for (FBatchItem& Item : Items)
        {
            Item.CommitState = {};
            Item.Staged = {};
            Item.Document = {};
        }
    };

    for (FBatchItem& Item : Items)
    {
        Result = FCkUiDocumentParser::TryParse(Item.Markup, Item.Stylesheet, Item.View->_Tokens,
            Item.Document, Item.Source, Item.View->_CustomRegistry);
        if (!Result.Succeeded) { return Fail(MoveTemp(Result)); }
    }
    for (FBatchItem& Item : Items)
    {
        if (!Item.View->ValidateDocument(Item.Document, Item.Source, Result.Errors))
        {
            Result.Succeeded = false;
            return Fail(MoveTemp(Result));
        }
    }

    auto NativeOwners = TSet<const SWidget*>{};
    const auto CollectNativeRoots = [](const FCkUiView& View, const FCkUiNode& Node, TSet<const SWidget*>& OutRoots, auto&& Self) -> void
    {
        if (Node.Kind == ECkUiNodeKind::Native)
        {
            if (const TSharedPtr<SWidget>* Widget = View._Bindings.Find(Node.Binding); Widget != nullptr && Widget->IsValid())
            { OutRoots.Add(Widget->Get()); }
        }
        for (const FCkUiNode& Child : Node.Children)
        { Self(View, Child, OutRoots, Self); }
    };
    for (const FBatchItem& Item : Items)
    {
        auto ParticipantNativeRoots = TSet<const SWidget*>{};
        for (const auto& [Name, Root] : Item.Document.Regions)
        { CollectNativeRoots(*Item.View, Root, ParticipantNativeRoots, CollectNativeRoots); }
        for (const SWidget* Widget : ParticipantNativeRoots)
        {
            if (NativeOwners.Contains(Widget))
            {
                Result.Errors.Add(ck_ui_surface::Error(Item.Source, TEXT("Reload participants cannot share an authored native widget.")));
                return Fail(MoveTemp(Result));
            }
            NativeOwners.Add(Widget);
        }
    }

    for (FBatchItem& Item : Items)
    {
        if (!Item.View->StageDocument(Item.Document, Item.Staged, Result.Errors))
        {
            for (FString& Error : Result.Errors) { Error = ck_ui_surface::Error(Item.Source, Error); }
            Result.Succeeded = false;
            return Fail(MoveTemp(Result));
        }
    }
    for (FBatchItem& Item : Items)
    { if (!FlattenNested(Item.Staged, Result.Errors)) { return Fail(MoveTemp(Result)); } }
    TSet<const SWidget*> NestedMounts;
    for (const FBatchItem& Item : Items)
    {
        Item.View->CollectChildMounts(NestedMounts);
        for (const auto& Child : Item.Staged.Nested)
        { for (const auto& [Name, Mount] : Child->View->_RegionMounts) { NestedMounts.Add(Mount.Get()); } }
    }
    const auto CollectOwnedWidgets = [&NestedMounts](const TSharedPtr<SWidget>& Root, TSet<const SWidget*>& OutWidgets, auto&& Self) -> void
    {
        if (!Root.IsValid() || Root.Get() == &SNullWidget::NullWidget.Get() || NestedMounts.Contains(Root.Get()) || OutWidgets.Contains(Root.Get())) { return; }
        OutWidgets.Add(Root.Get());
        FChildren* Children = Root->GetChildren();
        if (Children == nullptr) { return; }
        for (int32 Index = 0; Index < Children->Num(); ++Index)
        { Self(Children->GetChildAt(Index), OutWidgets, Self); }
    };
    auto StagedWidgets = TSet<const SWidget*>{};
    for (const FBatchItem& Item : Items)
    {
        auto ParticipantWidgets = TSet<const SWidget*>{};
        for (const auto& [Name, Root] : Item.Staged.Regions)
        { CollectOwnedWidgets(Root, ParticipantWidgets, CollectOwnedWidgets); }
        for (const FRetainedRecord& Record : Item.Staged.Retained)
        { CollectOwnedWidgets(Record.Widget, ParticipantWidgets, CollectOwnedWidgets); CollectOwnedWidgets(Record.Port, ParticipantWidgets, CollectOwnedWidgets); }
        for (const TSharedPtr<SWidget>& Widget : Item.Staged.CustomWidgets)
        { CollectOwnedWidgets(Widget, ParticipantWidgets, CollectOwnedWidgets); }
        for (const TSharedPtr<SWidget>& Widget : Item.Staged.OwnedWidgets)
        { CollectOwnedWidgets(Widget, ParticipantWidgets, CollectOwnedWidgets); }
        for (const FStagedDocument::FScrollContent& Content : Item.Staged.ScrollContents)
        { CollectOwnedWidgets(Content.Scroll, ParticipantWidgets, CollectOwnedWidgets); CollectOwnedWidgets(Content.Child, ParticipantWidgets, CollectOwnedWidgets); }
        for (const SWidget* Widget : ParticipantWidgets)
        {
            if (StagedWidgets.Contains(Widget))
            {
                Result.Errors.Add(ck_ui_surface::Error(Item.Source, TEXT("Reload participants cannot share staged widget ownership.")));
                return Fail(MoveTemp(Result));
            }
            StagedWidgets.Add(Widget);
        }
    }
    for (const FBatchItem& Item : Items)
    {
        for (const auto& Child : Item.Staged.Nested)
        {
            TSet<const SWidget*> ChildWidgets;
            for (const auto& [Name, Mount] : Child->View->_RegionMounts) { ChildWidgets.Add(Mount.Get()); }
            for (const auto& [Name, Root] : Child->Staged->Regions) { CollectOwnedWidgets(Root, ChildWidgets, CollectOwnedWidgets); }
            for (const auto& Record : Child->Staged->Retained)
            { CollectOwnedWidgets(Record.Widget, ChildWidgets, CollectOwnedWidgets); CollectOwnedWidgets(Record.Port, ChildWidgets, CollectOwnedWidgets); }
            for (const auto& Root : Child->Staged->OwnedWidgets) { CollectOwnedWidgets(Root, ChildWidgets, CollectOwnedWidgets); }
            for (const auto& Content : Child->Staged->ScrollContents) { CollectOwnedWidgets(Content.Child, ChildWidgets, CollectOwnedWidgets); }
            for (const auto* Widget : ChildWidgets)
            {
                if (StagedWidgets.Contains(Widget)) { Result.Errors.Add(TEXT("Repeat children cannot share widget ownership.")); return Fail(MoveTemp(Result)); }
                StagedWidgets.Add(Widget);
            }
        }
    }

    for (FBatchItem& Item : Items)
    {
        Item.CommitState = Item.View->CaptureCommit(Item.Staged);
        for (const auto& Child : Item.Staged.Nested) { Child->State = Child->View->CaptureCommit(*Child->Staged); }
    }
    for (FBatchItem& Item : Items) { Item.View->PublishConfiguration(MoveTemp(Item.Staged), Item.CommitState); }
    for (FBatchItem& Item : Items)
    { for (const auto& Child : Item.Staged.Nested) { Child->View->PublishConfiguration(MoveTemp(*Child->Staged), Child->State); } }
    for (FBatchItem& Item : Items)
    {
        Item.View->MountChildren();
        for (const auto& Child : Item.Staged.Nested) { Child->View->MountChildren(); }
    }
    for (FBatchItem& Item : Items)
    {
        Item.View->ReconcileInteractions(Item.CommitState);
        for (const auto& Child : Item.Staged.Nested) { Child->View->ReconcileInteractions(Child->State); }
    }
    Result.Succeeded = true;
    for (const FBatchItem& Item : Items) { Item.View->_LastResult = Result; }
    return Result;
}

auto FCkUiView::SetFiles(const FString& InMarkupPath, const FString& InStylesheetPath) -> void
{
    _MarkupPath = InMarkupPath;
    _StylesheetPath = InStylesheetPath;
    _LastPolledMarkup.Reset();
    _LastPolledStylesheet.Reset();
    _HasPolledContent = false;
}

auto FCkUiView::ReadFiles(FString& OutMarkup, FString& OutStylesheet, FString& OutSource) const -> FCkUiLoadResult
{
    constexpr int64 MaxDocumentBytes = 4 * 1024 * 1024;
    auto Result = FCkUiLoadResult{};
    OutSource = FString::Printf(TEXT("%s + %s"), *_MarkupPath, *_StylesheetPath);
    if (_MarkupPath.IsEmpty() || _StylesheetPath.IsEmpty())
    {
        Result.Errors.Add(TEXT("Both markup and stylesheet file paths must be set before reload."));
        return Result;
    }
    const int64 MarkupSize = IFileManager::Get().FileSize(*_MarkupPath);
    const int64 StylesheetSize = IFileManager::Get().FileSize(*_StylesheetPath);
    const bool MarkupWithinLimit = MarkupSize >= 0 && MarkupSize <= MaxDocumentBytes;
    const bool StylesheetWithinLimit = StylesheetSize >= 0 && StylesheetSize <= MaxDocumentBytes;
    if (!MarkupWithinLimit) { Result.Errors.Add(ck_ui_surface::Error(OutSource, FString::Printf(TEXT("Markup file '%s' is missing or exceeds the 4 MiB limit."), *_MarkupPath))); }
    if (!StylesheetWithinLimit) { Result.Errors.Add(ck_ui_surface::Error(OutSource, FString::Printf(TEXT("Stylesheet file '%s' is missing or exceeds the 4 MiB limit."), *_StylesheetPath))); }
    const bool MarkupRead = MarkupWithinLimit && FFileHelper::LoadFileToString(OutMarkup, *_MarkupPath);
    const bool StylesheetRead = StylesheetWithinLimit && FFileHelper::LoadFileToString(OutStylesheet, *_StylesheetPath);
    if (MarkupWithinLimit && !MarkupRead) { Result.Errors.Add(ck_ui_surface::Error(OutSource, FString::Printf(TEXT("Could not read markup file '%s'."), *_MarkupPath))); }
    if (StylesheetWithinLimit && !StylesheetRead) { Result.Errors.Add(ck_ui_surface::Error(OutSource, FString::Printf(TEXT("Could not read stylesheet file '%s'."), *_StylesheetPath))); }
    Result.Succeeded = Result.Errors.IsEmpty();
    return Result;
}

auto FCkUiView::ReloadFiles(const FString& InMarkupPath, const FString& InStylesheetPath) -> FCkUiLoadResult
{
    SetFiles(InMarkupPath, InStylesheetPath);
    auto Markup = FString{};
    auto Stylesheet = FString{};
    auto Source = FString{};
    FCkUiLoadResult Result = ReadFiles(Markup, Stylesheet, Source);
    if (!Result.Succeeded)
    {
        _LastResult = Result;
        return _LastResult;
    }
    _LastPolledMarkup = Markup;
    _LastPolledStylesheet = Stylesheet;
    _HasPolledContent = true;
    return TryReload(Markup, Stylesheet, Source);
}

auto FCkUiView::PollFiles() -> bool
{
    auto Markup = FString{};
    auto Stylesheet = FString{};
    auto Source = FString{};
    FCkUiLoadResult ReadResult = ReadFiles(Markup, Stylesheet, Source);
    if (!ReadResult.Succeeded)
    {
        const FString FailureMarkup = FString::Printf(TEXT("<unreadable:%s>"), *_MarkupPath);
        const FString FailureStylesheet = FString::Printf(TEXT("<unreadable:%s>"), *_StylesheetPath);
        if (_HasPolledContent && _LastPolledMarkup == FailureMarkup && _LastPolledStylesheet == FailureStylesheet) { return false; }
        _LastPolledMarkup = FailureMarkup;
        _LastPolledStylesheet = FailureStylesheet;
        _HasPolledContent = true;
        _LastResult = MoveTemp(ReadResult);
        return true;
    }
    if (_HasPolledContent && _LastPolledMarkup == Markup && _LastPolledStylesheet == Stylesheet) { return false; }
    _LastPolledMarkup = Markup;
    _LastPolledStylesheet = Stylesheet;
    _HasPolledContent = true;
    TryReload(Markup, Stylesheet, Source);
    return true;
}
