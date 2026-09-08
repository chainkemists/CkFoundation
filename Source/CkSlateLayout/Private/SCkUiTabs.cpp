#include "CkSlateLayout/SCkUiTabs.h"
#include "CkSlateLayout/CkFlexLayoutTypes.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Input/Events.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

struct SCkUiTabs::FEntry
{
    FString Key;
    TAttribute<FText> Label;
    TAttribute<bool> Enabled;
    TSharedPtr<SWidget> Content;
    TFunction<void()> OnDeactivate;
    TSharedPtr<SButton> Header;
    TSharedPtr<STextBlock> HeaderLabel;
    TSharedPtr<SBox> ContentHost;
};

SCkUiTabs::SCkUiTabs() = default;
SCkUiTabs::~SCkUiTabs() = default;

void SCkUiTabs::Construct(const FArguments& InArgs)
{
    SAssignNew(_Headers, SWrapBox).UseAllottedSize(true);
    SAssignNew(_Panels, SVerticalBox);
    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()[_Headers.ToSharedRef()]
        + SVerticalBox::Slot().FillHeight(1.0f)[_Panels.ToSharedRef()]
    ];
    AddMetadata(MakeShared<FCkFlexMeasureMetaData>(
        [WeakTabs = TWeakPtr<SCkUiTabs>(SharedThis(this))](const FCkFlexMeasureArgs& Args) -> FVector2D
        {
            const TSharedPtr<SCkUiTabs> Tabs = WeakTabs.Pin();
            if (!Tabs.IsValid() || !IsValid_CkFlexMeasureArgs(Args)) { return FVector2D::ZeroVector; }
            const bool HasWidth = Args.WidthMode != YGMeasureModeUndefined && FMath::IsFinite(Args.AvailableWidth) && Args.AvailableWidth >= 0.0f;
            float HeaderWidth = 0.0f;
            float HeaderHeight = 0.0f;
            float LineWidth = 0.0f;
            float LineHeight = 0.0f;
            for (const FEntry& Entry : Tabs->_Entries)
            {
                if (!Entry.Header.IsValid()) { continue; }
                const FVector2D Size = Entry.Header->GetDesiredSize();
                if (HasWidth && LineWidth > 0.0f && LineWidth + Size.X > Args.AvailableWidth)
                { HeaderWidth = FMath::Max(HeaderWidth, LineWidth); HeaderHeight += LineHeight; LineWidth = 0.0f; LineHeight = 0.0f; }
                LineWidth += Size.X;
                LineHeight = FMath::Max<double>(LineHeight, Size.Y);
            }
            HeaderWidth = FMath::Max(HeaderWidth, LineWidth);
            HeaderHeight += LineHeight;
            FVector2D Body = FVector2D::ZeroVector;
            const FEntry* Selected = Tabs->_Entries.FindByPredicate([Tabs](const FEntry& Entry) { return Tabs->IsKeySelected(Entry.Key); });
            if (Selected != nullptr && Selected->Content.IsValid())
            {
                const TSharedPtr<FCkFlexMeasureMetaData> Measure = Selected->Content->GetMetaData<FCkFlexMeasureMetaData>();
                auto BodyArgs = Args;
                BodyArgs.AvailableHeight = YGUndefined;
                BodyArgs.HeightMode = YGMeasureModeUndefined;
                Body = Measure.IsValid() ? Measure->Measure(BodyArgs) : Selected->Content->GetDesiredSize();
            }
            FVector2D Result(FMath::Max<double>(HeaderWidth, Body.X), HeaderHeight + Body.Y);
            if (HasWidth) { Result.X = Args.WidthMode == YGMeasureModeExactly ? Args.AvailableWidth : FMath::Min<double>(Result.X, Args.AvailableWidth); }
            if (Args.HeightMode == YGMeasureModeExactly) { Result.Y = Args.AvailableHeight; }
            else if (Args.HeightMode == YGMeasureModeAtMost) { Result.Y = FMath::Min<double>(Result.Y, Args.AvailableHeight); }
            return Result;
        },
        [WeakTabs = TWeakPtr<SCkUiTabs>(SharedThis(this))](const float Width, const float Height)
        {
            const TSharedPtr<SCkUiTabs> Tabs = WeakTabs.Pin();
            if (!Tabs.IsValid()) { return; }
            float LineWidth = 0.0f;
            float LineHeight = 0.0f;
            float HeaderHeight = 0.0f;
            for (const FEntry& Entry : Tabs->_Entries)
            {
                if (!Entry.Header.IsValid()) { continue; }
                const FVector2D Size = Entry.Header->GetDesiredSize();
                if (LineWidth > 0.0f && LineWidth + Size.X > Width)
                { HeaderHeight += LineHeight; LineWidth = 0.0f; LineHeight = 0.0f; }
                LineWidth += Size.X;
                LineHeight = FMath::Max<double>(LineHeight, Size.Y);
            }
            HeaderHeight += LineHeight;
            const FEntry* Selected = Tabs->_Entries.FindByPredicate([Tabs](const FEntry& Entry) { return Tabs->IsKeySelected(Entry.Key); });
            if (Selected != nullptr && Selected->Content.IsValid())
            {
                if (const TSharedPtr<FCkFlexMeasureMetaData> Measure = Selected->Content->GetMetaData<FCkFlexMeasureMetaData>(); Measure.IsValid())
                { Measure->NotifyArranged(Width, FMath::Max(0.0f, Height - HeaderHeight)); }
            }
        }));
}

void SCkUiTabs::SetConfiguration(TArray<FPanel> InPanels, TAttribute<FString> InValue,
    FCkUiOnStringChanged InChanged, TAttribute<bool> InCanDispatchEvents, FSlateFontInfo InFont)
{
    TSet<FString> Keys;
    for (const FPanel& Panel : InPanels)
    {
        const bool bDuplicate = Keys.Contains(Panel.Key);
        const FEntry* Existing = _Entries.FindByPredicate([&Panel](const FEntry& Entry)
        { return Entry.Key.Equals(Panel.Key, ESearchCase::CaseSensitive); });
        const bool bReusesMountedContent = Existing != nullptr && Existing->Content == Panel.Content;
        if (Panel.Key.IsEmpty() || bDuplicate || !Panel.Content.IsValid()
            || (Panel.Content->GetParentWidget().IsValid() && !bReusesMountedContent))
        {
            // The staged view validates this contract before publication.  The
            // public native boundary still rejects the whole malformed update,
            // preserving the currently mounted retained configuration.
            return;
        }
        Keys.Add(Panel.Key);
    }
    // All input is copied.  In particular, callers may release their staged
    // arrays immediately after publication.
    ++_ConfigurationRevision;
    _PendingPanels = MoveTemp(InPanels);
    _Value = MoveTemp(InValue);
    _Changed = MoveTemp(InChanged);
    _CanDispatchEvents = MoveTemp(InCanDispatchEvents);
    _Font = MoveTemp(InFont);
    _ReconcilePending = true;
    // View publication may repair a focused retained child immediately after
    // this call.  Mount staged panel ancestry now, while selection/focus stays
    // on the last Tick-applied key.
    Reconcile();
}

void SCkUiTabs::Deactivate()
{
    _Active = false;
    _Changed.Unbind();
    _CanDispatchEvents = TAttribute<bool>(false);
    for (FEntry& Entry : _Entries)
    {
        if (Entry.ContentHost.IsValid()) { Entry.ContentHost->SetVisibility(EVisibility::Collapsed); }
    }
}

void SCkUiTabs::Tick(const FGeometry& InAllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    SCompoundWidget::Tick(InAllottedGeometry, InCurrentTime, InDeltaTime);
    if (!_Active) { return; }
    if (_ReconcilePending) { Reconcile(); }
    ReconcileSelectionAndOwnedFocus();
    ReconcileDisabledHeaderFocus();
}

FReply SCkUiTabs::OnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InEvent)
{
    const FKey Key = InEvent.GetKey();
    if (Key != EKeys::Left && Key != EKeys::Right && Key != EKeys::Up && Key != EKeys::Down && Key != EKeys::Home && Key != EKeys::End)
    {
        return SCompoundWidget::OnKeyDown(InGeometry, InEvent);
    }
    const TSharedPtr<SWidget> FocusedWidget = FSlateApplication::Get().GetUserFocusedWidget(InEvent.GetUserIndex());
    const FEntry* Focused = _Entries.FindByPredicate([&FocusedWidget](const FEntry& Entry) { return Entry.Header.IsValid() && Entry.Header == FocusedWidget; });
    if (Focused == nullptr) { return SCompoundWidget::OnKeyDown(InGeometry, InEvent); }
    if (!_Active || !_CanDispatchEvents.Get(false) || !Focused->Enabled.Get(true)) { return FReply::Unhandled(); }
    if (Key == EKeys::Home) { MoveHeaderFocus(Focused->Key, -1, true, InEvent.GetUserIndex()); }
    else if (Key == EKeys::End) { MoveHeaderFocus(Focused->Key, 1, true, InEvent.GetUserIndex()); }
    else { MoveHeaderFocus(Focused->Key, Key == EKeys::Left || Key == EKeys::Up ? -1 : 1, false, InEvent.GetUserIndex()); }
    return FReply::Handled();
}

bool SCkUiTabs::IsKeyEnabled(const FString& InKey) const
{
    const FEntry* Entry = _Entries.FindByPredicate([&InKey](const FEntry& Candidate) { return Candidate.Key.Equals(InKey, ESearchCase::CaseSensitive); });
    return Entry != nullptr && Entry->Enabled.Get(true);
}

bool SCkUiTabs::IsKeySelected(const FString& InKey) const
{
    return _Active && _AppliedSelectedKey.Equals(InKey, ESearchCase::CaseSensitive) && IsKeyEnabled(InKey);
}

bool SCkUiTabs::IsRetainedHeader(const TSharedPtr<SWidget>& InWidget) const
{
    if (!InWidget.IsValid()) { return false; }
    return _Entries.ContainsByPredicate([&InWidget](const FEntry& Entry) { return Entry.Header == InWidget; });
}

void SCkUiTabs::ActivateKey(const FString& InKey)
{
    if (!_Active || _Dispatching || !_CanDispatchEvents.Get(false) || !IsKeyEnabled(InKey) || !_Changed.IsBound()) { return; }
    const TSharedRef<SCkUiTabs> KeepAlive = SharedThis(this);
    const FCkUiOnStringChanged Callback = _Changed;
    TGuardValue<bool> Guard(_Dispatching, true);
    Callback.Execute(InKey);
    // A callback may have reloaded or released the entire view.  Tick reads the
    // authoritative binding later; this path never forces local selection.
}

void SCkUiTabs::MoveHeaderFocus(const FString& InKey, const int32 InDirection, const bool bToBoundary, const int32 InUserIndex)
{
    if (!_Active || !_CanDispatchEvents.Get(false) || !FSlateApplication::IsInitialized()) { return; }
    TArray<const FEntry*> Enabled;
    for (const FEntry& Entry : _Entries)
    {
        if (Entry.Enabled.Get(true) && Entry.Header.IsValid()) { Enabled.Add(&Entry); }
    }
    if (Enabled.IsEmpty()) { return; }
    int32 Current = Enabled.IndexOfByPredicate([&InKey](const FEntry* Entry) { return Entry->Key.Equals(InKey, ESearchCase::CaseSensitive); });
    int32 Target = bToBoundary ? (InDirection < 0 ? 0 : Enabled.Num() - 1) : (Current == INDEX_NONE ? 0 : (Current + InDirection + Enabled.Num()) % Enabled.Num());
    FSlateApplication::Get().SetUserFocus(InUserIndex, Enabled[Target]->Header.ToSharedRef(), EFocusCause::Navigation);
}

void SCkUiTabs::Reconcile()
{
    _ReconcilePending = false;
    if (!_Headers.IsValid() || !_Panels.IsValid()) { return; }

    TArray<FEntry> Next;
    Next.Reserve(_PendingPanels.Num());
    TSet<FString> Seen;
    for (const FPanel& Panel : _PendingPanels)
    {
        if (Panel.Key.IsEmpty() || Seen.Contains(Panel.Key) || !Panel.Content.IsValid())
        {
            // SetConfiguration prevalidates this boundary.  Never turn an
            // unexpected violation into a partially published tab strip.
            return;
        }
        Seen.Add(Panel.Key);
        const int32 ExistingIndex = _Entries.IndexOfByPredicate([&Panel](const FEntry& Entry) { return Entry.Key.Equals(Panel.Key, ESearchCase::CaseSensitive); });
        FEntry Entry;
        if (ExistingIndex != INDEX_NONE) { Entry = MoveTemp(_Entries[ExistingIndex]); }
        Entry.Key = Panel.Key;
        Entry.Label = Panel.Label;
        Entry.Enabled = Panel.Enabled;
        Entry.OnDeactivate = Panel.OnDeactivate;
        if (!Entry.Header.IsValid())
        {
            SAssignNew(Entry.HeaderLabel, STextBlock)
                .Text_Lambda([WeakTabs = TWeakPtr<SCkUiTabs>(SharedThis(this)), Key = Entry.Key]()
                { const TSharedPtr<SCkUiTabs> Owner = WeakTabs.Pin(); const FEntry* Found = Owner.IsValid() ? Owner->_Entries.FindByPredicate([&Key](const FEntry& Item) { return Item.Key.Equals(Key, ESearchCase::CaseSensitive); }) : nullptr; return Found != nullptr ? Found->Label.Get(FText::GetEmpty()) : FText::GetEmpty(); })
                .Font(_Font);
            SAssignNew(Entry.Header, SButton)
                .ButtonStyle(&FCoreStyle::Get().GetWidgetStyle<FButtonStyle>(TEXT("Button")))
                .IsEnabled_Lambda([WeakTabs = TWeakPtr<SCkUiTabs>(SharedThis(this)), Key = Entry.Key]()
                { const TSharedPtr<SCkUiTabs> Owner = WeakTabs.Pin(); return Owner.IsValid() && Owner->_Active && Owner->_CanDispatchEvents.Get(false) && Owner->IsKeyEnabled(Key); })
                .OnClicked_Lambda([WeakTabs = TWeakPtr<SCkUiTabs>(SharedThis(this)), Key = Entry.Key]()
                { if (const TSharedPtr<SCkUiTabs> Owner = WeakTabs.Pin()) { Owner->ActivateKey(Key); } return FReply::Handled(); })
                .ButtonColorAndOpacity_Lambda([WeakTabs = TWeakPtr<SCkUiTabs>(SharedThis(this)), Key = Entry.Key]()
                { const TSharedPtr<SCkUiTabs> Owner = WeakTabs.Pin(); return Owner.IsValid() && Owner->IsKeySelected(Key) ? FLinearColor(0.18f, 0.42f, 0.90f, 1.0f) : FLinearColor::White; })
                [Entry.HeaderLabel.ToSharedRef()];
        }
        else if (Entry.HeaderLabel.IsValid()) { Entry.HeaderLabel->SetFont(_Font); }
        if (!Entry.ContentHost.IsValid())
        {
            SAssignNew(Entry.ContentHost, SBox)
                .Visibility_Lambda([WeakTabs = TWeakPtr<SCkUiTabs>(SharedThis(this)), Key = Entry.Key]()
                { const TSharedPtr<SCkUiTabs> Owner = WeakTabs.Pin(); return Owner.IsValid() && Owner->IsKeySelected(Key) ? EVisibility::Visible : EVisibility::Collapsed; });
        }
        if (Entry.Content != Panel.Content)
        {
            // A compatible keyed reload retains the host; staged content is the
            // only object allowed to replace its child.
            if (!Panel.Content->GetParentWidget().IsValid() || Entry.Content == Panel.Content)
            {
                Entry.ContentHost->SetContent(Panel.Content.ToSharedRef());
                Entry.Content = Panel.Content;
            }
        }
        Next.Add(MoveTemp(Entry));
    }
    _Entries = MoveTemp(Next);
    _Headers->ClearChildren();
    _Panels->ClearChildren();
    for (FEntry& Entry : _Entries)
    {
        _Headers->AddSlot()[Entry.Header.ToSharedRef()];
        _Panels->AddSlot().FillHeight(1.0f)[Entry.ContentHost.ToSharedRef()];
    }
    _PendingPanels.Reset();
}

void SCkUiTabs::ReconcileSelectionAndOwnedFocus()
{
    const FString RequestedKey = _Value.Get(FString{});
    const FString Selected = IsKeyEnabled(RequestedKey) ? RequestedKey : FString{};
    if (_AppliedSelectedKey.Equals(Selected, ESearchCase::CaseSensitive)) { return; }
    if (_ChangingPresentation) { return; }
    const TSharedRef<SCkUiTabs> KeepAlive = SharedThis(this);
    TGuardValue<bool> PresentationGuard(_ChangingPresentation, true);
    const uint64 Revision = _ConfigurationRevision;
    const FEntry* OldEntry = _Entries.FindByPredicate([this](const FEntry& Entry)
    { return Entry.Key.Equals(_AppliedSelectedKey, ESearchCase::CaseSensitive); });
    const TFunction<void()> OnDeactivate = OldEntry != nullptr ? OldEntry->OnDeactivate : TFunction<void()>{};
    if (OnDeactivate) { OnDeactivate(); }
    // Cleanup may invoke native cancellation/focus callbacks that replace this configuration.
    if (!_Active || Revision != _ConfigurationRevision || !_Value.Get(FString{}).Equals(RequestedKey, ESearchCase::CaseSensitive)) { return; }
    const FString Previous = MoveTemp(_AppliedSelectedKey);
    _AppliedSelectedKey = Selected;
    for (const FEntry& Entry : _Entries)
    {
        if (Entry.ContentHost.IsValid()) { Entry.ContentHost->Invalidate(EInvalidateWidgetReason::Visibility); }
        if (Entry.Header.IsValid()) { Entry.Header->Invalidate(EInvalidateWidgetReason::Paint); }
    }
    if (Previous.IsEmpty() || !FSlateApplication::IsInitialized()) { return; }
    const FEntry* PreviousEntry = _Entries.FindByPredicate([&Previous](const FEntry& Entry) { return Entry.Key.Equals(Previous, ESearchCase::CaseSensitive); });
    if (PreviousEntry == nullptr || !PreviousEntry->ContentHost.IsValid()) { return; }
    const TSharedPtr<SWidget> PreviousHost = PreviousEntry->ContentHost;
    const FEntry* TargetEntry = _Entries.FindByPredicate([this](const FEntry& Entry) { return IsKeySelected(Entry.Key); });
    const TSharedPtr<SWidget> TargetHeader = TargetEntry != nullptr ? TargetEntry->Header : nullptr;
    FSlateApplication& Slate = FSlateApplication::Get();
    Slate.ForEachUser([&Slate, PreviousHost, TargetHeader](FSlateUser& User)
    {
        if (!User.IsWidgetInFocusPath(PreviousHost)) { return; }
        if (TargetHeader.IsValid()) { Slate.SetUserFocus(User.GetUserIndex(), TargetHeader, EFocusCause::SetDirectly); }
        else { Slate.ClearUserFocus(User.GetUserIndex(), EFocusCause::SetDirectly); }
    }, true);
}

void SCkUiTabs::ReconcileDisabledHeaderFocus()
{
    if (!_Active || !FSlateApplication::IsInitialized()) { return; }
    const TSharedRef<SCkUiTabs> KeepAlive = SharedThis(this);
    const uint64 Revision = _ConfigurationRevision;
    TSharedPtr<SWidget> Target;
    for (const FEntry& Entry : _Entries)
    {
        if (!Entry.Enabled.Get(true)) { continue; }
        if (!Target.IsValid() || Entry.Key.Equals(_AppliedSelectedKey, ESearchCase::CaseSensitive)) { Target = Entry.Header; }
        if (Entry.Key.Equals(_AppliedSelectedKey, ESearchCase::CaseSensitive)) { break; }
    }
    struct FFocusedHeader { int32 UserIndex; TSharedPtr<SWidget> Widget; };
    TArray<FFocusedHeader> DisabledHeaders;
    FSlateApplication& Slate = FSlateApplication::Get();
    Slate.ForEachUser([this, &DisabledHeaders](FSlateUser& User)
    {
        const TSharedPtr<SWidget> Focused = User.GetFocusedWidget();
        for (const FEntry& Entry : _Entries)
        {
            if (Entry.Header == Focused && !Entry.Enabled.Get(true))
            { DisabledHeaders.Add({User.GetUserIndex(), Focused}); break; }
        }
    }, true);
    for (const FFocusedHeader& Focused : DisabledHeaders)
    {
        if (!_Active || Revision != _ConfigurationRevision) { return; }
        if (Slate.GetUserFocusedWidget(Focused.UserIndex) != Focused.Widget) { continue; }
        if (Target.IsValid()) { Slate.SetUserFocus(Focused.UserIndex, Target, EFocusCause::SetDirectly); }
        else { Slate.ClearUserFocus(Focused.UserIndex, EFocusCause::SetDirectly); }
    }
}
