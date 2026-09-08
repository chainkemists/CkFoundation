#include "CkSlateLayout/CkUiMenuSession.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/SNullWidget.h"

namespace ck_ui_menu_session
{
    auto FindEntry(const FCkUiMenuSession::FEntry& InEntry, const TArray<int32>& InPath, const int32 InDepth)
        -> const FCkUiMenuSession::FEntry*
    {
        if (!InPath.IsValidIndex(InDepth)) { return nullptr; }
        const int32 Index = InPath[InDepth];
        if (!InEntry.Children.IsValidIndex(Index)) { return nullptr; }
        const FCkUiMenuSession::FEntry& Child = InEntry.Children[Index];
        return InDepth + 1 == InPath.Num() ? &Child : FindEntry(Child, InPath, InDepth + 1);
    }

    auto FindEntry(const TArray<FCkUiMenuSession::FEntry>& InEntries, const TArray<int32>& InPath)
        -> const FCkUiMenuSession::FEntry*
    {
        if (!InPath.IsValidIndex(0) || !InEntries.IsValidIndex(InPath[0])) { return nullptr; }
        const FCkUiMenuSession::FEntry& Entry = InEntries[InPath[0]];
        return InPath.Num() == 1 ? &Entry : FindEntry(Entry, InPath, 1);
    }

    auto IsPathAvailable(const TArray<FCkUiMenuSession::FEntry>& InEntries, const TArray<int32>& InPath, const bool InRequireEnabled)
        -> bool
    {
        if (FindEntry(InEntries, InPath) == nullptr) { return false; }

        auto Prefix = TArray<int32>{};
        Prefix.Reserve(InPath.Num());
        for (const int32 Index : InPath)
        {
            Prefix.Add(Index);
            const FCkUiMenuSession::FEntry* Ancestor = FindEntry(InEntries, Prefix);
            if (Ancestor == nullptr || !Ancestor->Visible.Get(true)) { return false; }
            if (InRequireEnabled && !Ancestor->Enabled.Get(true)) { return false; }
        }
        return true;
    }

    auto HasVisibleEntry(const TArray<FCkUiMenuSession::FEntry>& InEntries) -> bool
    {
        for (const FCkUiMenuSession::FEntry& Entry : InEntries)
        {
            if (!Entry.Visible.Get(true)) { continue; }
            if (!Entry.Separator) { return true; }
            if (HasVisibleEntry(Entry.Children)) { return true; }
        }
        return false;
    }


}

struct FCkUiMenuSession::FSnapshot final
{
    uint64 Revision = 0;
    TArray<FEntry> Entries;
    TAttribute<bool> Enabled;
    TAttribute<bool> CanDispatchEvents;
};

void FCkUiMenuSession::Configure(TArray<FEntry> InEntries, TAttribute<bool> InEnabled, TAttribute<bool> InCanDispatchEvents)
{
    const TSharedRef<FSnapshot> Next = MakeShared<FSnapshot>();
    Next->Revision = ++_Revision;
    Next->Entries = MoveTemp(InEntries);
    Next->Enabled = MoveTemp(InEnabled);
    Next->CanDispatchEvents = MoveTemp(InCanDispatchEvents);
    _Snapshot = Next;
}

auto FCkUiMenuSession::BuildMenu() -> TSharedRef<SWidget>
{
    const TSharedPtr<const FSnapshot> Snapshot = _Snapshot;
    if (!_Active || !Snapshot.IsValid() || !IsSnapshotCurrent(Snapshot.ToSharedRef()))
    {
        return SNullWidget::NullWidget;
    }

    FMenuBuilder Builder{true, nullptr};
    BuildEntries(Builder, Snapshot.ToSharedRef(), {});
    const TSharedRef<SWidget> Content = Builder.MakeWidget();
    RegisterOwnedRoot(Content);
    return Content;
}

auto FCkUiMenuSession::IsEnabled() const -> bool
{
    return _Active && _Snapshot.IsValid() && _Snapshot->Enabled.Get(true) && _Snapshot->CanDispatchEvents.Get(false);
}

auto FCkUiMenuSession::HasVisibleEntries() const -> bool
{
    return _Snapshot.IsValid() && IsSnapshotCurrent(_Snapshot.ToSharedRef()) && _Snapshot->Enabled.Get(true)
        && ck_ui_menu_session::HasVisibleEntry(_Snapshot->Entries);
}

void FCkUiMenuSession::ClearOwnedFocus()
{
    TArray<TWeakPtr<SWidget>> OwnedRoots = MoveTemp(_OwnedMenuRoots);
    if (!FSlateApplication::IsInitialized()) { return; }

    FSlateApplication& Slate = FSlateApplication::Get();
    Slate.ForEachUser([&OwnedRoots, &Slate](FSlateUser& User)
    {
        for (const TWeakPtr<SWidget>& WeakRoot : OwnedRoots)
        {
            const TSharedPtr<SWidget> Root = WeakRoot.Pin();
            if (Root.IsValid() && User.IsWidgetInFocusPath(Root))
            {
                Slate.ClearUserFocus(User.GetUserIndex(), EFocusCause::SetDirectly);
                break;
            }
        }
    }, true);
}

void FCkUiMenuSession::Deactivate()
{
    if (!_Active) { return; }
    _Active = false;
    ++_Revision;
    ClearOwnedFocus();
}

void FCkUiMenuSession::BuildEntries(FMenuBuilder& InBuilder, const TSharedRef<const FSnapshot>& InSnapshot, const TArray<int32>& InParentPath)
{
    const TArray<FEntry>* Entries = &InSnapshot->Entries;
    if (!InParentPath.IsEmpty())
    {
        const FEntry* Parent = ck_ui_menu_session::FindEntry(InSnapshot->Entries, InParentPath);
        if (Parent == nullptr) { return; }
        Entries = &Parent->Children;
    }

    const TWeakPtr<FCkUiMenuSession> WeakSession = AsShared();
    for (int32 Index = 0; Index < Entries->Num(); ++Index)
    {
        const FEntry& Entry = (*Entries)[Index];
        auto Path = InParentPath;
        Path.Add(Index);

        const TAttribute<FText> Label = TAttribute<FText>::CreateLambda([WeakSession, InSnapshot, Path]()
        {
            const TSharedPtr<FCkUiMenuSession> Session = WeakSession.Pin();
            const FEntry* Current = Session.IsValid() && Session->IsSnapshotCurrent(InSnapshot)
                ? ck_ui_menu_session::FindEntry(InSnapshot->Entries, Path)
                : nullptr;
            return Current != nullptr ? Current->Label.Get(FText::GetEmpty()) : FText::GetEmpty();
        });
        const TAttribute<FText> Tooltip = TAttribute<FText>::CreateLambda([WeakSession, InSnapshot, Path]()
        {
            const TSharedPtr<FCkUiMenuSession> Session = WeakSession.Pin();
            const FEntry* Current = Session.IsValid() && Session->IsSnapshotCurrent(InSnapshot)
                ? ck_ui_menu_session::FindEntry(InSnapshot->Entries, Path)
                : nullptr;
            return Current != nullptr ? Current->Tooltip.Get(FText::GetEmpty()) : FText::GetEmpty();
        });
        const TAttribute<EVisibility> Visibility = TAttribute<EVisibility>::CreateLambda([WeakSession, InSnapshot, Path]()
        {
            const TSharedPtr<FCkUiMenuSession> Session = WeakSession.Pin();
            return Session.IsValid() && Session->IsEntryVisible(InSnapshot, Path) ? EVisibility::Visible : EVisibility::Collapsed;
        });
        if (Entry.Separator)
        {
            InBuilder.AddSeparator(NAME_None, Visibility);
            continue;
        }

        const FUIAction UiAction{
            FExecuteAction::CreateLambda([WeakSession, InSnapshot, Path]()
            {
                if (const TSharedPtr<FCkUiMenuSession> Session = WeakSession.Pin()) { Session->Dispatch(InSnapshot, Path); }
            }),
            FCanExecuteAction::CreateLambda([WeakSession, InSnapshot, Path]()
            {
                const TSharedPtr<FCkUiMenuSession> Session = WeakSession.Pin();
                return Session.IsValid() && Session->CanDispatch(InSnapshot, Path);
            })};

        if (Entry.Children.IsEmpty())
        {
            InBuilder.AddMenuEntry(Label, Tooltip, FSlateIcon{}, UiAction, NAME_None,
                EUserInterfaceActionType::Button, NAME_None, TAttribute<FText>{}, Visibility);
            continue;
        }

        FMenuEntryParams Submenu;
        Submenu.LabelOverride = Label;
        Submenu.ToolTipOverride = Tooltip;
        Submenu.Visibility = Visibility;
        Submenu.DirectActions = UiAction;
        Submenu.UserInterfaceActionType = EUserInterfaceActionType::Button;
        Submenu.bIsSubMenu = true;
        Submenu.MenuBuilder = FOnGetContent::CreateLambda([WeakSession, InSnapshot, Path]() -> TSharedRef<SWidget>
        {
            const TSharedPtr<FCkUiMenuSession> Session = WeakSession.Pin();
            if (!Session.IsValid() || !Session->CanDispatch(InSnapshot, Path))
            {
                return SNullWidget::NullWidget;
            }
            FMenuBuilder Builder{true, nullptr};
            Session->BuildEntries(Builder, InSnapshot, Path);
            const TSharedRef<SWidget> Content = Builder.MakeWidget();
            Session->RegisterOwnedRoot(Content);
            return Content;
        });
        InBuilder.AddMenuEntry(Submenu);
    }
}

auto FCkUiMenuSession::IsSnapshotCurrent(const TSharedRef<const FSnapshot>& InSnapshot) const -> bool
{
    return _Active && _Snapshot.Get() == &InSnapshot.Get() && _Revision == InSnapshot->Revision;
}

auto FCkUiMenuSession::IsEntryVisible(const TSharedRef<const FSnapshot>& InSnapshot, const TArray<int32>& InPath) const -> bool
{
    return IsSnapshotCurrent(InSnapshot)
        && InSnapshot->Enabled.Get(true)
        && ck_ui_menu_session::IsPathAvailable(InSnapshot->Entries, InPath, false);
}

auto FCkUiMenuSession::CanDispatch(const TSharedRef<const FSnapshot>& InSnapshot, const TArray<int32>& InPath) const -> bool
{
    return IsEntryVisible(InSnapshot, InPath)
        && InSnapshot->CanDispatchEvents.Get(false)
        && ck_ui_menu_session::IsPathAvailable(InSnapshot->Entries, InPath, true);
}

void FCkUiMenuSession::Dispatch(const TSharedRef<const FSnapshot>& InSnapshot, const TArray<int32>& InPath)
{
    if (!CanDispatch(InSnapshot, InPath)) { return; }
    const FEntry* Entry = ck_ui_menu_session::FindEntry(InSnapshot->Entries, InPath);
    if (Entry == nullptr) { return; }

    const FSimpleDelegate Action = Entry->Action;
    if (!CanDispatch(InSnapshot, InPath)) { return; }
    Action.ExecuteIfBound();
}

void FCkUiMenuSession::RegisterOwnedRoot(const TSharedRef<SWidget>& InRoot)
{
    _OwnedMenuRoots.RemoveAll([](const TWeakPtr<SWidget>& Root) { return !Root.IsValid(); });
    _OwnedMenuRoots.Add(InRoot);
}
