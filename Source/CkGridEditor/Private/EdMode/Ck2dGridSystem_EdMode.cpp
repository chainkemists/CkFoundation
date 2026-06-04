#include "CkGridEditor/EdMode/Ck2dGridSystem_EdMode.h"

#include "CkGridEditor_Log.h"

#include "CkGridEditor/Draw/Ck2dGridSystem_AuthoredOverlay.h"
#include "CkGridEditor/EdMode/Ck2dGridSystem_EdModeToolkit.h"
#include "CkGridEditor/EdMode/Ck2dGridSystem_EdMode_Hit.h"

#include "CkGrid/2dGridSystem/Authoring/Ck2dGridSystem_Spec.h"

#include "CkEntitySpawner/CkEntitySpawner_Actor.h"

#include "Editor.h"
#include "EditorModeManager.h"
#include "EditorViewportClient.h"
#include "Engine/Selection.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"
#include "UnrealClient.h"
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
    // Interaction-overlay colors/metrics — these are PAINT-MODE ONLY (hover, select marker, blocker
    // drag/selection/group). The AUTHORED-state colors (enabled/disabled/blocker/tagged/pivot), the
    // per-cell color resolve, and the base-grid + state-marker draw all live in the shared overlay
    // (ck::grid_editor, Ck2dGridSystem_AuthoredOverlay.h) so the in-mode and out-of-mode previews match.

    constexpr auto ColorHover = FLinearColor(1.0f, 1.0f, 1.0f); // white

    // Select tool: the inspected cell, drawn as a thick cyan inset square — distinct from the white
    // hover marker so the two read separately when both land on the same cell.
    constexpr auto ColorSelected         = FLinearColor(0.0f, 1.0f, 1.0f); // cyan
    constexpr auto SelectedMarkerInset     = 0.02;
    constexpr auto SelectedMarkerThickness = 4.0f;

    // Blocker drag-rect candidate preview (cyan) and selected-blocker emphasis (bright orange).
    constexpr auto ColorBlockerDrag     = FLinearColor(0.0f, 1.0f, 1.0f);  // cyan
    constexpr auto ColorBlockerSelected = FLinearColor(1.0f, 0.75f, 0.2f); // bright orange

    constexpr auto BlockerDragThickness     = 3.0f;
    constexpr auto BlockerSelectedInset     = 0.04;
    constexpr auto BlockerSelectedThickness = 4.0f;

    // Select tool: when the pick lands on a blocker, the WHOLE blocker group is outlined as a single
    // bright-magenta rect (the bounding RangeMin..RangeMax border) — visually distinct from both the
    // single-cell cyan Select marker and the orange Blocker-tool per-cell inset.
    constexpr auto ColorBlockerGroup     = FLinearColor(1.0f, 0.0f, 1.0f); // magenta
    constexpr auto BlockerGroupThickness = 5.0f;

    // Hover highlight: a slightly smaller inset than the state markers (so it nests inside any state
    // marker on the same cell) drawn thick and white.
    constexpr auto HoverMarkerInset     = 0.06;
    constexpr auto HoverMarkerThickness = 3.0f;
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

auto
    UCk_2dGridSystem_EdMode::
    Set_ActiveTool(
        ECk_GridPaint_Tool InTool) -> void
{
    _ActiveTool = InTool;

    // The pending drag-rect corners only make sense while the Blocker tool is active.
    if (_ActiveTool != ECk_GridPaint_Tool::Blocker)
    {
        _BlockerDragStart.Reset();
        _BlockerDragCurrent.Reset();
    }

    // _SelectedBlockerIndex is shared by the Blocker tool (place/select/delete) and the Select tool (a
    // pick that lands on a blocker selects the whole group). It's meaningless under Shape/Tags, so clear
    // it on any tool change away from both — otherwise a stale highlight lingers.
    if (_ActiveTool != ECk_GridPaint_Tool::Blocker && _ActiveTool != ECk_GridPaint_Tool::Select)
    { _SelectedBlockerIndex = INDEX_NONE; }

    // The Select-tool pick highlight only makes sense while the Select tool is active; clear it on any
    // tool change away from Select so a stale highlight doesn't linger.
    if (_ActiveTool != ECk_GridPaint_Tool::Select)
    { _SelectedCell.Reset(); }
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

        // Resolve the hosted grid Spec via the shared helper (the single reflective-read site). Spec is
        // exposed mutably on the result so paint actions can Modify()+mutate it; Render only reads it.
        auto* Spec = ck::grid_editor::Resolve_SpecFromSpawner(Spawner);
        if (Spec == nullptr)
        { continue; }

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

    // Authored-state passes (base-grid wireframe + per-cell state markers + pivot) are drawn by the
    // SHARED overlay so the in-mode and out-of-mode (component visualizer) previews are identical. Only
    // the interaction overlays below (hover / select / blocker drag/selection/group) are paint-mode-only.
    ck::grid_editor::Draw_GridAuthoredOverlay(InPDI, Spec, Transform);

    // Selected-blocker highlight (Blocker tool): emphasize the selected blocker's cells with a
    // bright-orange thick inset square (drawn before hover so the white hover marker still reads on top).
    // The Select tool draws its blocker selection as a magenta GROUP outline further below instead, so
    // this per-cell inset is scoped to the Blocker tool to keep the two selection visuals distinct.
    if (_ActiveTool == ECk_GridPaint_Tool::Blocker &&
        _SelectedBlockerIndex != INDEX_NONE && Spec->Blockers.IsValidIndex(_SelectedBlockerIndex))
    {
        const auto& Blocker = Spec->Blockers[_SelectedBlockerIndex];
        const auto MinX = FMath::Min(Blocker.RangeMin.X, Blocker.RangeMax.X);
        const auto MaxX = FMath::Max(Blocker.RangeMin.X, Blocker.RangeMax.X);
        const auto MinY = FMath::Min(Blocker.RangeMin.Y, Blocker.RangeMax.Y);
        const auto MaxY = FMath::Max(Blocker.RangeMin.Y, Blocker.RangeMax.Y);

        for (auto Y = MinY; Y <= MaxY; ++Y)
        {
            for (auto X = MinX; X <= MaxX; ++X)
            {
                if (X >= 0 && X < Dimensions.X && Y >= 0 && Y < Dimensions.Y)
                {
                    DrawCellSquare(X, Y, ck_grid_editor_detail::ColorBlockerSelected,
                        ck_grid_editor_detail::BlockerSelectedInset, ck_grid_editor_detail::BlockerSelectedThickness);
                }
            }
        }
    }

    // Blocker drag-rect preview: outline the candidate rect spanning min..max of the start/current
    // corners as a single cyan border (full-cell edges of the bounding rect, no per-cell lines).
    if (_BlockerDragStart.IsSet())
    {
        const auto Start   = _BlockerDragStart.GetValue();
        const auto Current = _BlockerDragCurrent.IsSet() ? _BlockerDragCurrent.GetValue() : Start;

        const auto MinX = FMath::Min(Start.X, Current.X);
        const auto MaxX = FMath::Max(Start.X, Current.X);
        const auto MinY = FMath::Min(Start.Y, Current.Y);
        const auto MaxY = FMath::Max(Start.Y, Current.Y);

        const auto LoX = MinX               * CellSize.X;
        const auto LoY = MinY               * CellSize.Y;
        const auto HiX = (MaxX + 1)         * CellSize.X;
        const auto HiY = (MaxY + 1)         * CellSize.Y;

        const auto C00 = LocalToWorld(LoX, LoY);
        const auto C10 = LocalToWorld(HiX, LoY);
        const auto C11 = LocalToWorld(HiX, HiY);
        const auto C01 = LocalToWorld(LoX, HiY);

        InPDI->DrawLine(C00, C10, ck_grid_editor_detail::ColorBlockerDrag, SDPG_Foreground, ck_grid_editor_detail::BlockerDragThickness);
        InPDI->DrawLine(C10, C11, ck_grid_editor_detail::ColorBlockerDrag, SDPG_Foreground, ck_grid_editor_detail::BlockerDragThickness);
        InPDI->DrawLine(C11, C01, ck_grid_editor_detail::ColorBlockerDrag, SDPG_Foreground, ck_grid_editor_detail::BlockerDragThickness);
        InPDI->DrawLine(C01, C00, ck_grid_editor_detail::ColorBlockerDrag, SDPG_Foreground, ck_grid_editor_detail::BlockerDragThickness);
    }

    // Select-tool blocker-group highlight: when the Select pick landed on a blocker, outline the WHOLE
    // blocker (the bounding RangeMin..RangeMax rect) in bright magenta as a single border, distinct from
    // the orange per-cell inset the Blocker tool draws above and the cyan single-cell marker below. Only
    // drawn for the Select tool — under the Blocker tool the orange inset already conveys the selection.
    const auto bSelectBlockerGroup =
        _ActiveTool == ECk_GridPaint_Tool::Select &&
        _SelectedBlockerIndex != INDEX_NONE &&
        Spec->Blockers.IsValidIndex(_SelectedBlockerIndex);

    if (bSelectBlockerGroup)
    {
        const auto& Blocker = Spec->Blockers[_SelectedBlockerIndex];
        const auto BMinX = FMath::Max(0,                FMath::Min(Blocker.RangeMin.X, Blocker.RangeMax.X));
        const auto BMaxX = FMath::Min(Dimensions.X - 1, FMath::Max(Blocker.RangeMin.X, Blocker.RangeMax.X));
        const auto BMinY = FMath::Max(0,                FMath::Min(Blocker.RangeMin.Y, Blocker.RangeMax.Y));
        const auto BMaxY = FMath::Min(Dimensions.Y - 1, FMath::Max(Blocker.RangeMin.Y, Blocker.RangeMax.Y));

        if (BMinX <= BMaxX && BMinY <= BMaxY)
        {
            const auto LoX = BMinX       * CellSize.X;
            const auto LoY = BMinY       * CellSize.Y;
            const auto HiX = (BMaxX + 1) * CellSize.X;
            const auto HiY = (BMaxY + 1) * CellSize.Y;

            const auto G00 = LocalToWorld(LoX, LoY);
            const auto G10 = LocalToWorld(HiX, LoY);
            const auto G11 = LocalToWorld(HiX, HiY);
            const auto G01 = LocalToWorld(LoX, HiY);

            InPDI->DrawLine(G00, G10, ck_grid_editor_detail::ColorBlockerGroup, SDPG_Foreground, ck_grid_editor_detail::BlockerGroupThickness);
            InPDI->DrawLine(G10, G11, ck_grid_editor_detail::ColorBlockerGroup, SDPG_Foreground, ck_grid_editor_detail::BlockerGroupThickness);
            InPDI->DrawLine(G11, G01, ck_grid_editor_detail::ColorBlockerGroup, SDPG_Foreground, ck_grid_editor_detail::BlockerGroupThickness);
            InPDI->DrawLine(G01, G00, ck_grid_editor_detail::ColorBlockerGroup, SDPG_Foreground, ck_grid_editor_detail::BlockerGroupThickness);
        }
    }

    // Select-tool single-cell inspection highlight: a thick cyan inset square on the cell picked for the
    // Details panel. Suppressed when the pick resolved to a blocker (the magenta group outline above is
    // the selection indicator in that case). Drawn before hover so the white hover marker still reads on
    // top when both land on the same cell, and inset less than the state markers so it frames them.
    if (_SelectedCell.IsSet() && ! bSelectBlockerGroup)
    {
        const auto& Cell = _SelectedCell.GetValue();
        if (Cell.X >= 0 && Cell.X < Dimensions.X && Cell.Y >= 0 && Cell.Y < Dimensions.Y)
        {
            DrawCellSquare(Cell.X, Cell.Y, ck_grid_editor_detail::ColorSelected,
                ck_grid_editor_detail::SelectedMarkerInset, ck_grid_editor_detail::SelectedMarkerThickness);
        }
    }

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

void
    UCk_2dGridSystem_EdMode::
    DrawHUD(
        FEditorViewportClient* InViewportClient,
        FViewport*             InViewport,
        const FSceneView*      InView,
        FCanvas*               InCanvas)
{
    Super::DrawHUD(InViewportClient, InViewport, InView, InCanvas);

    // Per-cell tag TEXT labels (toggle: ck.Grid.PreviewShowTags, default off). Drawn via the shared
    // helper so the labels match the out-of-mode component-visualizer view.
    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    { return; }

    ck::grid_editor::Draw_GridTagLabels(InCanvas, InView, Selection.Spec, Selection.GridTransform);
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
    Set_ShapeCellDisabled(
        const FResolvedGridSelection& InSelection,
        const FIntPoint&              InCell,
        bool                          InDisabled) -> void
{
    if (! InSelection.IsValid())
    { return; }

    auto* Spec = InSelection.Spec;

    // Idempotent: skip the Modify()/rebuild when the cell is already in the requested state.
    const auto bAlreadyDisabled = Spec->DisabledCells.Contains(InCell);
    if (bAlreadyDisabled == InDisabled)
    { return; }

    Spec->Modify();

    if (InDisabled)
    { Spec->DisabledCells.Add(InCell); }
    else
    { Spec->DisabledCells.RemoveSingleSwap(InCell); }

    InSelection.Spawner->EditorOnly_RebuildEntity();
}

auto
    UCk_2dGridSystem_EdMode::
    Set_TagCell(
        const FResolvedGridSelection& InSelection,
        const FIntPoint&              InCell,
        bool                          InAdd) -> void
{
    if (! InSelection.IsValid())
    { return; }

    if (! _ActivePaintTag.IsValid())
    {
        ck::grid_editor::Warning(TEXT("Tags tool: no active paint tag set — skipping cell paint"));
        return;
    }

    auto* Spec = InSelection.Spec;

    if (InAdd)
    {
        // Idempotent ADD: no-op (and no Modify/rebuild) when the cell already carries the tag.
        if (const auto* Existing = Spec->PerCellTags.Find(InCell);
            Existing != nullptr && Existing->HasTagExact(_ActivePaintTag))
        { return; }

        Spec->Modify();
        auto& Container = Spec->PerCellTags.FindOrAdd(InCell);
        Container.AddTag(_ActivePaintTag);
        InSelection.Spawner->EditorOnly_RebuildEntity();
        return;
    }

    // REMOVE: no-op when the cell doesn't carry the tag.
    auto* Container = Spec->PerCellTags.Find(InCell);
    if (Container == nullptr || ! Container->HasTagExact(_ActivePaintTag))
    { return; }

    Spec->Modify();
    Container->RemoveTag(_ActivePaintTag);

    // Drop the map entry entirely when the cell no longer carries any per-cell tag (no entry == no
    // overrides), mirroring Remove_SelectedCellTag.
    if (Container->IsEmpty())
    { Spec->PerCellTags.Remove(InCell); }

    InSelection.Spawner->EditorOnly_RebuildEntity();
}

auto
    UCk_2dGridSystem_EdMode::
    Paint_StrokeCell(
        const FResolvedGridSelection& InSelection,
        const FIntPoint&              InCell,
        bool                          InErase) -> void
{
    switch (_ActiveTool)
    {
        case ECk_GridPaint_Tool::Shape:
        {
            // ADD disables the cell; ERASE re-enables it.
            Set_ShapeCellDisabled(InSelection, InCell, ! InErase);
            break;
        }
        case ECk_GridPaint_Tool::Tags:
        {
            // ADD writes the active tag; ERASE removes it.
            Set_TagCell(InSelection, InCell, ! InErase);
            break;
        }
        default:
        {
            // Blocker is a drag-rect, not a per-cell stroke; nothing to do here.
            break;
        }
    }
}

auto
    UCk_2dGridSystem_EdMode::
    Resolve_CellAtCursor(
        FEditorViewportClient* InViewportClient,
        int32                  InMouseX,
        int32                  InMouseY) const -> TOptional<FIntPoint>
{
    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    { return {}; }

    auto RayOrigin    = FVector::ZeroVector;
    auto RayDirection = FVector::ZeroVector;
    if (! Compute_CursorRay(InViewportClient, InMouseX, InMouseY, RayOrigin, RayDirection))
    { return {}; }

    return ck::grid_editor::Resolve_CellFromRay(
        Selection.GridTransform, Selection.Spec->CellSize, Selection.Spec->Dimensions, RayOrigin, RayDirection);
}

auto
    UCk_2dGridSystem_EdMode::
    Paint_StrokeAtCursor(
        FEditorViewportClient* InViewportClient,
        int32                  InMouseX,
        int32                  InMouseY) -> void
{
    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    { return; }

    // Record the latest painted pixel so EndTracking can land the cursor on the stroke's END cell.
    _StrokeLastMousePixel = FIntPoint(InMouseX, InMouseY);

    const auto Cell = Resolve_CellAtCursor(InViewportClient, InMouseX, InMouseY);
    if (! Cell.IsSet())
    { return; }

    // Dedupe: a cell is painted at most once per stroke, so mouse jitter inside it can't re-apply the
    // tool action repeatedly.
    if (_StrokeToggledCells.Contains(Cell.GetValue()))
    { return; }

    _StrokeToggledCells.Add(Cell.GetValue());
    Paint_StrokeCell(Selection, Cell.GetValue(), _StrokeErase);
}

auto
    UCk_2dGridSystem_EdMode::
    Find_BlockerCovering(
        const FResolvedGridSelection& InSelection,
        const FIntPoint&              InCell) const -> int32
{
    if (! InSelection.IsValid())
    { return INDEX_NONE; }

    const auto& Blockers = InSelection.Spec->Blockers;
    for (auto Index = 0; Index < Blockers.Num(); ++Index)
    {
        const auto& Blocker = Blockers[Index];
        const auto MinX = FMath::Min(Blocker.RangeMin.X, Blocker.RangeMax.X);
        const auto MaxX = FMath::Max(Blocker.RangeMin.X, Blocker.RangeMax.X);
        const auto MinY = FMath::Min(Blocker.RangeMin.Y, Blocker.RangeMax.Y);
        const auto MaxY = FMath::Max(Blocker.RangeMin.Y, Blocker.RangeMax.Y);

        if (InCell.X >= MinX && InCell.X <= MaxX && InCell.Y >= MinY && InCell.Y <= MaxY)
        { return Index; }
    }

    return INDEX_NONE;
}

auto
    UCk_2dGridSystem_EdMode::
    Resolve_SelectedCellInfo() const -> FSelectedCellInfo
{
    auto Result = FSelectedCellInfo{};

    if (! _SelectedCell.IsSet())
    { return Result; }

    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    { return Result; }

    const auto* Spec = Selection.Spec;
    const auto  Cell = _SelectedCell.GetValue();

    Result.bHasSelection   = true;
    Result.Coordinate      = Cell;
    Result.GridDefaultTags = Spec->DefaultCellTags;

    if (const auto* PerCell = Spec->PerCellTags.Find(Cell))
    { Result.CellTags = *PerCell; }

    // State priority mirrors the Render color convention: disabled > blocker > enabled.
    if (Spec->DisabledCells.Contains(Cell))
    {
        Result.State = ECellState::Disabled;
    }
    else
    {
        const auto BlockerIndex = Find_BlockerCovering(Selection, Cell);
        if (BlockerIndex != INDEX_NONE)
        {
            Result.State        = ECellState::Blocked;
            Result.BlockerIndex = BlockerIndex;
            Result.BlockerName  = Spec->Blockers[BlockerIndex].Name;
        }
        else
        {
            Result.State = ECellState::Enabled;
        }
    }

    return Result;
}

auto
    UCk_2dGridSystem_EdMode::
    Is_PlainLeftClick(
        const FViewportClick& InClick) const -> bool
{
    // LMB with NO Ctrl/Alt. Shift IS allowed — it selects the erase direction for the Shape/Tags paint
    // (Is_EraseModifier). Ctrl/Alt or a non-left button means the user is driving the camera (RMB-look,
    // Alt+LMB orbit, etc.).
    if (InClick.IsControlDown() || InClick.IsAltDown())
    { return false; }

    return InClick.GetKey() == EKeys::LeftMouseButton;
}

auto
    UCk_2dGridSystem_EdMode::
    Is_PlainLeftDrag(
        FEditorViewportClient* InViewportClient,
        FViewport*             InViewport) const -> bool
{
    if (InViewportClient == nullptr || InViewport == nullptr)
    { return false; }

    // Ctrl/Alt held → camera gesture (Alt-orbit etc.), not a paint stroke. Shift IS allowed (erase mode).
    if (InViewportClient->IsCtrlPressed() || InViewportClient->IsAltPressed())
    { return false; }

    // Left button must be down; right button must NOT be (LMB+RMB is the camera pan gesture).
    const auto bLeftDown  = InViewport->KeyState(EKeys::LeftMouseButton);
    const auto bRightDown = InViewport->KeyState(EKeys::RightMouseButton);

    return bLeftDown && ! bRightDown;
}

auto
    UCk_2dGridSystem_EdMode::
    Is_EraseModifier(
        const FViewportClick& InClick) const -> bool
{
    return InClick.IsShiftDown();
}

auto
    UCk_2dGridSystem_EdMode::
    Is_EraseModifier(
        FEditorViewportClient* InViewportClient) const -> bool
{
    return InViewportClient != nullptr && InViewportClient->IsShiftPressed();
}

auto
    UCk_2dGridSystem_EdMode::
    Schedule_RestoreCursorToStrokeEnd(
        FViewport*       InViewport,
        const FIntPoint& InMousePixel) -> void
{
    if (InViewport == nullptr || GEditor == nullptr)
    { return; }

    // Defer to the next editor tick: the OS restores the cursor to the raw-mouse capture origin when Slate
    // exits high-precision mode LATER in this same input pass, so repositioning now would be overwritten.
    // A next-tick timer fires after that, landing the cursor on the stroke's end cell.
    auto* RawViewport = InViewport;

    GEditor->GetTimerManager()->SetTimerForNextTick(
        FTimerDelegate::CreateLambda([RawViewport, InMousePixel]()
        {
            // RawViewport may have been torn down between scheduling and firing (mode exit, viewport close).
            // GEditor outlives the timer manager that owns this delegate, so the manager itself is safe; we
            // only guard the viewport pointer. There is no public "is this FViewport still alive" probe, but
            // a next-tick timer in practice fires before any viewport teardown that could invalidate it.
            if (RawViewport != nullptr)
            { RawViewport->SetMouse(InMousePixel.X, InMousePixel.Y); }
        }));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_2dGridSystem_EdMode::
    Apply_GridDefaultTag() -> void
{
    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    { return; }

    if (! _ActivePaintTag.IsValid())
    {
        ck::grid_editor::Warning(TEXT("Tags tool: no active paint tag set — cannot apply grid-default tag"));
        return;
    }

    auto* Spec = Selection.Spec;

    const auto Transaction = FScopedTransaction(
        NSLOCTEXT("Ck_2dGridSystem_EdMode", "ApplyGridDefaultTag", "Grid Paint: Add Grid-Default Tag"));

    Spec->Modify();
    Spec->DefaultCellTags.AddTag(_ActivePaintTag);
    Selection.Spawner->EditorOnly_RebuildEntity();
}

auto
    UCk_2dGridSystem_EdMode::
    Remove_GridDefaultTag() -> void
{
    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    { return; }

    if (! _ActivePaintTag.IsValid())
    {
        ck::grid_editor::Warning(TEXT("Tags tool: no active paint tag set — cannot remove grid-default tag"));
        return;
    }

    auto* Spec = Selection.Spec;

    const auto Transaction = FScopedTransaction(
        NSLOCTEXT("Ck_2dGridSystem_EdMode", "RemoveGridDefaultTag", "Grid Paint: Remove Grid-Default Tag"));

    Spec->Modify();
    Spec->DefaultCellTags.RemoveTag(_ActivePaintTag);
    Selection.Spawner->EditorOnly_RebuildEntity();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_2dGridSystem_EdMode::
    Get_SelectedBlockerName() const -> FGameplayTag
{
    if (_SelectedBlockerIndex == INDEX_NONE)
    { return FGameplayTag{}; }

    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    { return FGameplayTag{}; }

    if (! Selection.Spec->Blockers.IsValidIndex(_SelectedBlockerIndex))
    { return FGameplayTag{}; }

    return Selection.Spec->Blockers[_SelectedBlockerIndex].Name;
}

auto
    UCk_2dGridSystem_EdMode::
    Set_SelectedBlockerName(
        const FGameplayTag& InTag) -> void
{
    if (_SelectedBlockerIndex == INDEX_NONE)
    { return; }

    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    { return; }

    auto* Spec = Selection.Spec;
    if (! Spec->Blockers.IsValidIndex(_SelectedBlockerIndex))
    {
        ck::grid_editor::Warning(TEXT("Blocker tool: selected blocker index is stale — cannot set Name"));
        return;
    }

    const auto Transaction = FScopedTransaction(
        NSLOCTEXT("Ck_2dGridSystem_EdMode", "SetBlockerName", "Grid Paint: Set Blocker Tag"));

    Spec->Modify();
    Spec->Blockers[_SelectedBlockerIndex].Name = InTag;
    Selection.Spawner->EditorOnly_RebuildEntity();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_2dGridSystem_EdMode::
    Set_SelectedCellDisabled(
        bool InDisabled) -> void
{
    if (! _SelectedCell.IsSet())
    { return; }

    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    { return; }

    auto* Spec = Selection.Spec;
    const auto Cell = _SelectedCell.GetValue();

    const auto bAlreadyDisabled = Spec->DisabledCells.Contains(Cell);
    if (bAlreadyDisabled == InDisabled)
    { return; }

    const auto Transaction = FScopedTransaction(
        NSLOCTEXT("Ck_2dGridSystem_EdMode", "SetCellDisabled", "Grid Paint: Set Cell Disabled"));

    Spec->Modify();
    if (InDisabled)
    { Spec->DisabledCells.Add(Cell); }
    else
    { Spec->DisabledCells.RemoveSingleSwap(Cell); }

    Selection.Spawner->EditorOnly_RebuildEntity();
}

auto
    UCk_2dGridSystem_EdMode::
    Get_SelectedCellDisabled() const -> bool
{
    if (! _SelectedCell.IsSet())
    { return false; }

    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    { return false; }

    return Selection.Spec->DisabledCells.Contains(_SelectedCell.GetValue());
}

auto
    UCk_2dGridSystem_EdMode::
    Add_SelectedCellTag(
        const FGameplayTag& InTag) -> void
{
    if (! InTag.IsValid())
    { return; }

    if (! _SelectedCell.IsSet())
    { return; }

    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    { return; }

    auto* Spec = Selection.Spec;
    const auto Cell = _SelectedCell.GetValue();

    const auto Transaction = FScopedTransaction(
        NSLOCTEXT("Ck_2dGridSystem_EdMode", "AddCellTag", "Grid Paint: Add Cell Tag"));

    Spec->Modify();
    auto& Container = Spec->PerCellTags.FindOrAdd(Cell);
    Container.AddTag(InTag);

    Selection.Spawner->EditorOnly_RebuildEntity();
}

auto
    UCk_2dGridSystem_EdMode::
    Remove_SelectedCellTag(
        const FGameplayTag& InTag) -> void
{
    if (! _SelectedCell.IsSet())
    { return; }

    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    { return; }

    auto* Spec = Selection.Spec;
    const auto Cell = _SelectedCell.GetValue();

    auto* Container = Spec->PerCellTags.Find(Cell);
    if (Container == nullptr || ! Container->HasTagExact(InTag))
    { return; }

    const auto Transaction = FScopedTransaction(
        NSLOCTEXT("Ck_2dGridSystem_EdMode", "RemoveCellTag", "Grid Paint: Remove Cell Tag"));

    Spec->Modify();
    Container->RemoveTag(InTag);

    // Drop the map entry entirely when the cell no longer carries any per-cell tag, so the Spec doesn't
    // accumulate empty containers (mirrors the authored-data convention — no entry == no overrides).
    if (Container->IsEmpty())
    { Spec->PerCellTags.Remove(Cell); }

    Selection.Spawner->EditorOnly_RebuildEntity();
}

auto
    UCk_2dGridSystem_EdMode::
    Get_SelectedCellTags() const -> FGameplayTagContainer
{
    if (! _SelectedCell.IsSet())
    { return FGameplayTagContainer{}; }

    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    { return FGameplayTagContainer{}; }

    if (const auto* Container = Selection.Spec->PerCellTags.Find(_SelectedCell.GetValue()))
    { return *Container; }

    return FGameplayTagContainer{};
}

auto
    UCk_2dGridSystem_EdMode::
    Get_SelectedBlockerRange(
        FIntPoint& OutMin,
        FIntPoint& OutMax) const -> bool
{
    if (_SelectedBlockerIndex == INDEX_NONE)
    { return false; }

    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    { return false; }

    if (! Selection.Spec->Blockers.IsValidIndex(_SelectedBlockerIndex))
    { return false; }

    const auto& Blocker = Selection.Spec->Blockers[_SelectedBlockerIndex];
    OutMin = FIntPoint(FMath::Min(Blocker.RangeMin.X, Blocker.RangeMax.X), FMath::Min(Blocker.RangeMin.Y, Blocker.RangeMax.Y));
    OutMax = FIntPoint(FMath::Max(Blocker.RangeMin.X, Blocker.RangeMax.X), FMath::Max(Blocker.RangeMin.Y, Blocker.RangeMax.Y));
    return true;
}

auto
    UCk_2dGridSystem_EdMode::
    Delete_SelectedBlocker() -> void
{
    if (_SelectedBlockerIndex == INDEX_NONE)
    { return; }

    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    { return; }

    auto* Spec = Selection.Spec;
    if (! Spec->Blockers.IsValidIndex(_SelectedBlockerIndex))
    {
        ck::grid_editor::Warning(TEXT("Select tool: selected blocker index is stale — cannot delete"));
        _SelectedBlockerIndex = INDEX_NONE;
        return;
    }

    const auto Transaction = FScopedTransaction(
        NSLOCTEXT("Ck_2dGridSystem_EdMode", "DeleteBlockerSelect", "Grid Paint: Delete Blocker"));

    Spec->Modify();
    Spec->Blockers.RemoveAt(_SelectedBlockerIndex);
    Selection.Spawner->EditorOnly_RebuildEntity();

    _SelectedBlockerIndex = INDEX_NONE;
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

    // A Blocker drag also routes through StartTracking/EndTracking; if one just finished, swallow the
    // trailing HandleClick so it isn't mis-read as a select-click on the freshly-placed rect.
    if (_BlockerDragStart.IsSet())
    {
        _BlockerDragStart.Reset();
        _BlockerDragCurrent.Reset();
        return true;
    }

    // Only LMB clicks with no Ctrl/Alt paint or select; anything else falls through to default camera nav so
    // RMB-look / modifier gestures aren't hijacked. (Shift is allowed here — it selects erase for Shape/Tags;
    // the Blocker/Select branches re-reject it below since they don't honor the erase modifier.)
    if (! Is_PlainLeftClick(InClick))
    { return Super::HandleClick(InViewportClient, InHitProxy, InClick); }

    // Blocker and Select are plain-LMB only — Shift is NOT special there, so a Shift+LMB falls through to
    // camera nav rather than mis-firing a place/select.
    if (_ActiveTool == ECk_GridPaint_Tool::Blocker)
    {
        return InClick.IsShiftDown()
            ? Super::HandleClick(InViewportClient, InHitProxy, InClick)
            : HandleClick_Blocker(InViewportClient, InHitProxy, InClick);
    }

    if (_ActiveTool == ECk_GridPaint_Tool::Select)
    {
        return InClick.IsShiftDown()
            ? Super::HandleClick(InViewportClient, InHitProxy, InClick)
            : HandleClick_Select(InViewportClient, InHitProxy, InClick);
    }

    if (_ActiveTool != ECk_GridPaint_Tool::Shape && _ActiveTool != ECk_GridPaint_Tool::Tags)
    { return Super::HandleClick(InViewportClient, InHitProxy, InClick); }

    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    { return Super::HandleClick(InViewportClient, InHitProxy, InClick); }

    const auto Cell = ck::grid_editor::Resolve_CellFromRay(
        Selection.GridTransform, Selection.Spec->CellSize, Selection.Spec->Dimensions,
        InClick.GetOrigin(), InClick.GetDirection());
    if (! Cell.IsSet())
    { return Super::HandleClick(InViewportClient, InHitProxy, InClick); }

    // Plain LMB adds; Shift+LMB erases. Label the transaction by direction so undo reads correctly.
    const auto bErase = Is_EraseModifier(InClick);

    const auto TransactionLabel = bErase
        ? NSLOCTEXT("Ck_2dGridSystem_EdMode", "EraseCell", "Grid Erase: Cell")
        : NSLOCTEXT("Ck_2dGridSystem_EdMode", "PaintCell", "Grid Paint: Cell");

    const auto Transaction = FScopedTransaction(TransactionLabel);
    Paint_StrokeCell(Selection, Cell.GetValue(), bErase);

    return true;
}

auto
    UCk_2dGridSystem_EdMode::
    HandleClick_Blocker(
        FEditorViewportClient* InViewportClient,
        HHitProxy*             InHitProxy,
        const FViewportClick&  InClick) -> bool
{
    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    { return Super::HandleClick(InViewportClient, InHitProxy, InClick); }

    const auto Cell = ck::grid_editor::Resolve_CellFromRay(
        Selection.GridTransform, Selection.Spec->CellSize, Selection.Spec->Dimensions,
        InClick.GetOrigin(), InClick.GetDirection());
    if (! Cell.IsSet())
    {
        // Clicked off-grid: clear any blocker selection.
        _SelectedBlockerIndex = INDEX_NONE;
        return true;
    }

    // Select the blocker under the click (or clear the selection if the click is on a bare cell).
    _SelectedBlockerIndex = Find_BlockerCovering(Selection, Cell.GetValue());
    return true;
}

auto
    UCk_2dGridSystem_EdMode::
    HandleClick_Select(
        FEditorViewportClient* InViewportClient,
        HHitProxy*             InHitProxy,
        const FViewportClick&  InClick) -> bool
{
    // Resolve the clicked cell and store it for the Details panel (the pick itself never mutates the Spec
    // — the Details-panel editors do).
    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    {
        _SelectedCell.Reset();
        _SelectedBlockerIndex = INDEX_NONE;
        return true;
    }

    const auto Cell = ck::grid_editor::Resolve_CellFromRay(
        Selection.GridTransform, Selection.Spec->CellSize, Selection.Spec->Dimensions,
        InClick.GetOrigin(), InClick.GetDirection());

    // Off-grid click clears both selections; otherwise store the picked cell.
    _SelectedCell = Cell;

    // Blocker precedence: if the picked cell is covered by a blocker, select the WHOLE blocker group so the
    // Details panel switches to the blocker editor. A bare cell click clears any prior blocker selection so
    // the panel falls back to the single-cell editor.
    _SelectedBlockerIndex = Cell.IsSet()
        ? Find_BlockerCovering(Selection, Cell.GetValue())
        : INDEX_NONE;

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
    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    { return Super::StartTracking(InViewportClient, InViewport); }

    // Only a LMB drag with no Ctrl/Alt (LMB down, RMB up) begins a paint stroke / blocker rect; every other
    // gesture (RMB-look, Alt-orbit, LMB+RMB-pan, wheel) is left to the camera. Shift is allowed for Shape/Tags
    // (erase) but rejected for Blocker below.
    if (! Is_PlainLeftDrag(InViewportClient, InViewport))
    { return Super::StartTracking(InViewportClient, InViewport); }

    // The Select tool is read-only; it never begins a stroke (its pick happens in HandleClick).
    if (_ActiveTool == ECk_GridPaint_Tool::Select)
    { return Super::StartTracking(InViewportClient, InViewport); }

    // Blocker: begin a rubber-band rect drag. It does NOT honor the erase modifier — a Shift+LMB drag falls
    // through to camera nav rather than starting a rect. No per-cell transaction is opened — the single
    // append is transacted in EndTracking. Just record the start/current corner so Render can preview it.
    if (_ActiveTool == ECk_GridPaint_Tool::Blocker)
    {
        if (InViewportClient->IsShiftPressed())
        { return Super::StartTracking(InViewportClient, InViewport); }

        const auto StartCell = (InViewport != nullptr)
            ? Resolve_CellAtCursor(InViewportClient, InViewport->GetMouseX(), InViewport->GetMouseY())
            : TOptional<FIntPoint>{};
        if (! StartCell.IsSet())
        { return Super::StartTracking(InViewportClient, InViewport); }

        _BlockerDragStart   = StartCell;
        _BlockerDragCurrent = StartCell;
        return true;
    }

    if (_ActiveTool != ECk_GridPaint_Tool::Shape && _ActiveTool != ECk_GridPaint_Tool::Tags)
    { return Super::StartTracking(InViewportClient, InViewport); }

    // Capture the erase direction for the WHOLE stroke up front (so releasing Shift mid-drag doesn't flip
    // add↔erase part-way through). Plain LMB adds; Shift+LMB erases.
    _StrokeErase = Is_EraseModifier(InViewportClient);

    // Open ONE transaction for the whole drag stroke so a single Ctrl-Z undoes every cell painted
    // during the drag. Closed in EndTracking. Labelled by direction (Paint vs Erase).
    const auto StrokeLabel = _StrokeErase
        ? NSLOCTEXT("Ck_2dGridSystem_EdMode", "EraseStroke", "Grid Erase: Cells")
        : NSLOCTEXT("Ck_2dGridSystem_EdMode", "PaintStroke", "Grid Paint: Cells");
    GEditor->BeginTransaction(StrokeLabel);

    _IsPaintingStroke = true;
    _StrokeToggledCells.Reset();
    _StrokeLastMousePixel.Reset();

    // Paint the cell under the press point immediately (CapturedMouseMove only fires once the mouse
    // actually moves).
    if (InViewport != nullptr)
    {
        Paint_StrokeAtCursor(InViewportClient,
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
        Paint_StrokeAtCursor(InViewportClient, InMouseX, InMouseY);
        return true;
    }

    // Blocker drag: update the rect's trailing corner so Render previews the candidate rect.
    if (_BlockerDragStart.IsSet())
    {
        const auto Cell = Resolve_CellAtCursor(InViewportClient, InMouseX, InMouseY);
        if (Cell.IsSet())
        { _BlockerDragCurrent = Cell; }
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
    // Blocker rect drag finished: commit ONE blocker spanning min..max of the start/current corners in
    // a single FScopedTransaction (no per-cell stroke txn was ever opened).
    if (_BlockerDragStart.IsSet())
    {
        const auto Selection = Resolve_SelectedGridSpawner();
        const auto Start     = _BlockerDragStart.GetValue();
        const auto Current   = _BlockerDragCurrent.IsSet() ? _BlockerDragCurrent.GetValue() : Start;

        // NOTE: _BlockerDragStart is intentionally LEFT SET here — the trailing HandleClick (which
        // fires after EndTracking on a click-drag) consults it to avoid mis-reading the drag as a
        // select-click, then clears it.
        if (Selection.IsValid())
        {
            const auto RangeMin = FIntPoint(FMath::Min(Start.X, Current.X), FMath::Min(Start.Y, Current.Y));
            const auto RangeMax = FIntPoint(FMath::Max(Start.X, Current.X), FMath::Max(Start.Y, Current.Y));

            const auto Transaction = FScopedTransaction(
                NSLOCTEXT("Ck_2dGridSystem_EdMode", "PlaceBlocker", "Grid Paint: Place Blocker"));

            auto NewBlocker     = FCk_2dGridSystem_Spec_Blocker{};
            NewBlocker.RangeMin = RangeMin;
            NewBlocker.RangeMax = RangeMax;
            // Stamp the active blocker tag onto the new entry (invalid = anonymous). Inside the same
            // transaction as the append below so a single undo reverts both.
            NewBlocker.Name     = _ActiveBlockerTag;

            auto* Spec = Selection.Spec;
            Spec->Modify();
            Spec->Blockers.Add(NewBlocker);
            Selection.Spawner->EditorOnly_RebuildEntity();
        }

        return true;
    }

    if (! _IsPaintingStroke)
    { return Super::EndTracking(InViewportClient, InViewport); }

    _IsPaintingStroke = false;
    GEditor->EndTransaction();

    // Keep the cursor on the cell where the stroke ENDED instead of letting the OS snap it back to the press
    // point when high-precision raw-mouse mode releases. Deferred to next tick so it lands after that reset.
    if (_StrokeLastMousePixel.IsSet())
    {
        Schedule_RestoreCursorToStrokeEnd(InViewport, _StrokeLastMousePixel.GetValue());
        _StrokeLastMousePixel.Reset();
    }

    const auto bToggledAny = ! _StrokeToggledCells.IsEmpty();
    // NOTE: not cleared here — HandleClick fires AFTER EndTracking on a click-drag and consults this
    // to avoid a double-apply. It's reset at the start of the next stroke (and stays harmless until
    // then because painting only reads it within an active stroke).
    return bToggledAny;
}

bool
    UCk_2dGridSystem_EdMode::
    DisallowMouseDeltaTracking() const
{
    // While a per-cell stroke (Shape/Tags) OR a Blocker rect drag is active, suppress the gizmo/camera
    // delta-tracker so the drag paints/draws instead of moving the selected actor.
    return _IsPaintingStroke || _BlockerDragStart.IsSet();
}

bool
    UCk_2dGridSystem_EdMode::
    InputKey(
        FEditorViewportClient* InViewportClient,
        FViewport*             InViewport,
        FKey                   InKey,
        EInputEvent            InEvent)
{
    // Delete the selected blocker on Delete (or platform Delete) while the Blocker tool is active.
    const auto bIsDelete = InKey == EKeys::Delete || InKey == EKeys::Platform_Delete;
    if (InEvent == IE_Pressed && bIsDelete &&
        _ActiveTool == ECk_GridPaint_Tool::Blocker &&
        _SelectedBlockerIndex != INDEX_NONE)
    {
        const auto Selection = Resolve_SelectedGridSpawner();
        if (Selection.IsValid() && Selection.Spec->Blockers.IsValidIndex(_SelectedBlockerIndex))
        {
            const auto Transaction = FScopedTransaction(
                NSLOCTEXT("Ck_2dGridSystem_EdMode", "DeleteBlocker", "Grid Paint: Delete Blocker"));

            auto* Spec = Selection.Spec;
            Spec->Modify();
            Spec->Blockers.RemoveAt(_SelectedBlockerIndex);
            Selection.Spawner->EditorOnly_RebuildEntity();
        }

        _SelectedBlockerIndex = INDEX_NONE;
        return true;
    }

    return Super::InputKey(InViewportClient, InViewport, InKey, InEvent);
}

void
    UCk_2dGridSystem_EdMode::
    ActorSelectionChangeNotify()
{
    // The blocker selection indexes into a specific grid's Blockers array; invalidate it whenever the
    // editor's actor selection changes (best-effort — covers picking a different spawner/deselecting).
    _SelectedBlockerIndex = INDEX_NONE;
    _BlockerDragStart.Reset();
    _BlockerDragCurrent.Reset();

    // The inspected cell is meaningful only against a specific grid; drop it when the actor selection
    // changes (covers picking a different spawner / deselecting).
    _SelectedCell.Reset();

    Super::ActorSelectionChangeNotify();
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
