#include "CkGridEditor/EdMode/Ck2dGridSystem_EdMode.h"

#include "CkGridEditor_Log.h"

#include "CkGridEditor/EdMode/Ck2dGridSystem_EdModeToolkit.h"

#include "CkGrid/2dGridSystem/Authoring/Ck2dGridSystem_EntityScript.h"
#include "CkGrid/2dGridSystem/Authoring/Ck2dGridSystem_Spec.h"

#include "CkEntitySpawner/CkEntitySpawner_Actor.h"

#include "EditorModeManager.h"
#include "Engine/Selection.h"
#include "SceneManagement.h"
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

    constexpr auto CellLineThickness  = 1.0f;
    constexpr auto PivotMarkerSize     = 20.0f;
    constexpr auto PivotLineThickness  = 3.0f;

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

    const auto DrawCellEdges = [&](int32 InX, int32 InY, const FLinearColor& InColor)
    {
        const auto MinX = InX * CellSize.X;
        const auto MinY = InY * CellSize.Y;
        const auto MaxX = (InX + 1) * CellSize.X;
        const auto MaxY = (InY + 1) * CellSize.Y;

        const auto C00 = LocalToWorld(MinX, MinY);
        const auto C10 = LocalToWorld(MaxX, MinY);
        const auto C11 = LocalToWorld(MaxX, MaxY);
        const auto C01 = LocalToWorld(MinX, MaxY);

        InPDI->DrawLine(C00, C10, InColor, SDPG_Foreground, ck_grid_editor_detail::CellLineThickness);
        InPDI->DrawLine(C10, C11, InColor, SDPG_Foreground, ck_grid_editor_detail::CellLineThickness);
        InPDI->DrawLine(C11, C01, InColor, SDPG_Foreground, ck_grid_editor_detail::CellLineThickness);
        InPDI->DrawLine(C01, C00, InColor, SDPG_Foreground, ck_grid_editor_detail::CellLineThickness);
    };

    // Two passes so a special cell's color wins on the edges it shares with an enabled neighbour.
    // Each cell draws all four of its own edges; a shared edge is drawn by both adjacent cells, and
    // the later draw wins. Pass 1 draws every enabled (green) cell; pass 2 redraws the non-enabled
    // cells in their state color LAST, so a disabled/blocker/tagged cell's full outline is colored
    // (otherwise an adjacent green cell drawn afterwards would overwrite its top/left edges).
    for (auto Y = 0; Y < Dimensions.Y; ++Y)
    {
        for (auto X = 0; X < Dimensions.X; ++X)
        {
            const auto Color = ck_grid_editor_detail::Resolve_CellColor(Spec, FIntPoint(X, Y));
            if (Color == ck_grid_editor_detail::ColorEnabled)
            { DrawCellEdges(X, Y, Color); }
        }
    }

    for (auto Y = 0; Y < Dimensions.Y; ++Y)
    {
        for (auto X = 0; X < Dimensions.X; ++X)
        {
            const auto Color = ck_grid_editor_detail::Resolve_CellColor(Spec, FIntPoint(X, Y));
            if (Color != ck_grid_editor_detail::ColorEnabled)
            { DrawCellEdges(X, Y, Color); }
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
}

// --------------------------------------------------------------------------------------------------------------------

bool
    UCk_2dGridSystem_EdMode::
    HandleClick(
        FEditorViewportClient* InViewportClient,
        HHitProxy*             InHitProxy,
        const FViewportClick&  InClick)
{
    // Paint behavior is implemented in the next task.
    return Super::HandleClick(InViewportClient, InHitProxy, InClick);
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
    // Paint behavior is implemented in the next task.
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
    // Hover/paint behavior is implemented in the next task.
    return false;
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
