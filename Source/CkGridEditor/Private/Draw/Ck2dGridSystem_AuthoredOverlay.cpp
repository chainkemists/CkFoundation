#include "CkGridEditor/Draw/Ck2dGridSystem_AuthoredOverlay.h"

#include "CkGrid/2dGridSystem/Authoring/Ck2dGridSystem_EntityScript.h"
#include "CkGrid/2dGridSystem/Authoring/Ck2dGridSystem_Spec.h"

#include "CkEntitySpawner/CkEntitySpawner_Actor.h"

#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Engine.h"
#include "SceneManagement.h"
#include "SceneView.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::grid_editor
{
    // The per-cell tag-text toggle. Default off — labels are noisy on large grids. Read live by both the
    // UEdMode (DrawHUD) and the spawner visualizer (DrawVisualizationHUD) so the two views agree.
    static TAutoConsoleVariable<bool> CVar_GridPreviewShowTags(
        TEXT("ck.Grid.PreviewShowTags"),
        false,
        TEXT("When true, draws each grid cell's authored tags as world-space text in the editor preview ")
        TEXT("(both in Grid Paint mode and out of it). Default off (noisy on large grids)."),
        ECVF_Default);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::grid_editor::
    Resolve_SpecFromSpawner(
        const ACk_EntitySpawner_UE* InSpawner) -> UCk_2dGridSystem_Spec*
{
    if (InSpawner == nullptr)
    { return nullptr; }

    const auto& EntityScript = InSpawner->Get_EntityScript();
    auto* GridScript = Cast<UCk_2dGridSystem_EntityScript>(EntityScript);
    if (GridScript == nullptr)
    { return nullptr; }

    // The Spec is a private UPROPERTY on the grid script; read it reflectively (no public getter).
    const auto* SpecProperty = FindFProperty<FObjectProperty>(
        UCk_2dGridSystem_EntityScript::StaticClass(), TEXT("Spec"));
    if (SpecProperty == nullptr)
    { return nullptr; }

    return Cast<UCk_2dGridSystem_Spec>(
        SpecProperty->GetObjectPropertyValue_InContainer(GridScript));
}

auto
    ck::grid_editor::
    Is_CoveredByBlocker(
        const UCk_2dGridSystem_Spec* InSpec,
        const FIntPoint&             InCoordinate) -> bool
{
    if (InSpec == nullptr)
    { return false; }

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

auto
    ck::grid_editor::
    Has_AuthoredTag(
        const UCk_2dGridSystem_Spec* InSpec,
        const FIntPoint&             InCoordinate) -> bool
{
    if (InSpec == nullptr)
    { return false; }

    if (! InSpec->DefaultCellTags.IsEmpty())
    { return true; }

    if (const auto* PerCell = InSpec->PerCellTags.Find(InCoordinate))
    { return ! PerCell->IsEmpty(); }

    return false;
}

auto
    ck::grid_editor::
    Resolve_CellColor(
        const UCk_2dGridSystem_Spec* InSpec,
        const FIntPoint&             InCoordinate) -> FLinearColor
{
    if (InSpec == nullptr)
    { return ColorEnabled; }

    if (InSpec->DisabledCells.Contains(InCoordinate))
    { return ColorDisabled; }

    if (Is_CoveredByBlocker(InSpec, InCoordinate))
    { return ColorBlocker; }

    if (Has_AuthoredTag(InSpec, InCoordinate))
    { return ColorTagged; }

    return ColorEnabled;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::grid_editor::
    Draw_GridAuthoredOverlay(
        FPrimitiveDrawInterface*       InPDI,
        const UCk_2dGridSystem_Spec*   InSpec,
        const FTransform&              InTransform,
        const FAuthoredOverlayOptions& InOptions) -> void
{
    if (InPDI == nullptr || InSpec == nullptr)
    { return; }

    const auto CellSize   = InSpec->CellSize;
    const auto Dimensions = InSpec->Dimensions;

    if (CellSize.X <= 0.0 || CellSize.Y <= 0.0 || Dimensions.X <= 0 || Dimensions.Y <= 0)
    { return; }

    const auto LocalToWorld = [&](double InLocalX, double InLocalY) -> FVector
    {
        return InTransform.TransformPosition(FVector(InLocalX, InLocalY, 0.0));
    };

    // Square outline for cell (InX,InY), inset from the cell bounds by InInsetFraction on every side
    // (0 = full cell edges). The inset variant keeps the state marker off the green base-grid lines so a
    // coincident-line z-fight can't hide it (that was the partial-coloring bug in the original overlay).
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

    // Pass 1: base grid as spanning gridlines, each drawn ONCE (Dimensions+1 lines per axis) rather than
    // a per-cell square. A per-cell square redraws every interior edge twice (once per adjacent cell);
    // coincident PDI lines composite brighter, and that brightening grows with grid size — which read as
    // a "different green" on larger grids. Drawing each edge exactly once removes the size dependence.
    if (InOptions.bDrawBaseGrid)
    {
        const auto MaxLocalX = Dimensions.X * CellSize.X;
        const auto MaxLocalY = Dimensions.Y * CellSize.Y;

        for (auto X = 0; X <= Dimensions.X; ++X)
        {
            const auto LineX = X * CellSize.X;
            InPDI->DrawLine(LocalToWorld(LineX, 0.0), LocalToWorld(LineX, MaxLocalY),
                ColorEnabled, SDPG_Foreground, CellLineThickness);
        }
        for (auto Y = 0; Y <= Dimensions.Y; ++Y)
        {
            const auto LineY = Y * CellSize.Y;
            InPDI->DrawLine(LocalToWorld(0.0, LineY), LocalToWorld(MaxLocalX, LineY),
                ColorEnabled, SDPG_Foreground, CellLineThickness);
        }
    }

    // Pass 2: mark each non-enabled cell with an INSET square in its state color.
    if (InOptions.bDrawStateMarkers)
    {
        for (auto Y = 0; Y < Dimensions.Y; ++Y)
        {
            for (auto X = 0; X < Dimensions.X; ++X)
            {
                const auto Color = Resolve_CellColor(InSpec, FIntPoint(X, Y));
                if (Color != ColorEnabled)
                { DrawCellSquare(X, Y, Color, CellMarkerInset, CellMarkerThickness); }
            }
        }
    }

    // Pivot marker at the grid-local origin (cell (0,0) min corner).
    if (InOptions.bDrawPivot)
    {
        const auto PivotWorld = LocalToWorld(0.0, 0.0);
        const auto Half       = PivotMarkerSize;
        InPDI->DrawLine(PivotWorld - FVector(Half, 0, 0), PivotWorld + FVector(Half, 0, 0),
            ColorPivot, SDPG_Foreground, PivotLineThickness);
        InPDI->DrawLine(PivotWorld - FVector(0, Half, 0), PivotWorld + FVector(0, Half, 0),
            ColorPivot, SDPG_Foreground, PivotLineThickness);
        InPDI->DrawLine(PivotWorld - FVector(0, 0, Half), PivotWorld + FVector(0, 0, Half),
            ColorPivot, SDPG_Foreground, PivotLineThickness);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::grid_editor::
    Should_ShowCellTags() -> bool
{
    return CVar_GridPreviewShowTags.GetValueOnAnyThread();
}

auto
    ck::grid_editor::
    Get_CellTagText(
        const UCk_2dGridSystem_Spec* InSpec,
        const FIntPoint&             InCoordinate) -> FString
{
    if (InSpec == nullptr)
    { return {}; }

    auto Effective = InSpec->DefaultCellTags;
    if (const auto* PerCell = InSpec->PerCellTags.Find(InCoordinate))
    { Effective.AppendTags(*PerCell); }

    if (Effective.IsEmpty())
    { return {}; }

    return Effective.ToStringSimple();
}

auto
    ck::grid_editor::
    Draw_GridTagLabels(
        FCanvas*                     InCanvas,
        const FSceneView*            InView,
        const UCk_2dGridSystem_Spec* InSpec,
        const FTransform&            InTransform) -> void
{
    if (InCanvas == nullptr || InView == nullptr || InSpec == nullptr)
    { return; }

    if (! Should_ShowCellTags())
    { return; }

    const auto CellSize   = InSpec->CellSize;
    const auto Dimensions = InSpec->Dimensions;

    if (CellSize.X <= 0.0 || CellSize.Y <= 0.0 || Dimensions.X <= 0 || Dimensions.Y <= 0)
    { return; }

    auto* Font = GEngine != nullptr ? GEngine->GetTinyFont() : nullptr;
    if (Font == nullptr)
    { return; }

    for (auto Y = 0; Y < Dimensions.Y; ++Y)
    {
        for (auto X = 0; X < Dimensions.X; ++X)
        {
            const auto Coordinate = FIntPoint(X, Y);

            const auto TagText = Get_CellTagText(InSpec, Coordinate);
            if (TagText.IsEmpty())
            { continue; }

            // Cell-center in world (z=0 plane of the grid-local space).
            const auto CenterLocalX = (X + 0.5) * CellSize.X;
            const auto CenterLocalY = (Y + 0.5) * CellSize.Y;
            const auto CenterWorld  = InTransform.TransformPosition(FVector(CenterLocalX, CenterLocalY, 0.0));

            // Project to screen pixels; skip cells behind the camera / off-screen.
            const auto ScreenPos = InView->WorldToScreen(CenterWorld);
            if (ScreenPos.W <= 0.0f)
            { continue; }

            auto PixelPos = FVector2D::ZeroVector;
            if (! InView->ScreenToPixel(ScreenPos, PixelPos))
            { continue; }

            auto TextItem = FCanvasTextItem(PixelPos, FText::FromString(TagText), Font, FLinearColor::White);
            TextItem.bCentreX     = true;
            TextItem.bOutlined    = true;
            TextItem.OutlineColor = FLinearColor::Black;
            InCanvas->DrawItem(TextItem);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
