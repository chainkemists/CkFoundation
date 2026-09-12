#pragma once

#include "CkUiDocument.h"
#include "CkUiWidgetRegistry.h"
#include "SCkUiTable.h"
#include "SCkUiTree.h"
#include "CkUiContextMenu.h"
#include "CkUiMenuSession.h"

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Framework/SlateDelegates.h"
#include "Widgets/SWidget.h"

class SBox;
class SSearchBox;
class SScrollBox;
class SCkUiSplitter;
class SCkUiTabs;
class SCkUiMenuButton;
class SCkUiRepeat;
class FCkUiCollection;
class FCkUiFloatSeries;
struct FSlateBrush;
class FWidgetPath;

DECLARE_DELEGATE_OneParam(FCkUiOnItemAction, FString);
DECLARE_DELEGATE_TwoParams(FCkUiOnItemBoolChanged, FString, bool);
DECLARE_DELEGATE_ThreeParams(FCkUiOnItemNumberCommitted, FString, float, ETextCommit::Type);
DECLARE_DELEGATE_ThreeParams(FCkUiOnItemIntegerCommitted, FString, int32, ETextCommit::Type);

/**
 * A retained native surface with named ports for document-authored subtrees.
 *
 * The owner installs GetRegion() results into its native chrome (for example a splitter) before
 * loading a document. Reloads stage every authored widget first, then replace all port contents
 * together; a rejected document leaves the displayed tree and revision unchanged.
 * Mounted views are created, updated and released on the game thread.
 */
class CKSLATELAYOUT_API FCkUiView final : public TSharedFromThis<FCkUiView>
{
public:
    ~FCkUiView();

    using FNativeBindings = TMap<FString, TSharedPtr<SWidget>>;
    using FActions = TMap<FString, FSimpleDelegate>;
    using FTokens = TMap<FString, FString>;

    struct FDataBindings
    {
        TMap<FString, TAttribute<FText>> Text;
        TMap<FString, TAttribute<FString>> String;
        TMap<FString, FOnTextChanged> TextChanged;
        TMap<FString, FOnTextCommitted> TextCommitted;
        TMap<FString, FCkUiOnBoolChanged> BoolChanged;
        TMap<FString, FCkUiOnNumberChanged> NumberChanged;
        TMap<FString, FCkUiOnNumberCommitted> NumberCommitted;
        TMap<FString, FCkUiOnIntegerCommitted> IntegerCommitted;
        TMap<FString, FCkUiOnColorCommitted> ColorCommitted;
        TMap<FString, FCkUiOnNumberInteraction> NumberInteraction;
        TMap<FString, FCkUiOnStringChanged> StringChanged;
        TMap<FString, TAttribute<const FSlateBrush*>> Images;
        TMap<FString, TAttribute<float>> Number;
        TMap<FString, TAttribute<int32>> Integer;
        TMap<FString, TAttribute<FLinearColor>> Color;
        TMap<FString, TAttribute<bool>> Visibility;
        /** Optional inherited dispatch gate. Unset retains the normal enabled behavior. */
        TAttribute<bool> CanDispatchEvents;
        TMap<FString, TSharedPtr<FCkUiCollection>> Collections;
        /** Host-owned mutable series. The view retains only weak handles and never extends their lifetime. */
        TMap<FString, TWeakPtr<FCkUiFloatSeries>> FloatSeries;
        TMap<FString, TSharedPtr<FCkUiTreeCollection>> Trees;
        TMap<FString, FOnCkUiTreeSelectionChanged> TreeSelectionChanged;
        TMap<FString, FOnCkUiTableSelectionChanged> TableSelectionChanged;
        TMap<FString, FOnContextMenuOpening> TableContextMenus;
        TMap<FString, FOnCkUiContextAction> ContextActions;
        /** Repeat item actions receive the stable collection key of their active record scope. */
        TMap<FString, FCkUiOnItemAction> ItemActions;
        /** Repeat item BoolChanged callbacks receive the stable key and proposed value. */
        TMap<FString, FCkUiOnItemBoolChanged> ItemBoolChanged;
        /** Repeat item NumberCommitted callbacks receive the stable key, value, and commit reason. */
        TMap<FString, FCkUiOnItemNumberCommitted> ItemNumberCommitted;
        /** Repeat item IntegerCommitted callbacks receive the stable key, value, and commit reason. */
        TMap<FString, FCkUiOnItemIntegerCommitted> ItemIntegerCommitted;
        /** Host-owned focus/input context, inherited by nested views. INDEX_NONE means no explicit owner. */
        int32 SlateUserIndex = INDEX_NONE;
    };

    /** One participant in an all-or-nothing authored-document reload. */
    struct FReloadRequest
    {
        TSharedPtr<FCkUiView> View;
        FString Markup;
        FString Stylesheet;
        FString Source = TEXT("<memory>");
        /** Candidate CSS token set. Unset retains the view's applied tokens. */
        TOptional<FTokens> Tokens;
    };

    static auto Create(
        FNativeBindings InBindings,
        FActions InActions = {},
        FTokens InTokens = {},
        FSlateFontInfo InBaseFont = FSlateFontInfo(),
        FDataBindings InData = {},
        TSharedPtr<const FCkUiWidgetRegistrySnapshot> InCustomRegistry = {}) -> TSharedRef<FCkUiView>;

    /** Creates or returns the persistent native mount for one document region. */
    auto GetRegion(const FString& InName) -> TSharedRef<SWidget>;

    /** Parses, validates and stages a complete document before changing any mounted region. */
    auto TryReload(
        const FString& InMarkup,
        const FString& InStylesheet,
        const FString& InSource = TEXT("<memory>")) -> FCkUiLoadResult;

    /** Parses, validates and atomically applies a complete document with candidate CSS tokens. */
    auto TryReloadWithTokens(
        const FString& InMarkup,
        const FString& InStylesheet,
        FTokens InTokens,
        const FString& InSource = TEXT("<memory>")) -> FCkUiLoadResult;

    /** Reloads every participant atomically. A rejected participant leaves every mounted view unchanged. */
    static auto TryReloadBatch(const TArray<FReloadRequest>& InRequests) -> FCkUiLoadResult;

    /** Reads both files completely, then delegates to TryReload. File paths become PollFiles inputs. */
    auto ReloadFiles(
        const FString& InMarkupPath,
        const FString& InStylesheetPath) -> FCkUiLoadResult;

    /** Reattempts a file reload only when the complete markup/stylesheet content pair changed. */
    auto PollFiles() -> bool;

    /** Reattempts a file reload when either the complete source pair or candidate token set changed. */
    auto PollFiles(const FTokens& InTokens) -> bool;

    auto SetFiles(
        const FString& InMarkupPath,
        const FString& InStylesheetPath) -> void;

    const FCkUiLoadResult& GetLastResult() const { return _LastResult; }
    int64 GetRevision() const { return _Revision; }
    /** IDs are local; use container-id/slot-name/child-id to explicitly traverse custom slots. */
    TSharedPtr<SCkUiTable> GetTable(const FString& InId) const;
    TSharedPtr<SCkUiTree> GetTree(const FString& InId) const;
    TSharedPtr<SScrollBox> GetScroll(const FString& InId) const;
    TSharedPtr<SCkUiSplitter> GetSplitter(const FString& InId) const;
    TSharedPtr<SCkUiTabs> GetTabs(const FString& InId) const;
    TSharedPtr<SCkUiMenuButton> GetMenuButton(const FString& InId) const;
    TSharedPtr<SCkUiRepeat> GetRepeat(const FString& InId) const;

private:
    explicit FCkUiView(
        FNativeBindings InBindings,
        FActions InActions,
        FTokens InTokens,
        FSlateFontInfo InBaseFont,
        FDataBindings InData,
        TSharedPtr<const FCkUiWidgetRegistrySnapshot> InCustomRegistry);

    enum class ERetainedKind : uint8 { Native, Search, Custom, Table, Scroll, Splitter, Tree, Tabs, MenuButton, Repeat };
    struct FRetainedRecord
    {
        FString Id;
        ERetainedKind Kind = ERetainedKind::Native;
        FString CompatibilityKey;
        FString CustomTag;
        TSharedPtr<SWidget> Widget;
        TSharedPtr<ICkUiRetainedWidget> Component;
        TSharedPtr<SBox> Port;
        TSharedPtr<SCkUiTable> Table;
    };
    struct FStagedDocument;
    struct FCommitState;
    struct FRepeatScope;
    struct FRepeatState;
    struct FCustomSlot;
    struct FCustomSlotSet;
    struct FViewContext
    {
        FDataBindings Data;
        FActions Actions;
        TSet<FString> GeneratedRepeatActionAliases;
        TSet<FString> GeneratedRepeatFieldAliases;
        TSet<FString> GeneratedRepeatItemEventAliases;
    };
    struct FNestedUpdate;
    auto FindRetained(const FString& InId) const -> const FRetainedRecord*;
    struct FCapturedPointer
    {
        FCkUiPointerCapture Capture;
        FString RetainedId;
    };
    auto ContainsMountedPath(const FWidgetPath& InPath) const -> bool;
    auto CanDispatchEvents() const -> bool;
    auto GetOwnedPointerCaptures(EVisibility InVisibility = EVisibility::Visible) const -> TArray<FCapturedPointer>;
    auto ReleaseTransientInteractions(const TSharedRef<SWidget>& InRoot, bool bOwnerRelease = false) -> void;


    auto ValidateDocument(const FCkUiDocument& InDocument, const FString& InSource, TArray<FString>& OutErrors) const -> bool;
    auto BuildMenuEntries(const TMap<FString, FCkUiMenu>& InMenus, const FString& InMenuId,
        TOptional<FString> InContextKey = {}) const -> TArray<FCkUiMenuSession::FEntry>;
    auto MakeContextMenuBinding(const FCkUiNode& InNode, const TMap<FString, FCkUiMenu>& InMenus) const -> FOnCkUiContextMenuOpening;
    auto StageDocument(const FCkUiDocument& InDocument, FStagedDocument& OutStaged, TArray<FString>& OutErrors) const -> bool;
    auto PrepareRepeat(const FCkUiNode& InNode, FStagedDocument& OutStaged, TArray<FString>& OutErrors) const -> TSharedPtr<SWidget>;
    auto MakeCustomSlotView(const FCkUiNode& InRoot, const TSharedPtr<FCustomSlot>& InSlot,
        const TMap<FString, FCkUiMenu>& InMenus, FCkUiDocument& OutDocument, TOptional<FViewContext>& OutReplacementContext,
        TArray<FString>& OutErrors) const -> TSharedPtr<FCkUiView>;
    auto PrepareCustomSlots(const FCkUiNode& InNode, const FCkUiCustomWidgetSchema& InSchema,
        FStagedDocument& OutStaged, FCkUiCustomWidgetArguments& OutArguments, TArray<FString>& OutErrors) const -> bool;
    auto MakeRepeatItem(const FCkUiNode& InNode, const TSharedPtr<FCkUiCollection>& Collection,
        const TSharedPtr<const FCkUiRecord>& Record, const TSharedPtr<FRepeatScope>& Scope,
        const TSharedPtr<FCkUiView>& Existing, FCkUiDocument& OutDocument, TOptional<FViewContext>& OutReplacementContext,
        TArray<FString>& OutErrors) const -> TSharedPtr<FCkUiView>;
    auto RefreshRepeat(const FString& InId) -> bool;
    static auto MountRepeat(FRepeatState& InState) -> void;
    template <typename TRecord>
    auto MakeCellView(const FCkUiNode& InCell, TWeakPtr<const TRecord> InRecord, bool InValidateOnly, TArray<FString>& OutErrors) const -> TSharedPtr<FCkUiView>;
    auto StageNode(const FCkUiNode& InNode, FStagedDocument& InOutStaged, TArray<FString>& OutErrors) const -> TSharedPtr<SWidget>;
    auto MakePort(const FCkUiNode& InNode, FStagedDocument& InOutStaged) const -> TSharedRef<SWidget>;
    auto ApplyStyle(const FCkUiStyle& InStyle, const TSharedRef<SWidget>& InContent) const -> TSharedRef<SWidget>;
    auto ReadFiles(FString& OutMarkup, FString& OutStylesheet, FString& OutSource) const -> FCkUiLoadResult;
    auto CaptureCommit(FStagedDocument& InStaged) -> FCommitState;
    auto PublishConfiguration(FStagedDocument&& InStaged, FCommitState& InOutState) -> void;
    auto ReconcileInteractions(FCommitState& InOutState) -> void;
    static auto FlattenNested(FStagedDocument& InStaged, TArray<FString>& OutErrors) -> bool;
    auto MountChildren() -> void;
    auto CollectChildMounts(TSet<const SWidget*>& OutMounts) const -> void;
    auto Commit(FStagedDocument&& InStaged, TArray<FString>& OutErrors) -> bool;

    FNativeBindings _Bindings;
    FActions _Actions;
    /** @item aliases generated for this item scope; nested scopes may safely replace only these. */
    TSet<FString> _GeneratedRepeatActionAliases;
    /** @field aliases generated for this item scope; nested schemas replace these across binding families. */
    TSet<FString> _GeneratedRepeatFieldAliases;
    /** Typed item-event aliases generated for this item scope; nested scopes may replace only these. */
    TSet<FString> _GeneratedRepeatItemEventAliases;
    FTokens _Tokens;
    FSlateFontInfo _BaseFont;
    FDataBindings _Data;
    TSharedPtr<const FCkUiWidgetRegistrySnapshot> _CustomRegistry;
    TMap<FString, TSharedPtr<SBox>> _RegionMounts;
    TMap<FString, FRetainedRecord> _CommittedRetained;
    TMap<FString, TSharedPtr<FRepeatState>> _RepeatStates;
    TMap<FString, TSharedPtr<FCustomSlotSet>> _CustomSlots;
    TMap<FString, TAttribute<FText>> _CommittedSearchHints;
    FCkUiLoadResult _LastResult;
    FString _MarkupPath;
    FString _StylesheetPath;
    FString _LastPolledMarkup;
    FString _LastPolledStylesheet;
    FTokens _LastPolledTokens;
    bool _HasPolledContent = false;
    bool _IsReloading = false;
    int64 _Revision = 0;
};
