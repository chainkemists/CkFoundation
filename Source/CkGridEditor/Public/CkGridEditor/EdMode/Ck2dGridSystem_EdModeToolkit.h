#pragma once

#include "CoreMinimal.h"

#include "Toolkits/BaseToolkit.h"

#include "Types/SlateEnums.h"

#include <GameplayTagContainer.h>

// --------------------------------------------------------------------------------------------------------------------

enum class ECk_GridPaint_Tool : uint8;
enum class ECk_GridPaint_TagScope : uint8;

class SGameplayTagPicker;
class SVerticalBox;
class ITableRow;
class STableViewBase;
template <typename ItemType> class SListView;

// --------------------------------------------------------------------------------------------------------------------

// One row in the Select-tab Blockers list.
struct FCk_GridBlockerListItem
{
    int32        Index = INDEX_NONE;
    FGameplayTag Name;
    FIntPoint    RangeMin = FIntPoint::ZeroValue;
    FIntPoint    RangeMax = FIntPoint::ZeroValue;
};

// One row in the Select-tab Tags list (doubles as the clickable legend).
struct FCk_GridTagListItem
{
    FGameplayTag Tag;
    int32        CellCount = 0;
};

// --------------------------------------------------------------------------------------------------------------------

// Toolkit for the Grid Paint editor mode: a 4-tool selector that sets the active tool on the owning
// UCk_2dGridSystem_EdMode, plus the per-tool sections it reveals.
class FCk_2dGridSystem_EdModeToolkit : public FModeToolkit
{
public:
    FCk_2dGridSystem_EdModeToolkit();

    // FModeToolkit interface
    virtual void Init(const TSharedPtr<IToolkitHost>& InToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode) override;
    virtual void GetToolPaletteNames(TArray<FName>& OutPaletteNames) const override;

    // IToolkit interface
    virtual FName GetToolkitFName() const override;
    virtual FText GetBaseToolkitName() const override;
    virtual TSharedPtr<SWidget> GetInlineContent() const override;

private:
    auto Set_ActiveTool(ECk_GridPaint_Tool InTool) -> void;

    // Defaults to Shape if the owning mode is gone.
    auto Get_ActiveTool() const -> ECk_GridPaint_Tool;

    auto Get_ToolCheckState(ECk_GridPaint_Tool InTool) const -> ECheckBoxState;
    auto On_ToolCheckChanged(ECheckBoxState InNewState, ECk_GridPaint_Tool InTool) -> void;

    // The always-visible Grid section: Dimensions and Cell Size numeric entry.
    auto Build_GridSection() -> TSharedRef<SWidget>;

    // Unset when no grid spawner is selected.
    auto Get_GridDimensionX() const -> TOptional<int32>;
    auto Get_GridDimensionY() const -> TOptional<int32>;
    auto Get_GridCellSizeX() const -> TOptional<double>;
    auto Get_GridCellSizeY() const -> TOptional<double>;

    // Each reads the other axis off the Spec, then pushes the combined value.
    auto On_GridDimensionXCommitted(int32 InValue, ETextCommit::Type InCommitType) -> void;
    auto On_GridDimensionYCommitted(int32 InValue, ETextCommit::Type InCommitType) -> void;
    auto On_GridCellSizeXCommitted(double InValue, ETextCommit::Type InCommitType) -> void;
    auto On_GridCellSizeYCommitted(double InValue, ETextCommit::Type InCommitType) -> void;

    // Tag picker + scope toggle + grid-default Apply/Remove; visible only while the Tags tool is active.
    auto Build_TagsSection() -> TSharedRef<SWidget>;
    auto Get_TagsSectionVisibility() const -> EVisibility;

    // SGameplayTagPicker hands back containers; the first tag becomes the EdMode's _ActivePaintTag.
    auto On_PaintTagChanged(const TArray<FGameplayTagContainer>& InContainers) -> void;

    auto Get_ScopeCheckState(ECk_GridPaint_TagScope InScope) const -> ECheckBoxState;
    auto On_ScopeCheckChanged(ECheckBoxState InNewState, ECk_GridPaint_TagScope InScope) -> void;

    // Enabled only in GridDefault scope.
    auto Get_GridDefaultButtonsEnabled() const -> bool;
    auto On_ApplyGridDefaultTag() -> FReply;
    auto On_RemoveGridDefaultTag() -> FReply;

    // Read-only per-tag color legend, rebuilt from the Spec whenever its signature changes.
    auto Rebuild_TagLegend() -> void;
    auto Compute_TagLegendSignature() const -> FString;

    // New-blocker + selected-blocker tag pickers; visible only while the Blocker tool is active.
    auto Build_BlockerSection() -> TSharedRef<SWidget>;
    auto Get_BlockerSectionVisibility() const -> EVisibility;

    // The first tag becomes the EdMode's _ActiveBlockerTag, stamped onto the next drag-rect blocker.
    auto On_NewBlockerTagChanged(const TArray<FGameplayTagContainer>& InContainers) -> void;

    // The first tag is written to the selected blocker via Set_SelectedBlockerName (transacted + rebuild).
    auto On_SelectedBlockerTagChanged(const TArray<FGameplayTagContainer>& InContainers) -> void;

    auto Get_SelectedBlockerEditorVisibility() const -> EVisibility;

    // Also drives a lazy re-seed of the selected-blocker picker when the selected index changes.
    auto Get_SelectedBlockerText() const -> FText;

    // The Select-tool Details panel: two MUTUALLY EXCLUSIVE editors — a single-CELL editor and a BLOCKER
    // editor shown when the pick landed on a blocker — switched by the sub-block visibilities below.
    auto Build_DetailsSection() -> TSharedRef<SWidget>;
    auto Get_DetailsSectionVisibility() const -> EVisibility;
    auto Get_DetailsCellEditorVisibility() const -> EVisibility;
    auto Get_DetailsBlockerEditorVisibility() const -> EVisibility;

    // Live-bound: each reads the EdMode's selected-cell info off the Spec every repaint.
    auto Get_DetailsCoordinateText() const -> FText;
    auto Get_DetailsStateText() const -> FText;
    auto Get_DetailsGridDefaultTagsText() const -> FText;

    auto Get_SelectedCellDisabledState() const -> ECheckBoxState;
    auto On_SelectedCellDisabledChanged(ECheckBoxState InNewState) -> void;

    // One row per tag with a Remove button. Re-driven from the live-bound coordinate getter when the
    // signature changes, so the list tracks live edits (picker add, row remove, external Spec edits).
    auto Rebuild_PerCellTagList() -> void;
    auto Compute_PerCellTagListSignature() const -> FString;

    auto On_AddCellTagChanged(const TArray<FGameplayTagContainer>& InContainers) -> void;

    // Bound per row to the row's tag.
    auto On_RemoveCellTag(FGameplayTag InTag) -> FReply;

    auto Get_DetailsBlockerText() const -> FText;
    auto On_DeleteSelectedBlocker() -> FReply;
    auto On_DetailsBlockerTagChanged(const TArray<FGameplayTagContainer>& InContainers) -> void;

    // Blockers + Tags list items, rebuilt from the Spec whenever their signature changes.
    auto Rebuild_SelectLists() -> void;
    auto Compute_SelectListsSignature() const -> FString;

    auto OnGenerate_BlockerRow(TSharedPtr<FCk_GridBlockerListItem> InItem, const TSharedRef<STableViewBase>& InOwner) -> TSharedRef<ITableRow>;
    auto OnGenerate_TagRow(TSharedPtr<FCk_GridTagListItem> InItem, const TSharedRef<STableViewBase>& InOwner) -> TSharedRef<ITableRow>;

    // Push the chosen blocker/tag onto the EdMode, which highlights it in the viewport.
    auto On_BlockerRowSelected(TSharedPtr<FCk_GridBlockerListItem> InItem, ESelectInfo::Type InSelectInfo) -> void;
    auto On_TagRowSelected(TSharedPtr<FCk_GridTagListItem> InItem, ESelectInfo::Type InSelectInfo) -> void;

private:
    TSharedPtr<SWidget>           InlineContent;
    TWeakObjectPtr<UEdMode>       OwningMode;
    TSharedPtr<SGameplayTagPicker> TagPicker;

    // Tags section: one read-only swatch+name row per distinct per-cell tag.
    TSharedPtr<SVerticalBox> TagLegendContainer;
    FString SeededTagLegendSignature;

    TSharedPtr<SGameplayTagPicker> NewBlockerTagPicker;
    TSharedPtr<SGameplayTagPicker> SelectedBlockerTagPicker;

    // Last index SelectedBlockerTagPicker was seeded for; the live-bound text getter re-seeds on a change.
    int32 SeededSelectedBlockerIndex = INDEX_NONE;

    TSharedPtr<SGameplayTagPicker> AddCellTagPicker;

    // One removable row per tag on the selected cell.
    TSharedPtr<SVerticalBox> PerCellTagListContainer;

    // Selected cell + its tags, as the rows were last built; the live-bound coordinate getter re-drives
    // Rebuild_PerCellTagList on a change.
    FString SeededPerCellTagSignature;

    TSharedPtr<SGameplayTagPicker> DetailsBlockerTagPicker;

    // Mirrors SeededSelectedBlockerIndex, but for the Select-tool Details picker.
    int32 SeededDetailsBlockerIndex = INDEX_NONE;

    TArray<TSharedPtr<FCk_GridBlockerListItem>> BlockerListItems;
    TArray<TSharedPtr<FCk_GridTagListItem>>     TagListItems;
    TSharedPtr<SListView<TSharedPtr<FCk_GridBlockerListItem>>> BlockerListView;
    TSharedPtr<SListView<TSharedPtr<FCk_GridTagListItem>>>     TagListView;
    FString SeededSelectListsSignature;
};

// --------------------------------------------------------------------------------------------------------------------
