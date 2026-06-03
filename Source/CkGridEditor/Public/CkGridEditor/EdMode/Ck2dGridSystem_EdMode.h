#pragma once

#include "CoreMinimal.h"

#include "Tools/LegacyEdModeWidgetHelpers.h"

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

// Selectable paint tools in the Grid Paint mode. Only Shape is functional this task; Tags/Blocker
// are selectable placeholders that future tasks fill in.
UENUM()
enum class ECk_GridPaint_Tool : uint8
{
    // Toggles a cell's membership in the Spec's DisabledCells (paints the grid footprint/shape).
    Shape,
    // (Next task) paints per-cell gameplay tags.
    Tags,
    // (Next task) paints blocker footprints.
    Blocker
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

public:
    // Active-tool accessors. The toolkit reads/sets these to drive the palette and what painting does.
    auto Get_ActiveTool() const -> ECk_GridPaint_Tool { return _ActiveTool; }
    auto Set_ActiveTool(ECk_GridPaint_Tool InTool) -> void { _ActiveTool = InTool; }

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

    // Paints the cell under the given viewport pixel during an active Shape drag stroke, deduping via
    // _StrokeToggledCells so jitter inside one cell can't flip it on/off repeatedly.
    auto Paint_ShapeStrokeAtCursor(
        FEditorViewportClient* InViewportClient,
        int32                  InMouseX,
        int32                  InMouseY) -> void;

private:
    // Current paint tool. Default Shape (the only functional tool this task).
    ECk_GridPaint_Tool _ActiveTool = ECk_GridPaint_Tool::Shape;

    // Cell currently under the cursor (for the hover highlight in Render). Unset when no cell is
    // hovered or no grid is selected.
    TOptional<FIntPoint> _HoveredCell;

    // True between StartTracking/EndTracking when a Shape drag stroke is active. While set, the mode
    // disables the gizmo delta-tracker so the drag paints instead of dragging an actor.
    bool _IsPaintingStroke = false;

    // Cells already toggled during the current stroke (so re-entering a cell on jitter is a no-op).
    TSet<FIntPoint> _StrokeToggledCells;
};

// --------------------------------------------------------------------------------------------------------------------
