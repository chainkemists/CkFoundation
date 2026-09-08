#pragma once

#include "CoreMinimal.h"

class FCkUiMenuSession;
class IMenu;
class SWidget;
class FWidgetPath;

DECLARE_DELEGATE_RetVal_OneParam(TSharedPtr<FCkUiMenuSession>, FOnCkUiContextMenuOpening, const FString&);
DECLARE_DELEGATE_OneParam(FOnCkUiContextAction, const FString&);

/** Owns one native Slate popup and the menu session that supplied its content. */
class CKSLATELAYOUT_API FCkUiContextMenuHost final : public TSharedFromThis<FCkUiContextMenuHost>
{
public:
    ~FCkUiContextMenuHost();

    /** Opens the supplied authored session at the requested screen position. */
    auto Open(const TSharedRef<SWidget>& InParent, const FWidgetPath& InOwnerPath, const FVector2f& InScreenPosition,
        TSharedPtr<FCkUiMenuSession> InSession, int32 InFocusUserIndex = INDEX_NONE) -> bool;
    /** Invalidates the owned session, clears its owned focus, and dismisses only this popup. */
    void Release();
    /** Releases the popup when its session is no longer dispatchable or has no visible content. */
    void Tick();
    auto IsOpen() const -> bool;

private:
    void OnMenuDismissed(const TSharedRef<IMenu>& InDismissedMenu);

    TSharedPtr<IMenu> _Menu;
    TSharedPtr<FCkUiMenuSession> _Session;
    uint64 _Generation = 0;
    bool _Opening = false;
};
