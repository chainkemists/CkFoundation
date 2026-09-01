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
            const UObject*                    InWorldContextObject,
            const FVector&                    InCentre,
            const FVector&                    InExtent,
            float                             InCellSizeUu,
            const FCk_GroundNav_AgentProfile& InProfile,
            int32                             InMaxCells)
        -> FCk_GroundNav_DebugSnapshot
    {
        auto Snapshot = FCk_GroundNav_DebugSnapshot{};

        const auto Region = FBox::BuildAABB(InCentre, InExtent);
        Snapshot._Region = Region;
        Snapshot._CellSizeUu = InCellSizeUu;

        const auto Backend = FCk_GroundNav_GeometryBackend_Jolt{InWorldContextObject};

        if (NOT Backend.Get_IsValid())
        {
            Snapshot._Status = EDebugSnapshotStatus::BackendUnavailable;
            return Snapshot;
        }

        const auto StartedAt = FPlatformTime::Seconds();

        auto Geometry = FCk_GroundNav_GeometryBatch{};
        Snapshot._SourceTriangleCount = Backend.Get_TrianglesInBounds(Region, Geometry);

        if (Geometry.Get_IsEmpty())
        {
            Snapshot._Status = EDebugSnapshotStatus::NoGeometryInRegion;
            return Snapshot;
        }

        const auto Config = FCk_GroundNav_BakeConfig{InCellSizeUu, InCellSizeUu * 0.4f};

        auto Spans = FCk_GroundNav_SpanField{};
        const auto RasterResult = DoRasterizeSpans(Geometry, Region, Config, InProfile, Spans);

        if (NOT RasterResult.Get_IsCompleted())
        {
            Snapshot._Status = EDebugSnapshotStatus::Failed;
            return Snapshot;
        }

        Snapshot._DroppedTriangleCount = RasterResult.Get_DroppedInputCount();

        auto Connections = FCk_GroundNav_ConnectionField{};

        if (NOT DoFilter_Walkability(InProfile, Spans, Connections).Get_IsCompleted())
        {
            Snapshot._Status = EDebugSnapshotStatus::Failed;
            return Snapshot;
        }

        auto Layers = FCk_GroundNav_LayerField{};

        if (NOT DoExtract_Layers(Spans, Connections, Layers).Get_IsCompleted())
        {
            Snapshot._Status = EDebugSnapshotStatus::Failed;
            return Snapshot;
        }

        auto Clearance = FCk_GroundNav_ClearanceField{};

        if (NOT DoCompute_Clearance(Layers, InCellSizeUu, Clearance).Get_IsCompleted())
        {
            Snapshot._Status = EDebugSnapshotStatus::Failed;
            return Snapshot;
        }

        auto Plates = FCk_GroundNav_PlateField{};

        if (NOT DoDecompose_Plates(Spans, Layers, FCk_GroundNav_MergeTunables{}, Plates).Get_IsCompleted())
        {
            Snapshot._Status = EDebugSnapshotStatus::Failed;
            return Snapshot;
        }

        const auto ElapsedMilliseconds = (FPlatformTime::Seconds() - StartedAt) * 1000.0;

        auto Built = Make_DebugSnapshot(Spans, Layers, Clearance, Plates, Region, InMaxCells);
        Built._SourceTriangleCount = Snapshot._SourceTriangleCount;
        Built._DroppedTriangleCount = Snapshot._DroppedTriangleCount;
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
        return FString::Printf(
            TEXT("[GroundNav] %s | tris %d (dropped %d) | spans %d | cells %d%s | layers %d | plates %d | max clearance %.1f uu | %.1f ms"),
            Get_StatusName(InSnapshot._Status),
            InSnapshot._SourceTriangleCount,
            InSnapshot._DroppedTriangleCount,
            InSnapshot._SpanCount,
            InSnapshot._WalkableCellCount,
            InSnapshot._CellsWereTruncated ? TEXT(" (draw capped)") : TEXT(""),
            InSnapshot._LayerCount,
            InSnapshot.Get_PlateCount(),
            InSnapshot._MaxClearanceUu,
            InSnapshot._BakeMilliseconds);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_groundnav_debugconsole
{
    static TAutoConsoleVariable<float> CVar_ExtentUu(
        TEXT("ck.GroundNav.Debug.ExtentUu"),
        1500.0f,
        TEXT("Half-extent in XY of the region ck.GroundNav.Bake covers around the player."));

    static TAutoConsoleVariable<float> CVar_HeightUu(
        TEXT("ck.GroundNav.Debug.HeightUu"),
        500.0f,
        TEXT("Half-extent in Z of the region ck.GroundNav.Bake covers around the player."));

    static TAutoConsoleVariable<float> CVar_CellSizeUu(
        TEXT("ck.GroundNav.Debug.CellSizeUu"),
        25.0f,
        TEXT("Cell size the debug bake rasterizes at. Smaller is finer and slower."));

    static TAutoConsoleVariable<float> CVar_AgentHeightUu(
        TEXT("ck.GroundNav.Debug.AgentHeightUu"),
        180.0f,
        TEXT("Standing height of the agent profile the debug bake filters for."));

    static TAutoConsoleVariable<int32> CVar_Mode(
        TEXT("ck.GroundNav.Debug.Mode"),
        0,
        TEXT("0 = merged plates, 1 = clearance ramp, 2 = layers."));

    static TAutoConsoleVariable<float> CVar_LifetimeSeconds(
        TEXT("ck.GroundNav.Debug.LifetimeSeconds"),
        60.0f,
        TEXT("How long the drawn field persists."));

    static TAutoConsoleVariable<int32> CVar_MaxCells(
        TEXT("ck.GroundNav.Debug.MaxCells"),
        20000,
        TEXT("Cap on drawn cells. Reported counts stay exact when the draw is capped."));

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
            default: return ck::groundnav::EDebugDrawMode::Plates;
        }
    }

    static FAutoConsoleCommandWithWorld ConsoleCommand_Bake(
        TEXT("ck.GroundNav.Bake"),
        TEXT("Bake the ground field around the player from live physics geometry and draw it."),
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

            const auto ExtentUu = CVar_ExtentUu.GetValueOnGameThread();

            auto Profile = FCk_GroundNav_AgentProfile{FCk_AnyShape{FCk_ShapeCapsule_Dimensions{
                (CVar_AgentHeightUu.GetValueOnGameThread() * 0.5f) - 34.0f, 34.0f}}};

            const auto Snapshot = ck::groundnav::Make_DebugSnapshotFromWorld(
                InWorld,
                Centre,
                FVector{ExtentUu, ExtentUu, CVar_HeightUu.GetValueOnGameThread()},
                CVar_CellSizeUu.GetValueOnGameThread(),
                Profile,
                CVar_MaxCells.GetValueOnGameThread());

            ck::groundnav::DoDraw_DebugSnapshot(InWorld, Snapshot, Get_DrawMode(),
                FCk_Time{static_cast<double>(CVar_LifetimeSeconds.GetValueOnGameThread())});

            ck::groundnav::Display(TEXT("{}"), ck::groundnav::Get_DebugSnapshotSummary(Snapshot));
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
}

// --------------------------------------------------------------------------------------------------------------------
