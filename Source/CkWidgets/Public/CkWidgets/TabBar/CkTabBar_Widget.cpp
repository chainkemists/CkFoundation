#include "CkTabBar_Widget.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkUICore/Types/CkUI_Types.h"

#include <Components/WidgetSwitcher.h>

// --------------------------------------------------------------------------------------------------------------------

UCk_TabBarWidget_UE::
    UCk_TabBarWidget_UE()
{
    if (IsRunningDedicatedServer())
    { return; }

    _Style = FCk_TabBar_Style::Get_Default();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_TabBarWidget_UE::
    SynchronizeProperties()
    -> void
{
    Super::SynchronizeProperties();

    if (ck::Is_NOT_Valid(_TabBar))
    { return; }

    DoSyncTabsWithSwitcher();

    _TabBar->Set_Style(&_Style);
    _TabBar->Set_Orientation(_Orientation);
    _TabBar->Set_TabConfigs(_Tabs);
    _TabBar->Set_ActiveTab(_ActiveTabIndex);
}

auto
    UCk_TabBarWidget_UE::
    ReleaseSlateResources(
        bool bReleaseChildren)
    -> void
{
    Super::ReleaseSlateResources(bReleaseChildren);

    if (ck::IsValid(_TabBar))
    { _ActiveTabIndex = _TabBar->Get_ActiveTabIndex(); }

    _TabBar.Reset();
}

#if WITH_EDITOR
auto
    UCk_TabBarWidget_UE::
    GetPaletteCategory()
    -> const FText
{
    return ck::widget_palette_categories::Default;
}
#endif

auto
    UCk_TabBarWidget_UE::
    RebuildWidget()
    -> TSharedRef<SWidget>
{
    DoSyncTabsWithSwitcher();

    _TabBar = SNew(SCk_TabBar)
        .Style(&_Style)
        .TabConfigs(_Tabs)
        .Orientation(_Orientation)
        .InitialTabIndex(_ActiveTabIndex)
        .OnTabSelected(FCk_Delegate_TabBar_TabSelected::CreateUObject(this, &UCk_TabBarWidget_UE::HandleTabSelected));

    return _TabBar.ToSharedRef();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_TabBarWidget_UE::
    Request_SetLinkedSwitcher(
        UWidgetSwitcher* InSwitcher)
    -> void
{
    _LinkedSwitcher = InSwitcher;

    if (ck::IsValid(_LinkedSwitcher))
    { _ActiveTabIndex = _LinkedSwitcher->GetActiveWidgetIndex(); }

    RebuildTabs();

    if (ck::IsValid(_TabBar))
    { _TabBar->Set_ActiveTab(_ActiveTabIndex); }
}

auto
    UCk_TabBarWidget_UE::
    Request_SetActiveTab(
        int32 InIndex)
    -> void
{
    CK_ENSURE_IF_NOT(_Tabs.IsValidIndex(InIndex),
        TEXT("Invalid tab index [{}] — TabBar has [{}] tab(s).{}"), InIndex, _Tabs.Num(), ck::Context(this))
    { return; }

    _ActiveTabIndex = InIndex;

    if (ck::IsValid(_TabBar))
    {
        _TabBar->Set_ActiveTab(InIndex);
        return;
    }

    if (ck::IsValid(_LinkedSwitcher))
    { _LinkedSwitcher->SetActiveWidgetIndex(InIndex); }
}

auto
    UCk_TabBarWidget_UE::
    Get_ActiveTabIndex() const
    -> int32
{
    return ck::IsValid(_TabBar) ? _TabBar->Get_ActiveTabIndex() : _ActiveTabIndex;
}

auto
    UCk_TabBarWidget_UE::
    Get_TabCount() const
    -> int32
{
    return _Tabs.Num();
}

auto
    UCk_TabBarWidget_UE::
    Request_SetTabConfig(
        int32 InIndex,
        const FCk_TabBar_TabConfig& InTabConfig)
    -> void
{
    CK_ENSURE_IF_NOT(_Tabs.IsValidIndex(InIndex),
        TEXT("Invalid tab index [{}] — TabBar has [{}] tab(s).{}"), InIndex, _Tabs.Num(), ck::Context(this))
    { return; }

    _Tabs[InIndex] = InTabConfig;

    if (ck::IsValid(_TabBar))
    { _TabBar->Set_TabConfig(InIndex, InTabConfig); }
}

auto
    UCk_TabBarWidget_UE::
    Get_TabConfig(
        int32 InIndex) const
    -> FCk_TabBar_TabConfig
{
    CK_ENSURE_IF_NOT(_Tabs.IsValidIndex(InIndex),
        TEXT("Invalid tab index [{}] — TabBar has [{}] tab(s).{}"), InIndex, _Tabs.Num(), ck::Context(this))
    { return {}; }

    return _Tabs[InIndex];
}

auto
    UCk_TabBarWidget_UE::
    AddTab(
        const FCk_TabBar_TabConfig& InTabConfig)
    -> void
{
    CK_ENSURE_IF_NOT(ck::Is_NOT_Valid(_LinkedSwitcher),
        TEXT("Cannot AddTab — the tab count follows the linked switcher's children. "
             "Add a child to the switcher instead.{}"), ck::Context(this))
    { return; }

    _Tabs.Add(InTabConfig);

    if (ck::IsValid(_TabBar))
    { _TabBar->Set_TabConfigs(_Tabs); }
}

auto
    UCk_TabBarWidget_UE::
    RemoveTab(
        int32 InIndex)
    -> void
{
    CK_ENSURE_IF_NOT(ck::Is_NOT_Valid(_LinkedSwitcher),
        TEXT("Cannot RemoveTab — the tab count follows the linked switcher's children. "
             "Remove the child from the switcher instead.{}"), ck::Context(this))
    { return; }

    CK_ENSURE_IF_NOT(_Tabs.IsValidIndex(InIndex),
        TEXT("Invalid tab index [{}] — TabBar has [{}] tab(s).{}"), InIndex, _Tabs.Num(), ck::Context(this))
    { return; }

    _Tabs.RemoveAt(InIndex);

    if (ck::IsValid(_TabBar))
    { _TabBar->Set_TabConfigs(_Tabs); }

    _ActiveTabIndex = Get_ActiveTabIndex();
}

auto
    UCk_TabBarWidget_UE::
    RebuildTabs()
    -> void
{
    DoSyncTabsWithSwitcher();

    if (ck::IsValid(_TabBar))
    { _TabBar->Set_TabConfigs(_Tabs); }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_TabBarWidget_UE::
    DoSyncTabsWithSwitcher()
    -> void
{
    if (ck::Is_NOT_Valid(_LinkedSwitcher))
    { return; }

    const auto NumChildren = _LinkedSwitcher->GetChildrenCount();

    if (_Tabs.Num() < NumChildren)
    {
        const auto FirstNewIndex = _Tabs.Num();
        _Tabs.SetNum(NumChildren);

        for (auto Index = FirstNewIndex; Index < NumChildren; ++Index)
        {
            _Tabs[Index].Set_TabName(
                FText::Format(NSLOCTEXT("CkUI", "TabBar_DefaultTabName", "Tab {0}"), FText::AsNumber(Index + 1)));
        }
    }
    else if (_Tabs.Num() > NumChildren)
    {
        _Tabs.SetNum(NumChildren);
    }
}

auto
    UCk_TabBarWidget_UE::
    HandleTabSelected(
        int32 InTabIndex)
    -> void
{
    _ActiveTabIndex = InTabIndex;

    if (ck::IsValid(_LinkedSwitcher))
    { _LinkedSwitcher->SetActiveWidgetIndex(InTabIndex); }

    OnTabSelected.Broadcast(InTabIndex);
}

// --------------------------------------------------------------------------------------------------------------------
