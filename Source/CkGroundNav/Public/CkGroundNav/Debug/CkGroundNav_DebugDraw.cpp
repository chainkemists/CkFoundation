#include "CkGroundNav_DebugDraw.h"

#include "CkGroundNav/Backend/CkGroundNav_GeometryBackend_Jolt.h"
#include "CkGroundNav/Bake/CkGroundNav_Clearance.h"
#include "CkGroundNav/Bake/CkGroundNav_Layers.h"
#include "CkGroundNav/Bake/CkGroundNav_Plates.h"
#include "CkGroundNav/Bake/CkGroundNav_Rasterize.h"
#include "CkGroundNav/Bake/CkGroundNav_Walkability.h"
#include "CkGroundNav/CkGroundNav_Log.h"

#include "CkShapes/Capsule/CkShapeCapsule_Fragment_Data.h"

#include <DrawDebugHelpers.h>
#include <Engine/World.h>
#include <GameFramework/Pawn.h>
#include <GameFramework/PlayerController.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace debugdraw_private
    {
        // Distinct at a glance rather than pretty: adjacent plates must never read as one plate.
        auto Get_LayerColor(int32 InLayerIndex) -> FColor
        {
            static const FColor Palette[] = {
                FColor{ 80, 200, 120},
                FColor{ 90, 150, 255},
                FColor{255, 190,  70},
                FColor{225, 110, 220},
                FColor{110, 230, 230},
                FColor{255, 130, 110}};

            return Palette[FMath::Abs(InLayerIndex) % UE_ARRAY_COUNT(Palette)];
        }

        // Blue where an agent has no room, red where it has the most in this bake. The ramp is
        // relative to the snapshot, so a corridor reads the same whatever else is in the level.
        auto Get_ClearanceColor(float InClearanceUu, float InMaxClearanceUu) -> FColor
        {
            if (InMaxClearanceUu <= 0.0f)
            { return FColor::Silver; }

            const auto Alpha = FMath::Clamp(InClearanceUu / InMaxClearanceUu, 0.0f, 1.0f);

            return FLinearColor::LerpUsingHSV(
                FLinearColor{0.1f, 0.3f, 1.0f}, FLinearColor{1.0f, 0.2f, 0.1f}, Alpha).ToFColor(false);
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Make_DebugSnapshotFromWorld(
            const UObject*                       InWorldContextObject,
            const FCk_GroundNav_DebugBakeParams& InParams)
        -> FCk_GroundNav_DebugSnapshot
    {
        auto Snapshot = FCk_GroundNav_DebugSnapshot{};

        const auto Region = FBox::BuildAABB(InParams._Centre, InParams._Extent);
        Snapshot._Region = Region;
        Snapshot._CellSizeUu = InParams._Config.Get_CellSizeUu();

        const auto Backend = FCk_GroundNav_GeometryBackend_Jolt{InWorldContextObject};

        if (NOT Backend.Get_IsValid())
        {
            Snapshot._Status = EDebugSnapshotStatus::BackendUnavailable;
            return Snapshot;
        }

        const auto StartedAt = FPlatformTime::Seconds();

        auto Geometry = FCk_GroundNav_GeometryBatch{};
        const auto SourceTriangles = Backend.Get_TrianglesInBounds(Region, Geometry);

        if (Geometry.Get_IsEmpty())
        {
            Snapshot._SourceTriangleCount = SourceTriangles;
            Snapshot._Status = EDebugSnapshotStatus::NoGeometryInRegion;
            return Snapshot;
        }

        auto Spans = FCk_GroundNav_SpanField{};
        const auto RasterResult = DoRasterizeSpans(
            Geometry, Region, InParams._Config, InParams._Profile, Spans);

        if (NOT RasterResult.Get_IsCompleted())
        {
            Snapshot._SourceTriangleCount = SourceTriangles;
            Snapshot._Status = EDebugSnapshotStatus::Failed;
            return Snapshot;
        }

        // Kept so the filters can be judged by what they REMOVED, not only by what survived.
        const auto SpansBeforeFiltering = Spans;

        auto Connections = FCk_GroundNav_ConnectionField{};

        if (NOT DoFilter_Walkability(InParams._Profile, Spans, Connections).Get_IsCompleted())
        {
            Snapshot._SourceTriangleCount = SourceTriangles;
            Snapshot._Status = EDebugSnapshotStatus::Failed;
            return Snapshot;
        }

        auto Layers = FCk_GroundNav_LayerField{};

        if (NOT DoExtract_Layers(Spans, Connections, Layers).Get_IsCompleted())
        {
            Snapshot._SourceTriangleCount = SourceTriangles;
            Snapshot._Status = EDebugSnapshotStatus::Failed;
            return Snapshot;
        }

        auto Clearance = FCk_GroundNav_ClearanceField{};

        if (NOT DoCompute_Clearance(Layers, InParams._Config.Get_CellSizeUu(), Clearance).Get_IsCompleted())
        {
            Snapshot._SourceTriangleCount = SourceTriangles;
            Snapshot._Status = EDebugSnapshotStatus::Failed;
            return Snapshot;
        }

        auto Plates = FCk_GroundNav_PlateField{};

        if (NOT DoDecompose_Plates(Spans, Layers, InParams._MergeTunables, Plates).Get_IsCompleted())
        {
            Snapshot._SourceTriangleCount = SourceTriangles;
            Snapshot._Status = EDebugSnapshotStatus::Failed;
            return Snapshot;
        }

        const auto ElapsedMilliseconds = (FPlatformTime::Seconds() - StartedAt) * 1000.0;

        auto Built = Make_DebugSnapshot(Spans, Layers, Clearance, Plates, Region, InParams._MaxCells);
        Do_RecordRejectedCells(SpansBeforeFiltering, Spans, Built);

        Built._SourceTriangleCount = SourceTriangles;
        Built._DroppedTriangleCount = RasterResult.Get_DroppedInputCount();
        Built._BakeMilliseconds = ElapsedMilliseconds;

        return Built;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        DoDraw_DebugSnapshot(
            UWorld*                            InWorld,
            const FCk_GroundNav_DebugSnapshot& InSnapshot,
            EDebugDrawMode                     InMode,
            FCk_Time                           InLifetime)
        -> void
    {
        const auto WorldIsValid = ck::IsValid(InWorld);

        CK_ENSURE_IF_NOT(WorldIsValid, TEXT("Cannot draw a GroundNav debug snapshot without a World"))
        { return; }

        const auto LifetimeSeconds = static_cast<float>(InLifetime.Get_Seconds());

        constexpr auto Persistent = true;
        constexpr auto DepthPriority = 0;

        // The region always draws, whatever the status. A viewer that showed nothing for a failed
        // bake would be indistinguishable from one pointed at empty space.
        DrawDebugBox(InWorld, InSnapshot._Region.GetCenter(), InSnapshot._Region.GetExtent(),
            FColor::White, Persistent, LifetimeSeconds, DepthPriority, 2.0f);

        if (NOT InSnapshot.Get_IsDrawable())
        { return; }

        using namespace debugdraw_private;

        if (InMode == EDebugDrawMode::Plates)
        {
            for (const auto& Plate : InSnapshot._Plates)
            {
                // A perfectly flat plate has no thickness to draw, so give it just enough to be a box.
                const auto Extent = Plate._Bounds.GetExtent() + FVector{0.0, 0.0, 1.0};

                DrawDebugBox(InWorld, Plate._Bounds.GetCenter(), Extent,
                    Get_LayerColor(Plate._LayerIndex), Persistent, LifetimeSeconds, DepthPriority, 1.5f);
            }

            return;
        }

        const auto PointSize = FMath::Max(4.0f, InSnapshot._CellSizeUu * 0.35f);

        if (InMode == EDebugDrawMode::Rejected)
        {
            // What survived, dimmed, so the rejections are read against the ground they were cut from
            // rather than against an empty screen.
            for (const auto& Cell : InSnapshot._Cells)
            {
                DrawDebugPoint(InWorld, Cell._SurfaceCentre, PointSize * 0.6f, FColor{60, 70, 60},
                    Persistent, LifetimeSeconds, DepthPriority);
            }

            for (const auto& Cell : InSnapshot._RejectedCells)
            {
                DrawDebugPoint(InWorld, Cell._SurfaceCentre, PointSize, FColor::Red, Persistent,
                    LifetimeSeconds, DepthPriority);
            }

            return;
        }

        for (const auto& Cell : InSnapshot._Cells)
        {
            const auto Color = InMode == EDebugDrawMode::Clearance
                ? Get_ClearanceColor(Cell._ClearanceUu, InSnapshot._MaxClearanceUu)
                : Get_LayerColor(Cell._LayerIndex);

            DrawDebugPoint(InWorld, Cell._SurfaceCentre, PointSize, Color, Persistent, LifetimeSeconds,
                DepthPriority);
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_DebugSnapshotSummary(
            const FCk_GroundNav_DebugSnapshot& InSnapshot)
        -> FString
    {
        const auto Centre = InSnapshot._Region.GetCenter();
        const auto Extent = InSnapshot._Region.GetExtent();

        return FString::Printf(
            TEXT("[GroundNav] %s | %.1f ms\n")
            TEXT("  region   : centre (%.0f, %.0f, %.0f)  half-extent (%.0f, %.0f, %.0f)\n")
            TEXT("  lattice  : %d x %d columns at %.1f uu, %d layer(s) -> %d cell slots\n")
            TEXT("  geometry : %d triangles in, %d dropped\n")
            TEXT("  spans    : %d rasterized -> %d walkable cells%s, %d REJECTED by the filters\n")
            TEXT("  layers   : %d\n")
            TEXT("  plates   : %d (collapse %.1f cells/plate, worst residual %.2f uu, worst height spread %.2f uu)\n")
            TEXT("  clearance: %.1f uu at the most open cell"),
            Get_StatusName(InSnapshot._Status),
            InSnapshot._BakeMilliseconds,
            Centre.X, Centre.Y, Centre.Z,
            Extent.X, Extent.Y, Extent.Z,
            InSnapshot._LatticeSizeX,
            InSnapshot._LatticeSizeY,
            InSnapshot._CellSizeUu,
            InSnapshot._LayerCount,
            InSnapshot._LatticeSizeX * InSnapshot._LatticeSizeY * InSnapshot._LayerCount,
            InSnapshot._SourceTriangleCount,
            InSnapshot._DroppedTriangleCount,
            InSnapshot._SpanCount,
            InSnapshot._WalkableCellCount,
            InSnapshot._CellsWereTruncated ? TEXT(" (draw capped)") : TEXT(""),
            InSnapshot._RejectedCellCount,
            InSnapshot._LayerCount,
            InSnapshot.Get_PlateCount(),
            InSnapshot._CollapseRatio,
            InSnapshot._MaxPlaneResidualUu,
            InSnapshot._MaxPlateHeightRangeUu,
            InSnapshot._MaxClearanceUu);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_groundnav_debugconsole
{
    // ---- Region -----------------------------------------------------------------------------------

    static TAutoConsoleVariable<float> CVar_ExtentUu(
        TEXT("ck.GroundNav.Debug.ExtentUu"), 1500.0f,
        TEXT("Half-extent in XY of the region baked around the player. Bake cost grows with its SQUARE."));

    static TAutoConsoleVariable<float> CVar_HeightUu(
        TEXT("ck.GroundNav.Debug.HeightUu"), 500.0f,
        TEXT("Half-extent in Z. Must reach every floor you want to see; raise it for tall interiors."));

    // ---- Bake config ------------------------------------------------------------------------------

    static TAutoConsoleVariable<float> CVar_CellSizeUu(
        TEXT("ck.GroundNav.Debug.CellSizeUu"), 25.0f,
        TEXT("Lattice resolution. Halving it QUADRUPLES cell count and bake cost. It also sets the ")
        TEXT("smallest gap the field can resolve, so a doorway narrower than a couple of cells will ")
        TEXT("not read as passable."));

    static TAutoConsoleVariable<float> CVar_CellHeightUu(
        TEXT("ck.GroundNav.Debug.CellHeightUu"), 10.0f,
        TEXT("Vertical quantum of the bake. Mostly affects how finely stacked surfaces separate."));

    // ---- Agent profile ----------------------------------------------------------------------------

    static TAutoConsoleVariable<float> CVar_AgentHeightUu(
        TEXT("ck.GroundNav.Debug.AgentHeightUu"), 180.0f,
        TEXT("Standing height. Sets the headroom test: anything with less clearance above it is demoted."));

    static TAutoConsoleVariable<float> CVar_AgentRadiusUu(
        TEXT("ck.GroundNav.Debug.AgentRadiusUu"), 34.0f,
        TEXT("Capsule radius of the profile shape. Does NOT filter the bake - radius is answered at ")
        TEXT("query time against the clearance field, which is why one bake serves every agent size."));

    static TAutoConsoleVariable<float> CVar_MaxSlopeDegrees(
        TEXT("ck.GroundNav.Debug.MaxSlopeDegrees"), 45.0f,
        TEXT("Steepest surface an agent can stand on. Too high and walls become floors; too low and ")
        TEXT("ramps vanish."));

    static TAutoConsoleVariable<float> CVar_MaxSlopeChangeDegrees(
        TEXT("ck.GroundNav.Debug.MaxSlopeChangeDegrees"), 30.0f,
        TEXT("How sharply the surface may turn between two connected cells. Too low and a curved ")
        TEXT("floor fragments into disconnected pieces."));

    static TAutoConsoleVariable<float> CVar_StepHeightUu(
        TEXT("ck.GroundNav.Debug.StepHeightUu"), 40.0f,
        TEXT("Tallest step an agent can climb. Also the distance within which stacked surfaces merge ")
        TEXT("into one span, so raising it can swallow a low floor into the one above it."));

    static TAutoConsoleVariable<float> CVar_LedgeSensitivity(
        TEXT("ck.GroundNav.Debug.LedgeSensitivity"), 1.0f,
        TEXT("Reciprocal of how many of the four sides of a cell must drop away before it stops being ")
        TEXT("standable. 1.0 demotes on ONE dropping side (safest, but erases one-cell catwalks); ")
        TEXT("0.5 demands two; 0.34 demands three; 0 disables the filter. Use draw mode 3 to see ")
        TEXT("exactly what it is cutting."));

    static TAutoConsoleVariable<float> CVar_RoughPerchToleranceUu(
        TEXT("ck.GroundNav.Debug.RoughPerchToleranceUu"), 0.0f,
        TEXT("Height difference under which two neighbours connect regardless of how sharply their ")
        TEXT("normals disagree. Raise it if gently uneven ground fragments; 0 applies the slope-change ")
        TEXT("test everywhere."));

    // ---- Merge tunables ---------------------------------------------------------------------------

    static TAutoConsoleVariable<float> CVar_PlaneFitToleranceUu(
        TEXT("ck.GroundNav.Debug.PlaneFitToleranceUu"), 10.0f,
        TEXT("How far a cell may sit from the plane of its plate and still join it. MUST STAY BELOW ")
        TEXT("THE SHALLOWEST STEP YOU NEED PRESERVED - at or above a riser height, the treads either ")
        TEXT("side merge and the step stops existing. Below about 1 uu a long ramp fragments on normal ")
        TEXT("quantization alone. Watch the worst height spread in the summary."));

    static TAutoConsoleVariable<float> CVar_NormalConeDegrees(
        TEXT("ck.GroundNav.Debug.NormalConeDegrees"), 10.0f,
        TEXT("How far the normal of a cell may turn from that of its plate. Rarely binds - differing ")
        TEXT("normals usually diverge in height first and the plane-fit tolerance rejects them. Below ")
        TEXT("about 3 degrees it starts fragmenting nominally-flat ground on its own."));

    // ---- Presentation -----------------------------------------------------------------------------

    static TAutoConsoleVariable<int32> CVar_Mode(
        TEXT("ck.GroundNav.Debug.Mode"), 0,
        TEXT("0 = merged plates, 1 = clearance ramp, 2 = layers, 3 = cells the filters rejected."));

    static TAutoConsoleVariable<float> CVar_LifetimeSeconds(
        TEXT("ck.GroundNav.Debug.LifetimeSeconds"), 60.0f,
        TEXT("How long the drawn field persists."));

    static TAutoConsoleVariable<int32> CVar_MaxCells(
        TEXT("ck.GroundNav.Debug.MaxCells"), 20000,
        TEXT("Cap on DRAWN cells. Reported counts stay exact when the draw is capped."));

    // ----------------------------------------------------------------------------------------------

    auto Get_ViewerLocation(UWorld* InWorld, FVector& OutLocation) -> bool
    {
        const auto Controller = InWorld->GetFirstPlayerController();

        if (ck::Is_NOT_Valid(Controller))
        { return false; }

        if (const auto Pawn = Controller->GetPawn(); ck::IsValid(Pawn))
        {
            OutLocation = Pawn->GetActorLocation();
            return true;
        }

        auto ViewLocation = FVector::ZeroVector;
        auto ViewRotation = FRotator::ZeroRotator;
        Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);

        OutLocation = ViewLocation;

        return true;
    }

    auto Get_DrawMode() -> ck::groundnav::EDebugDrawMode
    {
        switch (CVar_Mode.GetValueOnGameThread())
        {
            case 1:  return ck::groundnav::EDebugDrawMode::Clearance;
            case 2:  return ck::groundnav::EDebugDrawMode::Layers;
            case 3:  return ck::groundnav::EDebugDrawMode::Rejected;
            default: return ck::groundnav::EDebugDrawMode::Plates;
        }
    }

    auto Make_BakeParams(const FVector& InCentre) -> ck::groundnav::FCk_GroundNav_DebugBakeParams
    {
        const auto AgentHeight = CVar_AgentHeightUu.GetValueOnGameThread();
        const auto AgentRadius = CVar_AgentRadiusUu.GetValueOnGameThread();

        auto Profile = FCk_GroundNav_AgentProfile{FCk_AnyShape{FCk_ShapeCapsule_Dimensions{
            (AgentHeight * 0.5f) - AgentRadius, AgentRadius}}};

        Profile.Set_MaxSlopeDegrees(CVar_MaxSlopeDegrees.GetValueOnGameThread());
        Profile.Set_MaxSlopeChangeDegrees(CVar_MaxSlopeChangeDegrees.GetValueOnGameThread());
        Profile.Set_StepHeightUu(CVar_StepHeightUu.GetValueOnGameThread());
        Profile.Set_LedgeSensitivity(CVar_LedgeSensitivity.GetValueOnGameThread());
        Profile.Set_RoughPerchToleranceUu(CVar_RoughPerchToleranceUu.GetValueOnGameThread());

        const auto ExtentUu = CVar_ExtentUu.GetValueOnGameThread();

        auto Params = ck::groundnav::FCk_GroundNav_DebugBakeParams{};
        Params._Centre = InCentre;
        Params._Extent = FVector{ExtentUu, ExtentUu, CVar_HeightUu.GetValueOnGameThread()};
        Params._Config = FCk_GroundNav_BakeConfig{
            CVar_CellSizeUu.GetValueOnGameThread(), CVar_CellHeightUu.GetValueOnGameThread()};
        Params._Profile = Profile;
        Params._MergeTunables = FCk_GroundNav_MergeTunables{
            CVar_PlaneFitToleranceUu.GetValueOnGameThread(),
            CVar_NormalConeDegrees.GetValueOnGameThread()};
        Params._MaxCells = CVar_MaxCells.GetValueOnGameThread();

        return Params;
    }

    auto DoBakeAndDraw(UWorld* InWorld, const FVector& InCentre) -> void
    {
        const auto Snapshot = ck::groundnav::Make_DebugSnapshotFromWorld(InWorld, Make_BakeParams(InCentre));

        ck::groundnav::DoDraw_DebugSnapshot(InWorld, Snapshot, Get_DrawMode(),
            FCk_Time{static_cast<double>(CVar_LifetimeSeconds.GetValueOnGameThread())});

        ck::groundnav::Display(TEXT("{}"), ck::groundnav::Get_DebugSnapshotSummary(Snapshot));
    }

    static FAutoConsoleCommandWithWorld ConsoleCommand_Bake(
        TEXT("ck.GroundNav.Bake"),
        TEXT("Bake the ground field around the player from live physics geometry and draw it. ")
        TEXT("The region follows the pawn, so a flying viewer can leave the ground behind and below it — ")
        TEXT("use ck.GroundNav.BakeAt to aim at a fixed point instead."),
        FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* InWorld) -> void
        {
            const auto WorldIsValid = ck::IsValid(InWorld);

            CK_ENSURE_IF_NOT(WorldIsValid, TEXT("ck.GroundNav.Bake ran without a World"))
            { return; }

            auto Centre = FVector::ZeroVector;

            const auto FoundViewer = Get_ViewerLocation(InWorld, Centre);

            CK_ENSURE_IF_NOT(FoundViewer,
                TEXT("ck.GroundNav.Bake found no player to bake around. Run it in PIE or in a game world."))
            { return; }

            DoBakeAndDraw(InWorld, Centre);
        }));

    static FAutoConsoleCommandWithWorldAndArgs ConsoleCommand_BakeAt(
        TEXT("ck.GroundNav.BakeAt"),
        TEXT("Bake the ground field around an explicit world point: ck.GroundNav.BakeAt <X> <Y> <Z>. ")
        TEXT("Unlike ck.GroundNav.Bake the region does not move with the viewer, so what it covers is ")
        TEXT("the same on every run."),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& InArgs, UWorld* InWorld) -> void
        {
            const auto WorldIsValid = ck::IsValid(InWorld);

            CK_ENSURE_IF_NOT(WorldIsValid, TEXT("ck.GroundNav.BakeAt ran without a World"))
            { return; }

            // A mistyped console line is the user talking, not a broken invariant — say what was
            // expected and stop, rather than tripping an ensure over it.
            if (InArgs.Num() != 3)
            {
                ck::groundnav::Warning(
                    TEXT("ck.GroundNav.BakeAt needs three numbers: ck.GroundNav.BakeAt <X> <Y> <Z>"));
                return;
            }

            const auto Centre = FVector{
                FCString::Atod(*InArgs[0]), FCString::Atod(*InArgs[1]), FCString::Atod(*InArgs[2])};

            DoBakeAndDraw(InWorld, Centre);
        }));

    static FAutoConsoleCommandWithWorld ConsoleCommand_Clear(
        TEXT("ck.GroundNav.Clear"),
        TEXT("Clear everything ck.GroundNav.Bake drew."),
        FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* InWorld) -> void
        {
            if (ck::Is_NOT_Valid(InWorld))
            { return; }

            FlushPersistentDebugLines(InWorld);
        }));

    static FAutoConsoleCommand ConsoleCommand_Print(
        TEXT("ck.GroundNav.Print"),
        TEXT("Print every GroundNav debug tunable and its current value."),
        FConsoleCommandDelegate::CreateLambda([]() -> void
        {
            ck::groundnav::Display(TEXT("[GroundNav] current tunables")
                TEXT("\n  region  : ExtentUu {} HeightUu {}")
                TEXT("\n  bake    : CellSizeUu {} CellHeightUu {}")
                TEXT("\n  agent   : HeightUu {} RadiusUu {} MaxSlopeDegrees {} MaxSlopeChangeDegrees {}")
                TEXT("\n            StepHeightUu {} LedgeSensitivity {} RoughPerchToleranceUu {}")
                TEXT("\n  merge   : PlaneFitToleranceUu {} NormalConeDegrees {}")
                TEXT("\n  display : Mode {} LifetimeSeconds {} MaxCells {}"),
                CVar_ExtentUu.GetValueOnGameThread(),
                CVar_HeightUu.GetValueOnGameThread(),
                CVar_CellSizeUu.GetValueOnGameThread(),
                CVar_CellHeightUu.GetValueOnGameThread(),
                CVar_AgentHeightUu.GetValueOnGameThread(),
                CVar_AgentRadiusUu.GetValueOnGameThread(),
                CVar_MaxSlopeDegrees.GetValueOnGameThread(),
                CVar_MaxSlopeChangeDegrees.GetValueOnGameThread(),
                CVar_StepHeightUu.GetValueOnGameThread(),
                CVar_LedgeSensitivity.GetValueOnGameThread(),
                CVar_RoughPerchToleranceUu.GetValueOnGameThread(),
                CVar_PlaneFitToleranceUu.GetValueOnGameThread(),
                CVar_NormalConeDegrees.GetValueOnGameThread(),
                CVar_Mode.GetValueOnGameThread(),
                CVar_LifetimeSeconds.GetValueOnGameThread(),
                CVar_MaxCells.GetValueOnGameThread());
        }));
}

// --------------------------------------------------------------------------------------------------------------------
