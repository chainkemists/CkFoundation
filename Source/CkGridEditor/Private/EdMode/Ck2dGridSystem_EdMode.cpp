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
    // Interaction-overlay colors/metrics — PAINT-MODE ONLY. The authored-state colors and the base-grid +
    // state-marker draw live in the shared overlay (ck::grid_editor) so the in/out-of-mode previews match.
    // Every color below is deliberately distinct so overlapping highlights on one cell still read apart.

    constexpr auto ColorHover = FLinearColor(1.0f, 1.0f, 1.0f); // white

    constexpr auto ColorSelected         = FLinearColor(0.0f, 1.0f, 1.0f); // cyan
    constexpr auto SelectedMarkerInset     = 0.02;
    constexpr auto SelectedMarkerThickness = 4.0f;

    constexpr auto ColorBlockerDrag     = FLinearColor(0.0f, 1.0f, 1.0f);  // cyan
    constexpr auto ColorBlockerSelected = FLinearColor(1.0f, 0.75f, 0.2f); // bright orange

    constexpr auto BlockerDragThickness     = 3.0f;
    constexpr auto BlockerSelectedInset     = 0.04;
    constexpr auto BlockerSelectedThickness = 4.0f;

    constexpr auto ColorBlockerGroup     = FLinearColor(1.0f, 0.0f, 1.0f); // magenta
    constexpr auto BlockerGroupThickness = 5.0f;

    constexpr auto ColorTagSelected     = FLinearColor(1.0f, 1.0f, 1.0f); // white
    constexpr auto TagSelectedThickness = 4.0f;

    // Inset smaller than the state markers so the hover marker nests inside any marker on the same cell.
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

    // Every rect-drag tool (Shape, Tags, Blocker) tracks through _DragStart/_DragCurrent — a tool
    // switch always invalidates the in-flight rect.
    _DragStart.Reset();
    _DragCurrent.Reset();

    // _SelectedBlockerIndex is shared by the Blocker tool (place/select/delete) and the Select tool (a pick
    // that lands on a blocker selects the whole group); it is meaningless under Shape/Tags.
    if (_ActiveTool != ECk_GridPaint_Tool::Blocker && _ActiveTool != ECk_GridPaint_Tool::Select)
    { _SelectedBlockerIndex = INDEX_NONE; }

    if (_ActiveTool != ECk_GridPaint_Tool::Select)
    {
        _SelectedCell.Reset();
        _SelectedTag.Reset();
    }
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

    // Corner-origin convention (matches UCk_Utils_Grid2D_UE::Get_CoordinateAsLocation).
    const auto LocalToWorld = [&](double InLocalX, double InLocalY) -> FVector
    {
        return Transform.TransformPosition(FVector(InLocalX, InLocalY, 0.0));
    };

    // An inset keeps a marker off the green base-grid lines — coincident SDPG_Foreground lines z-fight
    // order-independently, which used to hide it.
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

    // Authored-state passes come from the SHARED overlay so the in-mode and out-of-mode previews are
    // identical; only the interaction overlays below are paint-mode-only.
    ck::grid_editor::Draw_GridAuthoredOverlay(InPDI, Spec, Transform);

    // Blocker tool's per-cell selected-blocker emphasis, drawn before hover so the white hover marker
    // still reads on top. The Select tool uses the magenta GROUP outline further below instead.
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

    if (_DragStart.IsSet())
    {
        const auto Start   = _DragStart.GetValue();
        const auto Current = _DragCurrent.IsSet() ? _DragCurrent.GetValue() : Start;

        const auto MinX = FMath::Min(Start.X, Current.X);
        const auto MaxX = FMath::Max(Start.X, Current.X);
        const auto MinY = FMath::Min(Start.Y, Current.Y);
        const auto MaxY = FMath::Max(Start.Y, Current.Y);

        const auto LoX = MinX       * CellSize.X;
        const auto LoY = MinY       * CellSize.Y;
        const auto HiX = (MaxX + 1) * CellSize.X;
        const auto HiY = (MaxY + 1) * CellSize.Y;

        const auto C00 = LocalToWorld(LoX, LoY);
        const auto C10 = LocalToWorld(HiX, LoY);
        const auto C11 = LocalToWorld(HiX, HiY);
        const auto C01 = LocalToWorld(LoX, HiY);

        auto PreviewColor = ck_grid_editor_detail::ColorBlockerDrag;
        switch (_ActiveTool)
        {
            case ECk_GridPaint_Tool::Shape:
            { PreviewColor = _DragErase ? ck::grid_editor::ColorEnabled : ck::grid_editor::ColorDisabled; break; }
            case ECk_GridPaint_Tool::Tags:
            { PreviewColor = _DragErase ? ck::grid_editor::ColorDisabled : ck::grid_editor::Resolve_TagColor(_ActivePaintTag); break; }
            default: break; // Blocker keeps cyan
        }

        InPDI->DrawLine(C00, C10, PreviewColor, SDPG_Foreground, ck_grid_editor_detail::BlockerDragThickness);
        InPDI->DrawLine(C10, C11, PreviewColor, SDPG_Foreground, ck_grid_editor_detail::BlockerDragThickness);
        InPDI->DrawLine(C11, C01, PreviewColor, SDPG_Foreground, ck_grid_editor_detail::BlockerDragThickness);
        InPDI->DrawLine(C01, C00, PreviewColor, SDPG_Foreground, ck_grid_editor_detail::BlockerDragThickness);
    }

    // Select-tool whole-blocker outline; under the Blocker tool the orange per-cell inset conveys it instead.
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

    // Tag-group highlight; mutually exclusive with cell/blocker selection (Set_SelectedTag clears those).
    if (_ActiveTool == ECk_GridPaint_Tool::Select && _SelectedTag.IsSet())
    {
        for (const auto& Cell : ck::grid_editor::Get_CellsWithTag(Spec, _SelectedTag.GetValue()))
        {
            if (Cell.X >= 0 && Cell.X < Dimensions.X && Cell.Y >= 0 && Cell.Y < Dimensions.Y)
            {
                DrawCellSquare(Cell.X, Cell.Y, ck_grid_editor_detail::ColorTagSelected,
                    /*Inset*/ 0.0, ck_grid_editor_detail::TagSelectedThickness);
            }
        }
    }

    // Single-cell inspection highlight, suppressed when the pick resolved to a blocker (the magenta group
    // outline above is the selection indicator then). Inset less than the state markers so it frames them.
    if (_SelectedCell.IsSet() && ! bSelectBlockerGroup)
    {
        const auto& Cell = _SelectedCell.GetValue();
        if (Cell.X >= 0 && Cell.X < Dimensions.X && Cell.Y >= 0 && Cell.Y < Dimensions.Y)
        {
            DrawCellSquare(Cell.X, Cell.Y, ck_grid_editor_detail::ColorSelected,
                ck_grid_editor_detail::SelectedMarkerInset, ck_grid_editor_detail::SelectedMarkerThickness);
        }
    }

    // Drawn last so the hover marker sits on top of every other marker.
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

    // Canonical engine paint-mode deproject; mirrors FEdModeFoliage::MouseMove / CapturedMouseMove.
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

    // Ortho puts the origin on the near plane — push it back so Resolve_CellFromRay's behind-origin reject
    // does not swallow the hit.
    if (InViewportClient->IsOrtho())
    { OutRayOrigin += -WORLD_MAX * OutRayDirection; }

    return true;
}

auto
    UCk_2dGridSystem_EdMode::
    Set_ShapeCellDisabled(
        const FResolvedGridSelection& InSelection,
        const FIntPoint&              InCell,
        bool                          InDisabled) -> bool
{
    if (! InSelection.IsValid())
    { return false; }

    auto* Spec = InSelection.Spec;

    const auto bAlreadyDisabled = Spec->DisabledCells.Contains(InCell);
    if (bAlreadyDisabled == InDisabled)
    { return false; }

    Spec->Modify();

    if (InDisabled)
    { Spec->DisabledCells.Add(InCell); }
    else
    { Spec->DisabledCells.RemoveSingleSwap(InCell); }

    return true;
}

auto
    UCk_2dGridSystem_EdMode::
    Set_TagCell(
        const FResolvedGridSelection& InSelection,
        const FIntPoint&              InCell,
        bool                          InAdd) -> bool
{
    // Invalid tag is a silent no-op here — the single warning is emitted by the batch caller (Apply_RectFill).
    if (! InSelection.IsValid() || ! _ActivePaintTag.IsValid())
    { return false; }

    auto* Spec = InSelection.Spec;

    if (InAdd)
    {
        if (const auto* Existing = Spec->PerCellTags.Find(InCell);
            Existing != nullptr && Existing->HasTagExact(_ActivePaintTag))
        { return false; }

        Spec->Modify();
        auto& Container = Spec->PerCellTags.FindOrAdd(InCell);
        Container.AddTag(_ActivePaintTag);
        return true;
    }

    auto* Container = Spec->PerCellTags.Find(InCell);
    if (Container == nullptr || ! Container->HasTagExact(_ActivePaintTag))
    { return false; }

    Spec->Modify();
    Container->RemoveTag(_ActivePaintTag);

    // No entry == no overrides: drop the map entry so empty containers never accumulate.
    if (Container->IsEmpty())
    { Spec->PerCellTags.Remove(InCell); }

    return true;
}

auto
    UCk_2dGridSystem_EdMode::
    Paint_Cell(
        const FResolvedGridSelection& InSelection,
        const FIntPoint&              InCell,
        bool                          InErase) -> bool
{
    switch (_ActiveTool)
    {
        case ECk_GridPaint_Tool::Shape:
        { return Set_ShapeCellDisabled(InSelection, InCell, ! InErase); }
        case ECk_GridPaint_Tool::Tags:
        { return Set_TagCell(InSelection, InCell, ! InErase); }
        default:
        { return false; }
    }
}

auto
    UCk_2dGridSystem_EdMode::
    Apply_RectFill(
        const FResolvedGridSelection& InSelection,
        const FIntPoint&              InMin,
        const FIntPoint&              InMax,
        bool                          InErase) -> void
{
    if (! InSelection.IsValid())
    { return; }

    // Tags tool needs an active tag; warn ONCE (not per cell) and bail.
    if (_ActiveTool == ECk_GridPaint_Tool::Tags && ! _ActivePaintTag.IsValid())
    {
        ck::grid_editor::Warning(TEXT("Tags tool: no active paint tag set — skipping rect fill"));
        return;
    }

    const auto Cells = ck::grid_editor::Compute_RectCells(InMin, InMax, InSelection.Spec->Dimensions);

    auto bAnyChange = false;
    for (const auto& Cell : Cells)
    { bAnyChange |= Paint_Cell(InSelection, Cell, InErase); }

    if (bAnyChange)
    { InSelection.Spawner->EditorOnly_RebuildEntity(); }
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

    if (InViewportClient->IsCtrlPressed() || InViewportClient->IsAltPressed())
    { return false; }

    // RMB must be up: LMB+RMB is the camera pan gesture.
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

    // No entry == no overrides: drop the map entry so empty containers never accumulate.
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
    // A click that turned into a drag was already committed by EndTracking, which intentionally leaves
    // _DragStart set so this trailing HandleClick swallows the click instead of applying it a second time.
    if (_DragStart.IsSet())
    {
        _DragStart.Reset();
        _DragCurrent.Reset();
        return true;
    }

    if (! Is_PlainLeftClick(InClick))
    { return Super::HandleClick(InViewportClient, InHitProxy, InClick); }

    // Blocker and Select do not honor the erase modifier, so a Shift+LMB falls through to camera nav
    // rather than mis-firing a place/select.
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

    // A single click is a 1x1 rect fill, so click and drag share one commit path and one undo step.
    const auto bErase = Is_EraseModifier(InClick);

    const auto TransactionLabel = bErase
        ? NSLOCTEXT("Ck_2dGridSystem_EdMode", "EraseCell", "Grid Erase: Cell")
        : NSLOCTEXT("Ck_2dGridSystem_EdMode", "PaintCell", "Grid Paint: Cell");

    const auto Transaction = FScopedTransaction(TransactionLabel);
    Apply_RectFill(Selection, Cell.GetValue(), Cell.GetValue(), bErase);

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
        _SelectedBlockerIndex = INDEX_NONE;
        return true;
    }

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

    _SelectedCell = Cell;

    // Blocker precedence: a pick that lands on a blocker switches the Details panel to the blocker editor.
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
    // Drag painting flows through CapturedMouseMove; nothing to handle here.
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

    // Not handled — the move is only observed for the highlight; the base mode keeps processing it.
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

    if (! Is_PlainLeftDrag(InViewportClient, InViewport))
    { return Super::StartTracking(InViewportClient, InViewport); }

    // The Select tool is read-only; its pick happens in HandleClick, never a drag.
    if (_ActiveTool == ECk_GridPaint_Tool::Select)
    { return Super::StartTracking(InViewportClient, InViewport); }

    if (_ActiveTool != ECk_GridPaint_Tool::Shape &&
        _ActiveTool != ECk_GridPaint_Tool::Tags &&
        _ActiveTool != ECk_GridPaint_Tool::Blocker)
    { return Super::StartTracking(InViewportClient, InViewport); }

    // Blocker does not honor the erase modifier — a Shift+LMB drag falls through to camera nav.
    if (_ActiveTool == ECk_GridPaint_Tool::Blocker && InViewportClient->IsShiftPressed())
    { return Super::StartTracking(InViewportClient, InViewport); }

    const auto StartCell = (InViewport != nullptr)
        ? Resolve_CellAtCursor(InViewportClient, InViewport->GetMouseX(), InViewport->GetMouseY())
        : TOptional<FIntPoint>{};
    if (! StartCell.IsSet())
    { return Super::StartTracking(InViewportClient, InViewport); }

    // Capture the erase direction up-front so releasing Shift mid-drag cannot flip add<->erase. No
    // transaction is opened here — the commit is transacted in EndTracking.
    _DragErase   = (_ActiveTool != ECk_GridPaint_Tool::Blocker) && Is_EraseModifier(InViewportClient);
    _DragStart   = StartCell;
    _DragCurrent = StartCell;
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
    if (_DragStart.IsSet())
    {
        const auto Cell = Resolve_CellAtCursor(InViewportClient, InMouseX, InMouseY);
        if (Cell.IsSet())
        { _DragCurrent = Cell; }
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
    if (! _DragStart.IsSet())
    { return Super::EndTracking(InViewportClient, InViewport); }

    const auto Selection = Resolve_SelectedGridSpawner();
    const auto Start     = _DragStart.GetValue();
    const auto Current   = _DragCurrent.IsSet() ? _DragCurrent.GetValue() : Start;

    const auto RangeMin = FIntPoint(FMath::Min(Start.X, Current.X), FMath::Min(Start.Y, Current.Y));
    const auto RangeMax = FIntPoint(FMath::Max(Start.X, Current.X), FMath::Max(Start.Y, Current.Y));

    // NOTE: _DragStart is intentionally LEFT SET — the trailing HandleClick (fires after EndTracking on a
    // click-drag) consults it to swallow the click, then clears it.
    if (Selection.IsValid())
    {
        switch (_ActiveTool)
        {
            case ECk_GridPaint_Tool::Blocker:
            {
                const auto Transaction = FScopedTransaction(
                    NSLOCTEXT("Ck_2dGridSystem_EdMode", "PlaceBlocker", "Grid Paint: Place Blocker"));

                auto NewBlocker     = FCk_2dGridSystem_Spec_Blocker{};
                NewBlocker.RangeMin = RangeMin;
                NewBlocker.RangeMax = RangeMax;
                NewBlocker.Name     = _ActiveBlockerTag; // invalid = anonymous

                auto* Spec = Selection.Spec;
                Spec->Modify();
                Spec->Blockers.Add(NewBlocker);
                Selection.Spawner->EditorOnly_RebuildEntity();
                break;
            }
            case ECk_GridPaint_Tool::Shape:
            case ECk_GridPaint_Tool::Tags:
            {
                const auto Label = _DragErase
                    ? NSLOCTEXT("Ck_2dGridSystem_EdMode", "EraseCells", "Grid Erase: Cells")
                    : NSLOCTEXT("Ck_2dGridSystem_EdMode", "PaintCells", "Grid Paint: Cells");

                const auto Transaction = FScopedTransaction(Label);
                Apply_RectFill(Selection, RangeMin, RangeMax, _DragErase);
                break;
            }
            default: break;
        }
    }

    return true;
}

bool
    UCk_2dGridSystem_EdMode::
    DisallowMouseDeltaTracking() const
{
    // Suppress the gizmo/camera delta-tracker mid-drag so the LMB drag draws the rect instead of moving
    // the selected actor.
    return _DragStart.IsSet();
}

bool
    UCk_2dGridSystem_EdMode::
    InputKey(
        FEditorViewportClient* InViewportClient,
        FViewport*             InViewport,
        FKey                   InKey,
        EInputEvent            InEvent)
{
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
    // Every selection below indexes into ONE grid's Spec, so a change of actor selection invalidates them.
    _SelectedBlockerIndex = INDEX_NONE;
    _DragStart.Reset();
    _DragCurrent.Reset();

    _SelectedCell.Reset();
    _SelectedTag.Reset();

    Super::ActorSelectionChangeNotify();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_2dGridSystem_EdMode::
    Get_SelectedSpec() const -> UCk_2dGridSystem_Spec*
{
    return Resolve_SelectedGridSpawner().Spec;
}

auto
    UCk_2dGridSystem_EdMode::
    Set_SelectedBlockerIndex(
        int32 InIndex) -> void
{
    const auto Selection = Resolve_SelectedGridSpawner();
    const auto bValid = Selection.IsValid() && Selection.Spec->Blockers.IsValidIndex(InIndex);

    _SelectedBlockerIndex = bValid ? InIndex : INDEX_NONE;
    if (bValid)
    {
        _SelectedTag.Reset();
    }
}

auto
    UCk_2dGridSystem_EdMode::
    Set_SelectedTag(
        const TOptional<FGameplayTag>& InTag) -> void
{
    _SelectedTag = (InTag.IsSet() && InTag.GetValue().IsValid()) ? InTag : TOptional<FGameplayTag>{};
    if (_SelectedTag.IsSet())
    {
        _SelectedCell.Reset();
        _SelectedBlockerIndex = INDEX_NONE;
    }
}

auto
    UCk_2dGridSystem_EdMode::
    Set_GridDimensions(
        FIntPoint InDimensions) -> void
{
    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    { return; }

    const auto Clamped = FIntPoint(FMath::Max(1, InDimensions.X), FMath::Max(1, InDimensions.Y));

    auto* Spec = Selection.Spec;
    if (Spec->Dimensions == Clamped)
    { return; }

    const auto Transaction = FScopedTransaction(
        NSLOCTEXT("Ck_2dGridSystem_EdMode", "SetGridDimensions", "Grid Paint: Set Dimensions"));

    Spec->Modify();
    Spec->Dimensions = Clamped;
    Selection.Spawner->EditorOnly_RebuildEntity();
}

auto
    UCk_2dGridSystem_EdMode::
    Set_GridCellSize(
        FVector2D InCellSize) -> void
{
    const auto Selection = Resolve_SelectedGridSpawner();
    if (! Selection.IsValid())
    { return; }

    constexpr auto MinCellSize = 1.0;
    const auto Clamped = FVector2D(FMath::Max(MinCellSize, InCellSize.X), FMath::Max(MinCellSize, InCellSize.Y));

    auto* Spec = Selection.Spec;
    if (Spec->CellSize.Equals(Clamped))
    { return; }

    const auto Transaction = FScopedTransaction(
        NSLOCTEXT("Ck_2dGridSystem_EdMode", "SetGridCellSize", "Grid Paint: Set Cell Size"));

    Spec->Modify();
    Spec->CellSize = Clamped;
    Selection.Spawner->EditorOnly_RebuildEntity();
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
