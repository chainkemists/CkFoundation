#pragma once

#include "CoreMinimal.h"

#include "Tools/LegacyEdModeWidgetHelpers.h"

#include <GameplayTagContainer.h>

#include "Ck2dGridSystem_EdMode.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class ACk_EntitySpawner_UE;
class UCk_2dGridSystem_AuthoringSpec;

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

// "Grid Paint" editor mode: renders the AUTHORED state of the selected grid spawner's Spec and paints it.
// Subclasses UBaseLegacyWidgetEdMode, not UEdMode, for the overridable Render/HandleClick/InputDelta;
// plain UEdMode only forwards input to its InteractiveTools context. Registration is auto-discovery, so
// all it needs is a stable ID and a visible FEditorModeInfo assigned in the constructor.
UCLASS()
class CKGRIDEDITOR_API UCk_2dGridSystem_EdMode : public UBaseLegacyWidgetEdMode
{
    GENERATED_BODY()

public:
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

    // ILegacyEdModeWidgetInterface — clears the blocker/cell selection when the actor selection changes.
    virtual void ActorSelectionChangeNotify() override;

public:
    auto Get_ActiveTool() const -> ECk_GridPaint_Tool { return _ActiveTool; }
    // Switching tools clears the selections and pending drag that only make sense for the old tool.
    auto Set_ActiveTool(ECk_GridPaint_Tool InTool) -> void;

    auto Get_ActivePaintTag() const -> FGameplayTag { return _ActivePaintTag; }
    auto Set_ActivePaintTag(const FGameplayTag& InTag) -> void { _ActivePaintTag = InTag; }

    auto Get_TagScope() const -> ECk_GridPaint_TagScope { return _TagScope; }
    auto Set_TagScope(ECk_GridPaint_TagScope InScope) -> void { _TagScope = InScope; }

    // Stamped onto the next drag-rect blocker's Name. Invalid = anonymous (empty Name).
    auto Get_ActiveBlockerTag() const -> FGameplayTag { return _ActiveBlockerTag; }
    auto Set_ActiveBlockerTag(const FGameplayTag& InTag) -> void { _ActiveBlockerTag = InTag; }

    // INDEX_NONE when no blocker is selected.
    auto Get_SelectedBlockerIndex() const -> int32 { return _SelectedBlockerIndex; }

    // Resolved live from the Spec; invalid when nothing is selected or the index is stale.
    auto Get_SelectedBlockerName() const -> FGameplayTag;

    // Transacted + rebuild. Bounds-checked against Spec->Blockers; no-op when nothing is selected.
    auto Set_SelectedBlockerName(const FGameplayTag& InTag) -> void;

    // Add/remove _ActivePaintTag in the selected grid's DefaultCellTags (transacted + rebuild). No-op
    // when no grid is selected or the tag is invalid.
    auto Apply_GridDefaultTag() -> void;
    auto Remove_GridDefaultTag() -> void;

    // Unset until a cell is clicked under the Select tool, and again after an off-grid click.
    auto Get_SelectedCell() const -> TOptional<FIntPoint> { return _SelectedCell; }

    // Read-only snapshot of a cell's authored state, resolved live from the Spec for the Details panel.
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
        // Only meaningful when State == Blocked; INDEX_NONE / invalid tag otherwise.
        int32                 BlockerIndex  = INDEX_NONE;
        FGameplayTag          BlockerName;
        FGameplayTagContainer CellTags;
        FGameplayTagContainer GridDefaultTags;
    };

    // bHasSelection == false when no cell or no grid spawner is selected.
    auto Resolve_SelectedCellInfo() const -> FSelectedCellInfo;

    // A blocker selection takes precedence over the single-cell one: _SelectedBlockerIndex is set and
    // _SelectedCell still records the click, and the toolkit shows the BLOCKER editor.
    auto Get_HasBlockerSelection() const -> bool { return _SelectedBlockerIndex != INDEX_NONE; }

    // Every mutator below is transacted + rebuild, and no-ops when no cell / blocker / grid is selected.
    auto Set_SelectedCellDisabled(bool InDisabled) -> void;
    auto Get_SelectedCellDisabled() const -> bool;
    auto Add_SelectedCellTag(const FGameplayTag& InTag) -> void;
    // Drops the map entry entirely if the cell's container becomes empty.
    auto Remove_SelectedCellTag(const FGameplayTag& InTag) -> void;
    auto Get_SelectedCellTags() const -> FGameplayTagContainer;

    // nullptr when no grid spawner is selected.
    auto Get_SelectedSpec() const -> UCk_2dGridSystem_AuthoringSpec*;

    // Bounds-checked; clears the tag-group selection. INDEX_NONE clears the blocker selection.
    auto Set_SelectedBlockerIndex(int32 InIndex) -> void;

    auto Get_SelectedTag() const -> TOptional<FGameplayTag> { return _SelectedTag; }
    // Setting a valid tag clears the cell/blocker selection so the three Select highlights stay exclusive.
    auto Set_SelectedTag(const TOptional<FGameplayTag>& InTag) -> void;

    // Clamped to >= 1 / a small positive minimum per axis. No-op when the value is unchanged.
    auto Set_GridDimensions(FIntPoint InDimensions) -> void;
    auto Set_GridCellSize(FVector2D InCellSize) -> void;

    // Returns false, out params UNTOUCHED, when no blocker is selected or the index is stale.
    auto Get_SelectedBlockerRange(FIntPoint& OutMin, FIntPoint& OutMax) const -> bool;

    auto Delete_SelectedBlocker() -> void;

private:
    // Spec is exposed mutably so paint actions can Modify() + mutate it; Render only reads through it.
    struct FResolvedGridSelection
    {
        ACk_EntitySpawner_UE*  Spawner       = nullptr;
        UCk_2dGridSystem_AuthoringSpec* Spec          = nullptr;
        FTransform             GridTransform = FTransform::Identity;

        auto IsValid() const -> bool { return Spawner != nullptr && Spec != nullptr; }
    };

    // The FIRST grid spawner among the selected actors, or an invalid result.
    auto Resolve_SelectedGridSpawner() const -> FResolvedGridSelection;

    // False when a transient scene view could not be built for the viewport.
    auto Compute_CursorRay(
        FEditorViewportClient* InViewportClient,
        int32                  InMouseX,
        int32                  InMouseY,
        FVector&               OutRayOrigin,
        FVector&               OutRayDirection) const -> bool;

    // Both per-cell paints are idempotent (Spec->Modify() only on a real change), return true when they
    // changed the Spec, do NOT rebuild — the caller rebuilds once per batch — and assume a caller-owned
    // transaction is open. Set_TagCell drops a cell's map entry when its container becomes empty.
    auto Set_ShapeCellDisabled(const FResolvedGridSelection& InSelection, const FIntPoint& InCell, bool InDisabled) -> bool;
    auto Set_TagCell(const FResolvedGridSelection& InSelection, const FIntPoint& InCell, bool InAdd) -> bool;

    // Dispatches the per-cell paint for the active tool; Blocker/Select are not per-cell paints -> false.
    auto Paint_Cell(const FResolvedGridSelection& InSelection, const FIntPoint& InCell, bool InErase) -> bool;

    // Paints every in-bounds cell of the inclusive rect (corners pre-ordered by the caller), then rebuilds
    // ONCE. Assumes a caller-owned transaction is open (HandleClick for a click, EndTracking for a drag).
    auto Apply_RectFill(const FResolvedGridSelection& InSelection, const FIntPoint& InMin, const FIntPoint& InMax, bool InErase) -> void;

    auto Resolve_CellAtCursor(
        FEditorViewportClient* InViewportClient,
        int32                  InMouseX,
        int32                  InMouseY) const -> TOptional<FIntPoint>;

    // The FIRST blocker whose inclusive rect covers InCell, or INDEX_NONE.
    auto Find_BlockerCovering(const FResolvedGridSelection& InSelection, const FIntPoint& InCell) const -> int32;

    auto HandleClick_Blocker(
        FEditorViewportClient* InViewportClient,
        HHitProxy*             InHitProxy,
        const FViewportClick&  InClick) -> bool;

    // Read-only — never mutates the Spec.
    auto HandleClick_Select(
        FEditorViewportClient* InViewportClient,
        HHitProxy*             InHitProxy,
        const FViewportClick&  InClick) -> bool;

    // Both engage painting and the Select/Blocker pick. Shift IS permitted (it is the erase modifier), so
    // the Blocker and Select tools — which do NOT honor erase — re-reject it at their own call sites.
    // Ctrl/Alt + LMB, RMB, and LMB+RMB stay with camera nav (look, Alt-orbit, pan).
    auto Is_PlainLeftClick(const FViewportClick& InClick) const -> bool;
    auto Is_PlainLeftDrag(
        FEditorViewportClient* InViewportClient,
        FViewport*             InViewport) const -> bool;

    // Shift is ERASE for Shape/Tags (Shape enables the cell, Tags removes the active tag); plain LMB ADDS.
    // The viewport-client variant serves the drag path, captured into _DragErase at StartTracking.
    auto Is_EraseModifier(const FViewportClick& InClick) const -> bool;
    auto Is_EraseModifier(FEditorViewportClient* InViewportClient) const -> bool;

private:
    ECk_GridPaint_Tool _ActiveTool = ECk_GridPaint_Tool::Shape;

    // Unset when no cell is hovered or no grid is selected.
    TOptional<FIntPoint> _HoveredCell;

    // Tags tool: what the bulk-paint stroke / GridDefault actions write. Invalid until the picker sets one.
    FGameplayTag _ActivePaintTag;

    ECk_GridPaint_TagScope _TagScope = ECk_GridPaint_TagScope::PerCellBulk;

    // Blocker tool: stamped onto each NEW blocker's Name. Invalid = anonymous blocker.
    FGameplayTag _ActiveBlockerTag;

    // The two corners of the in-flight rubber-band rect. Set on StartTracking for Shape/Tags/Blocker.
    TOptional<FIntPoint> _DragStart;
    TOptional<FIntPoint> _DragCurrent;

    // Captured at StartTracking and held for the whole drag, so releasing Shift mid-drag cannot flip the
    // direction. Unused by Blocker, which ignores Shift.
    bool _DragErase = false;

    // Index into Spec->Blockers, or INDEX_NONE. Shared by the Blocker tool and the Select tool, where a
    // pick that lands on a blocker takes precedence over the single-cell selection.
    int32 _SelectedBlockerIndex = INDEX_NONE;

    // Select tool: the picked cell. Set even when the pick also resolved to a blocker.
    TOptional<FIntPoint> _SelectedCell;

    // Select tool: mutually exclusive with _SelectedCell / _SelectedBlockerIndex.
    TOptional<FGameplayTag> _SelectedTag;
};

// --------------------------------------------------------------------------------------------------------------------
