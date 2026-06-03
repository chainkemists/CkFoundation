#include "CkGridEditor/EdMode/Ck2dGridSystem_EdMode.h"

#include "CkGridEditor_Log.h"

#include "CkGridEditor/EdMode/Ck2dGridSystem_EdModeToolkit.h"
#include "CkGridEditor/EdMode/Ck2dGridSystem_EdMode_Hit.h"

#include "CkGrid/2dGridSystem/Authoring/Ck2dGridSystem_EntityScript.h"
#include "CkGrid/2dGridSystem/Authoring/Ck2dGridSystem_Spec.h"

#include "CkEntitySpawner/CkEntitySpawner_Actor.h"

#include "Editor.h"
#include "EditorModeManager.h"
#include "EditorViewportClient.h"
#include "Engine/Selection.h"
#include "SceneManagement.h"
#include "SceneView.h"
#include "ScopedTransaction.h"
#include "Textures/SlateIcon.h"

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "Ck_2dGridSystem_EdMode"

// --------------------------------------------------------------------------------------------------------------------

const FEditorModeID UCk_2dGridSystem_EdMode::EM_Ck2dGridSystemPaintModeId = TEXT("Ck.2dGridSystem.PaintMode");

// --------------------------------------------------------------------------------------------------------------------

namespace ck_grid_editor_detail
{
    // Authored-state color convention for the cell overlay. Priority is disabled > blocker > tagged >
    // enabled (a cell that is both disabled and inside a blocker reads as disabled).
    constexpr auto ColorEnabled  = FLinearColor(0.0f, 1.0f, 0.0f); // green
    constexpr auto ColorDisabled = FLinearColor(1.0f, 0.0f, 0.0f); // red
    constexpr auto ColorBlocker  = FLinearColor(1.0f, 0.5f, 0.0f); // orange
    constexpr auto ColorTagged   = FLinearColor(0.2f, 0.4f, 1.0f); // blue tint
    constexpr auto ColorPivot    = FLinearColor(1.0f, 1.0f, 0.0f); // yellow
    constexpr auto ColorHover    = FLinearColor(1.0f, 1.0f, 1.0f); // white

    constexpr auto CellLineThickness  = 1.0f;
    constexpr auto PivotMarkerSize     = 20.0f;
    constexpr auto PivotLineThickness  = 3.0f;

    // Per-state marker: an inset square drawn inside a non-enabled cell (not coincident with the
    // green base-grid lines, so it can't be z-fought away).
    constexpr auto CellMarkerInset     = 0.12;
    constexpr auto CellMarkerThickness = 2.0f;

    // Hover highlight: a slightly smaller inset than the state markers (so it nests inside any state
    // marker on the same cell) drawn thick and white.
    constexpr auto HoverMarkerInset     = 0.06;
    constexpr auto HoverMarkerThickness = 3.0f;

    // Returns true if InCoordinate is covered by any blocker's inclusive [RangeMin, RangeMax] rect.
    auto Is_CoveredByBlocker(
        const UCk_2dGridSystem_Spec* InSpec,
        const FIntPoint&             InCoordinate) -> bool
    {
        for (const auto& Blocker : InSpec->Blockers)
        {
            const auto MinX = FMath::Min(Blocker.RangeMin.X, Blocker.RangeMax.X);
            const auto MaxX = FMath::Max(Blocker.RangeMin.X, Blocker.RangeMax.X);
            const auto MinY = FMath::Min(Blocker.RangeMin.Y, Blocker.RangeMax.Y);
            const auto MaxY = FMath::Max(Blocker.RangeMin.Y, Blocker.RangeMax.Y);

            if (InCoordinate.X >= MinX && InCoordinate.X <= MaxX &&
                InCoordinate.Y >= MinY && InCoordinate.Y <= MaxY)
            { return true; }
        }
        return false;
    }

    // Returns true if InCoordinate carries any authored tag (grid-wide default or per-cell override).
    auto Has_AuthoredTag(
        const UCk_2dGridSystem_Spec* InSpec,
        const FIntPoint&             InCoordinate) -> bool
    {
        if (! InSpec->DefaultCellTags.IsEmpty())
        { return true; }

        if (const auto* PerCell = InSpec->PerCellTags.Find(InCoordinate))
        { return ! PerCell->IsEmpty(); }

        return false;
    }

    // Authored color for a single cell, applying the priority order.
    auto Resolve_CellColor(
        const UCk_2dGridSystem_Spec* InSpec,
        const FIntPoint&             InCoordinate) -> FLinearColor
    {
        if (InSpec->DisabledCells.Contains(InCoordinate))
        { return ColorDisabled; }

        if (Is_CoveredByBlocker(InSpec, InCoordinate))
        { return ColorBlocker; }

        if (Has_AuthoredTag(InSpec, InCoordinate))
        { return ColorTagged; }

        return ColorEnabled;
    }
}

// --------------------------------------------------------------------------------------------------------------------

UCk_2dGridSystem_EdMode::UCk_2dGridSystem_EdMode()
{
    Info = FEditorModeInfo(
        EM_Ck2dGridSystemPaintModeId,
        LOCTEXT("Ck2dGridSystemPaintMode", "Grid Paint"),
        FSlateIcon(),
        /*bVisibleInUI*/ true);
}

void
    UCk_2dGridSystem_EdMode::
    CreateToolkit()
{
    Toolkit = MakeShared<FCk_2dGridSystem_EdModeToolkit>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_2dGridSystem_EdMode::
    Resolve_SelectedGridSpawner() const -> FResolvedGridSelection
{
    auto Result = FResolvedGridSelection{};

    const auto* ModeManager = GetModeManager();
    if (ModeManager == nullptr)
    { return Result; }

    const auto* Selection = ModeManager->GetSelectedActors();
    if (Selection == nullptr)
    { return Result; }

    for (auto Index = 0; Index < Selection->Num(); ++Index)
    {
        auto* Spawner = Cast<ACk_EntitySpawner_UE>(Selection->GetSelectedObject(Index));
        if (Spawner == nullptr)
        { continue; }

        const auto& EntityScript = Spawner->Get_EntityScript();
        auto* GridScript = Cast<UCk_2dGridSystem_EntityScript>(EntityScript);
        if (GridScript == nullptr)
        { continue; }

        // The Spec is a private UPROPERTY on the grid script; read it reflectively (no public getter).
        const auto* SpecProperty = FindFProperty<FObjectProperty>(
            UCk_2dGridSystem_EntityScript::StaticClass(), TEXT("Spec"));
        if (SpecProperty == nullptr)
        { continue; }

        auto* Spec = Cast<UCk_2dGridSystem_Spec>(
            SpecProperty->GetObjectPropertyValue_InContainer(GridScript));
        if (Spec == nullptr)
        { continue; }

        // Spec is exposed mutably on the result; Render only reads through it.

        Result.Spawner       = Spawner;
        Result.Spec          = Spec;
        Result.GridTransform = Spawner->GetActorTransform();
        return Result;
    }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_2dGridSystem_EdMode::
    Render(
        const FSceneView*        InView,
        FViewport*               InViewport,
        FPrimitiveDrawInterface* InPDI)
{
    Super::Render(InView, InViewport, InPDI);

    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    { return; }

    const auto* Spec       = Selection.Spec;
    const auto& Transform  = Selection.GridTransform;
    const auto  CellSize   = Spec->CellSize;
    const auto  Dimensions = Spec->Dimensions;

    if (CellSize.X <= 0.0 || CellSize.Y <= 0.0 || Dimensions.X <= 0 || Dimensions.Y <= 0)
    { return; }

    // Corner-origin convention (matches UCk_Utils_Grid2D_UE::Get_CoordinateAsLocation): cell (x,y)'s
    // min corner is at grid-local (x*CellSize.X, y*CellSize.Y, 0). InGridTransform maps to world.
    const auto LocalToWorld = [&](double InLocalX, double InLocalY) -> FVector
    {
        return Transform.TransformPosition(FVector(InLocalX, InLocalY, 0.0));
    };

    // Draws a square outline for cell (InX,InY), inset from the cell bounds by InInsetFraction of the
    // cell on every side (0 = full cell edges). The inset variant is used for the per-state marker so
    // it does NOT sit coincident with the green base-grid lines (coincident SDPG_Foreground lines
    // z-fight and the winner is order-independent — that was the partial-coloring bug).
    const auto DrawCellSquare = [&](int32 InX, int32 InY, const FLinearColor& InColor,
                                    double InInsetFraction, float InThickness)
    {
        const auto MinX = (InX + InInsetFraction)       * CellSize.X;
        const auto MinY = (InY + InInsetFraction)       * CellSize.Y;
        const auto MaxX = (InX + 1.0 - InInsetFraction) * CellSize.X;
        const auto MaxY = (InY + 1.0 - InInsetFraction) * CellSize.Y;

        const auto C00 = LocalToWorld(MinX, MinY);
        const auto C10 = LocalToWorld(MaxX, MinY);
        const auto C11 = LocalToWorld(MaxX, MaxY);
        const auto C01 = LocalToWorld(MinX, MaxY);

        InPDI->DrawLine(C00, C10, InColor, SDPG_Foreground, InThickness);
        InPDI->DrawLine(C10, C11, InColor, SDPG_Foreground, InThickness);
        InPDI->DrawLine(C11, C01, InColor, SDPG_Foreground, InThickness);
        InPDI->DrawLine(C01, C00, InColor, SDPG_Foreground, InThickness);
    };

    // Pass 1: the full grid wireframe in one uniform color (enabled/green). Shared edges are drawn
    // twice but always in the SAME color, so there is no coincident-color conflict.
    for (auto Y = 0; Y < Dimensions.Y; ++Y)
    {
        for (auto X = 0; X < Dimensions.X; ++X)
        { DrawCellSquare(X, Y, ck_grid_editor_detail::ColorEnabled, 0.0, ck_grid_editor_detail::CellLineThickness); }
    }

    // Pass 2: mark each non-enabled cell with an INSET square in its state color. Inset so it never
    // overlaps the green base lines — guaranteeing the marker is always visible on all four sides.
    for (auto Y = 0; Y < Dimensions.Y; ++Y)
    {
        for (auto X = 0; X < Dimensions.X; ++X)
        {
            const auto Color = ck_grid_editor_detail::Resolve_CellColor(Spec, FIntPoint(X, Y));
            if (Color != ck_grid_editor_detail::ColorEnabled)
            { DrawCellSquare(X, Y, Color, ck_grid_editor_detail::CellMarkerInset, ck_grid_editor_detail::CellMarkerThickness); }
        }
    }

    // Pivot marker at the grid-local origin (cell (0,0) min corner).
    const auto PivotWorld = LocalToWorld(0.0, 0.0);
    const auto Half       = ck_grid_editor_detail::PivotMarkerSize;
    InPDI->DrawLine(PivotWorld - FVector(Half, 0, 0), PivotWorld + FVector(Half, 0, 0),
        ck_grid_editor_detail::ColorPivot, SDPG_Foreground, ck_grid_editor_detail::PivotLineThickness);
    InPDI->DrawLine(PivotWorld - FVector(0, Half, 0), PivotWorld + FVector(0, Half, 0),
        ck_grid_editor_detail::ColorPivot, SDPG_Foreground, ck_grid_editor_detail::PivotLineThickness);
    InPDI->DrawLine(PivotWorld - FVector(0, 0, Half), PivotWorld + FVector(0, 0, Half),
        ck_grid_editor_detail::ColorPivot, SDPG_Foreground, ck_grid_editor_detail::PivotLineThickness);

    // Hover highlight: a thick white inset square on the cell currently under the cursor. Drawn last so
    // it sits on top of the state markers. Only valid coordinates are stored in _HoveredCell.
    if (_HoveredCell.IsSet())
    {
        const auto& Cell = _HoveredCell.GetValue();
        if (Cell.X >= 0 && Cell.X < Dimensions.X && Cell.Y >= 0 && Cell.Y < Dimensions.Y)
        {
            DrawCellSquare(Cell.X, Cell.Y, ck_grid_editor_detail::ColorHover,
                ck_grid_editor_detail::HoverMarkerInset, ck_grid_editor_detail::HoverMarkerThickness);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_2dGridSystem_EdMode::
    Compute_CursorRay(
        FEditorViewportClient* InViewportClient,
        int32                  InMouseX,
        int32                  InMouseY,
        FVector&               OutRayOrigin,
        FVector&               OutRayDirection) const -> bool
{
    if (InViewportClient == nullptr)
    { return false; }

    // Canonical engine paint-mode deproject: build a transient scene-view family for this viewport,
    // calc the view, then read the world ray for the pixel off FViewportCursorLocation. Mirrors
    // FEdModeFoliage::MouseMove / CapturedMouseMove.
    auto ViewFamily = FSceneViewFamilyContext(FSceneViewFamily::ConstructionValues(
        InViewportClient->Viewport,
        InViewportClient->GetScene(),
        InViewportClient->EngineShowFlags)
        .SetRealtimeUpdate(InViewportClient->IsRealtime()));

    auto* View = InViewportClient->CalcSceneView(&ViewFamily);
    if (View == nullptr)
    { return false; }

    const auto CursorRay = FViewportCursorLocation(View, InViewportClient, InMouseX, InMouseY);
    OutRayOrigin    = CursorRay.GetOrigin();
    OutRayDirection = CursorRay.GetDirection();

    // In ortho views the origin sits on the near plane; push it back so the plane intersection in
    // Resolve_CellFromRay (which rejects hits behind the origin) stays in front.
    if (InViewportClient->IsOrtho())
    { OutRayOrigin += -WORLD_MAX * OutRayDirection; }

    return true;
}

auto
    UCk_2dGridSystem_EdMode::
    Toggle_ShapeCell(
        const FResolvedGridSelection& InSelection,
        const FIntPoint&              InCell) -> void
{
    if (! InSelection.IsValid())
    { return; }

    auto* Spec = InSelection.Spec;

    Spec->Modify();

    if (Spec->DisabledCells.Contains(InCell))
    { Spec->DisabledCells.RemoveSingleSwap(InCell); }
    else
    { Spec->DisabledCells.Add(InCell); }

    InSelection.Spawner->EditorOnly_RebuildEntity();
}

auto
    UCk_2dGridSystem_EdMode::
    Paint_ShapeStrokeAtCursor(
        FEditorViewportClient* InViewportClient,
        int32                  InMouseX,
        int32                  InMouseY) -> void
{
    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    { return; }

    auto RayOrigin    = FVector::ZeroVector;
    auto RayDirection = FVector::ZeroVector;
    if (! Compute_CursorRay(InViewportClient, InMouseX, InMouseY, RayOrigin, RayDirection))
    { return; }

    const auto Cell = ck::grid_editor::Resolve_CellFromRay(
        Selection.GridTransform, Selection.Spec->CellSize, Selection.Spec->Dimensions, RayOrigin, RayDirection);
    if (! Cell.IsSet())
    { return; }

    // Dedupe: a cell is toggled at most once per stroke, so mouse jitter inside it can't flip it
    // on/off repeatedly.
    if (_StrokeToggledCells.Contains(Cell.GetValue()))
    { return; }

    _StrokeToggledCells.Add(Cell.GetValue());
    Toggle_ShapeCell(Selection, Cell.GetValue());
}

// --------------------------------------------------------------------------------------------------------------------

bool
    UCk_2dGridSystem_EdMode::
    HandleClick(
        FEditorViewportClient* InViewportClient,
        HHitProxy*             InHitProxy,
        const FViewportClick&  InClick)
{
    // A click that turned into a drag is painted by the StartTracking/CapturedMouseMove/EndTracking
    // path; swallow the trailing HandleClick so the cell isn't toggled a second time. EndTracking
    // leaves _StrokeToggledCells populated precisely so we can detect that here; consume it now so a
    // later pure single-click isn't wrongly swallowed.
    if (_IsPaintingStroke || (! _StrokeToggledCells.IsEmpty()))
    {
        _StrokeToggledCells.Reset();
        return true;
    }

    if (_ActiveTool != ECk_GridPaint_Tool::Shape)
    { return Super::HandleClick(InViewportClient, InHitProxy, InClick); }

    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    { return Super::HandleClick(InViewportClient, InHitProxy, InClick); }

    const auto Cell = ck::grid_editor::Resolve_CellFromRay(
        Selection.GridTransform, Selection.Spec->CellSize, Selection.Spec->Dimensions,
        InClick.GetOrigin(), InClick.GetDirection());
    if (! Cell.IsSet())
    { return Super::HandleClick(InViewportClient, InHitProxy, InClick); }

    const auto Transaction = FScopedTransaction(
        NSLOCTEXT("Ck_2dGridSystem_EdMode", "PaintShapeCell", "Grid Paint: Toggle Cell"));
    Toggle_ShapeCell(Selection, Cell.GetValue());

    return true;
}

bool
    UCk_2dGridSystem_EdMode::
    InputDelta(
        FEditorViewportClient* InViewportClient,
        FViewport*             InViewport,
        FVector&               InDrag,
        FRotator&              InRot,
        FVector&               InScale)
{
    // Drag painting flows through CapturedMouseMove (we disable gizmo delta-tracking while a Shape
    // stroke is active); no InputDelta handling needed.
    return Super::InputDelta(InViewportClient, InViewport, InDrag, InRot, InScale);
}

bool
    UCk_2dGridSystem_EdMode::
    MouseMove(
        FEditorViewportClient* InViewportClient,
        FViewport*             InViewport,
        int32                  InMouseX,
        int32                  InMouseY)
{
    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    {
        _HoveredCell.Reset();
        return false;
    }

    auto RayOrigin    = FVector::ZeroVector;
    auto RayDirection = FVector::ZeroVector;
    if (! Compute_CursorRay(InViewportClient, InMouseX, InMouseY, RayOrigin, RayDirection))
    {
        _HoveredCell.Reset();
        return false;
    }

    _HoveredCell = ck::grid_editor::Resolve_CellFromRay(
        Selection.GridTransform, Selection.Spec->CellSize, Selection.Spec->Dimensions, RayOrigin, RayDirection);

    // Not handled — let the base mode keep processing the move; we only observe it for the highlight.
    return false;
}

bool
    UCk_2dGridSystem_EdMode::
    StartTracking(
        FEditorViewportClient* InViewportClient,
        FViewport*             InViewport)
{
    if (_ActiveTool != ECk_GridPaint_Tool::Shape)
    { return Super::StartTracking(InViewportClient, InViewport); }

    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    { return Super::StartTracking(InViewportClient, InViewport); }

    // Open ONE transaction for the whole drag stroke so a single Ctrl-Z undoes every cell toggled
    // during the drag. Closed in EndTracking.
    GEditor->BeginTransaction(
        NSLOCTEXT("Ck_2dGridSystem_EdMode", "PaintShapeStroke", "Grid Paint: Paint Cells"));

    _IsPaintingStroke = true;
    _StrokeToggledCells.Reset();

    // Paint the cell under the press point immediately (CapturedMouseMove only fires once the mouse
    // actually moves).
    if (InViewport != nullptr)
    {
        Paint_ShapeStrokeAtCursor(InViewportClient,
            InViewport->GetMouseX(), InViewport->GetMouseY());
    }

    return true;
}

bool
    UCk_2dGridSystem_EdMode::
    CapturedMouseMove(
        FEditorViewportClient* InViewportClient,
        FViewport*             InViewport,
        int32                  InMouseX,
        int32                  InMouseY)
{
    if (_IsPaintingStroke)
    {
        // Keep the hover highlight tracking the cursor during the stroke too.
        Paint_ShapeStrokeAtCursor(InViewportClient, InMouseX, InMouseY);
        return true;
    }

    return Super::CapturedMouseMove(InViewportClient, InViewport, InMouseX, InMouseY);
}

bool
    UCk_2dGridSystem_EdMode::
    EndTracking(
        FEditorViewportClient* InViewportClient,
        FViewport*             InViewport)
{
    if (! _IsPaintingStroke)
    { return Super::EndTracking(InViewportClient, InViewport); }

    _IsPaintingStroke = false;
    GEditor->EndTransaction();

    const auto bToggledAny = ! _StrokeToggledCells.IsEmpty();
    // NOTE: not cleared here — HandleClick fires AFTER EndTracking on a click-drag and consults this
    // to avoid a double-toggle. It's reset at the start of the next stroke (and stays harmless until
    // then because painting only reads it within an active stroke).
    return bToggledAny;
}

bool
    UCk_2dGridSystem_EdMode::
    DisallowMouseDeltaTracking() const
{
    // While a Shape stroke is active, suppress the gizmo/camera delta-tracker so the drag paints
    // cells instead of moving the selected actor.
    return _IsPaintingStroke;
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
