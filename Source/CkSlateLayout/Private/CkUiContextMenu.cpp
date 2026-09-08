#include "CkSlateLayout/CkUiContextMenu.h"

#include "CkSlateLayout/CkUiMenuSession.h"
#include "Framework/Application/IMenu.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/MenuStack.h"

FCkUiContextMenuHost::~FCkUiContextMenuHost()
{
    Release();
}

auto FCkUiContextMenuHost::Open(const TSharedRef<SWidget>& InParent, const FWidgetPath& InOwnerPath,
    const FVector2f& InScreenPosition, TSharedPtr<FCkUiMenuSession> InSession, const int32 InFocusUserIndex) -> bool
{
    if (_Opening) { return false; }
    TGuardValue<bool> OpeningGuard(_Opening, true);
    Release();
    const uint64 OpeningGeneration = _Generation;
    if (!FSlateApplication::IsInitialized() || !InSession.IsValid() || !InSession->IsEnabled() || !InSession->HasVisibleEntries()) { return false; }

    const TSharedRef<SWidget> Content = InSession->BuildMenu();
    const TSharedPtr<IMenu> Menu = FSlateApplication::Get().PushMenu(InParent, InOwnerPath, Content, InScreenPosition,
        FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu), true, FVector2f::ZeroVector, TOptional<EPopupMethod>{}, true, InFocusUserIndex);
    if (!Menu.IsValid())
    {
        InSession->Deactivate();
        return false;
    }

    if (_Generation != OpeningGeneration || !InSession->IsEnabled())
    {
        InSession->Deactivate();
        FSlateApplication::Get().DismissMenu(Menu);
        return false;
    }
    _Session = MoveTemp(InSession);
    _Menu = Menu;
    const TWeakPtr<FCkUiContextMenuHost> WeakHost = AsShared();
    Menu->GetOnMenuDismissed().AddLambda([WeakHost](const TSharedRef<IMenu>& InDismissedMenu)
    {
        if (const TSharedPtr<FCkUiContextMenuHost> Host = WeakHost.Pin()) { Host->OnMenuDismissed(InDismissedMenu); }
    });
    return true;
}

void FCkUiContextMenuHost::Release()
{
    ++_Generation;
    const TSharedPtr<FCkUiMenuSession> Session = MoveTemp(_Session);
    const TSharedPtr<IMenu> Menu = MoveTemp(_Menu);
    if (Session.IsValid()) { Session->Deactivate(); }
    if (Menu.IsValid() && FSlateApplication::IsInitialized()) { FSlateApplication::Get().DismissMenu(Menu); }
}

void FCkUiContextMenuHost::Tick()
{
    if (!_Menu.IsValid() || (_Session.IsValid() && (!_Session->IsEnabled() || !_Session->HasVisibleEntries()))) { Release(); }
}

auto FCkUiContextMenuHost::IsOpen() const -> bool
{
    return _Menu.IsValid();
}

void FCkUiContextMenuHost::OnMenuDismissed(const TSharedRef<IMenu>& InDismissedMenu)
{
    if (_Menu != InDismissedMenu) { return; }
    _Menu.Reset();
}
