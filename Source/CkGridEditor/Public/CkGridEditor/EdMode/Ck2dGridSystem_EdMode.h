#pragma once

#include "CoreMinimal.h"

#include "Tools/LegacyEdModeWidgetHelpers.h"

#include <GameplayTagContainer.h>

#include "Ck2dGridSystem_EdMode.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class ACk_EntitySpawner_UE;
class UCk_2dGridSystem_Spec;

class FCanvas;
class FEditorViewportClient;
class FPrimitiveDrawInterface;
class FSceneView;
class FViewport;
class HHitProxy;
struct FViewportClick;

// --------------------------------------------------------------------------------------------------------------------

// Selectable paint tools in the Grid Paint mode.
UENUM()
enum class ECk_GridPaint_Tool : uint8
{
    // Toggles a cell's membership in the Spec's DisabledCells (paints the grid footprint/shape).
    Shape,
    // Paints per-cell gameplay tags (or edits the grid-wide DefaultCellTags).
    Tags,
    // Drag-rect places blocker footprints; click selects an existing blocker (Delete removes it).
    Blocker,
    // Click selects a cell (or, if the click lands on a blocker, the whole blocker group); the toolkit's
    // Details panel then EDITS that cell (disabled toggle + per-cell tag add/remove) or that blocker
    // (tag edit + delete).
    Select
};

// --------------------------------------------------------------------------------------------------------------------

// Scope the Tags tool writes to: PerCellBulk bulk-paints _ActivePaintTag onto each painted cell's
// PerCellTags; GridDefault edits the Spec's grid-wide DefaultCellTags (via the toolkit Apply/Remove
// buttons rather than viewport painting).
UENUM()
enum class ECk_GridPaint_TagScope : uint8
{
    PerCellBulk,
    GridDefault
};

// --------------------------------------------------------------------------------------------------------------------

// "Grid Paint" editor mode. Renders the AUTHORED state of the selected grid spawner's Spec as a
// wireframe overlay and (in a later task) lets the user paint cell state by clicking the viewport.
//
// Subclasses UBaseLegacyWidgetEdMode rather than UEdMode directly: the legacy-widget base exposes
// overridable Render/HandleClick/InputDelta (via ILegacyEdModeWidgetInterface +
// ILegacyEdModeViewportInterface), which is what a draw/click-based mode needs — plain UEdMode only
// forwards input to its InteractiveTools context.
//
// Registration is AUTO-DISCOVERY: UAssetEditorSubsystem::RegisterEditorModes() iterates every
// non-abstract UEdMode CDO on OnAllModuleLoadingPhasesComplete and registers it by its
// FEditorModeInfo::ID. No explicit RegisterMode() call is required; we only need a stable ID and a
// visible FEditorModeInfo assigned in the constructor.
UCLASS()
class CKGRIDEDITOR_API UCk_2dGridSystem_EdMode : public UBaseLegacyWidgetEdMode
{
    GENERATED_BODY()

public:
    // Stable identifier for this editor mode. Used by the toolbar and by auto-discovery.
    static const FEditorModeID EM_Ck2dGridSystemPaintModeId;

    UCk_2dGridSystem_EdMode();

    // UEdMode interface
    virtual void CreateToolkit() override;

    // ILegacyEdModeWidgetInterface (via UBaseLegacyWidgetEdMode)
    virtual void Render(const FSceneView* InView, FViewport* InViewport, FPrimitiveDrawInterface* InPDI) override;
    virtual void DrawHUD(FEditorViewportClient* InViewportClient, FViewport* InViewport, const FSceneView* InView, FCanvas* InCanvas) override;
    virtual bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale) override;

    // ILegacyEdModeViewportInterface (via UBaseLegacyWidgetEdMode)
    virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* InHitProxy, const FViewportClick& InClick) override;
    virtual bool MouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY) override;
    virtual bool StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport) override;
    virtual bool EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport) override;
    virtual bool CapturedMouseMove(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 InMouseX, int32 InMouseY) override;
    virtual bool DisallowMouseDeltaTracking() const override;
    virtual bool InputKey(FEditorViewportClient* InViewportClient, FViewport* InViewport, FKey InKey, EInputEvent InEvent) override;

    // ILegacyEdModeWidgetInterface (via UBaseLegacyWidgetEdMode) — clears blocker selection on a
    // change of the selected actor set.
    virtual void ActorSelectionChangeNotify() override;

public:
    // Active-tool accessors. The toolkit reads/sets these to drive the palette and what painting does.
    auto Get_ActiveTool() const -> ECk_GridPaint_Tool { return _ActiveTool; }
    // Switching away from the Blocker tool clears any pending blocker selection (best-effort).
    auto Set_ActiveTool(ECk_GridPaint_Tool InTool) -> void;

    // Active paint tag (Tags tool). The toolkit's tag picker writes the chosen tag here.
    auto Get_ActivePaintTag() const -> FGameplayTag { return _ActivePaintTag; }
    auto Set_ActivePaintTag(const FGameplayTag& InTag) -> void { _ActivePaintTag = InTag; }

    // Tag scope (Tags tool): PerCellBulk paints per-cell, GridDefault edits the grid-wide default.
    auto Get_TagScope() const -> ECk_GridPaint_TagScope { return _TagScope; }
    auto Set_TagScope(ECk_GridPaint_TagScope InScope) -> void { _TagScope = InScope; }

    // Active blocker tag (Blocker tool). The toolkit's tag picker writes the chosen tag here; the next
    // drag-rect commit stamps it onto the new blocker's Name. Invalid = anonymous (empty Name).
    auto Get_ActiveBlockerTag() const -> FGameplayTag { return _ActiveBlockerTag; }
    auto Set_ActiveBlockerTag(const FGameplayTag& InTag) -> void { _ActiveBlockerTag = InTag; }

    // Blocker tool: the index of the currently selected blocker (INDEX_NONE when none). Read by the
    // toolkit so it can show/edit the selected blocker's Name tag.
    auto Get_SelectedBlockerIndex() const -> int32 { return _SelectedBlockerIndex; }

    // Blocker tool: the Name tag of the currently selected blocker, resolved live from the selected
    // grid's Spec. Invalid when no blocker is selected, no grid is selected, or the index is stale.
    auto Get_SelectedBlockerName() const -> FGameplayTag;

    // Blocker tool: write InTag to the selected blocker's Name (transacted + rebuild). Bounds-checked
    // against the current Spec->Blockers; no-op if no blocker is selected or no grid is selected.
    auto Set_SelectedBlockerName(const FGameplayTag& InTag) -> void;

    // GridDefault-scope actions invoked from the toolkit: add/remove _ActivePaintTag in the selected
    // grid's DefaultCellTags (transacted + rebuild). No-op if no grid is selected or the tag is invalid.
    auto Apply_GridDefaultTag() -> void;
    auto Remove_GridDefaultTag() -> void;

    // Select tool: the cell currently selected for inspection (unset until the user clicks a cell while
    // the Select tool is active, or after clicking off-grid). Read by the toolkit's Details panel.
    auto Get_SelectedCell() const -> TOptional<FIntPoint> { return _SelectedCell; }

    // Read-only snapshot of a cell's authored state, resolved live from the selected grid's Spec for the
    // toolkit's Details panel. Valid only when bHasSelection is true (a cell is selected AND a grid
    // spawner is selected).
    enum class ECellState : uint8
    {
        Enabled,
        Disabled,
        Blocked
    };

    struct FSelectedCellInfo
    {
        bool                  bHasSelection = false;
        FIntPoint             Coordinate    = FIntPoint::ZeroValue;
        ECellState            State         = ECellState::Enabled;
        // Index into Spec->Blockers when State == Blocked; INDEX_NONE otherwise.
        int32                 BlockerIndex  = INDEX_NONE;
        // The covering blocker's Name tag when State == Blocked (invalid = unnamed/anonymous).
        FGameplayTag          BlockerName;
        FGameplayTagContainer CellTags;
        FGameplayTagContainer GridDefaultTags;
    };

    // Resolves the selected cell's authored state live from the selected grid spawner's Spec. Returns a
    // snapshot with bHasSelection == false when no cell or no grid spawner is selected.
    auto Resolve_SelectedCellInfo() const -> FSelectedCellInfo;

    // Select tool: true when the current Select-tool pick landed on a blocker (so the toolkit shows the
    // BLOCKER editor instead of the single-cell editor). A blocker selection takes precedence over the
    // single-cell selection: _SelectedBlockerIndex is set and _SelectedCell still records the click.
    auto Get_HasBlockerSelection() const -> bool { return _SelectedBlockerIndex != INDEX_NONE; }

    // Select tool: write InDisabled for the currently selected cell — adds the cell to (true) or removes
    // it from (false) the Spec's DisabledCells. Transacted + rebuild. No-op if no cell or no grid is
    // selected. The cell is bounds-clamped to the Spec dimensions implicitly (any FIntPoint is storable).
    auto Set_SelectedCellDisabled(bool InDisabled) -> void;

    // Select tool: returns whether the currently selected cell is in the Spec's DisabledCells. False when
    // no cell or no grid is selected (used to drive the toolkit's Disabled checkbox state).
    auto Get_SelectedCellDisabled() const -> bool;

    // Select tool: add InTag to the currently selected cell's PerCellTags container (FindOrAdd). Transacted
    // + rebuild. No-op if the tag is invalid or no cell/grid is selected.
    auto Add_SelectedCellTag(const FGameplayTag& InTag) -> void;

    // Select tool: remove InTag from the currently selected cell's PerCellTags container; if the container
    // becomes empty, the map entry is dropped. Transacted + rebuild. No-op if no cell/grid is selected.
    auto Remove_SelectedCellTag(const FGameplayTag& InTag) -> void;

    // Select tool: snapshot of the currently selected cell's per-cell tags (empty when no cell/grid is
    // selected). Read by the toolkit to build the editable per-cell tag list.
    auto Get_SelectedCellTags() const -> FGameplayTagContainer;

    // Read-only access to the selected grid's authoring Spec (or nullptr when no grid spawner is selected).
    // Used by the toolkit to build the Blockers/Tags lists without resolving the spawner itself.
    auto Get_SelectedSpec() const -> UCk_2dGridSystem_Spec*;

    // Select tool: set/clear the blocker selected from the toolkit's Blockers list (bounds-checked; clears
    // the tag-group selection). INDEX_NONE clears it.
    auto Set_SelectedBlockerIndex(int32 InIndex) -> void;

    // Select tool: the tag whose cells are highlighted (set from the toolkit's Tags list). Unset = none.
    auto Get_SelectedTag() const -> TOptional<FGameplayTag> { return _SelectedTag; }
    // Setting a valid tag clears the cell/blocker selection so the three Select highlights stay exclusive.
    auto Set_SelectedTag(const TOptional<FGameplayTag>& InTag) -> void;

    // Grid section (toolkit): set the selected grid's Dimensions (clamped to >= 1 on each axis). Transacted
    // + rebuild. No-op if no grid is selected or the value is unchanged.
    auto Set_GridDimensions(FIntPoint InDimensions) -> void;

    // Grid section (toolkit): set the selected grid's CellSize (clamped to a small positive minimum on each
    // axis). Transacted + rebuild. No-op if no grid is selected or the value is unchanged.
    auto Set_GridCellSize(FVector2D InCellSize) -> void;

    // Select tool: range (min/max corners) of the currently selected blocker, written to the out params.
    // Returns false (out params untouched) when no blocker is selected or the index is stale.
    auto Get_SelectedBlockerRange(FIntPoint& OutMin, FIntPoint& OutMax) const -> bool;

    // Select tool: delete the currently selected blocker from the Spec's Blockers (transacted + rebuild)
    // and clear the blocker selection. No-op if no blocker is selected or the index is stale.
    auto Delete_SelectedBlocker() -> void;

private:
    // Resolved grid selection: the selected spawner whose _EntityScript is a grid script, plus the
    // Spec it hosts and the spawner's world transform. Invalid when the current selection is not a
    // grid spawner.
    //
    // Spec is exposed mutably so paint actions can Modify() + mutate it; Render takes the same struct
    // but only reads through it (it never calls a non-const member).
    struct FResolvedGridSelection
    {
        ACk_EntitySpawner_UE*  Spawner       = nullptr;
        UCk_2dGridSystem_Spec* Spec          = nullptr;
        FTransform             GridTransform = FTransform::Identity;

        auto IsValid() const -> bool { return Spawner != nullptr && Spec != nullptr; }
    };

    // Walks the mode's selected actors and returns the first grid spawner selection (or an invalid
    // result if none is selected).
    auto Resolve_SelectedGridSpawner() const -> FResolvedGridSelection;

    // Builds a world-space ray for the given viewport pixel by constructing a transient scene view
    // and reading the cursor ray off it (the canonical engine paint-mode pattern). Returns false if a
    // scene view could not be built.
    auto Compute_CursorRay(
        FEditorViewportClient* InViewportClient,
        int32                  InMouseX,
        int32                  InMouseY,
        FVector&               OutRayOrigin,
        FVector&               OutRayDirection) const -> bool;

    // Shape-tool cell paint: idempotently sets InCell's membership in the selection's DisabledCells.
    // Calls Spec->Modify() only when it actually changes state. Returns true if it changed the Spec.
    // Does NOT rebuild — the caller rebuilds once after a batch. Assumes a caller-owned transaction is open.
    auto Set_ShapeCellDisabled(const FResolvedGridSelection& InSelection, const FIntPoint& InCell, bool InDisabled) -> bool;

    // Tags-tool (PerCellBulk) cell paint: ADDS (InAdd) or REMOVES _ActivePaintTag on InCell's PerCellTags.
    // On remove, drops the map entry if its container becomes empty. Calls Spec->Modify() only on change.
    // Returns true if it changed the Spec; false (no-op) if _ActivePaintTag is invalid or already in state.
    // Does NOT rebuild — the caller rebuilds once. Assumes a caller-owned transaction is open.
    auto Set_TagCell(const FResolvedGridSelection& InSelection, const FIntPoint& InCell, bool InAdd) -> bool;

    // Dispatches the per-cell paint for the active tool (Shape/Tags). ADD when InErase == false. Returns
    // true if the Spec changed. Blocker/Select are not per-cell paints and return false.
    auto Paint_Cell(const FResolvedGridSelection& InSelection, const FIntPoint& InCell, bool InErase) -> bool;

    // Box-select rect fill for the Shape/Tags tools: applies Paint_Cell to every in-bounds cell of the
    // inclusive rect InMin..InMax (corners pre-ordered by the caller), then rebuilds ONCE if anything
    // changed. For the Tags tool, emits a single warning and no-ops if _ActivePaintTag is invalid. Assumes
    // a caller-owned transaction is open (HandleClick for a click; EndTracking for a drag).
    auto Apply_RectFill(const FResolvedGridSelection& InSelection, const FIntPoint& InMin, const FIntPoint& InMax, bool InErase) -> void;

    // Resolves the cell under the given viewport pixel into a grid coordinate (or unset). Shared by the
    // hover/stroke/blocker-drag paths.
    auto Resolve_CellAtCursor(
        FEditorViewportClient* InViewportClient,
        int32                  InMouseX,
        int32                  InMouseY) const -> TOptional<FIntPoint>;

    // Returns the index of the first blocker whose inclusive rect covers InCell, or INDEX_NONE.
    auto Find_BlockerCovering(const FResolvedGridSelection& InSelection, const FIntPoint& InCell) const -> int32;

    // HandleClick branch for the Blocker tool: select the blocker under the click (or clear).
    auto HandleClick_Blocker(
        FEditorViewportClient* InViewportClient,
        HHitProxy*             InHitProxy,
        const FViewportClick&  InClick) -> bool;

    // HandleClick branch for the Select tool: resolve the clicked cell into _SelectedCell (clear on an
    // off-grid click). Read-only — never mutates the Spec.
    auto HandleClick_Select(
        FEditorViewportClient* InViewportClient,
        HHitProxy*             InHitProxy,
        const FViewportClick&  InClick) -> bool;

    // True when InClick is a Left-Mouse click with NO Ctrl/Alt held (Shift IS permitted — it selects the
    // erase direction for the Shape/Tags paint, see Is_EraseModifier). Painting and the Select/Blocker pick
    // engage for this; Ctrl/Alt + LMB and RMB fall through to default camera nav. The Blocker and Select
    // tools additionally require Shift to be UP (they don't honor the erase modifier) — that extra check is
    // applied at their call sites, not here.
    auto Is_PlainLeftClick(const FViewportClick& InClick) const -> bool;

    // True when the current tracking gesture is a Left-Mouse drag with NO Ctrl/Alt held (Shift IS permitted).
    // Read off the viewport (LMB down, RMB up) + the viewport client (no Ctrl/Alt). Painting strokes and the
    // blocker rect begin when this holds, so RMB-look / Alt-orbit / LMB+RMB-pan stay with the camera. The
    // Blocker tool additionally requires Shift up (checked at its StartTracking call site).
    auto Is_PlainLeftDrag(
        FEditorViewportClient* InViewportClient,
        FViewport*             InViewport) const -> bool;

    // True when the erase modifier (Shift) is held for a click. Shift+LMB on the Shape/Tags tools selects
    // the ERASE direction (Shape enables the cell / Tags removes the active tag); plain LMB ADDS.
    auto Is_EraseModifier(const FViewportClick& InClick) const -> bool;

    // Viewport-client variant of Is_EraseModifier for the drag path (Shift read off the client at
    // StartTracking time, captured into _DragErase for the whole drag).
    auto Is_EraseModifier(FEditorViewportClient* InViewportClient) const -> bool;

private:
    // Current paint tool. Default Shape.
    ECk_GridPaint_Tool _ActiveTool = ECk_GridPaint_Tool::Shape;

    // Cell currently under the cursor (for the hover highlight in Render). Unset when no cell is
    // hovered or no grid is selected.
    TOptional<FIntPoint> _HoveredCell;

    // Tags tool: the tag the bulk-paint stroke / GridDefault actions write. Invalid until the toolkit
    // picker sets one.
    FGameplayTag _ActivePaintTag;

    // Tags tool: which collection the tag writes target (per-cell bulk vs grid-wide default).
    ECk_GridPaint_TagScope _TagScope = ECk_GridPaint_TagScope::PerCellBulk;

    // Blocker tool: the tag stamped onto the Name of each NEW blocker created by a drag-rect commit.
    // Invalid until the toolkit picker sets one (invalid = anonymous blocker).
    FGameplayTag _ActiveBlockerTag;

    // All paint tools: the cell where the current rubber-band rect drag began (one rect corner). Set on
    // StartTracking for Shape/Tags/Blocker; unset otherwise.
    TOptional<FIntPoint> _DragStart;

    // All paint tools: the cell currently under the cursor during a drag (the rect's other corner).
    TOptional<FIntPoint> _DragCurrent;

    // Captured at StartTracking from the Shift state for the Shape/Tags drag: true = erase (Shape enables /
    // Tags removes the active tag), false = add. Held for the whole drag. Unused by Blocker (ignores Shift).
    bool _DragErase = false;

    // Blocker AND Select tool: index into Spec->Blockers of the currently selected blocker (for highlight,
    // Delete, and the Select-tool blocker editor), or INDEX_NONE. In the Select tool this is set when the
    // pick lands on a blocker (blocker selection takes precedence over the single-cell selection). Cleared
    // when the tool/selection changes.
    int32 _SelectedBlockerIndex = INDEX_NONE;

    // Select tool: the cell the user picked (highlighted in Render, edited by the toolkit's Details panel).
    // Unset until a cell is clicked, or after an off-grid click / actor-selection change. When the pick
    // landed on a blocker, _SelectedBlockerIndex is also set and the Details panel shows the blocker editor.
    TOptional<FIntPoint> _SelectedCell;

    // Select tool: the tag whose cells are highlighted (driven by the toolkit's Tags list). Mutually
    // exclusive with _SelectedCell / _SelectedBlockerIndex. Cleared on tool change away from Select and on
    // actor-selection change.
    TOptional<FGameplayTag> _SelectedTag;
};

// --------------------------------------------------------------------------------------------------------------------
