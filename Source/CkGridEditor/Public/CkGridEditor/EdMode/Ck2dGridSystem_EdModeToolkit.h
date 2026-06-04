#pragma once

#include "CoreMinimal.h"

#include "Toolkits/BaseToolkit.h"

#include <GameplayTagContainer.h>

// --------------------------------------------------------------------------------------------------------------------

enum class ECk_GridPaint_Tool : uint8;
enum class ECk_GridPaint_TagScope : uint8;

class SGameplayTagPicker;

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

    // Builds the Select-tool Details widget (read-only cell inspector). Its visibility is bound to "is
    // the Select tool active"; the value text blocks read the EdMode + Spec live each frame.
    auto Build_DetailsSection() -> TSharedRef<SWidget>;

    // Visible only while the Select tool is the active tool.
    auto Get_DetailsSectionVisibility() const -> EVisibility;

    // Live-bound text for the Details panel, each reading the EdMode's selected-cell info off the Spec.
    auto Get_DetailsCoordinateText() const -> FText;
    auto Get_DetailsStateText() const -> FText;
    auto Get_DetailsCellTagsText() const -> FText;
    auto Get_DetailsGridDefaultTagsText() const -> FText;

private:
    TSharedPtr<SWidget>           InlineContent;
    TWeakObjectPtr<UEdMode>       OwningMode;
    TSharedPtr<SGameplayTagPicker> TagPicker;

    // Blocker tool: picker for the tag stamped onto NEW blockers (writes EdMode::_ActiveBlockerTag).
    TSharedPtr<SGameplayTagPicker> NewBlockerTagPicker;

    // Blocker tool: picker editing the SELECTED blocker's Name tag (writes via Set_SelectedBlockerName).
    // Re-seeded imperatively whenever the selected blocker index changes (see Get_SelectedBlockerText).
    TSharedPtr<SGameplayTagPicker> SelectedBlockerTagPicker;

    // Last selected-blocker index the SelectedBlockerTagPicker was seeded for. Lets the live-bound text
    // getter detect a selection change and re-seed the picker's displayed value to match the new blocker.
    int32 SeededSelectedBlockerIndex = INDEX_NONE;
};

// --------------------------------------------------------------------------------------------------------------------
