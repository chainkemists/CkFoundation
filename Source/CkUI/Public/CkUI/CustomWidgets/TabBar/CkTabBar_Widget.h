#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkUI/CustomWidgets/TabBar/CkTabBar_Slate.h"
#include "CkUI/CustomWidgets/TabBar/CkTabBar_Types.h"

#include <CoreMinimal.h>
#include <Components/Widget.h>
#include <Types/SlateEnums.h>

#include "CkTabBar_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UWidgetSwitcher;

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCk_TabBar_TabSelectedEvent, int32, TabIndex);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Tab bar that can optionally drive a UWidgetSwitcher.
 * When a switcher is linked, the tab count mirrors the switcher's children and selecting
 * a tab activates the matching switcher index. Without one, tabs come from the Tabs array
 * and selection is reported through OnTabSelected only.
 * Configure via the properties below; usable directly with no WBP wrapper.
 */
UCLASS(meta = (DisplayName = "Ck Tab Bar"))
class CKUI_API UCk_TabBarWidget_UE : public UWidget
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_TabBarWidget_UE);

public:
    UCk_TabBarWidget_UE();

public:
    // Fired with the newly selected tab index whenever the selection changes.
    UPROPERTY(BlueprintAssignable, Category = "Ck|UI|TabBar|Events")
    FCk_TabBar_TabSelectedEvent OnTabSelected;

public:
    // Links a switcher: tabs follow its children and selection drives its active index.
    // Pass nullptr to unlink.
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|TabBar")
    void
    Set_LinkedSwitcher(
        UWidgetSwitcher* InSwitcher);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|TabBar")
    void
    Set_ActiveTab(
        int32 InIndex);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ck|UI|TabBar")
    int32
    Get_ActiveTabIndex() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ck|UI|TabBar")
    int32
    Get_TabCount() const;

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|TabBar")
    void
    Set_TabConfig(
        int32 InIndex,
        const FCk_TabBar_TabConfig& InTabConfig);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ck|UI|TabBar")
    FCk_TabBar_TabConfig
    Get_TabConfig(
        int32 InIndex) const;

    // Only valid without a linked switcher — when linked, the tab count follows the
    // switcher's children, so add/remove children on the switcher instead.
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|TabBar")
    void
    AddTab(
        const FCk_TabBar_TabConfig& InTabConfig);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|TabBar")
    void
    RemoveTab(
        int32 InIndex);

    // Re-syncs with the linked switcher and rebuilds the tab buttons.
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|TabBar")
    void
    RebuildTabs();

public:
    auto SynchronizeProperties() -> void override;
    auto ReleaseSlateResources(bool bReleaseChildren) -> void override;

#if WITH_EDITOR
    auto GetPaletteCategory() -> const FText override;
#endif

protected:
    auto RebuildWidget() -> TSharedRef<SWidget> override;

protected:
    UPROPERTY(EditAnywhere, Category = "TabBar",
              meta = (AllowPrivateAccess = true, TitleProperty = "_TabName"))
    TArray<FCk_TabBar_TabConfig> _Tabs;

    UPROPERTY(EditAnywhere, Category = "TabBar",
              meta = (AllowPrivateAccess = true))
    TEnumAsByte<EOrientation> _Orientation = EOrientation::Orient_Horizontal;

    UPROPERTY(EditAnywhere, Category = "TabBar",
              meta = (AllowPrivateAccess = true, ClampMin = "0"))
    int32 _ActiveTabIndex = 0;

    UPROPERTY(EditAnywhere, Category = "TabBar",
              meta = (AllowPrivateAccess = true))
    FCk_TabBar_Style _Style;

private:
    auto DoSyncTabsWithSwitcher() -> void;
    auto HandleTabSelected(int32 InTabIndex) -> void;

private:
    UPROPERTY(Transient)
    TObjectPtr<UWidgetSwitcher> _LinkedSwitcher;

    TSharedPtr<SCk_TabBar> _TabBar;
};

// --------------------------------------------------------------------------------------------------------------------
