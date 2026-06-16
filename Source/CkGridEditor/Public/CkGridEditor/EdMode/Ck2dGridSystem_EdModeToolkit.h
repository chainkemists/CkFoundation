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

// Toolkit for the Grid Paint editor mode. Hosts a 4-tool selector (Shape / Tags / Blocker / Select)
// that sets the active tool on the owning UCk_2dGridSystem_EdMode. The Tags tool reveals a tag section
// and the Select tool a read-only cell-details section.
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
    // Pushes the chosen tool onto the owning EdMode.
    auto Set_ActiveTool(ECk_GridPaint_Tool InTool) -> void;

    // Reads the owning EdMode's current tool (defaults to Shape if the mode is gone).
    auto Get_ActiveTool() const -> ECk_GridPaint_Tool;

    // Radio-button check state for one tool button.
    auto Get_ToolCheckState(ECk_GridPaint_Tool InTool) const -> ECheckBoxState;
    auto On_ToolCheckChanged(ECheckBoxState InNewState, ECk_GridPaint_Tool InTool) -> void;

    // Builds the Tags-section widget (tag picker + scope toggle + grid-default Apply/Remove). The
    // whole section's visibility is bound to "is the Tags tool active".
    auto Build_TagsSection() -> TSharedRef<SWidget>;

    // Visible only while the Tags tool is the active tool.
    auto Get_TagsSectionVisibility() const -> EVisibility;

    // Tag picker callback: SGameplayTagPicker hands back containers; we take the first tag and push it
    // onto the EdMode's _ActivePaintTag.
    auto On_PaintTagChanged(const TArray<FGameplayTagContainer>& InContainers) -> void;

    // Scope toggle (PerCellBulk vs GridDefault).
    auto Get_ScopeCheckState(ECk_GridPaint_TagScope InScope) const -> ECheckBoxState;
    auto On_ScopeCheckChanged(ECheckBoxState InNewState, ECk_GridPaint_TagScope InScope) -> void;

    // Grid-default Apply/Remove buttons: enabled only in GridDefault scope.
    auto Get_GridDefaultButtonsEnabled() const -> bool;
    auto On_ApplyGridDefaultTag() -> FReply;
    auto On_RemoveGridDefaultTag() -> FReply;

    // Builds a compact, read-only per-tag color legend (swatch + name) shown inside the Tags section so the
    // painter can see which color each existing per-cell tag maps to. Rebuilt each repaint from the Spec.
    auto Rebuild_TagLegend() -> void;

    // A cheap signature of the current per-cell tag set, used to detect when the legend needs a rebuild.
    auto Compute_TagLegendSignature() const -> FString;

    // Builds the Blocker-section widget (new-blocker tag picker + selected-blocker tag editor). The
    // whole section's visibility is bound to "is the Blocker tool active".
    auto Build_BlockerSection() -> TSharedRef<SWidget>;

    // Visible only while the Blocker tool is the active tool.
    auto Get_BlockerSectionVisibility() const -> EVisibility;

    // New-blocker tag picker callback: take the first tag and push it onto the EdMode's
    // _ActiveBlockerTag (stamped onto the next drag-rect blocker).
    auto On_NewBlockerTagChanged(const TArray<FGameplayTagContainer>& InContainers) -> void;

    // Selected-blocker tag picker callback: take the first tag and write it to the selected blocker via
    // the EdMode's Set_SelectedBlockerName (transacted + rebuild).
    auto On_SelectedBlockerTagChanged(const TArray<FGameplayTagContainer>& InContainers) -> void;

    // Visible only when a blocker is currently selected (gates the selected-blocker editor sub-block).
    auto Get_SelectedBlockerEditorVisibility() const -> EVisibility;

    // Live-bound text describing which blocker is selected (index + tag), or "none selected". Also
    // drives a lazy re-seed of the selected-blocker picker when the selected index changes.
    auto Get_SelectedBlockerText() const -> FText;

    // Builds the Select-tool Details widget. Its visibility is bound to "is the Select tool active". The
    // panel hosts TWO mutually-exclusive editors (sub-block visibilities below switch between them): a
    // single-CELL editor (read-only state line + grid-default tags, Disabled toggle, per-cell tag list +
    // add picker) and a BLOCKER editor (index/range/tag text + tag picker + Delete) shown when the pick
    // landed on a blocker. Value text blocks read the EdMode + Spec live each frame.
    auto Build_DetailsSection() -> TSharedRef<SWidget>;

    // Visible only while the Select tool is the active tool.
    auto Get_DetailsSectionVisibility() const -> EVisibility;

    // Single-cell editor sub-block: visible while the Select tool is active AND the pick did NOT land on a
    // blocker (otherwise the blocker editor takes over).
    auto Get_DetailsCellEditorVisibility() const -> EVisibility;

    // Blocker editor sub-block: visible while the Select tool is active AND the pick landed on a blocker.
    auto Get_DetailsBlockerEditorVisibility() const -> EVisibility;

    // Live-bound text for the single-cell editor, each reading the EdMode's selected-cell info off the Spec.
    auto Get_DetailsCoordinateText() const -> FText;
    auto Get_DetailsStateText() const -> FText;
    auto Get_DetailsGridDefaultTagsText() const -> FText;

    // Disabled toggle for the selected cell: reflects/sets the cell's membership in Spec->DisabledCells.
    auto Get_SelectedCellDisabledState() const -> ECheckBoxState;
    auto On_SelectedCellDisabledChanged(ECheckBoxState InNewState) -> void;

    // Rebuilds the per-cell tag list (one row per tag with a Remove button) into PerCellTagListContainer.
    // Called from Init and re-driven each frame from Get_SelectedCellTagsSignature when the cell's tag set
    // changes, so the list tracks live edits (add via picker, remove via button, or external Spec edits).
    auto Rebuild_PerCellTagList() -> void;

    // A cheap hash of (selected cell + its per-cell tags) used to detect when the per-cell tag list needs
    // a rebuild. Called by the live-bound coordinate text getter so the rebuild rides the normal repaint.
    auto Compute_PerCellTagListSignature() const -> FString;

    // Add-picker callback: take the first chosen tag and add it to the selected cell via Add_SelectedCellTag.
    auto On_AddCellTagChanged(const TArray<FGameplayTagContainer>& InContainers) -> void;

    // Remove one tag from the selected cell (bound per row to the row's tag).
    auto On_RemoveCellTag(FGameplayTag InTag) -> FReply;

    // Live-bound text for the blocker editor (index + range + tag), and the Delete button handler.
    auto Get_DetailsBlockerText() const -> FText;
    auto On_DeleteSelectedBlocker() -> FReply;

    // Blocker editor tag picker callback: write the chosen tag to the selected blocker's Name (reuses the
    // EdMode's Set_SelectedBlockerName, shared with the Blocker tool).
    auto On_DetailsBlockerTagChanged(const TArray<FGameplayTagContainer>& InContainers) -> void;

    // Rebuilds the Blockers + Tags list item arrays from the selected grid's Spec, then refreshes both views.
    auto Rebuild_SelectLists() -> void;

    // A signature of (blocker count + per-cell tag set) used to detect when the lists need rebuilding.
    auto Compute_SelectListsSignature() const -> FString;

    // SListView row generators.
    auto OnGenerate_BlockerRow(TSharedPtr<FCk_GridBlockerListItem> InItem, const TSharedRef<STableViewBase>& InOwner) -> TSharedRef<ITableRow>;
    auto OnGenerate_TagRow(TSharedPtr<FCk_GridTagListItem> InItem, const TSharedRef<STableViewBase>& InOwner) -> TSharedRef<ITableRow>;

    // Selection callbacks: push the chosen blocker/tag onto the EdMode (highlights it in the viewport).
    auto On_BlockerRowSelected(TSharedPtr<FCk_GridBlockerListItem> InItem, ESelectInfo::Type InSelectInfo) -> void;
    auto On_TagRowSelected(TSharedPtr<FCk_GridTagListItem> InItem, ESelectInfo::Type InSelectInfo) -> void;

private:
    TSharedPtr<SWidget>           InlineContent;
    TWeakObjectPtr<UEdMode>       OwningMode;
    TSharedPtr<SGameplayTagPicker> TagPicker;

    // Tags section: container holding one read-only swatch+name row per distinct per-cell tag.
    TSharedPtr<SVerticalBox> TagLegendContainer;
    FString SeededTagLegendSignature;

    // Blocker tool: picker for the tag stamped onto NEW blockers (writes EdMode::_ActiveBlockerTag).
    TSharedPtr<SGameplayTagPicker> NewBlockerTagPicker;

    // Blocker tool: picker editing the SELECTED blocker's Name tag (writes via Set_SelectedBlockerName).
    // Re-seeded imperatively whenever the selected blocker index changes (see Get_SelectedBlockerText).
    TSharedPtr<SGameplayTagPicker> SelectedBlockerTagPicker;

    // Last selected-blocker index the SelectedBlockerTagPicker was seeded for. Lets the live-bound text
    // getter detect a selection change and re-seed the picker's displayed value to match the new blocker.
    int32 SeededSelectedBlockerIndex = INDEX_NONE;

    // Select-tool single-cell editor: picker whose chosen tag is ADDED to the selected cell's PerCellTags.
    TSharedPtr<SGameplayTagPicker> AddCellTagPicker;

    // Select-tool single-cell editor: container holding one removable row per tag on the selected cell.
    // Rebuilt imperatively (Rebuild_PerCellTagList) whenever the cell or its tag set changes.
    TSharedPtr<SVerticalBox> PerCellTagListContainer;

    // Signature of the per-cell tag list the rows were last built for (selected cell + its tags). Lets the
    // live-bound coordinate getter detect a change and re-drive Rebuild_PerCellTagList.
    FString SeededPerCellTagSignature;

    // Select-tool blocker editor: picker editing the SELECTED blocker's Name (reuses Set_SelectedBlockerName).
    TSharedPtr<SGameplayTagPicker> DetailsBlockerTagPicker;

    // Last blocker index the DetailsBlockerTagPicker was seeded for (re-seed detection, mirrors the Blocker
    // tool's SeededSelectedBlockerIndex but specific to the Select-tool Details picker).
    int32 SeededDetailsBlockerIndex = INDEX_NONE;

    // Select-tab lists. Item arrays are rebuilt from the Spec via Rebuild_SelectLists.
    TArray<TSharedPtr<FCk_GridBlockerListItem>> BlockerListItems;
    TArray<TSharedPtr<FCk_GridTagListItem>>     TagListItems;
    TSharedPtr<SListView<TSharedPtr<FCk_GridBlockerListItem>>> BlockerListView;
    TSharedPtr<SListView<TSharedPtr<FCk_GridTagListItem>>>     TagListView;
    FString SeededSelectListsSignature;
};

// --------------------------------------------------------------------------------------------------------------------
