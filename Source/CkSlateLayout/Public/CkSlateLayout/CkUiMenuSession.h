#pragma once

#include "CoreMinimal.h"
#include "Misc/Attribute.h"

class FMenuBuilder;
class SWidget;

/**
 * Shared native menu presenter state. Configurations are revisioned immutable snapshots:
 * publishing or deactivating makes callbacks retained by an older menu inert.
 * This object owns only menu-content focus roots; the Slate host owns popup presentation.
 */
class CKSLATELAYOUT_API FCkUiMenuSession final : public TSharedFromThis<FCkUiMenuSession>
{
public:
    struct FEntry
    {
        FString Key;
        TAttribute<FText> Label;
        TAttribute<FText> Tooltip;
        TAttribute<bool> Enabled;
        TAttribute<bool> Visible;
        FSimpleDelegate Action;
        TArray<FEntry> Children;
        bool Separator = false;
    };

    /** Publishes a snapshot without executing actions or changing focus. */
    void Configure(TArray<FEntry> InEntries, TAttribute<bool> InEnabled, TAttribute<bool> InCanDispatchEvents);
    /** Builds native menu content from the current snapshot. */
    auto BuildMenu() -> TSharedRef<SWidget>;
    /** Returns the current host enabled gate. */
    auto IsEnabled() const -> bool;
    /** Returns whether the current configuration has visible non-separator content. */
    auto HasVisibleEntries() const -> bool;
    /** Clears focus only when it is inside menu roots built by this session. */
    void ClearOwnedFocus();
    /** Invalidates retained callbacks before clearing owned menu focus. */
    void Deactivate();

private:
    struct FSnapshot;

    void BuildEntries(FMenuBuilder& InBuilder, const TSharedRef<const FSnapshot>& InSnapshot, const TArray<int32>& InParentPath);
    auto IsSnapshotCurrent(const TSharedRef<const FSnapshot>& InSnapshot) const -> bool;
    auto IsEntryVisible(const TSharedRef<const FSnapshot>& InSnapshot, const TArray<int32>& InPath) const -> bool;
    auto CanDispatch(const TSharedRef<const FSnapshot>& InSnapshot, const TArray<int32>& InPath) const -> bool;
    void Dispatch(const TSharedRef<const FSnapshot>& InSnapshot, const TArray<int32>& InPath);
    void RegisterOwnedRoot(const TSharedRef<SWidget>& InRoot);

    TSharedPtr<const FSnapshot> _Snapshot;
    TArray<TWeakPtr<SWidget>> _OwnedMenuRoots;
    uint64 _Revision = 0;
    bool _Active = true;
};
