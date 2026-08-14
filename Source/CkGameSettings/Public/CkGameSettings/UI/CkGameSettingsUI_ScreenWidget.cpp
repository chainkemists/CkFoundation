#include "CkGameSettingsUI_ScreenWidget.h"

#include "CkGameSettings/CkGameSettings_Log.h"
#include "CkGameSettings/Subsystem/CkGameSettings_Utils.h"

#include "CkWidgets/TabBar/CkTabBar_Widget.h"

#include "Components/PanelWidget.h"

#include <CommonButtonBase.h>
#include <Engine/AssetManager.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_game_settings_ui_screen_widget
{
    const auto GeneralCategory = FName{TEXT("General")};

    auto
        Get_RootCategoryName(
            const FGameplayTagContainer& InCategoryTags)
        -> FName
    {
        if (InCategoryTags.IsEmpty())
        { return GeneralCategory; }

        const auto TagString = InCategoryTags.First().GetTagName().ToString();

        auto DotIndex = int32{INDEX_NONE};

        if (TagString.FindChar(TEXT('.'), DotIndex))
        { return FName{*TagString.Left(DotIndex)}; }

        return FName{*TagString};
    }

}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSettingsUI_ScreenWidget::
    Request_RebuildRows()
    -> void
{
    if (IsDesignTime())
    { return; }

    DoGatherCategories();
    DoConfigureTabs();
    DoPopulateActiveTab();
}

auto
    UCk_GameSettingsUI_ScreenWidget::
    Get_GeneratedRowCount() const
    -> int32
{
    return _ActiveRowCount;
}

auto
    UCk_GameSettingsUI_ScreenWidget::
    Get_CategoryTabCount() const
    -> int32
{
    return _TabCategories.Num();
}

auto
    UCk_GameSettingsUI_ScreenWidget::
    Request_SetActiveCategory(
        FName InCategory)
    -> bool
{
    // Remembered even when the category does not exist YET: a game selecting its opening tab from
    // Construct runs before the first gather, and the request must survive to it (and to every
    // later rebuild) instead of silently degrading to the flat every-category list.
    _RequestedCategory = InCategory;
    _CategoryFilterActive = true;

    const auto CategoryIndex = _TabCategories.IndexOfByKey(InCategory);

    if (CategoryIndex == INDEX_NONE)
    { return false; }

    _ActiveTabIndex = CategoryIndex;

    if (ck::IsValid(_CategoryTabBar))
    {
        _ConfiguringTabs = true;
        _CategoryTabBar->Request_SetActiveTab(_ActiveTabIndex);
        _ConfiguringTabs = false;
    }

    DoPopulateActiveTab();

    return true;
}

auto
    UCk_GameSettingsUI_ScreenWidget::
    Request_ShowAllCategories()
    -> void
{
    _CategoryFilterActive = false;
    _RequestedCategory = NAME_None;

    DoPopulateActiveTab();
}

auto
    UCk_GameSettingsUI_ScreenWidget::
    Get_HasRowForKey(
        FName InKey) const
    -> bool
{
    return ck::IsValid(Get_RowClassForKey(InKey).Get());
}

auto
    UCk_GameSettingsUI_ScreenWidget::
    Get_RowClassForKey(
        FName InKey) const
    -> TSubclassOf<UCk_GameSettingsUI_RowWidgetBase>
{
    for (const auto& [RowClass, Pool] : _RowPools)
    {
        for (const auto& Row : Pool._Rows)
        {
            if (ck::IsValid(Row) && Row->GetVisibility() != ESlateVisibility::Collapsed && Row->Get_SettingKey() == InKey)
            { return Row->GetClass(); }
        }
    }

    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSettingsUI_ScreenWidget::
    NativeOnInitialized()
    -> void
{
    Super::NativeOnInitialized();

    ck::game_settings_ui::BindClick(_ApplyButton, this, &UCk_GameSettingsUI_ScreenWidget::HandleApplyClicked);
    ck::game_settings_ui::BindClick(_CancelButton, this, &UCk_GameSettingsUI_ScreenWidget::HandleCancelClicked);
}

auto
    UCk_GameSettingsUI_ScreenWidget::
    NativeOnActivated()
    -> void
{
    Super::NativeOnActivated();

    if (ck::Is_NOT_Valid(_RowContainer))
    {
        DoReportMisconfig(TEXT("GameSettings screen [{}] activated with no _RowContainer bound — rows have "
            "no injection point. Bind a panel widget named _RowContainer in the WBP"));
    }

    if (NOT _AutoApplyMode && NOT _SessionBegun)
    {
        _SessionBegun = UCk_Utils_GameSettings_UE::Request_BeginPendingChanges(this);
    }

    // In session mode the Apply button tracks dirtiness: disabled until a row commits a change,
    // driven by the wildcard change signal. Auto-apply mode leaves it alone.
    if (_SessionBegun && ck::IsValid(_ApplyButton))
    {
        _ApplyButton->SetIsEnabled(false);

        auto OnChanged = FCk_Delegate_GameSettings_OnSettingChanged{};
        OnChanged.BindDynamic(this, &UCk_GameSettingsUI_ScreenWidget::HandleSettingChangedForApply);
        UCk_Utils_GameSettings_UE::BindTo_OnSettingChanged(this, NAME_None, OnChanged);
    }

    Request_RebuildRows();
}

auto
    UCk_GameSettingsUI_ScreenWidget::
    NativeOnDeactivated()
    -> void
{
    if (_SessionBegun)
    {
        UCk_Utils_GameSettings_UE::Request_RevertPendingChanges(this);
        _SessionBegun = false;

        auto OnChanged = FCk_Delegate_GameSettings_OnSettingChanged{};
        OnChanged.BindDynamic(this, &UCk_GameSettingsUI_ScreenWidget::HandleSettingChangedForApply);
        UCk_Utils_GameSettings_UE::UnbindFrom_OnSettingChanged(this, NAME_None, OnChanged);
    }

    Super::NativeOnDeactivated();
}

auto
    UCk_GameSettingsUI_ScreenWidget::
    HandleSettingChangedForApply(
        FName InKey,
        const FString& InNewValue)
    -> void
{
    if (ck::IsValid(_ApplyButton))
    { _ApplyButton->SetIsEnabled(UCk_Utils_GameSettings_UE::Get_HasPendingChanges(this)); }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSettingsUI_ScreenWidget::
    HandleTabSelected(
        int32 InTabIndex)
    -> void
{
    if (_ConfiguringTabs)
    { return; }

    _ActiveTabIndex = InTabIndex;
    DoPopulateActiveTab();
}

auto
    UCk_GameSettingsUI_ScreenWidget::
    HandleApplyClicked()
    -> void
{
    if (NOT _SessionBegun)
    { return; }

    UCk_Utils_GameSettings_UE::Request_ApplyPendingChanges(this);
    _SessionBegun = UCk_Utils_GameSettings_UE::Request_BeginPendingChanges(this);

    if (ck::IsValid(_ApplyButton))
    { _ApplyButton->SetIsEnabled(false); }
}

auto
    UCk_GameSettingsUI_ScreenWidget::
    HandleCancelClicked()
    -> void
{
    DeactivateWidget();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSettingsUI_ScreenWidget::
    DoGatherCategories()
    -> void
{
    using namespace ck_game_settings_ui_screen_widget;

    _TabCategories.Reset();
    _KeysByCategory.Reset();

    for (const auto& Key : UCk_Utils_GameSettings_UE::Get_AllSettingKeys(this))
    {
        auto Definition = FCk_GameSettings_SettingDefinition{};

        if (NOT UCk_Utils_GameSettings_UE::Get_SettingDefinition(this, Key, Definition))
        { continue; }

        if (ck::game_settings_ui::Get_EditDisposition(Definition.Get_EditConditions()) == ck::game_settings_ui::EEditDisposition::Hidden)
        { continue; }

        const auto Category = Get_RootCategoryName(Definition.Get_CategoryTags());

        if (NOT _KeysByCategory.Contains(Category))
        {
            _TabCategories.Emplace(Category);
            _KeysByCategory.Emplace(Category);
        }

        _KeysByCategory[Category].Emplace(Key);
    }

    for (const auto& Category : _TabCategories)
    {
        const auto CuratedKeys = Get_CuratedKeysForCategory(Category);

        if (CuratedKeys.IsEmpty())
        { continue; }

        auto& CategoryKeys = _KeysByCategory[Category];
        CategoryKeys.Reset();

        for (const auto& Key : CuratedKeys)
        {
            CK_ENSURE_IF_NOT(UCk_Utils_GameSettings_UE::Get_IsSettingRegistered(this, Key),
                TEXT("Settings screen [{}] curates key [{}] for category [{}], which is not registered. It was skipped."),
                this, Key, Category)
            { continue; }

            CategoryKeys.Emplace(Key);
        }
    }

    if (const auto RequestedIndex = _TabCategories.IndexOfByKey(_RequestedCategory);
        RequestedIndex != INDEX_NONE)
    { _ActiveTabIndex = RequestedIndex; }

    _ActiveTabIndex = FMath::Clamp(_ActiveTabIndex, 0, FMath::Max(0, _TabCategories.Num() - 1));
}

auto
    UCk_GameSettingsUI_ScreenWidget::
    Get_CuratedKeysForCategory_Implementation(
        FName InCategory)
    -> TArray<FName>
{
    return {};
}

auto
    UCk_GameSettingsUI_ScreenWidget::
    DoConfigureTabs()
    -> void
{
    if (ck::Is_NOT_Valid(_CategoryTabBar))
    { return; }

    _ConfiguringTabs = true;

    _CategoryTabBar->OnTabSelected.AddUniqueDynamic(this, &UCk_GameSettingsUI_ScreenWidget::HandleTabSelected);

    while (_CategoryTabBar->Get_TabCount() > _TabCategories.Num())
    {
        _CategoryTabBar->RemoveTab(_CategoryTabBar->Get_TabCount() - 1);
    }

    for (auto Index = 0; Index < _TabCategories.Num(); ++Index)
    {
        const auto TabConfig = FCk_TabBar_TabConfig{}.Set_TabName(FText::FromName(_TabCategories[Index]));

        if (Index < _CategoryTabBar->Get_TabCount())
        {
            _CategoryTabBar->Request_SetTabConfig(Index, TabConfig);
        }
        else
        {
            _CategoryTabBar->AddTab(TabConfig);
        }
    }

    _CategoryTabBar->Request_SetActiveTab(_ActiveTabIndex);

    _ConfiguringTabs = false;
}

auto
    UCk_GameSettingsUI_ScreenWidget::
    Get_KeysToShow() const
    -> TArray<FName>
{
    // Filtering follows the SELECTION, not the tab-bar widget: a game driving its own rail through
    // Request_SetActiveCategory filters identically to one binding _CategoryTabBar.
    if ((ck::IsValid(_CategoryTabBar) || _CategoryFilterActive) && _TabCategories.IsValidIndex(_ActiveTabIndex))
    {
        return _KeysByCategory[_TabCategories[_ActiveTabIndex]];
    }

    // No tab bar and no explicit selection: every category's rows render as one flat list
    auto AllKeys = TArray<FName>{};

    for (const auto& Category : _TabCategories)
    {
        AllKeys.Append(_KeysByCategory[Category]);
    }

    return AllKeys;
}

auto
    UCk_GameSettingsUI_ScreenWidget::
    DoPopulateActiveTab()
    -> void
{
    auto ClassesToLoad = TArray<FSoftObjectPath>{};

    for (const auto& Key : Get_KeysToShow())
    {
        auto Definition = FCk_GameSettings_SettingDefinition{};

        if (NOT UCk_Utils_GameSettings_UE::Get_SettingDefinition(this, Key, Definition))
        { continue; }

        const auto ResolvedClass = ck::game_settings_ui::Get_ResolvedRowClass(Definition);

        if (NOT ResolvedClass.IsNull() && ck::Is_NOT_Valid(ResolvedClass.Get()))
        {
            ClassesToLoad.AddUnique(ResolvedClass.ToSoftObjectPath());
        }
    }

    if (ClassesToLoad.IsEmpty())
    {
        DoPopulateRowsNow();
        return;
    }

    // The screen shows nothing until the row classes finish loading — no sync loads in widget code
    DoCollapseAllRows();
    _ActiveRowCount = 0;

    _RowClassLoadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(ClassesToLoad,
        FStreamableDelegate::CreateWeakLambda(this, [this]()
        {
            DoPopulateRowsNow();
        }));
}

auto
    UCk_GameSettingsUI_ScreenWidget::
    DoPopulateRowsNow()
    -> void
{
    using namespace ck_game_settings_ui_screen_widget;

    DoCollapseAllRows();
    _ActiveRowCount = 0;

    for (const auto& Key : Get_KeysToShow())
    {
        auto Definition = FCk_GameSettings_SettingDefinition{};

        if (NOT UCk_Utils_GameSettings_UE::Get_SettingDefinition(this, Key, Definition))
        { continue; }

        const auto RowClass = DoGet_LoadedRowClass(Definition);

        if (ck::Is_NOT_Valid(RowClass))
        {
            ck::game_settings::Warning(TEXT("GameSettings screen [{}] could not resolve a loaded row class for key [{}] — row skipped"), this, Key);
            continue;
        }

        const auto Row = DoAcquireRow(RowClass);

        if (ck::Is_NOT_Valid(Row))
        { continue; }

        // Re-added in iteration order every populate: a pooled row keeps the slot it was created
        // in otherwise, and ShiftChild is NOT an alternative — it reorders the UMG slot array
        // without rebuilding the live Slate panel, so nothing moves on screen. Remove+Add
        // rebuilds. Consequence: designed children of _RowContainer would end up ABOVE the rows —
        // the container is documented rows-only, chrome goes in siblings.
        if (ck::IsValid(_RowContainer))
        {
            _RowContainer->RemoveChild(Row);
            _RowContainer->AddChild(Row);
        }

        Row->InjectSetting(this, Key);
        Row->SetVisibility(ESlateVisibility::Visible);
        ++_ActiveRowCount;

        OnRowGenerated(Row, Get_RootCategoryName(Definition.Get_CategoryTags()));
    }
}

auto
    UCk_GameSettingsUI_ScreenWidget::
    DoCollapseAllRows()
    -> void
{
    for (const auto& [RowClass, Pool] : _RowPools)
    {
        for (const auto& Row : Pool._Rows)
        {
            if (ck::IsValid(Row))
            { Row->SetVisibility(ESlateVisibility::Collapsed); }
        }
    }
}

auto
    UCk_GameSettingsUI_ScreenWidget::
    DoAcquireRow(
        UClass* InRowClass)
    -> UCk_GameSettingsUI_RowWidgetBase*
{
    auto& Pool = _RowPools.FindOrAdd(InRowClass);

    for (const auto& PooledRow : Pool._Rows)
    {
        if (ck::IsValid(PooledRow) && PooledRow->GetVisibility() == ESlateVisibility::Collapsed)
        { return PooledRow; }
    }

    const auto NewRow = CreateWidget<UCk_GameSettingsUI_RowWidgetBase>(this, InRowClass);

    if (ck::Is_NOT_Valid(NewRow))
    { return {}; }

    if (ck::IsValid(_RowContainer))
    { _RowContainer->AddChild(NewRow); }

    Pool._Rows.Emplace(NewRow);

    return NewRow;
}

auto
    UCk_GameSettingsUI_ScreenWidget::
    DoGet_LoadedRowClass(
        const FCk_GameSettings_SettingDefinition& InDefinition) const
    -> UClass*
{
    const auto Resolved = ck::game_settings_ui::Get_ResolvedRowClass(InDefinition);

    if (const auto ResolvedClass = Resolved.Get();
        ck::IsValid(ResolvedClass))
    { return ResolvedClass; }

    // Override not in memory (design time, or a failed load): fall back to the built-in default
    const auto Fallback = ck::game_settings_ui::Get_ResolvedRowClass(InDefinition,
        TMap<ECk_GameSettings_ValueType, TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>>{},
        TSoftClassPtr<UCk_GameSettingsUI_RowWidgetBase>{});

    return Fallback.Get();
}

auto
    UCk_GameSettingsUI_ScreenWidget::
    DoReportMisconfig(
        const TCHAR* InMessage)
    -> void
{
    if (IsDesignTime())
    {
        if (_ReportedMisconfig)
        { return; }

        _ReportedMisconfig = true;
        ck::game_settings::Warning(InMessage, this);
    }
    else
    {
        CK_TRIGGER_ENSURE(InMessage, this);
    }
}

// --------------------------------------------------------------------------------------------------------------------
