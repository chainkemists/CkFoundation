#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkGameSettings/CkGameSettings_Common.h"
#include "CkGameSettings/UI/CkGameSettingsUI_RowWidgets.h"

#include "CkUICore/UserWidget/CkActivatableWidget.h"

#include <Engine/StreamableManager.h>

#include "CkGameSettingsUI_ScreenWidget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCommonButtonBase;
class UCk_TabBarWidget_UE;
class UPanelWidget;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct FCk_GameSettingsUI_RowPool
{
    GENERATED_BODY()

    UPROPERTY(Transient)
    TArray<TObjectPtr<UCk_GameSettingsUI_RowWidgetBase>> _Rows;
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * The settings screen: rows are generated FROM THE REGISTRY (categories from each setting's first
 * category tag's root segment; uncategorized settings land in a "General" tab), resolved through
 * the type→row-class mapping (project-settings overrides → built-in rows), pooled per class, and
 * bound by key. Edit-condition-hidden settings are omitted; disabled ones render disabled with
 * the authored reason as tooltip.
 *
 * The WBP owns the tree — the widget owns the plumbing. Bind _RowContainer (required — rows are
 * injected into it) and optionally _CategoryTabBar / _ApplyButton / _CancelButton. Without a
 * bound tab bar, every category's rows render as one flat list.
 *
 * A layer participant — push via UCk_Utils_UI_Layout_UE::PushWidgetToLayer (games own the
 * open/close flow; nothing pushes this screen automatically).
 *
 * _AutoApplyMode (default) writes live on every row commit. Disabled, activation begins the
 * pending-changes session: row commits preview live, Apply commits them, Cancel/back reverts.
 */
UCLASS(BlueprintType, Blueprintable)
class CKGAMESETTINGS_API UCk_GameSettingsUI_ScreenWidget : public UCk_ActivatableWidget_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_GameSettingsUI_ScreenWidget);

public:
    /** Regenerates tabs + rows from the registry. Called automatically on activation. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|GameSettings|Screen",
              DisplayName = "[Ck][GameSettings] Request Rebuild Rows")
    void
    Request_RebuildRows();

    UFUNCTION(BlueprintPure,
              Category = "Ck|UI|GameSettings|Screen")
    int32
    Get_GeneratedRowCount() const;

    UFUNCTION(BlueprintPure,
              Category = "Ck|UI|GameSettings|Screen")
    int32
    Get_CategoryTabCount() const;

    /** Switches to the named category tab (root category name, e.g. "General"). Returns whether
     *  that tab exists RIGHT NOW — the request is remembered either way and re-resolved after every
     *  rebuild, so selecting an opening tab before the categories are gathered still lands. */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|GameSettings|Screen",
              DisplayName = "[Ck][GameSettings] Request Set Active Category")
    bool
    Request_SetActiveCategory(
        FName InCategory);

    /** Drops back to the flat every-category list (the no-tab-bar default). */
    UFUNCTION(BlueprintCallable,
              Category = "Ck|UI|GameSettings|Screen",
              DisplayName = "[Ck][GameSettings] Request Show All Categories")
    void
    Request_ShowAllCategories();

    UFUNCTION(BlueprintPure,
              Category = "Ck|UI|GameSettings|Screen")
    bool
    Get_HasRowForKey(
        FName InKey) const;

    /** The row widget class currently showing InKey, or null when no visible row is bound to it. */
    UFUNCTION(BlueprintPure,
              Category = "Ck|UI|GameSettings|Screen")
    TSubclassOf<UCk_GameSettingsUI_RowWidgetBase>
    Get_RowClassForKey(
        FName InKey) const;

    /**
     * Which keys InCategory shows, in this order. Returning empty (the default) keeps the
     * registry's own every-registered-key-in-registration-order behavior for that category.
     *
     * This is the curation hook: a hand-authored settings page decides its own contents, so a
     * pack that registers more than the game wants to show, or an order that does not match the
     * design, costs a list here instead of a fork. Keys the registry does not know are reported
     * and skipped.
     */
    UFUNCTION(BlueprintNativeEvent,
              Category = "Ck|UI|GameSettings|Screen")
    TArray<FName>
    Get_CuratedKeysForCategory(
        FName InCategory);

protected:
    /** Fired for every row after it is injected during a populate — style hooks (alternating
     *  backgrounds, per-category grouping) belong in the WBP, not in code. */
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|GameSettings|Screen")
    void
    OnRowGenerated(
        UCk_GameSettingsUI_RowWidgetBase* InRow,
        FName InCategory);

protected:
    auto NativeOnInitialized() -> void override;
    auto NativeOnActivated() -> void override;
    auto NativeOnDeactivated() -> void override;

private:
    UFUNCTION()
    void
    HandleTabSelected(
        int32 InTabIndex);

    UFUNCTION()
    void
    HandleSettingChangedForApply(
        FName InKey,
        const FString& InNewValue);

private:
    auto HandleApplyClicked() -> void;
    auto HandleCancelClicked() -> void;

private:
    auto DoGatherCategories() -> void;
    auto DoConfigureTabs() -> void;
    auto DoPopulateActiveTab() -> void;
    auto DoPopulateRowsNow() -> void;
    auto DoCollapseAllRows() -> void;
    auto Get_KeysToShow() const -> TArray<FName>;
    auto DoAcquireRow(UClass* InRowClass) -> UCk_GameSettingsUI_RowWidgetBase*;
    auto DoGet_LoadedRowClass(const FCk_GameSettings_SettingDefinition& InDefinition) const -> UClass*;
    auto DoReportMisconfig(const TCHAR* InMessage) -> void;

private:
    /** The panel rows are injected into — required (rows have no injection point without it).
     *  WBP-compile-enforced; headless native instantiation still runs with it null.
     *  ROWS ONLY: every populate re-appends rows in display order, which pushes any designed
     *  child above them — chrome (headers, embedded blocks) goes in siblings of this panel. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidget, AllowPrivateAccess = true))
    TObjectPtr<UPanelWidget> _RowContainer;

    /** Category tabs. Absent, every category's rows render as one flat list. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UCk_TabBarWidget_UE> _CategoryTabBar;

    /** Commits the pending-changes session. Ignored in _AutoApplyMode. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UCommonButtonBase> _ApplyButton;

    /** Reverts + deactivates. In _AutoApplyMode simply deactivates. */
    UPROPERTY(BlueprintReadOnly,
              meta = (BindWidgetOptional, AllowPrivateAccess = true))
    TObjectPtr<UCommonButtonBase> _CancelButton;

    /** True: every row commit writes live (the classic behavior). False: activation begins the
     *  pending-changes session — commits preview live, Apply persists, Cancel/back reverts. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Ck|UI|GameSettings|Screen",
              meta = (AllowPrivateAccess = true))
    bool _AutoApplyMode = true;

    /** Set by Request_SetActiveCategory — makes a game-built rail filter without a bound tab bar. */
    bool _CategoryFilterActive = false;

    /** The last category asked for, re-resolved after every gather so a selection made before the
     *  categories exist (or held across a rebuild) is not lost. */
    FName _RequestedCategory;

private:
    UPROPERTY(Transient)
    TMap<TObjectPtr<UClass>, FCk_GameSettingsUI_RowPool> _RowPools;

    TArray<FName> _TabCategories;
    TMap<FName, TArray<FName>> _KeysByCategory;
    int32 _ActiveTabIndex = 0;
    int32 _ActiveRowCount = 0;
    bool _SessionBegun = false;
    bool _ConfiguringTabs = false;
    bool _ReportedMisconfig = false;

    TSharedPtr<FStreamableHandle> _RowClassLoadHandle;

public:
    CK_PROPERTY_GET(_AutoApplyMode);
};

// --------------------------------------------------------------------------------------------------------------------
