#pragma once

#include "CoreMinimal.h"

#include "Tools/LegacyEdModeWidgetHelpers.h"

#include <GameplayTagContainer.h>

#include "Ck2dGridSystem_EdMode.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class ACk_EntitySpawner_UE;
class UCk_2dGridSystem_Spec;

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
    // Read-only: click selects a cell and the toolkit's Details panel inspects its authored state.
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
        FGameplayTagContainer CellTags;
        FGameplayTagContainer GridDefaultTags;
    };

    // Resolves the selected cell's authored state live from the selected grid spawner's Spec. Returns a
    // snapshot with bHasSelection == false when no cell or no grid spawner is selected.
    auto Resolve_SelectedCellInfo() const -> FSelectedCellInfo;

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

    // Shape-tool cell toggle: flips InCell's membership in the selection's DisabledCells, brackets the
    // mutation in Spec->Modify(), and rebuilds the live preview. Assumes a caller-owned transaction is
    // already open (single-click opens an FScopedTransaction; the drag stroke opens a GEditor txn).
    auto Toggle_ShapeCell(const FResolvedGridSelection& InSelection, const FIntPoint& InCell) -> void;

    // Tags-tool (PerCellBulk) cell paint: idempotently ADDS _ActivePaintTag to InCell's PerCellTags
    // (never toggles — bulk-paint only adds). Brackets in Spec->Modify() + rebuild. Assumes a
    // caller-owned transaction is open. No-op if _ActivePaintTag is invalid.
    auto Paint_TagCell(const FResolvedGridSelection& InSelection, const FIntPoint& InCell) -> void;

    // Dispatches the per-cell stroke action for the active tool (Shape toggles, Tags paints). Deduped
    // by the caller via _StrokeToggledCells. Blocker is NOT a per-cell stroke (it is a drag-rect).
    auto Paint_StrokeCell(const FResolvedGridSelection& InSelection, const FIntPoint& InCell) -> void;

    // Resolves the cell under the given viewport pixel during an active drag stroke and applies the
    // active tool's per-cell action, deduping via _StrokeToggledCells so jitter inside one cell can't
    // re-apply repeatedly.
    auto Paint_StrokeAtCursor(
        FEditorViewportClient* InViewportClient,
        int32                  InMouseX,
        int32                  InMouseY) -> void;

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

    // True when InClick is a plain Left-Mouse click with NO Shift/Ctrl/Alt modifier held. Painting (and
    // the Select pick) only engages for plain-LMB; everything else falls through to default camera nav.
    auto Is_PlainLeftClick(const FViewportClick& InClick) const -> bool;

    // True when the current tracking gesture is a plain Left-Mouse drag with NO modifier held. Read off
    // the viewport (LMB down, RMB up) + the viewport client (no Shift/Ctrl/Alt). Painting strokes and the
    // blocker rect only begin when this holds, so RMB-look / Alt-orbit / LMB+RMB-pan stay with the camera.
    auto Is_PlainLeftDrag(
        FEditorViewportClient* InViewportClient,
        FViewport*             InViewport) const -> bool;

private:
    // Current paint tool. Default Shape (the only functional tool this task).
    ECk_GridPaint_Tool _ActiveTool = ECk_GridPaint_Tool::Shape;

    // Cell currently under the cursor (for the hover highlight in Render). Unset when no cell is
    // hovered or no grid is selected.
    TOptional<FIntPoint> _HoveredCell;

    // True between StartTracking/EndTracking when a per-cell drag stroke (Shape or Tags) is active.
    // While set, the mode disables the gizmo delta-tracker so the drag paints instead of dragging an
    // actor.
    bool _IsPaintingStroke = false;

    // Cells already painted during the current stroke (so re-entering a cell on jitter is a no-op).
    TSet<FIntPoint> _StrokeToggledCells;

    // Tags tool: the tag the bulk-paint stroke / GridDefault actions write. Invalid until the toolkit
    // picker sets one.
    FGameplayTag _ActivePaintTag;

    // Tags tool: which collection the tag writes target (per-cell bulk vs grid-wide default).
    ECk_GridPaint_TagScope _TagScope = ECk_GridPaint_TagScope::PerCellBulk;

    // Blocker tool: the cell where the current rubber-band rect drag began. Set on StartTracking while
    // tool == Blocker; unset otherwise.
    TOptional<FIntPoint> _BlockerDragStart;

    // Blocker tool: the cell currently under the cursor during a blocker drag (the rect's other corner).
    TOptional<FIntPoint> _BlockerDragCurrent;

    // Blocker tool: index into Spec->Blockers of the currently selected blocker (for highlight + Delete),
    // or INDEX_NONE. Cleared when the tool/selection changes.
    int32 _SelectedBlockerIndex = INDEX_NONE;

    // Select tool: the cell the user picked for inspection (highlighted in Render, read by the toolkit's
    // Details panel). Unset until a cell is clicked, or after an off-grid click / actor-selection change.
    TOptional<FIntPoint> _SelectedCell;
};

// --------------------------------------------------------------------------------------------------------------------
