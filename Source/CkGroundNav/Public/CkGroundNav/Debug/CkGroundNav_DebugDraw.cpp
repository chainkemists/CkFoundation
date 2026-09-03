#include "CkGroundNav_DebugDraw.h"

#include "CkGroundNav/Backend/CkGroundNav_GeometryBackend_Jolt.h"
#include "CkGroundNav/Bake/CkGroundNav_Clearance.h"
#include "CkGroundNav/Bake/CkGroundNav_Layers.h"
#include "CkGroundNav/Bake/CkGroundNav_MarkupMask.h"
#include "CkGroundNav/Bake/CkGroundNav_Plates.h"
#include "CkGroundNav/Bake/CkGroundNav_Portals.h"
#include "CkGroundNav/Bake/CkGroundNav_Rasterize.h"
#include "CkGroundNav/Bake/CkGroundNav_Walkability.h"
#include "CkGroundNav/Debug/CkGroundNav_DebugGates.h"
#include "CkGroundNav/Facade/CkGroundNav_WorldFieldRegistry.h"
#include "CkGroundNav/Field/CkGroundNav_Field.h"
#include "CkGroundNav/Query/CkGroundNav_Funnel.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Boundary.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Points.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Projection.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Reachability.h"
#include "CkGroundNav/Query/CkGroundNav_Query_SurfaceWalk.h"
#include "CkGroundNav/Search/CkGroundNav_PathPostProcess.h"
#include "CkGroundNav/Search/CkGroundNav_PathSearch.h"
#include "CkGroundNav/Search/CkGroundNav_PlatePortalGraph.h"
#include "CkGroundNav/Volume/CkGroundNavVolume_Utils.h"
#include "CkGroundNav/CkGroundNav_Log.h"

#include "CkNavigation/NavSurface/CkNavSurface_Utils.h"

#include "CkShapes/Capsule/CkShapeCapsule_Fragment_Data.h"

#include <DrawDebugHelpers.h>
#include <Engine/Engine.h>
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

        /**
         * The open Solid bodies, in red, before anything else and in every mode.
         *
         * Unconditional on the draw mode and on whether the snapshot is drawable at all: an open body
         * invalidates the ground the rest of the view is showing, and a bake that failed can still have
         * found the body that is the reason it failed. A developer who never switches mode must still
         * be unable to miss this.
         */
        auto Do_DrawOpenBodies(
            UWorld*                            InWorld,
            const FCk_GroundNav_DebugSnapshot& InSnapshot,
            float                              InLifetimeSeconds) -> void
        {
            constexpr auto Persistent = true;
            constexpr auto DepthPriority = 0;
            constexpr auto DrawShadow = true;

            // Clear of the surface the edge lies on, so an underside hole still reads as a line rather
            // than z-fighting the floor it is flush with.
            const auto Lift = FVector{0.0, 0.0, 2.0};

            for (const auto& OpenBody : InSnapshot._OpenBodies)
            {
                DrawDebugBox(InWorld, OpenBody._Bounds.GetCenter(), OpenBody._Bounds.GetExtent(),
                    FColor::Red, Persistent, InLifetimeSeconds, DepthPriority, 4.0f);

                for (auto Index = 0; Index + 1 < OpenBody._OpenEdgePoints.Num(); Index += 2)
                {
                    DrawDebugLine(InWorld, OpenBody._OpenEdgePoints[Index] + Lift,
                        OpenBody._OpenEdgePoints[Index + 1] + Lift, FColor::Red, Persistent,
                        InLifetimeSeconds, DepthPriority, 6.0f);
                }

                const auto Centre = OpenBody._Bounds.GetCenter();

                DrawDebugString(InWorld, FVector{Centre.X, Centre.Y, OpenBody._Bounds.Max.Z},
                    FString::Printf(TEXT("OPEN COLLISION - %s - %d open edges"),
                        *OpenBody._Description, OpenBody._OpenEdgeCount),
                    nullptr, FColor::Red, InLifetimeSeconds, DrawShadow);
            }
        }

        auto Do_DrawDashedBox(
            UWorld*     InWorld,
            const FBox& InBounds,
            FColor      InColor,
            float       InLifetimeSeconds) -> void
        {
            constexpr auto Persistent = true;
            constexpr auto DepthPriority = 0;
            constexpr auto Thickness = 2.0f;
            constexpr auto DashUu = 20.0;
            constexpr auto CornerCount = 8;
            constexpr auto EdgeCount = 12;

            const auto Min = InBounds.Min;
            const auto Max = InBounds.Max;

            const FVector Corners[CornerCount] = {
                FVector{Min.X, Min.Y, Min.Z},
                FVector{Max.X, Min.Y, Min.Z},
                FVector{Max.X, Max.Y, Min.Z},
                FVector{Min.X, Max.Y, Min.Z},
                FVector{Min.X, Min.Y, Max.Z},
                FVector{Max.X, Min.Y, Max.Z},
                FVector{Max.X, Max.Y, Max.Z},
                FVector{Min.X, Max.Y, Max.Z}};

            const int32 Edges[EdgeCount][2] = {
                {0, 1}, {1, 2}, {2, 3}, {3, 0},
                {4, 5}, {5, 6}, {6, 7}, {7, 4},
                {0, 4}, {1, 5}, {2, 6}, {3, 7}};

            for (auto EdgeIndex = 0; EdgeIndex < EdgeCount; ++EdgeIndex)
            {
                const auto Start = Corners[Edges[EdgeIndex][0]];
                const auto End = Corners[Edges[EdgeIndex][1]];
                const auto EdgeLengthUu = (End - Start).Length();

                if (EdgeLengthUu <= 0.0)
                { continue; }

                const auto Direction = (End - Start) / EdgeLengthUu;
                const auto DashLengthUu = FMath::Min(DashUu, EdgeLengthUu);
                const auto StrideUu = DashUu * 2.0;
                const auto DashCount = FMath::Max(1, FMath::CeilToInt32(EdgeLengthUu / StrideUu));

                for (auto DashIndex = 0; DashIndex < DashCount; ++DashIndex)
                {
                    const auto AlongUu = static_cast<double>(DashIndex) * StrideUu;

                    if (AlongUu >= EdgeLengthUu)
                    { break; }

                    const auto DashStart = Start + (Direction * AlongUu);
                    const auto DashEnd =
                        Start + (Direction * FMath::Min(AlongUu + DashLengthUu, EdgeLengthUu));

                    DrawDebugLine(InWorld, DashStart, DashEnd, InColor, Persistent,
                        InLifetimeSeconds, DepthPriority, Thickness);
                }
            }
        }

        auto Do_DrawMarkups(
            UWorld*                                    InWorld,
            TConstArrayView<FCk_GroundNav_DebugMarkup> InMarkups,
            float                                      InLifetimeSeconds) -> void
        {
            constexpr auto Persistent = true;
            constexpr auto DepthPriority = 0;
            constexpr auto DrawShadow = true;
            constexpr auto SolidThickness = 3.0f;

            const auto ImpassableColor = FColor{220, 60, 60};
            const auto CostColor = FColor{255, 180, 60};
            const auto DisabledColor = FColor{130, 130, 130};

            for (const auto& Markup : InMarkups)
            {
                // A degenerate or unauthored shape has no box, and Get_MarkupWorldBounds says so
                // rather than answering a point. There is nothing to outline.
                if (NOT Markup._Bounds.IsValid)
                { continue; }

                const auto KindColor = Markup._Kind == ECk_GroundNav_MarkupKind::Walkability
                    ? ImpassableColor
                    : CostColor;

                const auto Color = Markup._IsEnabled ? KindColor : DisabledColor;

                if (Markup._IsEnabled)
                {
                    DrawDebugBox(InWorld, Markup._Bounds.GetCenter(), Markup._Bounds.GetExtent(),
                        Color, Persistent, InLifetimeSeconds, DepthPriority, SolidThickness);
                }
                else
                {
                    Do_DrawDashedBox(InWorld, Markup._Bounds, Color, InLifetimeSeconds);
                }

                const auto MultiplierLabel = Markup._Kind == ECk_GroundNav_MarkupKind::Cost
                    ? FString::Printf(TEXT(" x%.2f"), Markup._CostMultiplier)
                    : FString{};

                const auto Label = FString::Printf(TEXT("markup #%d %s | %s%s | %s | %s"),
                    Markup._RecordId,
                    *ck::Format_UE(TEXT("{}"), Markup._Kind),
                    *Markup._AreaTagName.ToString(),
                    *MultiplierLabel,
                    Markup._IsEnabled ? TEXT("enabled") : TEXT("DISABLED"),
                    Markup._IsLive ? TEXT("live") : TEXT("NOT live"));

                DrawDebugString(InWorld,
                    FVector{Markup._Bounds.GetCenter().X, Markup._Bounds.GetCenter().Y,
                        Markup._Bounds.Max.Z},
                    Label, nullptr, Color, InLifetimeSeconds, DrawShadow);
            }
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

        // The same check a field bake runs, over the bodies of this one region. It reads each body's
        // WHOLE mesh, so it is deliberately not part of the triangle fetch above: a mesh clipped to the
        // region has cut edges that look exactly like the holes this is looking for.
        auto Bodies = TArray<FCk_GroundNav_BodyRef>{};
        Backend.Get_StaticBodiesInBounds(Region, Bodies);

        auto CheckedBodies = TSet<uint64>{};
        auto OpenBodies = TArray<FCk_GroundNav_OpenBody>{};
        auto ProbesForClosure = 0;

        DoCheck_GeometryClosure(Backend, Bodies, CheckedBodies, OpenBodies, ProbesForClosure);
        DoReport_OpenBodies(OpenBodies);

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

        if (NOT DoCompute_Clearance(Layers, Connections, InParams._Config.Get_CellSizeUu(), Clearance).Get_IsCompleted())
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

        auto Portals = FCk_GroundNav_PortalField{};

        if (NOT DoExtract_Portals(Spans, Layers, Connections, Plates, Clearance, Portals).Get_IsCompleted())
        {
            Snapshot._SourceTriangleCount = SourceTriangles;
            Snapshot._Status = EDebugSnapshotStatus::Failed;
            return Snapshot;
        }

        const auto ElapsedMilliseconds = (FPlatformTime::Seconds() - StartedAt) * 1000.0;

        auto Built = Make_DebugSnapshot(Spans, Layers, Clearance, Plates, Portals, Region, InParams._MaxCells);
        Do_RecordRejectedCells(SpansBeforeFiltering, Spans, Built);

        Built._SourceTriangleCount = SourceTriangles;
        Built._DroppedTriangleCount = RasterResult.Get_DroppedInputCount();
        Built._BakeMilliseconds = ElapsedMilliseconds;

        Built._OpenBodies.Reserve(OpenBodies.Num());

        for (const auto& OpenBody : OpenBodies)
        {
            auto DebugOpenBody = FCk_GroundNav_DebugOpenBody{};

            DebugOpenBody._Description = OpenBody._Description;
            DebugOpenBody._Bounds = OpenBody._Bounds;
            DebugOpenBody._TriangleCount = OpenBody._TriangleCount;
            DebugOpenBody._OpenEdgeCount = OpenBody._OpenEdgeCount;
            DebugOpenBody._OpenEdgePoints = OpenBody._OpenEdgePoints;

            Built._OpenBodies.Emplace(MoveTemp(DebugOpenBody));
        }

        return Built;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Make_FieldDebugSnapshotFromWorld(
            const UObject*                       InWorldContextObject,
            const FCk_GroundNav_DebugBakeParams& InParams)
        -> FCk_GroundNav_DebugSnapshot
    {
        auto DiscardedField = FCk_GroundNav_FieldPtr{};

        return Make_FieldDebugSnapshotFromWorld(InWorldContextObject, InParams, DiscardedField);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Make_FieldDebugSnapshotFromWorld(
            const UObject*                       InWorldContextObject,
            const FCk_GroundNav_DebugBakeParams& InParams,
            FCk_GroundNav_FieldPtr&              OutField)
        -> FCk_GroundNav_DebugSnapshot
    {
        OutField = {};

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

        auto FieldParams = FCk_GroundNav_FieldParams{};

        FieldParams._OriginXY = FVector2D{Region.Min.X, Region.Min.Y};
        FieldParams._MinZUu = static_cast<float>(Region.Min.Z);
        FieldParams._MaxZUu = static_cast<float>(Region.Max.Z);
        FieldParams._Config = InParams._Config;
        FieldParams._Profile = InParams._Profile;
        FieldParams._MergeTunables = InParams._MergeTunables;

        // The ceiling is the tile size here rather than a separate knob. The debug bake exists to show
        // what tiling does, and a ceiling wider than a tile would make every halo overlap its
        // neighbour entirely - which is exactly the case where seams cannot be wrong.
        FieldParams._MaxClearanceUu = InParams._Config.Get_TileSizeUu() * 0.25f;

        const auto SpanUu = FieldParams.Get_TileSpanUu();

        if (SpanUu > 0.0)
        {
            const auto Size = Region.GetSize();

            FieldParams._Divisions = FIntPoint{
                FMath::Max(1, FMath::CeilToInt32(Size.X / SpanUu)),
                FMath::Max(1, FMath::CeilToInt32(Size.Y / SpanUu))};
        }

        const auto StartedAt = FPlatformTime::Seconds();

        auto Field = FCk_GroundNav_Field{};
        const auto Result = DoBake_Field(Backend, FieldParams, FCk_GroundNav_Epoch{1}, Field);

        const auto ElapsedMilliseconds = (FPlatformTime::Seconds() - StartedAt) * 1000.0;

        if (NOT Result.Get_IsCompleted())
        {
            Snapshot._Status = EDebugSnapshotStatus::Failed;
            return Snapshot;
        }

        auto Built = Make_DebugSnapshotFromField(Field, InParams._MaxCells);

        Built._BakeMilliseconds = ElapsedMilliseconds;
        Built._DroppedTriangleCount = Result.Get_DroppedInputCount();

        OutField = MakeShared<const FCk_GroundNav_Field>(MoveTemp(Field));

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

        debugdraw_private::Do_DrawOpenBodies(InWorld, InSnapshot, LifetimeSeconds);

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

            Do_DrawMarkups(InWorld, InSnapshot._Markups, LifetimeSeconds);

            return;
        }

        if (InMode == EDebugDrawMode::Tiles)
        {
            for (const auto& Tile : InSnapshot._Tiles)
            {
                // An unbuilt tile is drawn, in its own colour. Omitting it would make a place nothing
                // is known about look identical to a place with no floor, which is the distinction the
                // whole status vocabulary exists to preserve.
                const auto Color = Tile._IsBuilt ? FColor{90, 150, 255} : FColor{200, 90, 60};

                DrawDebugBox(InWorld, Tile._Bounds.GetCenter(), Tile._Bounds.GetExtent(), Color,
                    Persistent, LifetimeSeconds, DepthPriority, Tile._IsBuilt ? 1.5f : 3.0f);
            }

            auto WidestSeamUu = 0.0f;

            for (const auto& Seam : InSnapshot._Seams)
            { WidestSeamUu = FMath::Max(WidestSeamUu, Seam._TraversalClearanceUu); }

            const auto Lift = FVector{0.0, 0.0, static_cast<double>(InSnapshot._CellSizeUu) * 0.5};

            for (const auto& Seam : InSnapshot._Seams)
            {
                DrawDebugLine(InWorld, Seam._MinEnd + Lift, Seam._MaxEnd + Lift,
                    Get_ClearanceColor(Seam._TraversalClearanceUu, WidestSeamUu),
                    Persistent, LifetimeSeconds, DepthPriority, 8.0f);
            }

            return;
        }

        if (InMode == EDebugDrawMode::Portals)
        {
            // The plates, dimmed, so a crossing is read against the two things it joins rather than
            // floating in space.
            for (const auto& Plate : InSnapshot._Plates)
            {
                DrawDebugBox(InWorld, Plate._Bounds.GetCenter(), Plate._Bounds.GetExtent() + FVector{0.0, 0.0, 1.0},
                    FColor{55, 60, 65}, Persistent, LifetimeSeconds, DepthPriority, 1.0f);
            }

            // The ramp runs against the WIDEST crossing in this bake, not against the most open cell:
            // portals are compared with each other, and a field of doorways would otherwise all read
            // as equally tight next to an open plaza.
            auto WidestPortalUu = 0.0f;

            for (const auto& Portal : InSnapshot._Portals)
            { WidestPortalUu = FMath::Max(WidestPortalUu, Portal._TraversalClearanceUu); }

            // Lifted clear of the floor. A crossing drawn exactly on the surface it belongs to
            // disappears into it from every angle a player actually looks from.
            const auto Lift = FVector{0.0, 0.0, static_cast<double>(InSnapshot._CellSizeUu) * 0.25};

            for (const auto& Portal : InSnapshot._Portals)
            {
                const auto Color = Get_ClearanceColor(Portal._TraversalClearanceUu, WidestPortalUu);

                DrawDebugLine(InWorld, Portal._MinEnd + Lift, Portal._MaxEnd + Lift, Color, Persistent,
                    LifetimeSeconds, DepthPriority, 6.0f);

                if (NOT Portal._IsCrossLayer)
                { continue; }

                // A crossing that changes floor gets a mast at its midpoint, because colour is
                // already spoken for and the two ends can be a whole storey apart.
                const auto Midpoint = (Portal._MinEnd + Portal._MaxEnd) * 0.5;

                DrawDebugLine(InWorld, Midpoint + Lift, Midpoint + Lift + FVector{0.0, 0.0, 60.0},
                    Color, Persistent, LifetimeSeconds, DepthPriority, 3.0f);
            }

            return;
        }

        if (InMode == EDebugDrawMode::Boundary)
        {
            // Lifted for the same reason a crossing is: a run drawn exactly on the surface it bounds
            // disappears into that surface from every angle a player actually looks from.
            const auto Lift = FVector{0.0, 0.0, static_cast<double>(InSnapshot._CellSizeUu) * 0.5};

            // Outside the layer palette on purpose. A rim run is a wall only until the neighbouring
            // tile is baked, and it must never read as one the bake is sure of.
            const auto TileRimColor = FColor{255, 140, 0};

            for (const auto& Run : InSnapshot._Boundary)
            {
                const auto Color = Run._IsTileRim ? TileRimColor : Get_LayerColor(Run._LayerIndex);

                DrawDebugLine(InWorld, Run._Start + Lift, Run._End + Lift, Color, Persistent,
                    LifetimeSeconds, DepthPriority, 3.0f);

                // Which side of the run the floor is on. Two runs a cell apart facing each other and
                // two facing away are a corridor and a pillar, and the lines alone cannot say which.
                const auto Midpoint = ((Run._Start + Run._End) * 0.5) + Lift;
                const auto Inward = FVector{Run._InwardNormalXY.X, Run._InwardNormalXY.Y, 0.0};

                DrawDebugLine(InWorld, Midpoint, Midpoint + (Inward * 20.0), Color, Persistent,
                    LifetimeSeconds, DepthPriority, 1.5f);
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
        Make_DebugMarkupsFromWorld(
            UWorld* InWorld)
        -> TArray<FCk_GroundNav_DebugMarkup>
    {
        auto Markups = TArray<FCk_GroundNav_DebugMarkup>{};

        if (ck::Is_NOT_Valid(InWorld))
        { return Markups; }

        auto VolumeEntities = world_fields::Get_VolumeEntities(InWorld);

        for (auto& VolumeEntity : VolumeEntities)
        {
            if (ck::Is_NOT_Valid(VolumeEntity))
            { continue; }

            const auto Volume = UCk_Utils_GroundNavVolume_UE::Cast(VolumeEntity);

            if (ck::Is_NOT_Valid(Volume))
            { continue; }

            for (const auto& Entry : UCk_Utils_GroundNavVolume_UE::Get_MarkupRecords(Volume))
            {
                const auto& Record = Entry.Get_Record();

                auto MarkupEntity = Entry.Get_MarkupEntity();

                // The NEUTRAL probe, never this module's own answer: a viewer that asked GroundNav
                // directly would report a paint as live on a world some other provider is serving.
                const auto Markup = UCk_Utils_NavSurface_UE::Cast(MarkupEntity);

                auto Drawn = FCk_GroundNav_DebugMarkup{};

                Drawn._Bounds = Get_MarkupWorldBounds(Record);
                Drawn._AreaTagName = Record.Get_AreaTag().GetTagName();
                Drawn._RecordId = Record.Get_Id();
                Drawn._CostMultiplier = Record.Get_CostMultiplier();
                Drawn._RequestedAtEpoch = Record.Get_RequestedAtEpoch();
                Drawn._Kind = Record.Get_Kind();
                Drawn._IsEnabled = Record.Get_Enable() == ECk_EnableDisable::Enable;
                Drawn._IsLive = UCk_Utils_NavSurface_UE::Get_IsMarkupLive(Markup);

                Markups.Emplace(MoveTemp(Drawn));
            }
        }

        return Markups;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        DoDraw_DebugMarkups(
            UWorld*                                    InWorld,
            TConstArrayView<FCk_GroundNav_DebugMarkup> InMarkups,
            FCk_Time                                   InLifetime)
        -> void
    {
        const auto WorldIsValid = ck::IsValid(InWorld);

        CK_ENSURE_IF_NOT(WorldIsValid, TEXT("Cannot draw GroundNav markup outlines without a World"))
        { return; }

        debugdraw_private::Do_DrawMarkups(
            InWorld, InMarkups, static_cast<float>(InLifetime.Get_Seconds()));
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_DebugSnapshotSummary(
            const FCk_GroundNav_DebugSnapshot& InSnapshot)
        -> FString
    {
        const auto Centre = InSnapshot._Region.GetCenter();
        const auto Extent = InSnapshot._Region.GetExtent();

        auto CrossLayerPortalCount = 0;

        for (const auto& Portal : InSnapshot._Portals)
        {
            if (Portal._IsCrossLayer)
            { ++CrossLayerPortalCount; }
        }

        auto TileRimBoundaryCount = 0;

        for (const auto& Run : InSnapshot._Boundary)
        {
            if (Run._IsTileRim)
            { ++TileRimBoundaryCount; }
        }

        const auto IsTiledField = InSnapshot.Get_TileCount() > 0;

        // A field's tiles carry these forward from their own bake, so the tiled and single-region
        // paths report the same three numbers rather than one of them reporting "not tracked". For a
        // field they are summed over tile HALO lattices, which overlap at every seam — the work the
        // bake did, not a census of the world — and the line says so, or a reader comparing the two
        // bake kinds would see a field "find" more geometry than the region it covers.
        const auto RasterizationBlock = FString::Printf(
            TEXT("  geometry : %d triangles in, %d dropped%s\n")
            TEXT("  spans    : %d rasterized -> %d walkable cells%s, %d REJECTED by the filters\n"),
            InSnapshot._SourceTriangleCount,
            InSnapshot._DroppedTriangleCount,
            IsTiledField ? TEXT(" (summed over tile halos, which overlap)") : TEXT(""),
            InSnapshot._SpanCount,
            InSnapshot._WalkableCellCount,
            InSnapshot._CellsWereTruncated ? TEXT(" (draw capped)") : TEXT(""),
            InSnapshot._RejectedCellCount);

        // Immediately under the status line and ahead of everything a developer reads to judge a bake,
        // because none of those numbers mean anything while a body under them is open.
        auto OpenCollisionBlock = FString{};

        if (NOT InSnapshot._OpenBodies.IsEmpty())
        {
            OpenCollisionBlock = FString::Printf(
                TEXT("  !!! OPEN COLLISION: %d static bod%s not closed — the bake sees faces only, so a ")
                TEXT("wall with no underside is INVISIBLE to it and agents will path through. ")
                TEXT("Fix the collision (closed mesh, or simple/convex collision). ")
                TEXT("Drawn in RED in every mode.\n"),
                InSnapshot.Get_OpenBodyCount(),
                InSnapshot.Get_OpenBodyCount() == 1 ? TEXT("y") : TEXT("ies"));

            for (const auto& OpenBody : InSnapshot._OpenBodies)
            {
                OpenCollisionBlock += FString::Printf(
                    TEXT("      - %s: %d open edge%s of %d triangle%s\n"),
                    *OpenBody._Description,
                    OpenBody._OpenEdgeCount,
                    OpenBody._OpenEdgeCount == 1 ? TEXT("") : TEXT("s"),
                    OpenBody._TriangleCount,
                    OpenBody._TriangleCount == 1 ? TEXT("") : TEXT("s"));
            }
        }

        return FString::Printf(
            TEXT("[GroundNav] %s | %.1f ms\n")
            TEXT("%s")
            TEXT("  region   : centre (%.0f, %.0f, %.0f)  half-extent (%.0f, %.0f, %.0f)\n")
            TEXT("  lattice  : %d x %d columns at %.1f uu, %d layer(s) -> %d cell slots\n")
            TEXT("%s")
            TEXT("  layers   : %d\n")
            TEXT("  plates   : %d (collapse %.1f cells/plate, worst residual %.2f uu, worst height spread %.2f uu)\n")
            TEXT("  portals  : %d crossings (%d change floor, tightest lets %.1f uu through)\n")
            TEXT("  boundary : %d runs (%d on tile rims)%s\n")
            TEXT("  tiles    : %d of %d built, %d seams between them\n")
            TEXT("  clearance: %.1f uu at the most open cell\n")
            TEXT("  memory   : %.1f KB held by %s"),
            Get_StatusName(InSnapshot._Status),
            InSnapshot._BakeMilliseconds,
            *OpenCollisionBlock,
            Centre.X, Centre.Y, Centre.Z,
            Extent.X, Extent.Y, Extent.Z,
            InSnapshot._LatticeSizeX,
            InSnapshot._LatticeSizeY,
            InSnapshot._CellSizeUu,
            InSnapshot._LayerCount,
            InSnapshot._LatticeSizeX * InSnapshot._LatticeSizeY * InSnapshot._LayerCount,
            *RasterizationBlock,
            InSnapshot._LayerCount,
            InSnapshot.Get_PlateCount(),
            InSnapshot._CollapseRatio,
            InSnapshot._MaxPlaneResidualUu,
            InSnapshot._MaxPlateHeightRangeUu,
            InSnapshot.Get_PortalCount(),
            CrossLayerPortalCount,
            InSnapshot.Get_NarrowestPortalUu(),
            InSnapshot.Get_BoundaryCount(),
            TileRimBoundaryCount,
            IsTiledField ? TEXT("") : TEXT(" (field bakes only)"),
            InSnapshot.Get_BuiltTileCount(),
            InSnapshot.Get_TileCount(),
            InSnapshot.Get_SeamCount(),
            InSnapshot._MaxClearanceUu,
            static_cast<double>(InSnapshot._AllocatedBytes) / 1024.0,
            IsTiledField ? TEXT("the published field") : TEXT("the bake products"));
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

    static TAutoConsoleVariable<float> CVar_TileSizeUu(
        TEXT("ck.GroundNav.Debug.TileSizeUu"), 800.0f,
        TEXT("Edge length of one tile, used by ck.GroundNav.BakeFieldAt. Smaller tiles mean more seams ")
        TEXT("and more halo re-rasterized per tile; larger ones mean fewer, coarser units of rebuild. ")
        TEXT("Snapped up to a whole number of cells, so a size that does not divide evenly grows."));

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
        TEXT("0 = merged plates, 1 = clearance ramp, 2 = layers, 3 = cells the filters rejected, ")
        TEXT("4 = the crossings between plates, 5 = the tile lattice and the seams between tiles, ")
        TEXT("6 = the plate edges nothing crosses, with the runs on a tile rim in orange."));

    static TAutoConsoleVariable<float> CVar_LifetimeSeconds(
        TEXT("ck.GroundNav.Debug.LifetimeSeconds"), 60.0f,
        TEXT("How long the drawn field persists."));

    static TAutoConsoleVariable<int32> CVar_MaxCells(
        TEXT("ck.GroundNav.Debug.MaxCells"), 20000,
        TEXT("Cap on DRAWN cells. Reported counts stay exact when the draw is capped."));

    // ---- Projection probe -------------------------------------------------------------------------

    static TAutoConsoleVariable<float> CVar_ProbeExtentUu(
        TEXT("ck.GroundNav.Debug.ProbeExtentUu"), 100.0f,
        TEXT("Horizontal half-extent of the projection search box."));

    static TAutoConsoleVariable<float> CVar_ProbeUpUu(
        TEXT("ck.GroundNav.Debug.ProbeUpUu"), 100.0f,
        TEXT("How far ABOVE the query point a projection may find a surface."));

    static TAutoConsoleVariable<float> CVar_ProbeDownUu(
        TEXT("ck.GroundNav.Debug.ProbeDownUu"), 200.0f,
        TEXT("How far BELOW the query point a projection may find a surface."));

    static TAutoConsoleVariable<int32> CVar_ProbeMode(
        TEXT("ck.GroundNav.Debug.ProbeMode"), 0,
        TEXT("0 = closest, 1 = down only, 2 = up only. The probe's agent radius is ")
        TEXT("ck.GroundNav.Debug.AgentRadiusUu, the same one the bake profile uses."));

    // ---- Path cost --------------------------------------------------------------------------------

    static TAutoConsoleVariable<float> CVar_SlopePenaltyK(
        TEXT("ck.GroundNav.Debug.SlopePenaltyK"), 0.0f,
        TEXT("What a leg pays per unit of rise over run, on top of its ground distance. 0 prices a ")
        TEXT("ramp exactly as flat ground; raising it buys a level detour around a slope the agent ")
        TEXT("can still climb."));

    static TAutoConsoleVariable<float> CVar_ClearanceBiasK(
        TEXT("ck.GroundNav.Debug.ClearanceBiasK"), 0.0f,
        TEXT("What a leg pays for crossing a tight door, in cell widths of clearance at the crossing. ")
        TEXT("It chooses a different DOOR - it never moves the string inside a plate, because the ")
        TEXT("funnel does not read it."));

    static TAutoConsoleVariable<float> CVar_CornerOffsetK(
        TEXT("ck.GroundNav.Debug.CornerOffsetK"), 1.0f,
        TEXT("How far an inside corner is pushed off the wall it hugs, as a multiple of the body ")
        TEXT("radius. 0 draws the raw funnel; the offset never lands a waypoint off walkable ground ")
        TEXT("or inside the radius, so a corner that cannot take the whole offset takes less of it."));

    // ----------------------------------------------------------------------------------------------

    // A query runs against a FIELD, and only the field bake produces one - a region bake has no
    // tiles to address. Held for the whole process because a field is immutable and reaches back to
    // nothing: no world, no actor, no registry.
    static ck::groundnav::FCk_GroundNav_FieldPtr LastDebugField;

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
            case 4:  return ck::groundnav::EDebugDrawMode::Portals;
            case 5:  return ck::groundnav::EDebugDrawMode::Tiles;
            case 6:  return ck::groundnav::EDebugDrawMode::Boundary;
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
        Params._Config.Set_TileSizeUu(CVar_TileSizeUu.GetValueOnGameThread());
        Params._Profile = Profile;
        Params._MergeTunables = FCk_GroundNav_MergeTunables{
            CVar_PlaneFitToleranceUu.GetValueOnGameThread(),
            CVar_NormalConeDegrees.GetValueOnGameThread()};
        Params._MaxCells = CVar_MaxCells.GetValueOnGameThread();

        return Params;
    }

    auto DoDrawAndReport(UWorld* InWorld, const ck::groundnav::FCk_GroundNav_DebugSnapshot& InSnapshot) -> void
    {
        ck::groundnav::DoDraw_DebugSnapshot(InWorld, InSnapshot, Get_DrawMode(),
            FCk_Time{static_cast<double>(CVar_LifetimeSeconds.GetValueOnGameThread())});

        ck::groundnav::Display(TEXT("{}"), ck::groundnav::Get_DebugSnapshotSummary(InSnapshot));

        // The log scrolls and the red boxes may be behind the viewer. A developer who ran a bake for
        // an unrelated reason still has to be told the ground under an open body cannot be trusted.
        if (InSnapshot.Get_OpenBodyCount() > 0 && ck::IsValid(GEngine))
        {
            // Stable, so a repeated bake replaces its own line instead of stacking one per run.
            constexpr auto MessageKey = uint64{0x436B474E4F50454Eull};
            constexpr auto MessageSeconds = 30.0f;

            GEngine->AddOnScreenDebugMessage(MessageKey, MessageSeconds, FColor::Red,
                FString::Printf(
                    TEXT("GroundNav: %d static bodies have OPEN collision — agents can path through ")
                    TEXT("them. See the log / red boxes."),
                    InSnapshot.Get_OpenBodyCount()));
        }
    }

    // Collected here rather than inside the bake: the bake produces a value that outlives its world,
    // and the markup a world's volumes hold is only readable while that world is still there.
    auto DoStamp_Markups(UWorld* InWorld, ck::groundnav::FCk_GroundNav_DebugSnapshot& InOutSnapshot) -> void
    {
        if (NOT ck::groundnav::debug::Get_IsMarkupDrawEnabled())
        { return; }

        InOutSnapshot._Markups = ck::groundnav::Make_DebugMarkupsFromWorld(InWorld);
    }

    auto DoDraw_MarkupsIfEnabled(UWorld* InWorld, float InLifetimeSeconds) -> void
    {
        if (NOT ck::groundnav::debug::Get_IsMarkupDrawEnabled())
        { return; }

        const auto Markups = ck::groundnav::Make_DebugMarkupsFromWorld(InWorld);

        ck::groundnav::DoDraw_DebugMarkups(
            InWorld, Markups, FCk_Time{static_cast<double>(InLifetimeSeconds)});
    }

    auto DoBakeAndDraw(UWorld* InWorld, const FVector& InCentre) -> void
    {
        auto Snapshot = ck::groundnav::Make_DebugSnapshotFromWorld(InWorld, Make_BakeParams(InCentre));

        DoStamp_Markups(InWorld, Snapshot);
        DoDrawAndReport(InWorld, Snapshot);
    }

    auto DoBakeFieldAndDraw(UWorld* InWorld, const FVector& InCentre) -> void
    {
        auto Snapshot = ck::groundnav::Make_FieldDebugSnapshotFromWorld(
            InWorld, Make_BakeParams(InCentre), LastDebugField);

        DoStamp_Markups(InWorld, Snapshot);
        DoDrawAndReport(InWorld, Snapshot);
    }

    auto Get_ProbeMode() -> ECk_NavSurface_ProjectionMode
    {
        switch (CVar_ProbeMode.GetValueOnGameThread())
        {
            case 1:  return ECk_NavSurface_ProjectionMode::Down;
            case 2:  return ECk_NavSurface_ProjectionMode::Up;
            default: return ECk_NavSurface_ProjectionMode::Closest;
        }
    }

    auto DoProbeAndDraw(UWorld* InWorld, const FVector& InPoint) -> void
    {
        if (ck::Is_NOT_Valid(LastDebugField))
        {
            ck::groundnav::Warning(TEXT("ck.GroundNav.Probe needs a FIELD bake first: run ")
                TEXT("ck.GroundNav.BakeFieldAt <X> <Y> <Z> or press Y in the tuning range"));
            return;
        }

        const auto& Field = *LastDebugField;

        const auto ExtentUu = CVar_ProbeExtentUu.GetValueOnGameThread();
        const auto UpUu = CVar_ProbeUpUu.GetValueOnGameThread();
        const auto DownUu = CVar_ProbeDownUu.GetValueOnGameThread();
        const auto RadiusUu = CVar_AgentRadiusUu.GetValueOnGameThread();
        const auto ProbeMode = Get_ProbeMode();

        auto ProjectionQuery = ck::groundnav::FCk_GroundNav_ProjectionQuery{};

        ProjectionQuery._Location = InPoint;
        ProjectionQuery._HorizontalExtentUu = ExtentUu;
        ProjectionQuery._UpExtentUu = UpUu;
        ProjectionQuery._DownExtentUu = DownUu;
        ProjectionQuery._Mode = ProbeMode;
        ProjectionQuery._Agent._RadiusUu = RadiusUu;

        const auto Projection = ck::groundnav::Get_ProjectPoint(Field, ProjectionQuery);

        auto NavigableQuery = ck::groundnav::FCk_GroundNav_IsNavigableQuery{};

        // Two vertical quanta: tight enough that the answer is about THIS height rather than the
        // column, wide enough to survive the bake's own quantization of the surface.
        NavigableQuery._Location = InPoint;
        NavigableQuery._VerticalToleranceUu = Field._Params._Config.Get_CellHeightUu() * 2.0f;
        NavigableQuery._Agent._RadiusUu = RadiusUu;

        const auto Navigable = ck::groundnav::Get_IsNavigable(Field, NavigableQuery);

        constexpr auto Persistent = true;
        constexpr auto DepthPriority = 0;
        constexpr auto DrawShadow = true;
        constexpr auto SphereSegments = 12;

        const auto LifetimeSeconds = CVar_LifetimeSeconds.GetValueOnGameThread();

        DrawDebugSphere(InWorld, InPoint, 8.0f, SphereSegments, FColor::White, Persistent,
            LifetimeSeconds, DepthPriority);

        // The box is exactly the volume the query searches: the two reaches are independent, and a
        // one-sided mode has only the reach on its own side. A box showing reach the query does not
        // have is worse than no box at all.
        const auto ReachUp = ProbeMode == ECk_NavSurface_ProjectionMode::Down ? 0.0 : static_cast<double>(UpUu);
        const auto ReachDown = ProbeMode == ECk_NavSurface_ProjectionMode::Up ? 0.0 : static_cast<double>(DownUu);
        const auto BoxHalfHeight = (ReachUp + ReachDown) * 0.5;
        const auto BoxCentreZOffset = (ReachUp - ReachDown) * 0.5;

        DrawDebugBox(InWorld, InPoint + FVector{0.0, 0.0, BoxCentreZOffset},
            FVector{static_cast<double>(ExtentUu), static_cast<double>(ExtentUu), BoxHalfHeight},
            FColor{130, 130, 130}, Persistent, LifetimeSeconds, DepthPriority, 1.5f);

        if (Projection.Get_IsSuccess())
        {
            constexpr auto ArrowSize = 12.0f;

            DrawDebugSphere(InWorld, Projection._Location, 10.0f, SphereSegments, FColor::Green,
                Persistent, LifetimeSeconds, DepthPriority);

            DrawDebugLine(InWorld, InPoint, Projection._Location, FColor::Green, Persistent,
                LifetimeSeconds, DepthPriority, 2.0f);

            DrawDebugDirectionalArrow(InWorld, Projection._Location,
                Projection._Location + (Projection._SurfaceNormal * 60.0), ArrowSize, FColor::Cyan,
                Persistent, LifetimeSeconds, DepthPriority, 2.0f);
        }
        else
        {
            DrawDebugSphere(InWorld, InPoint, 12.0f, SphereSegments, FColor::Red, Persistent,
                LifetimeSeconds, DepthPriority);
        }

        const auto Summary = FString::Printf(
            TEXT("probe %s | tile %d layer %d plate %d | clearance %.1f uu | %d cells read\n")
            TEXT("navigable: %s"),
            *ck::Format_UE(TEXT("{}"), Projection._Status),
            Projection._Surface._TileIndex,
            Projection._Surface._LayerIndex,
            Projection._Surface._PlateIndex,
            Projection._ClearanceUu,
            Projection._Cost._CellsRead,
            *ck::Format_UE(TEXT("{}"), Navigable._Status));

        DrawDebugString(InWorld, InPoint + FVector{0.0, 0.0, 20.0}, Summary, nullptr,
            Projection.Get_IsSuccess() ? FColor::White : FColor::Red, LifetimeSeconds, DrawShadow);

        ck::groundnav::Display(TEXT("[GroundNav] at ({}, {}, {}) {}"),
            InPoint.X, InPoint.Y, InPoint.Z, Summary);
    }

    // One log call per line rather than one per report: the log truncates a long message, and every
    // consumer of this output reads it a line at a time anyway.
    auto DoLog_Report(const FString& InReport) -> void
    {
        constexpr auto CullEmptyLines = false;

        auto Lines = TArray<FString>{};
        InReport.ParseIntoArrayLines(Lines, CullEmptyLines);

        for (const auto& Line : Lines)
        { ck::groundnav::Display(TEXT("{}"), Line); }
    }

    auto Get_PlateReportAt(
        const ck::groundnav::FCk_GroundNav_Field& InField,
        const FVector&                            InPoint) -> FString
    {
        auto ProjectionQuery = ck::groundnav::FCk_GroundNav_ProjectionQuery{};

        ProjectionQuery._Location = InPoint;
        ProjectionQuery._HorizontalExtentUu = CVar_ProbeExtentUu.GetValueOnGameThread();
        ProjectionQuery._UpExtentUu = CVar_ProbeUpUu.GetValueOnGameThread();
        ProjectionQuery._DownExtentUu = CVar_ProbeDownUu.GetValueOnGameThread();
        ProjectionQuery._Mode = Get_ProbeMode();
        ProjectionQuery._Agent._RadiusUu = CVar_AgentRadiusUu.GetValueOnGameThread();

        const auto Projection = ck::groundnav::Get_ProjectPoint(InField, ProjectionQuery);
        const auto& Surface = Projection._Surface;

        if (NOT Projection.Get_IsSuccess() || NOT InField._Tiles.IsValidIndex(Surface._TileIndex))
        {
            return FString::Printf(TEXT("    plate under the point : none (%s)\n"),
                *ck::Format_UE(TEXT("{}"), Projection._Status));
        }

        const auto& Plates = InField._Tiles[Surface._TileIndex]._Plates;

        if (NOT Plates._Plates.IsValidIndex(Surface._PlateIndex))
        {
            return FString::Printf(
                TEXT("    plate under the point : tile %d layer %d, no plate\n"),
                Surface._TileIndex, Surface._LayerIndex);
        }

        const auto& Plate = Plates._Plates[Surface._PlateIndex];
        const auto& Policy = Plates.Get_AreaPolicy(Plate._AreaPolicyIndex);

        return FString::Printf(
            TEXT("    plate under the point : tile %d layer %d plate %d | policy %d [%s] | cost x%.2f\n"),
            Surface._TileIndex,
            Surface._LayerIndex,
            Surface._PlateIndex,
            Plate._AreaPolicyIndex,
            Policy.IsEmpty() ? TEXT("none") : *Policy.ToStringSimple(),
            Plate._CostMultiplier);
    }

    auto DoMarkupAndReport(UWorld* InWorld, const FVector& InPoint) -> void
    {
        const auto LifetimeSeconds = CVar_LifetimeSeconds.GetValueOnGameThread();

        const auto Markups = ck::groundnav::Make_DebugMarkupsFromWorld(InWorld);

        ck::groundnav::DoDraw_DebugMarkups(
            InWorld, Markups, FCk_Time{static_cast<double>(LifetimeSeconds)});

        auto VolumeEntities = ck::groundnav::world_fields::Get_VolumeEntities(InWorld);

        auto Report = FString::Printf(TEXT("[GroundNav] markup at (%.0f, %.0f, %.0f)\n"),
            InPoint.X, InPoint.Y, InPoint.Z);

        auto VolumeCount = 0;

        for (auto& VolumeEntity : VolumeEntities)
        {
            if (ck::Is_NOT_Valid(VolumeEntity))
            { continue; }

            const auto Volume = UCk_Utils_GroundNavVolume_UE::Cast(VolumeEntity);

            if (ck::Is_NOT_Valid(Volume))
            { continue; }

            ++VolumeCount;

            const auto Records = UCk_Utils_GroundNavVolume_UE::Get_MarkupRecords(Volume);

            // Every volume is listed, with where the point falls on it, rather than only the one
            // covering the point: a record painted before anything baked lives on a volume that
            // covers nothing yet, and that is exactly the case worth looking at.
            Report += FString::Printf(
                TEXT("  volume %s : %d record(s), build epoch %lld, %s at the point\n"),
                *ck::Format_UE(TEXT("{}"), Volume),
                Records.Num(),
                static_cast<long long>(UCk_Utils_GroundNavVolume_UE::Get_BuildEpoch(Volume)),
                *ck::Format_UE(TEXT("{}"),
                    UCk_Utils_GroundNavVolume_UE::Get_RegionStatusAt(Volume, InPoint)));

            for (const auto& Entry : Records)
            {
                const auto& Record = Entry.Get_Record();

                auto MarkupEntity = Entry.Get_MarkupEntity();

                // The NEUTRAL probe, never this module's own answer: a record reads as live only if
                // the provider actually serving this world says the paint reached its surface.
                const auto Markup = UCk_Utils_NavSurface_UE::Cast(MarkupEntity);

                const auto Bounds = ck::groundnav::Get_MarkupWorldBounds(Record);

                Report += FString::Printf(
                    TEXT("    #%d %s [%s] x%.2f | %s | live=%s | requested at epoch %lld | bounds %s\n"),
                    Record.Get_Id(),
                    *ck::Format_UE(TEXT("{}"), Record.Get_Kind()),
                    *Record.Get_AreaTag().ToString(),
                    Record.Get_CostMultiplier(),
                    Record.Get_Enable() == ECk_EnableDisable::Enable ? TEXT("enabled") : TEXT("DISABLED"),
                    UCk_Utils_NavSurface_UE::Get_IsMarkupLive(Markup) ? TEXT("yes") : TEXT("no"),
                    static_cast<long long>(Record.Get_RequestedAtEpoch()),
                    Bounds.IsValid ? *Bounds.ToString() : TEXT("degenerate - not a box"));
            }

            const auto Field = UCk_Utils_GroundNavVolume_UE::Get_Field(Volume);

            Report += ck::IsValid(Field)
                ? Get_PlateReportAt(*Field, InPoint)
                : FString{TEXT("    plate under the point : nothing published\n")};
        }

        if (VolumeCount == 0)
        {
            Report += TEXT("  no ground-nav volume has published a field in this world, so there is ")
                      TEXT("no markup to report and no plate to look under\n");
        }

        DoLog_Report(Report);
    }

    auto Get_FivePointArgs(const TArray<FString>& InArgs, FVector& OutStart, FVector2D& OutTargetXY) -> bool
    {
        if (InArgs.Num() != 5)
        { return false; }

        OutStart = FVector{
            FCString::Atod(*InArgs[0]), FCString::Atod(*InArgs[1]), FCString::Atod(*InArgs[2])};

        OutTargetXY = FVector2D{FCString::Atod(*InArgs[3]), FCString::Atod(*InArgs[4])};

        return true;
    }

    auto Get_SixPointArgs(const TArray<FString>& InArgs, FVector& OutStart, FVector& OutEnd) -> bool
    {
        if (InArgs.Num() != 6)
        { return false; }

        OutStart = FVector{
            FCString::Atod(*InArgs[0]), FCString::Atod(*InArgs[1]), FCString::Atod(*InArgs[2])};

        OutEnd = FVector{
            FCString::Atod(*InArgs[3]), FCString::Atod(*InArgs[4]), FCString::Atod(*InArgs[5])};

        return true;
    }

    auto Get_HasDebugField(const FString& InCommandName) -> bool
    {
        if (ck::IsValid(LastDebugField))
        { return true; }

        ck::groundnav::Warning(TEXT("{} needs a FIELD bake first: run ")
            TEXT("ck.GroundNav.BakeFieldAt <X> <Y> <Z> or press Y in the tuning range"), InCommandName);

        return false;
    }

    auto DoWalkAndDraw(UWorld* InWorld, const FVector& InStart, const FVector2D& InTargetXY) -> void
    {
        if (NOT Get_HasDebugField(TEXT("ck.GroundNav.WalkAt")))
        { return; }

        const auto& Field = *LastDebugField;

        // The target keeps the start's height. A walk resolves its own surface from the start and
        // reads the target in XY alone, so a third number there would be a value nothing consumes.
        const auto Target = FVector{InTargetXY.X, InTargetXY.Y, InStart.Z};

        auto WalkQuery = ck::groundnav::FCk_GroundNav_SurfaceWalkQuery{};

        WalkQuery._Start = InStart;
        WalkQuery._Target = Target;
        WalkQuery._StartVerticalToleranceUu = Field._Params._Config.Get_CellHeightUu() * 2.0f;
        WalkQuery._Agent._RadiusUu = CVar_AgentRadiusUu.GetValueOnGameThread();

        auto Diagnostics = ck::groundnav::FCk_GroundNav_SurfaceWalkDiagnostics{};

        const auto Walk = ck::groundnav::Get_MoveAlongSurface(Field, WalkQuery, Diagnostics);

        constexpr auto Persistent = true;
        constexpr auto DepthPriority = 0;
        constexpr auto DrawShadow = true;
        constexpr auto SphereSegments = 12;

        const auto LifetimeSeconds = CVar_LifetimeSeconds.GetValueOnGameThread();

        DrawDebugSphere(InWorld, InStart, 8.0f, SphereSegments, FColor::White, Persistent,
            LifetimeSeconds, DepthPriority);

        DrawDebugSphere(InWorld, Target, 8.0f, SphereSegments, FColor{130, 130, 130}, Persistent,
            LifetimeSeconds, DepthPriority);

        if (Walk.Get_IsSuccess())
        {
            DrawDebugSphere(InWorld, Walk._Location, 10.0f, SphereSegments, FColor::Green, Persistent,
                LifetimeSeconds, DepthPriority);

            DrawDebugLine(InWorld, InStart, Walk._Location, FColor::Green, Persistent, LifetimeSeconds,
                DepthPriority, 2.0f);

            // The ground the walk could NOT cover, drawn thin so it reads as what was asked for rather
            // than as part of the answer: a walk that stops short is a Success, so colour cannot say it.
            if (NOT Walk._ReachedTarget)
            {
                DrawDebugLine(InWorld, Walk._Location, Target, FColor{130, 130, 130}, Persistent,
                    LifetimeSeconds, DepthPriority, 1.0f);
            }
        }
        else
        {
            DrawDebugSphere(InWorld, InStart, 12.0f, SphereSegments, FColor::Red, Persistent,
                LifetimeSeconds, DepthPriority);
        }

        const auto Summary = FString::Printf(
            TEXT("walk %s | reached target %s | %d cells stepped, %d slides, %d portal + %d seam crossings\n")
            TEXT("early-out %s | bound hit %s | %d cells read"),
            *ck::Format_UE(TEXT("{}"), Walk._Status),
            Walk._ReachedTarget ? TEXT("yes") : TEXT("no"),
            Diagnostics._CellsStepped,
            Diagnostics._SlideCount,
            Diagnostics._PortalCrossings,
            Diagnostics._SeamCrossings,
            Diagnostics._TookPlateEarlyOut ? TEXT("yes") : TEXT("no"),
            Diagnostics._HitIterationBound ? TEXT("yes") : TEXT("no"),
            Walk._Cost._CellsRead);

        DrawDebugString(InWorld, InStart + FVector{0.0, 0.0, 20.0}, Summary, nullptr,
            Walk.Get_IsSuccess() ? FColor::White : FColor::Red, LifetimeSeconds, DrawShadow);

        ck::groundnav::Display(TEXT("[GroundNav] from ({}, {}, {}) toward ({}, {}) {}"),
            InStart.X, InStart.Y, InStart.Z, Target.X, Target.Y, Summary);
    }

    auto DoRaycastAndDraw(UWorld* InWorld, const FVector& InStart, const FVector2D& InTargetXY) -> void
    {
        if (NOT Get_HasDebugField(TEXT("ck.GroundNav.RayAt")))
        { return; }

        const auto& Field = *LastDebugField;

        const auto End = FVector{InTargetXY.X, InTargetXY.Y, InStart.Z};

        auto RaycastQuery = ck::groundnav::FCk_GroundNav_RaycastQuery{};

        RaycastQuery._Start = InStart;
        RaycastQuery._End = End;
        RaycastQuery._StartVerticalToleranceUu = Field._Params._Config.Get_CellHeightUu() * 2.0f;
        RaycastQuery._Agent._RadiusUu = CVar_AgentRadiusUu.GetValueOnGameThread();
        RaycastQuery._MaxCost = 0.0f;

        const auto Raycast = ck::groundnav::Get_SurfaceRaycast(Field, RaycastQuery);

        constexpr auto Persistent = true;
        constexpr auto DepthPriority = 0;
        constexpr auto DrawShadow = true;
        constexpr auto SphereSegments = 12;
        constexpr auto ArrowSize = 12.0f;

        const auto LifetimeSeconds = CVar_LifetimeSeconds.GetValueOnGameThread();
        const auto WasBlocked = Raycast._Status == ECk_NavSurface_QueryStatus::Blocked;

        if (Raycast.Get_IsClear())
        {
            DrawDebugLine(InWorld, InStart, End, FColor::Green, Persistent, LifetimeSeconds,
                DepthPriority, 2.0f);

            DrawDebugSphere(InWorld, End, 8.0f, SphereSegments, FColor::Green, Persistent,
                LifetimeSeconds, DepthPriority);
        }
        else if (WasBlocked)
        {
            DrawDebugLine(InWorld, InStart, Raycast._HitLocation, FColor::Red, Persistent,
                LifetimeSeconds, DepthPriority, 2.0f);

            DrawDebugSphere(InWorld, Raycast._HitLocation, 10.0f, SphereSegments, FColor::Red,
                Persistent, LifetimeSeconds, DepthPriority);

            // A cost stop has no edge to face away from, so the arrow is what separates the two
            // reasons a ray reports Blocked without reading the flag off the log line.
            if (NOT Raycast._HitNormal.IsNearlyZero())
            {
                DrawDebugDirectionalArrow(InWorld, Raycast._HitLocation,
                    Raycast._HitLocation + (Raycast._HitNormal * 50.0), ArrowSize, FColor::Yellow,
                    Persistent, LifetimeSeconds, DepthPriority, 2.0f);
            }
        }
        else
        {
            DrawDebugSphere(InWorld, InStart, 12.0f, SphereSegments, FColor::Red, Persistent,
                LifetimeSeconds, DepthPriority);
        }

        const auto HitText = WasBlocked
            ? FString::Printf(TEXT("(%.0f, %.0f, %.0f)"),
                Raycast._HitLocation.X, Raycast._HitLocation.Y, Raycast._HitLocation.Z)
            : FString{TEXT("none")};

        const auto Summary = FString::Printf(
            TEXT("ray %s | hit %s | cost %.2f | stopped on cost %s"),
            *ck::Format_UE(TEXT("{}"), Raycast._Status),
            *HitText,
            Raycast._AccumulatedCost,
            Raycast._StoppedOnCost ? TEXT("yes") : TEXT("no"));

        DrawDebugString(InWorld, InStart + FVector{0.0, 0.0, 20.0}, Summary, nullptr,
            Raycast.Get_IsClear() ? FColor::White : FColor::Red, LifetimeSeconds, DrawShadow);

        ck::groundnav::Display(TEXT("[GroundNav] from ({}, {}, {}) to ({}, {}) {}"),
            InStart.X, InStart.Y, InStart.Z, End.X, End.Y, Summary);
    }

    auto DoEdgesAndDraw(UWorld* InWorld, const FVector& InPoint, float InRadiusUu) -> void
    {
        if (NOT Get_HasDebugField(TEXT("ck.GroundNav.EdgesAt")))
        { return; }

        const auto& Field = *LastDebugField;

        const auto AgentRadiusUu = CVar_AgentRadiusUu.GetValueOnGameThread();

        // Two vertical quanta, the same window the probe uses: tight enough that the answer is about
        // THIS height rather than the whole column, wide enough to survive the bake's quantization.
        const auto VerticalWindowUu = Field._Params._Config.Get_CellHeightUu() * 2.0f;

        auto BoundaryQuery = ck::groundnav::FCk_GroundNav_BoundaryQuery{};

        BoundaryQuery._Location = InPoint;
        BoundaryQuery._RadiusUu = InRadiusUu;
        BoundaryQuery._VerticalWindowUu = VerticalWindowUu;
        BoundaryQuery._Agent._RadiusUu = AgentRadiusUu;
        BoundaryQuery._MaxSegments = 0;

        auto Segments = TArray<ck::groundnav::FCk_GroundNav_BoundarySegment>{};

        const auto Status = ck::groundnav::Get_BoundarySegments(Field, BoundaryQuery, Segments);

        auto ClosestQuery = ck::groundnav::FCk_GroundNav_ClosestBoundaryQuery{};

        ClosestQuery._Location = InPoint;
        ClosestQuery._MaxRadiusUu = InRadiusUu;
        ClosestQuery._VerticalWindowUu = VerticalWindowUu;
        ClosestQuery._Agent._RadiusUu = AgentRadiusUu;

        const auto Closest = ck::groundnav::Get_ClosestBoundary(Field, ClosestQuery);

        constexpr auto Persistent = true;
        constexpr auto DepthPriority = 0;
        constexpr auto DrawShadow = true;
        constexpr auto SphereSegments = 12;

        const auto LifetimeSeconds = CVar_LifetimeSeconds.GetValueOnGameThread();

        // Clear of the floor the runs bound, or every one of them z-fights the surface it belongs to.
        const auto Lift = FVector{
            0.0, 0.0, static_cast<double>(Field._Params._Config.Get_CellSizeUu()) * 0.5};

        DrawDebugSphere(InWorld, InPoint, 8.0f, SphereSegments, FColor::White, Persistent,
            LifetimeSeconds, DepthPriority);

        for (const auto& Segment : Segments)
        {
            DrawDebugLine(InWorld, Segment._Start + Lift, Segment._End + Lift, FColor::Yellow,
                Persistent, LifetimeSeconds, DepthPriority, 3.0f);

            // Which side of the run the floor is on - the whole reason a consumer asks for edges
            // rather than for geometry.
            const auto Midpoint = ((Segment._Start + Segment._End) * 0.5) + Lift;
            const auto Inward = FVector{Segment._InwardNormalXY.X, Segment._InwardNormalXY.Y, 0.0};

            DrawDebugLine(InWorld, Midpoint, Midpoint + (Inward * 20.0), FColor::Yellow, Persistent,
                LifetimeSeconds, DepthPriority, 1.5f);
        }

        if (Closest.Get_IsSuccess())
        {
            DrawDebugLine(InWorld, InPoint, Closest._ClosestPoint, FColor::Magenta, Persistent,
                LifetimeSeconds, DepthPriority, 2.0f);

            DrawDebugSphere(InWorld, Closest._ClosestPoint, 6.0f, SphereSegments, FColor::Magenta,
                Persistent, LifetimeSeconds, DepthPriority);
        }

        const auto Summary = FString::Printf(
            TEXT("edges %s | %d run(s) within %.0f uu | closest %s at %.1f uu"),
            *ck::Format_UE(TEXT("{}"), Status),
            Segments.Num(),
            InRadiusUu,
            *ck::Format_UE(TEXT("{}"), Closest._Status),
            Closest._DistanceUu);

        DrawDebugString(InWorld, InPoint + FVector{0.0, 0.0, 20.0}, Summary, nullptr,
            Status == ECk_NavSurface_QueryStatus::Success ? FColor::White : FColor::Red,
            LifetimeSeconds, DrawShadow);

        ck::groundnav::Display(TEXT("[GroundNav] at ({}, {}, {}) {}"),
            InPoint.X, InPoint.Y, InPoint.Z, Summary);
    }

    auto Get_ReachabilityName(ck::groundnav::ECk_GroundNav_Reachability InReachability) -> const TCHAR*
    {
        switch (InReachability)
        {
            case ck::groundnav::ECk_GroundNav_Reachability::Unreachable:
                return TEXT("unreachable");
            case ck::groundnav::ECk_GroundNav_Reachability::Unknown_OpenComponent:
                return TEXT("unknown - open component");
            default:
                return TEXT("possibly reachable");
        }
    }

    auto Get_ReachabilityColor(
        ECk_NavSurface_QueryStatus                InStatus,
        ck::groundnav::ECk_GroundNav_Reachability InReachability) -> FColor
    {
        // A verdict is only a verdict when both ends resolved to a surface. Grey rather than a
        // colour on the reachability scale, or a query that never ran would read as an answer.
        if (InStatus != ECk_NavSurface_QueryStatus::Success)
        { return FColor{130, 130, 130}; }

        switch (InReachability)
        {
            case ck::groundnav::ECk_GroundNav_Reachability::Unreachable:
                return FColor::Red;
            case ck::groundnav::ECk_GroundNav_Reachability::Unknown_OpenComponent:
                return FColor{255, 140, 0};
            default:
                return FColor::Green;
        }
    }

    auto DoReachAndDraw(UWorld* InWorld, const FVector& InStart, const FVector& InEnd) -> void
    {
        if (NOT Get_HasDebugField(TEXT("ck.GroundNav.ReachAt")))
        { return; }

        const auto& Field = *LastDebugField;

        auto ReachabilityQuery = ck::groundnav::FCk_GroundNav_ReachabilityQuery{};

        // Two vertical quanta at each end, the same window the probe and the walk use: tight enough
        // that the answer is about THIS height rather than the whole column, wide enough to survive
        // the bake's quantization.
        ReachabilityQuery._Start = InStart;
        ReachabilityQuery._End = InEnd;
        ReachabilityQuery._VerticalToleranceUu = Field._Params._Config.Get_CellHeightUu() * 2.0f;
        ReachabilityQuery._Agent._RadiusUu = CVar_AgentRadiusUu.GetValueOnGameThread();

        const auto Reach = ck::groundnav::Get_IsReachable(Field, ReachabilityQuery);

        constexpr auto Persistent = true;
        constexpr auto DepthPriority = 0;
        constexpr auto DrawShadow = true;
        constexpr auto SphereSegments = 12;

        const auto LifetimeSeconds = CVar_LifetimeSeconds.GetValueOnGameThread();

        DrawDebugSphere(InWorld, InStart, 8.0f, SphereSegments, FColor::White, Persistent,
            LifetimeSeconds, DepthPriority);

        DrawDebugSphere(InWorld, InEnd, 8.0f, SphereSegments, FColor::White, Persistent,
            LifetimeSeconds, DepthPriority);

        DrawDebugLine(InWorld, InStart, InEnd,
            Get_ReachabilityColor(Reach._Status, Reach._Reachability), Persistent, LifetimeSeconds,
            DepthPriority, 2.0f);

        const auto Summary = FString::Printf(
            TEXT("reach %s | %s | expansions %d"),
            *ck::Format_UE(TEXT("{}"), Reach._Status),
            Get_ReachabilityName(Reach._Reachability),
            Reach._ExpansionCount);

        const auto Midpoint = (InStart + InEnd) * 0.5;

        DrawDebugString(InWorld, Midpoint + FVector{0.0, 0.0, 20.0}, Summary, nullptr,
            Reach.Get_IsSuccess() ? FColor::White : FColor::Red, LifetimeSeconds, DrawShadow);

        ck::groundnav::Display(TEXT("[GroundNav] from ({}, {}, {}) to ({}, {}, {}) {}"),
            InStart.X, InStart.Y, InStart.Z, InEnd.X, InEnd.Y, InEnd.Z, Summary);
    }

    auto DoFloodAndDraw(UWorld* InWorld, const FVector& InSource, float InRadiusUu) -> void
    {
        if (NOT Get_HasDebugField(TEXT("ck.GroundNav.FloodAt")))
        { return; }

        const auto& Field = *LastDebugField;

        auto FloodQuery = ck::groundnav::FCk_GroundNav_FloodQuery{};

        FloodQuery._Source = InSource;
        FloodQuery._VerticalToleranceUu = Field._Params._Config.Get_CellHeightUu() * 2.0f;
        FloodQuery._Agent._RadiusUu = CVar_AgentRadiusUu.GetValueOnGameThread();
        FloodQuery._MaxDistanceUu = InRadiusUu;

        const auto Flood = ck::groundnav::Get_FloodFill(Field, FloodQuery);

        constexpr auto Persistent = true;
        constexpr auto DepthPriority = 0;
        constexpr auto DrawShadow = true;
        constexpr auto SphereSegments = 12;
        constexpr auto CornerCount = 4;

        const auto LifetimeSeconds = CVar_LifetimeSeconds.GetValueOnGameThread();

        DrawDebugSphere(InWorld, InSource, 8.0f, SphereSegments, FColor::White, Persistent,
            LifetimeSeconds, DepthPriority);

        const auto FlatPlateCount = ck::groundnav::Get_FlatPlateCount(Field);

        auto PlatesReached = 0;

        for (auto FlatPlate = 0; FlatPlate < FlatPlateCount; ++FlatPlate)
        {
            if (NOT Flood.Get_IsPlateReached(FlatPlate))
            { continue; }

            ++PlatesReached;

            // The source plate is reached at distance zero, so it anchors the ramp rather than
            // reading as the farthest thing on screen.
            auto LeastDistanceUu = FlatPlate == Flood._SourceFlatPlate
                ? 0.0
                : TNumericLimits<double>::Max();

            if (Flood._PlateEntries.IsValidIndex(FlatPlate))
            {
                for (const auto EntryIndex : Flood._PlateEntries[FlatPlate])
                {
                    if (NOT Flood._Crossings.IsValidIndex(EntryIndex))
                    { continue; }

                    LeastDistanceUu = FMath::Min(
                        LeastDistanceUu, Flood._Crossings[EntryIndex]._DistanceUu);
                }
            }

            int32 TileIndex = INDEX_NONE;
            int32 PlateIndex = INDEX_NONE;

            if (NOT ck::groundnav::Get_TileAndPlate(Field, FlatPlate, TileIndex, PlateIndex))
            { continue; }

            if (NOT Field._Tiles.IsValidIndex(TileIndex))
            { continue; }

            const auto& Tile = Field._Tiles[TileIndex];

            if (NOT Tile._Plates._Plates.IsValidIndex(PlateIndex))
            { continue; }

            const auto& Plate = Tile._Plates._Plates[PlateIndex];

            // One height for the whole loop, taken from the plate's highest cell: a plate that spans
            // a ramp would otherwise sink through the ground it is supposed to outline.
            auto HighestZ = TNumericLimits<double>::Lowest();

            for (auto Y = Plate._MinY; Y <= Plate._MaxY; ++Y)
            {
                for (auto X = Plate._MinX; X <= Plate._MaxX; ++X)
                {
                    if (NOT Tile.Get_HasSurfaceAt(X, Y, Plate._LayerIndex))
                    { continue; }

                    HighestZ = FMath::Max(HighestZ,
                        static_cast<double>(Tile.Get_SurfaceZAt(X, Y, Plate._LayerIndex)));
                }
            }

            const auto PlateHasSurface = HighestZ > TNumericLimits<double>::Lowest();

            if (NOT PlateHasSurface)
            { continue; }

            const auto CellUu = static_cast<double>(Tile._CellSizeUu);

            // Plate bounds are INCLUSIVE cell indices, so the far edge is one whole cell past MaxX/Y.
            const auto MinX = Tile._Origin.X + (static_cast<double>(Plate._MinX) * CellUu);
            const auto MinY = Tile._Origin.Y + (static_cast<double>(Plate._MinY) * CellUu);
            const auto MaxX = Tile._Origin.X + (static_cast<double>(Plate._MaxX + 1) * CellUu);
            const auto MaxY = Tile._Origin.Y + (static_cast<double>(Plate._MaxY + 1) * CellUu);
            const auto LoopZ = HighestZ + (CellUu * 0.5);

            const FVector Corners[CornerCount] = {
                FVector{MinX, MinY, LoopZ},
                FVector{MaxX, MinY, LoopZ},
                FVector{MaxX, MaxY, LoopZ},
                FVector{MinX, MaxY, LoopZ}};

            const auto Color = ck::groundnav::debugdraw_private::Get_ClearanceColor(
                static_cast<float>(LeastDistanceUu), InRadiusUu);

            for (auto CornerIndex = 0; CornerIndex < CornerCount; ++CornerIndex)
            {
                DrawDebugLine(InWorld, Corners[CornerIndex], Corners[(CornerIndex + 1) % CornerCount],
                    Color, Persistent, LifetimeSeconds, DepthPriority, 2.0f);
            }
        }

        auto FarthestUu = 0.0;

        for (const auto& Crossing : Flood._Crossings)
        {
            FarthestUu = FMath::Max(FarthestUu, Crossing._DistanceUu);

            DrawDebugSphere(InWorld, Crossing._EntryPoint, 4.0f, SphereSegments, FColor::Yellow,
                Persistent, LifetimeSeconds, DepthPriority);

            // A crossing left from the source plate has no predecessor, and the source point is what
            // its distance was string-pulled from - so the chain reads back to where it started.
            const auto PreviousPoint = Flood._Crossings.IsValidIndex(Crossing._Predecessor)
                ? Flood._Crossings[Crossing._Predecessor]._EntryPoint
                : Flood._SourcePoint;

            DrawDebugLine(InWorld, PreviousPoint, Crossing._EntryPoint, FColor::Yellow, Persistent,
                LifetimeSeconds, DepthPriority, 1.0f);
        }

        const auto Summary = FString::Printf(
            TEXT("flood %s | %d plates | %d crossings | expansions %d | farthest %.1f uu"),
            *ck::Format_UE(TEXT("{}"), Flood._Status),
            PlatesReached,
            Flood._Crossings.Num(),
            Flood._ExpansionCount,
            FarthestUu);

        DrawDebugString(InWorld, InSource + FVector{0.0, 0.0, 20.0}, Summary, nullptr,
            Flood.Get_IsSuccess() ? FColor::White : FColor::Red, LifetimeSeconds, DrawShadow);

        DoDraw_MarkupsIfEnabled(InWorld, LifetimeSeconds);

        ck::groundnav::Display(TEXT("[GroundNav] at ({}, {}, {}) within {} uu {}"),
            InSource.X, InSource.Y, InSource.Z, InRadiusUu, Summary);
    }

    auto DoDrawPlateOutline(
        UWorld*                                   InWorld,
        const ck::groundnav::FCk_GroundNav_Field& InField,
        int32                                     InFlatPlate,
        FColor                                    InColor,
        float                                     InLifetimeSeconds,
        float                                     InThickness) -> void
    {
        int32 TileIndex = INDEX_NONE;
        int32 PlateIndex = INDEX_NONE;

        if (NOT ck::groundnav::Get_TileAndPlate(InField, InFlatPlate, TileIndex, PlateIndex))
        { return; }

        if (NOT InField._Tiles.IsValidIndex(TileIndex))
        { return; }

        const auto& Tile = InField._Tiles[TileIndex];

        if (NOT Tile._Plates._Plates.IsValidIndex(PlateIndex))
        { return; }

        const auto& Plate = Tile._Plates._Plates[PlateIndex];

        // One height for the whole loop, taken from the plate's highest cell: a plate that spans a
        // ramp would otherwise sink through the ground it is supposed to outline.
        auto HighestZ = TNumericLimits<double>::Lowest();

        for (auto Y = Plate._MinY; Y <= Plate._MaxY; ++Y)
        {
            for (auto X = Plate._MinX; X <= Plate._MaxX; ++X)
            {
                if (NOT Tile.Get_HasSurfaceAt(X, Y, Plate._LayerIndex))
                { continue; }

                HighestZ = FMath::Max(HighestZ,
                    static_cast<double>(Tile.Get_SurfaceZAt(X, Y, Plate._LayerIndex)));
            }
        }

        const auto PlateHasSurface = HighestZ > TNumericLimits<double>::Lowest();

        if (NOT PlateHasSurface)
        { return; }

        constexpr auto Persistent = true;
        constexpr auto DepthPriority = 0;
        constexpr auto CornerCount = 4;

        const auto CellUu = static_cast<double>(Tile._CellSizeUu);

        // Plate bounds are INCLUSIVE cell indices, so the far edge is one whole cell past MaxX/Y.
        const auto MinX = Tile._Origin.X + (static_cast<double>(Plate._MinX) * CellUu);
        const auto MinY = Tile._Origin.Y + (static_cast<double>(Plate._MinY) * CellUu);
        const auto MaxX = Tile._Origin.X + (static_cast<double>(Plate._MaxX + 1) * CellUu);
        const auto MaxY = Tile._Origin.Y + (static_cast<double>(Plate._MaxY + 1) * CellUu);
        const auto LoopZ = HighestZ + (CellUu * 0.5);

        const FVector Corners[CornerCount] = {
            FVector{MinX, MinY, LoopZ},
            FVector{MaxX, MinY, LoopZ},
            FVector{MaxX, MaxY, LoopZ},
            FVector{MinX, MaxY, LoopZ}};

        for (auto CornerIndex = 0; CornerIndex < CornerCount; ++CornerIndex)
        {
            DrawDebugLine(InWorld, Corners[CornerIndex], Corners[(CornerIndex + 1) % CornerCount],
                InColor, Persistent, InLifetimeSeconds, DepthPriority, InThickness);
        }
    }

    auto DoPathAndDraw(UWorld* InWorld, const FVector& InStart, const FVector& InGoal) -> void
    {
        if (NOT Get_HasDebugField(TEXT("ck.GroundNav.PathAt")))
        { return; }

        const auto& Field = *LastDebugField;

        const auto AgentRadiusUu = CVar_AgentRadiusUu.GetValueOnGameThread();

        auto PathQuery = ck::groundnav::FCk_GroundNav_PathQuery{};

        // Two vertical quanta at each end, the same window the probe, the walk and the reachability
        // check are given, so the commands resolve one point onto one storey rather than disagreeing
        // about which floor it stands on.
        PathQuery._Start = InStart;
        PathQuery._Goal = InGoal;
        PathQuery._VerticalToleranceUu = Field._Params._Config.Get_CellHeightUu() * 2.0f;
        PathQuery._Agent._RadiusUu = AgentRadiusUu;

        // Admissible, so a route that looks wrong is the field's fault and never the weight's.
        PathQuery._GreedyWeightW = 1.0f;

        PathQuery._Cost._SlopePenaltyK = CVar_SlopePenaltyK.GetValueOnGameThread();
        PathQuery._Cost._ClearanceBiasK = CVar_ClearanceBiasK.GetValueOnGameThread();
        PathQuery._Cost._CornerOffsetK = CVar_CornerOffsetK.GetValueOnGameThread();

        const auto Path = ck::groundnav::Get_Path(LastDebugField, PathQuery);

        const auto PathIsPartial = Path._Status == ECk_GroundNav_PathStatus::Partial;

        // A Partial answers the prefix it actually reached, so it draws like a route and not like a
        // failure - the two are told apart by where the string ends, not by whether one is drawn.
        const auto PathIsWalkable =
            Path._Status == ECk_GroundNav_PathStatus::Ready || PathIsPartial;

        auto PreOffsetWaypoints = TArray<FVector>{};
        auto LengthUu = 0.0;
        auto Plan = ck::groundnav::FCk_GroundNav_PathPlan{};

        if (PathIsWalkable)
        {
            LengthUu = ck::groundnav::Get_Funnelled(Path, AgentRadiusUu, PreOffsetWaypoints);

            auto PostParams = ck::groundnav::FCk_GroundNav_PathPostParams{};
            PostParams._Agent = PathQuery._Agent;
            PostParams._VerticalToleranceUu = PathQuery._VerticalToleranceUu;
            PostParams._AgentLocation = Path._StartPoint;
            PostParams._Cost = PathQuery._Cost;

            Plan = ck::groundnav::Get_PathPlan(Path, Field, PostParams);
        }

        constexpr auto Persistent = true;
        constexpr auto DepthPriority = 0;
        constexpr auto DrawShadow = true;
        constexpr auto SphereSegments = 12;
        constexpr auto CorridorThickness = 1.5f;

        const auto LifetimeSeconds = CVar_LifetimeSeconds.GetValueOnGameThread();

        // The ends the search planned between, when it has them: the route is string-pulled from
        // those and not from the two points that were typed.
        const auto StartMarker = PathIsWalkable ? Path._StartPoint : InStart;
        const auto GoalMarker = PathIsWalkable ? Path._GoalPoint : InGoal;
        const auto MarkerColor = PathIsWalkable ? FColor::White : FColor::Red;

        DrawDebugSphere(InWorld, StartMarker, 8.0f, SphereSegments, MarkerColor, Persistent,
            LifetimeSeconds, DepthPriority);

        DrawDebugSphere(InWorld, GoalMarker, 8.0f, SphereSegments, MarkerColor, Persistent,
            LifetimeSeconds, DepthPriority);

        const auto CorridorColor = FColor{90, 150, 255};

        for (const auto FlatPlate : Path._PlateCorridor)
        {
            DoDrawPlateOutline(
                InWorld, Field, FlatPlate, CorridorColor, LifetimeSeconds, CorridorThickness);
        }

        for (const auto& Crossing : Path._Crossings)
        {
            DrawDebugLine(InWorld, Crossing._Left, Crossing._Right, FColor::Yellow, Persistent,
                LifetimeSeconds, DepthPriority, 2.0f);

            DrawDebugSphere(InWorld, ck::groundnav::Get_CrossingTransitionPoint(Crossing), 4.0f,
                SphereSegments, FColor::Yellow, Persistent, LifetimeSeconds, DepthPriority);
        }

        // Clear of the floors the route runs over, or the string z-fights every one of them.
        const auto Lift = FVector{0.0, 0.0, 4.0};

        // The funnel before the corner pass, thin and dim under the route that is actually walked,
        // so what the offset moved is the only thing that reads as two lines.
        constexpr auto PreOffsetThickness = 1.5f;
        const auto PreOffsetColor = FColor{110, 50, 110};

        for (auto WaypointIndex = 1; WaypointIndex < PreOffsetWaypoints.Num(); ++WaypointIndex)
        {
            DrawDebugLine(InWorld, PreOffsetWaypoints[WaypointIndex - 1] + Lift,
                PreOffsetWaypoints[WaypointIndex] + Lift, PreOffsetColor, Persistent,
                LifetimeSeconds, DepthPriority, PreOffsetThickness);
        }

        for (auto WaypointIndex = 1; WaypointIndex < Plan._Waypoints.Num(); ++WaypointIndex)
        {
            DrawDebugLine(InWorld, Plan._Waypoints[WaypointIndex - 1]._Location + Lift,
                Plan._Waypoints[WaypointIndex]._Location + Lift, FColor::Magenta, Persistent,
                LifetimeSeconds, DepthPriority, 4.0f);
        }

        constexpr auto ArrowSize = 10.0f;
        constexpr auto ArrowReachUu = 30.0;
        constexpr auto ArrowThickness = 2.0f;

        for (const auto& Waypoint : Plan._Waypoints)
        {
            if (Waypoint._DirectionToNext.IsNearlyZero())
            { continue; }

            const auto ArrowStart = Waypoint._Location + Lift;

            DrawDebugDirectionalArrow(InWorld, ArrowStart,
                ArrowStart + (Waypoint._DirectionToNext * ArrowReachUu), ArrowSize,
                FColor::Magenta, Persistent, LifetimeSeconds, DepthPriority, ArrowThickness);
        }

        // Where the search gave up, which is the one thing a Partial has to say and the goal marker
        // cannot: the white sphere still sits on ground the route never reached.
        if (PathIsPartial && NOT Plan._Waypoints.IsEmpty())
        {
            const auto TerminalColor = FColor{255, 140, 0};

            DrawDebugSphere(InWorld, Plan._Waypoints.Last()._Location, 12.0f, SphereSegments,
                TerminalColor, Persistent, LifetimeSeconds, DepthPriority);
        }

        const auto PlanCost = Plan._Waypoints.IsEmpty()
            ? 0.0
            : Plan._Waypoints.Last()._CostFromStart;

        const auto Summary = FString::Printf(
            TEXT("path %s | plates %d | crossings %d | expansions %d | len %.1f uu | cells %d")
            TEXT(" | wp %d | dist %.1f uu | cost %.1f"),
            *ck::Format_UE(TEXT("{}"), Path._Status),
            Path._PlateCorridor.Num(),
            Path._Crossings.Num(),
            Path._ExpansionCount,
            LengthUu,
            Path._Cost._CellsRead,
            Plan._Waypoints.Num(),
            Plan._LengthUu,
            PlanCost);

        DrawDebugString(InWorld, StartMarker + FVector{0.0, 0.0, 20.0}, Summary, nullptr,
            MarkerColor, LifetimeSeconds, DrawShadow);

        // A route that ignores a painted area and a route planned before the paint landed are the
        // same picture without the outline that says where the area is.
        DoDraw_MarkupsIfEnabled(InWorld, LifetimeSeconds);

        ck::groundnav::Display(TEXT("[GroundNav] from ({}, {}, {}) to ({}, {}, {}) {}"),
            InStart.X, InStart.Y, InStart.Z, InGoal.X, InGoal.Y, InGoal.Z, Summary);
    }

    auto DoDrawGeneratedPoints(
        UWorld*                                                    InWorld,
        const TArray<ck::groundnav::FCk_GroundNav_GeneratedPoint>& InPoints,
        float                                                      InLifetimeSeconds) -> void
    {
        constexpr auto Persistent = true;
        constexpr auto DepthPriority = 0;
        constexpr auto SphereSegments = 8;

        for (const auto& Point : InPoints)
        {
            DrawDebugSphere(InWorld, Point._Location, 4.0f, SphereSegments,
                ck::groundnav::debugdraw_private::Get_LayerColor(Point._Surface._LayerIndex),
                Persistent, InLifetimeSeconds, DepthPriority);
        }
    }

    auto DoRandomPointsAndDraw(
        UWorld*        InWorld,
        const FVector& InOrigin,
        float          InRadiusUu,
        int32          InCount) -> void
    {
        if (NOT Get_HasDebugField(TEXT("ck.GroundNav.PointsAt")))
        { return; }

        const auto& Field = *LastDebugField;

        auto PointsQuery = ck::groundnav::FCk_GroundNav_RandomPointsQuery{};

        PointsQuery._Origin = InOrigin;
        PointsQuery._RadiusUu = InRadiusUu;
        PointsQuery._Agent._RadiusUu = CVar_AgentRadiusUu.GetValueOnGameThread();
        PointsQuery._Count = InCount;
        PointsQuery._Seed = 0;

        const auto Points = ck::groundnav::Get_RandomPointsInRadius(Field, PointsQuery);

        constexpr auto Persistent = true;
        constexpr auto DepthPriority = 0;
        constexpr auto DrawShadow = true;
        constexpr auto SphereSegments = 12;

        const auto LifetimeSeconds = CVar_LifetimeSeconds.GetValueOnGameThread();

        DrawDebugSphere(InWorld, InOrigin, 8.0f, SphereSegments, FColor::White, Persistent,
            LifetimeSeconds, DepthPriority);

        DoDrawGeneratedPoints(InWorld, Points._Points, LifetimeSeconds);

        const auto Summary = FString::Printf(
            TEXT("points %s | %d of %d asked within %.0f uu | %d draw(s) spent | %d cells read"),
            *ck::Format_UE(TEXT("{}"), Points._Status),
            Points._Points.Num(),
            InCount,
            InRadiusUu,
            Points._Attempts,
            Points._Cost._CellsRead);

        DrawDebugString(InWorld, InOrigin + FVector{0.0, 0.0, 20.0}, Summary, nullptr,
            Points.Get_IsSuccess() ? FColor::White : FColor::Red, LifetimeSeconds, DrawShadow);

        ck::groundnav::Display(TEXT("[GroundNav] at ({}, {}, {}) {}"),
            InOrigin.X, InOrigin.Y, InOrigin.Z, Summary);
    }

    auto DoPathDistancePointsAndDraw(
        UWorld*        InWorld,
        const FVector& InOrigin,
        float          InMinDistanceUu,
        float          InMaxDistanceUu,
        int32          InCount) -> void
    {
        if (NOT Get_HasDebugField(TEXT("ck.GroundNav.FarPointsAt")))
        { return; }

        const auto& Field = *LastDebugField;

        auto PointsQuery = ck::groundnav::FCk_GroundNav_PathDistancePointsQuery{};

        // The same two vertical quanta the flood fill is given, because this generator IS a flood
        // fill: a wider window would resolve the origin onto a storey ck.GroundNav.FloodAt at the
        // same point would not, and the two commands would disagree about what is near.
        PointsQuery._Origin = InOrigin;
        PointsQuery._MinDistanceUu = InMinDistanceUu;
        PointsQuery._MaxDistanceUu = InMaxDistanceUu;
        PointsQuery._VerticalToleranceUu = Field._Params._Config.Get_CellHeightUu() * 2.0f;
        PointsQuery._Agent._RadiusUu = CVar_AgentRadiusUu.GetValueOnGameThread();
        PointsQuery._Count = InCount;
        PointsQuery._Seed = 0;

        const auto Points = ck::groundnav::Get_RandomPointsByPathDistance(Field, PointsQuery);

        constexpr auto Persistent = true;
        constexpr auto DepthPriority = 0;
        constexpr auto DrawShadow = true;
        constexpr auto SphereSegments = 12;

        const auto LifetimeSeconds = CVar_LifetimeSeconds.GetValueOnGameThread();

        DrawDebugSphere(InWorld, InOrigin, 8.0f, SphereSegments, FColor::White, Persistent,
            LifetimeSeconds, DepthPriority);

        DoDrawGeneratedPoints(InWorld, Points._Points, LifetimeSeconds);

        const auto Summary = FString::Printf(
            TEXT("far points %s | %d of %d asked walked [%.0f, %.0f] uu | %d draw(s) spent | %d cells read"),
            *ck::Format_UE(TEXT("{}"), Points._Status),
            Points._Points.Num(),
            InCount,
            InMinDistanceUu,
            InMaxDistanceUu,
            Points._Attempts,
            Points._Cost._CellsRead);

        DrawDebugString(InWorld, InOrigin + FVector{0.0, 0.0, 20.0}, Summary, nullptr,
            Points.Get_IsSuccess() ? FColor::White : FColor::Red, LifetimeSeconds, DrawShadow);

        ck::groundnav::Display(TEXT("[GroundNav] at ({}, {}, {}) {}"),
            InOrigin.X, InOrigin.Y, InOrigin.Z, Summary);
    }

    auto DoGridPointsAndDraw(
        UWorld*        InWorld,
        const FVector& InCentre,
        float          InHalfExtentUu,
        float          InSpacingUu) -> void
    {
        if (NOT Get_HasDebugField(TEXT("ck.GroundNav.GridAt")))
        { return; }

        const auto& Field = *LastDebugField;

        // Vertically the box reaches as far as the projection probe searches, and no further: a
        // lattice position and a probe at that position must agree on which storeys exist there.
        const auto HalfUu = static_cast<double>(InHalfExtentUu);
        const auto UpUu = static_cast<double>(CVar_ProbeUpUu.GetValueOnGameThread());
        const auto DownUu = static_cast<double>(CVar_ProbeDownUu.GetValueOnGameThread());

        const auto Bounds = FBox{
            FVector{InCentre.X - HalfUu, InCentre.Y - HalfUu, InCentre.Z - DownUu},
            FVector{InCentre.X + HalfUu, InCentre.Y + HalfUu, InCentre.Z + UpUu}};

        auto GridQuery = ck::groundnav::FCk_GroundNav_GridPointsQuery{};

        GridQuery._Bounds = Bounds;
        GridQuery._SpacingUu = InSpacingUu;
        GridQuery._AlignToLattice = ECk_EnableDisable::Enable;
        GridQuery._Agent._RadiusUu = CVar_AgentRadiusUu.GetValueOnGameThread();

        const auto Points = ck::groundnav::Get_GridPoints(Field, GridQuery);

        constexpr auto Persistent = true;
        constexpr auto DepthPriority = 0;
        constexpr auto DrawShadow = true;
        constexpr auto SphereSegments = 12;

        const auto LifetimeSeconds = CVar_LifetimeSeconds.GetValueOnGameThread();

        DrawDebugSphere(InWorld, InCentre, 8.0f, SphereSegments, FColor::White, Persistent,
            LifetimeSeconds, DepthPriority);

        DrawDebugBox(InWorld, Bounds.GetCenter(), Bounds.GetExtent(), FColor{130, 130, 130},
            Persistent, LifetimeSeconds, DepthPriority, 1.5f);

        DoDrawGeneratedPoints(InWorld, Points._Points, LifetimeSeconds);

        const auto Summary = FString::Printf(
            TEXT("grid %s | %d point(s) at %.0f uu spacing | attempts %d | %d cells read"),
            *ck::Format_UE(TEXT("{}"), Points._Status),
            Points._Points.Num(),
            InSpacingUu,
            Points._Attempts,
            Points._Cost._CellsRead);

        DrawDebugString(InWorld, InCentre + FVector{0.0, 0.0, 20.0}, Summary, nullptr,
            Points.Get_IsSuccess() ? FColor::White : FColor::Red, LifetimeSeconds, DrawShadow);

        ck::groundnav::Display(TEXT("[GroundNav] at ({}, {}, {}) {}"),
            InCentre.X, InCentre.Y, InCentre.Z, Summary);
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

    static FAutoConsoleCommandWithWorldAndArgs ConsoleCommand_BakeFieldAt(
        TEXT("ck.GroundNav.BakeFieldAt"),
        TEXT("Bake a TILED field around an explicit world point: ck.GroundNav.BakeFieldAt <X> <Y> <Z>. ")
        TEXT("Same pipeline as ck.GroundNav.BakeAt, but split into tiles that each bake with their own ")
        TEXT("halo and are then joined by seam crossings - draw mode 5 shows the lattice and those ")
        TEXT("seams. Tile size comes from ck.GroundNav.Debug.TileSizeUu."),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& InArgs, UWorld* InWorld) -> void
        {
            const auto WorldIsValid = ck::IsValid(InWorld);

            CK_ENSURE_IF_NOT(WorldIsValid, TEXT("ck.GroundNav.BakeFieldAt ran without a World"))
            { return; }

            if (InArgs.Num() != 3)
            {
                ck::groundnav::Warning(
                    TEXT("ck.GroundNav.BakeFieldAt needs three numbers: ")
                    TEXT("ck.GroundNav.BakeFieldAt <X> <Y> <Z>"));
                return;
            }

            const auto Centre = FVector{
                FCString::Atod(*InArgs[0]), FCString::Atod(*InArgs[1]), FCString::Atod(*InArgs[2])};

            DoBakeFieldAndDraw(InWorld, Centre);
        }));

    static FAutoConsoleCommandWithWorldAndArgs ConsoleCommand_ProbeAt(
        TEXT("ck.GroundNav.ProbeAt"),
        TEXT("Project a point onto the last field bake and draw the answer: ")
        TEXT("ck.GroundNav.ProbeAt <X> <Y> <Z>. Needs ck.GroundNav.BakeFieldAt to have run - a ")
        TEXT("region bake produces no field to query. The search box comes from ")
        TEXT("ck.GroundNav.Debug.Probe*, the body radius from ck.GroundNav.Debug.AgentRadiusUu."),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& InArgs, UWorld* InWorld) -> void
        {
            const auto WorldIsValid = ck::IsValid(InWorld);

            CK_ENSURE_IF_NOT(WorldIsValid, TEXT("ck.GroundNav.ProbeAt ran without a World"))
            { return; }

            if (InArgs.Num() != 3)
            {
                ck::groundnav::Warning(
                    TEXT("ck.GroundNav.ProbeAt needs three numbers: ck.GroundNav.ProbeAt <X> <Y> <Z>"));
                return;
            }

            const auto Point = FVector{
                FCString::Atod(*InArgs[0]), FCString::Atod(*InArgs[1]), FCString::Atod(*InArgs[2])};

            DoProbeAndDraw(InWorld, Point);
        }));

    static FAutoConsoleCommandWithWorldAndArgs ConsoleCommand_MarkupAt(
        TEXT("ck.GroundNav.MarkupAt"),
        TEXT("Report the area markup this world's ground-nav volumes hold, and the plate under a ")
        TEXT("point: ck.GroundNav.MarkupAt <X> <Y> <Z>. Every volume is listed with where the point ")
        TEXT("falls on it, because a record painted before anything baked lives on a volume that ")
        TEXT("covers nothing yet. Each record prints its id, kind, area tag, cost multiplier, ")
        TEXT("enabled state, world bounds, the epoch it was submitted against, and whether the ")
        TEXT("NEUTRAL facade reports it live - a record the volume holds is not the same thing as a ")
        TEXT("paint the surface has applied, and the gap between the two is what a repath that ")
        TEXT("crosses a fresh paint is made of. The plate line names the policy index the plate ")
        TEXT("carries, the tags that index names and what crossing it costs. Every record also ")
        TEXT("outlines in the world: impassable red, cost amber, disabled dashed grey. Unlike the ")
        TEXT("query commands this reads the volumes' PUBLISHED fields, so ck.GroundNav.BakeFieldAt ")
        TEXT("is not needed."),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& InArgs, UWorld* InWorld) -> void
        {
            const auto WorldIsValid = ck::IsValid(InWorld);

            CK_ENSURE_IF_NOT(WorldIsValid, TEXT("ck.GroundNav.MarkupAt ran without a World"))
            { return; }

            if (InArgs.Num() != 3)
            {
                ck::groundnav::Warning(
                    TEXT("ck.GroundNav.MarkupAt needs three numbers: ck.GroundNav.MarkupAt <X> <Y> <Z>"));
                return;
            }

            const auto Point = FVector{
                FCString::Atod(*InArgs[0]), FCString::Atod(*InArgs[1]), FCString::Atod(*InArgs[2])};

            DoMarkupAndReport(InWorld, Point);
        }));

    static FAutoConsoleCommandWithWorld ConsoleCommand_Probe(
        TEXT("ck.GroundNav.Probe"),
        TEXT("Project the player's own position onto the last field bake and draw the answer. ")
        TEXT("Use ck.GroundNav.ProbeAt to aim at a fixed point instead."),
        FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* InWorld) -> void
        {
            const auto WorldIsValid = ck::IsValid(InWorld);

            CK_ENSURE_IF_NOT(WorldIsValid, TEXT("ck.GroundNav.Probe ran without a World"))
            { return; }

            auto Point = FVector::ZeroVector;

            const auto FoundViewer = Get_ViewerLocation(InWorld, Point);

            CK_ENSURE_IF_NOT(FoundViewer,
                TEXT("ck.GroundNav.Probe found no player to probe from. Run it in PIE or in a game world."))
            { return; }

            DoProbeAndDraw(InWorld, Point);
        }));

    static FAutoConsoleCommandWithWorldAndArgs ConsoleCommand_WalkAt(
        TEXT("ck.GroundNav.WalkAt"),
        TEXT("Walk a body along the last field bake and draw where it ended: ")
        TEXT("ck.GroundNav.WalkAt <X> <Y> <Z> <TX> <TY>. The target takes the start's height, ")
        TEXT("because a walk reads it in XY only. Needs ck.GroundNav.BakeFieldAt to have run - a ")
        TEXT("region bake produces no field to query. The body radius comes from ")
        TEXT("ck.GroundNav.Debug.AgentRadiusUu."),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& InArgs, UWorld* InWorld) -> void
        {
            const auto WorldIsValid = ck::IsValid(InWorld);

            CK_ENSURE_IF_NOT(WorldIsValid, TEXT("ck.GroundNav.WalkAt ran without a World"))
            { return; }

            auto Start = FVector::ZeroVector;
            auto TargetXY = FVector2D::ZeroVector;

            if (NOT Get_FivePointArgs(InArgs, Start, TargetXY))
            {
                ck::groundnav::Warning(
                    TEXT("ck.GroundNav.WalkAt needs five numbers: ")
                    TEXT("ck.GroundNav.WalkAt <X> <Y> <Z> <TX> <TY>"));
                return;
            }

            DoWalkAndDraw(InWorld, Start, TargetXY);
        }));

    static FAutoConsoleCommandWithWorldAndArgs ConsoleCommand_RayAt(
        TEXT("ck.GroundNav.RayAt"),
        TEXT("Test whether a body can walk a straight segment on the last field bake and draw the ")
        TEXT("answer: ck.GroundNav.RayAt <X> <Y> <Z> <TX> <TY>. The end takes the start's height. ")
        TEXT("Unlike ck.GroundNav.WalkAt the ray does not slide - the first refused step is the hit, ")
        TEXT("drawn with the crossed edge's normal facing back along the ray."),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& InArgs, UWorld* InWorld) -> void
        {
            const auto WorldIsValid = ck::IsValid(InWorld);

            CK_ENSURE_IF_NOT(WorldIsValid, TEXT("ck.GroundNav.RayAt ran without a World"))
            { return; }

            auto Start = FVector::ZeroVector;
            auto TargetXY = FVector2D::ZeroVector;

            if (NOT Get_FivePointArgs(InArgs, Start, TargetXY))
            {
                ck::groundnav::Warning(
                    TEXT("ck.GroundNav.RayAt needs five numbers: ")
                    TEXT("ck.GroundNav.RayAt <X> <Y> <Z> <TX> <TY>"));
                return;
            }

            DoRaycastAndDraw(InWorld, Start, TargetXY);
        }));

    static FAutoConsoleCommandWithWorldAndArgs ConsoleCommand_EdgesAt(
        TEXT("ck.GroundNav.EdgesAt"),
        TEXT("Ask the last field bake which walls are near a point and draw them: ")
        TEXT("ck.GroundNav.EdgesAt <X> <Y> <Z> <R>. Every run within R draws in yellow with a tick ")
        TEXT("showing which side of it is walkable; the nearest run's closest point draws in magenta. ")
        TEXT("Needs ck.GroundNav.BakeFieldAt to have run - a region bake produces no field to query. ")
        TEXT("The body radius comes from ck.GroundNav.Debug.AgentRadiusUu."),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& InArgs, UWorld* InWorld) -> void
        {
            const auto WorldIsValid = ck::IsValid(InWorld);

            CK_ENSURE_IF_NOT(WorldIsValid, TEXT("ck.GroundNav.EdgesAt ran without a World"))
            { return; }

            if (InArgs.Num() != 4)
            {
                ck::groundnav::Warning(
                    TEXT("ck.GroundNav.EdgesAt needs four numbers: ")
                    TEXT("ck.GroundNav.EdgesAt <X> <Y> <Z> <R>"));
                return;
            }

            const auto Point = FVector{
                FCString::Atod(*InArgs[0]), FCString::Atod(*InArgs[1]), FCString::Atod(*InArgs[2])};

            DoEdgesAndDraw(InWorld, Point, FCString::Atof(*InArgs[3]));
        }));

    static FAutoConsoleCommandWithWorldAndArgs ConsoleCommand_ReachAt(
        TEXT("ck.GroundNav.ReachAt"),
        TEXT("Ask the last field bake whether two points can possibly be joined, and draw the ")
        TEXT("verdict: ck.GroundNav.ReachAt <X> <Y> <Z> <TX> <TY> <TZ>. The answer is read off the ")
        TEXT("bake's component labels and expands nothing, so it can prove two points APART but ")
        TEXT("never prove them joined for a particular body - a doorway they share may be too ")
        TEXT("narrow for it. Green is possibly reachable, red unreachable, orange means one ")
        TEXT("component borders unbaked ground so the crossing that would join them may simply not ")
        TEXT("have been looked at, and grey means an end resolved to no surface. Needs ")
        TEXT("ck.GroundNav.BakeFieldAt to have run - a region bake produces no field to query. The ")
        TEXT("body radius comes from ck.GroundNav.Debug.AgentRadiusUu."),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& InArgs, UWorld* InWorld) -> void
        {
            const auto WorldIsValid = ck::IsValid(InWorld);

            CK_ENSURE_IF_NOT(WorldIsValid, TEXT("ck.GroundNav.ReachAt ran without a World"))
            { return; }

            auto Start = FVector::ZeroVector;
            auto End = FVector::ZeroVector;

            if (NOT Get_SixPointArgs(InArgs, Start, End))
            {
                ck::groundnav::Warning(
                    TEXT("ck.GroundNav.ReachAt needs six numbers: ")
                    TEXT("ck.GroundNav.ReachAt <X> <Y> <Z> <TX> <TY> <TZ>"));
                return;
            }

            DoReachAndDraw(InWorld, Start, End);
        }));

    static FAutoConsoleCommandWithWorldAndArgs ConsoleCommand_PathAt(
        TEXT("ck.GroundNav.PathAt"),
        TEXT("Ask the last field bake for a path between two points and draw it: ")
        TEXT("ck.GroundNav.PathAt <X> <Y> <Z> <TX> <TY> <TZ>. Unlike ck.GroundNav.ReachAt this ")
        TEXT("expands the plate graph, so it answers for a body of a particular size: the corridor's ")
        TEXT("plates outline in blue, every crossing draws as its interval in yellow with a sphere ")
        TEXT("at the point a leg is priced through, the raw funnel draws as a thin dim line, and the ")
        TEXT("route walked over it draws in bright magenta with an arrow at each waypoint, between ")
        TEXT("the ends the search resolved. Partial draws the prefix the search actually reached, ")
        TEXT("ending in an orange sphere where it gave up. Any other status short of Ready draws the ")
        TEXT("two ends in red and nothing between them - a corridor that ran out of budget is not a ")
        TEXT("shorter corridor. Needs ck.GroundNav.BakeFieldAt to have run - a region bake produces ")
        TEXT("no field to query. The body radius comes from ck.GroundNav.Debug.AgentRadiusUu, and is ")
        TEXT("the inset the funnel walks the route through; the cost model comes from ")
        TEXT("ck.GroundNav.Debug.SlopePenaltyK, .ClearanceBiasK and .CornerOffsetK."),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& InArgs, UWorld* InWorld) -> void
        {
            const auto WorldIsValid = ck::IsValid(InWorld);

            CK_ENSURE_IF_NOT(WorldIsValid, TEXT("ck.GroundNav.PathAt ran without a World"))
            { return; }

            auto Start = FVector::ZeroVector;
            auto Goal = FVector::ZeroVector;

            if (NOT Get_SixPointArgs(InArgs, Start, Goal))
            {
                ck::groundnav::Warning(
                    TEXT("ck.GroundNav.PathAt needs six numbers: ")
                    TEXT("ck.GroundNav.PathAt <X> <Y> <Z> <TX> <TY> <TZ>"));
                return;
            }

            DoPathAndDraw(InWorld, Start, Goal);
        }));

    static FAutoConsoleCommandWithWorldAndArgs ConsoleCommand_FloodAt(
        TEXT("ck.GroundNav.FloodAt"),
        TEXT("Flood the last field bake outward from a point and draw how far it got: ")
        TEXT("ck.GroundNav.FloodAt <X> <Y> <Z> <R>. R is a WALKED-distance limit, not a radius on ")
        TEXT("screen - a plate a few metres away around a corner can be far past R, and a crossing ")
        TEXT("whose walked distance exceeds R is never settled. Every plate the flood reached draws ")
        TEXT("as a rectangle coloured by the SHORTEST walked distance to enter it, on the same ramp ")
        TEXT("the clearance view uses: the source plate sits at one end of it and a plate entered ")
        TEXT("at R at the other. Each settled crossing draws in yellow at the point the shortest ")
        TEXT("path passes through it, joined back to the crossing it came from. Needs ")
        TEXT("ck.GroundNav.BakeFieldAt to have run - a region bake produces no field to query. The ")
        TEXT("body radius comes from ck.GroundNav.Debug.AgentRadiusUu, and a crossing narrower than ")
        TEXT("it is never admitted."),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& InArgs, UWorld* InWorld) -> void
        {
            const auto WorldIsValid = ck::IsValid(InWorld);

            CK_ENSURE_IF_NOT(WorldIsValid, TEXT("ck.GroundNav.FloodAt ran without a World"))
            { return; }

            if (InArgs.Num() != 4)
            {
                ck::groundnav::Warning(
                    TEXT("ck.GroundNav.FloodAt needs four numbers: ")
                    TEXT("ck.GroundNav.FloodAt <X> <Y> <Z> <R>"));
                return;
            }

            const auto Source = FVector{
                FCString::Atod(*InArgs[0]), FCString::Atod(*InArgs[1]), FCString::Atod(*InArgs[2])};

            DoFloodAndDraw(InWorld, Source, FCString::Atof(*InArgs[3]));
        }));

    static FAutoConsoleCommandWithWorldAndArgs ConsoleCommand_PointsAt(
        TEXT("ck.GroundNav.PointsAt"),
        TEXT("Draw N random points on the last field bake within a horizontal radius of a point: ")
        TEXT("ck.GroundNav.PointsAt <X> <Y> <Z> <R> <N>. The disc is horizontal, so every storey it ")
        TEXT("touches is drawn from - a balcony over the same ground is as eligible as the ground. ")
        TEXT("Points are uniform by AREA and take no notice of whether they can be walked to: a ")
        TEXT("point across a wall is inside the radius, and ck.GroundNav.FarPointsAt is the command ")
        TEXT("that asks the other question. Each point draws as a sphere coloured by its layer. ")
        TEXT("Needs ck.GroundNav.BakeFieldAt to have run - a region bake produces no field to query. ")
        TEXT("The body radius comes from ck.GroundNav.Debug.AgentRadiusUu."),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& InArgs, UWorld* InWorld) -> void
        {
            const auto WorldIsValid = ck::IsValid(InWorld);

            CK_ENSURE_IF_NOT(WorldIsValid, TEXT("ck.GroundNav.PointsAt ran without a World"))
            { return; }

            if (InArgs.Num() != 5)
            {
                ck::groundnav::Warning(
                    TEXT("ck.GroundNav.PointsAt needs five numbers: ")
                    TEXT("ck.GroundNav.PointsAt <X> <Y> <Z> <R> <N>"));
                return;
            }

            const auto Origin = FVector{
                FCString::Atod(*InArgs[0]), FCString::Atod(*InArgs[1]), FCString::Atod(*InArgs[2])};

            DoRandomPointsAndDraw(
                InWorld, Origin, FCString::Atof(*InArgs[3]), FCString::Atoi(*InArgs[4]));
        }));

    static FAutoConsoleCommandWithWorldAndArgs ConsoleCommand_FarPointsAt(
        TEXT("ck.GroundNav.FarPointsAt"),
        TEXT("Draw N random points whose WALKED distance from a point falls in a band: ")
        TEXT("ck.GroundNav.FarPointsAt <X> <Y> <Z> <MIN> <MAX> <N>. The band is measured the way ")
        TEXT("ck.GroundNav.FloodAt measures - around corners and through the doorways the body fits ")
        TEXT("through - so a point a stride away across a wall is far, not near. The draw count is ")
        TEXT("bounded, so fewer than N points is a legitimate answer and the label says how many ")
        TEXT("draws it spent. Each point draws as a sphere coloured by its layer. Needs ")
        TEXT("ck.GroundNav.BakeFieldAt to have run - a region bake produces no field to query. The ")
        TEXT("body radius comes from ck.GroundNav.Debug.AgentRadiusUu."),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& InArgs, UWorld* InWorld) -> void
        {
            const auto WorldIsValid = ck::IsValid(InWorld);

            CK_ENSURE_IF_NOT(WorldIsValid, TEXT("ck.GroundNav.FarPointsAt ran without a World"))
            { return; }

            if (InArgs.Num() != 6)
            {
                ck::groundnav::Warning(
                    TEXT("ck.GroundNav.FarPointsAt needs six numbers: ")
                    TEXT("ck.GroundNav.FarPointsAt <X> <Y> <Z> <MIN> <MAX> <N>"));
                return;
            }

            const auto Origin = FVector{
                FCString::Atod(*InArgs[0]), FCString::Atod(*InArgs[1]), FCString::Atod(*InArgs[2])};

            DoPathDistancePointsAndDraw(InWorld, Origin, FCString::Atof(*InArgs[3]),
                FCString::Atof(*InArgs[4]), FCString::Atoi(*InArgs[5]));
        }));

    static FAutoConsoleCommandWithWorldAndArgs ConsoleCommand_GridAt(
        TEXT("ck.GroundNav.GridAt"),
        TEXT("Draw a regular lattice of points over the walkable ground of the last field bake ")
        TEXT("inside a box: ck.GroundNav.GridAt <X> <Y> <Z> <HALF> <SPACING>. HALF is the box's ")
        TEXT("horizontal half-extent; vertically the box reaches as far as the projection probe ")
        TEXT("searches (ck.GroundNav.Debug.ProbeUpUu / ProbeDownUu), and every storey with an ")
        TEXT("admitted cell at a lattice position contributes a point there. The lattice is phased ")
        TEXT("to the FIELD origin rather than to the box, so two overlapping runs agree on every ")
        TEXT("shared position. Each point draws as a sphere coloured by its layer. Needs ")
        TEXT("ck.GroundNav.BakeFieldAt to have run - a region bake produces no field to query. The ")
        TEXT("body radius comes from ck.GroundNav.Debug.AgentRadiusUu."),
        FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
            [](const TArray<FString>& InArgs, UWorld* InWorld) -> void
        {
            const auto WorldIsValid = ck::IsValid(InWorld);

            CK_ENSURE_IF_NOT(WorldIsValid, TEXT("ck.GroundNav.GridAt ran without a World"))
            { return; }

            if (InArgs.Num() != 5)
            {
                ck::groundnav::Warning(
                    TEXT("ck.GroundNav.GridAt needs five numbers: ")
                    TEXT("ck.GroundNav.GridAt <X> <Y> <Z> <HALF> <SPACING>"));
                return;
            }

            const auto Centre = FVector{
                FCString::Atod(*InArgs[0]), FCString::Atod(*InArgs[1]), FCString::Atod(*InArgs[2])};

            DoGridPointsAndDraw(
                InWorld, Centre, FCString::Atof(*InArgs[3]), FCString::Atof(*InArgs[4]));
        }));

    static FAutoConsoleCommandWithWorld ConsoleCommand_Clear(
        TEXT("ck.GroundNav.Clear"),
        TEXT("Clear everything ck.GroundNav.Bake drew, and drop the field the probe commands query."),
        FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* InWorld) -> void
        {
            LastDebugField = {};

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
                TEXT("\n  probe   : ProbeExtentUu {} ProbeUpUu {} ProbeDownUu {} ProbeMode {}")
                TEXT("\n  cost    : SlopePenaltyK {} ClearanceBiasK {} CornerOffsetK {}")
                TEXT("\n  display : Mode {} LifetimeSeconds {} MaxCells {} DrawMarkup {}")
                TEXT("\n  gates   : MarkupLiveGate bypassed {}"),
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
                CVar_ProbeExtentUu.GetValueOnGameThread(),
                CVar_ProbeUpUu.GetValueOnGameThread(),
                CVar_ProbeDownUu.GetValueOnGameThread(),
                CVar_ProbeMode.GetValueOnGameThread(),
                CVar_SlopePenaltyK.GetValueOnGameThread(),
                CVar_ClearanceBiasK.GetValueOnGameThread(),
                CVar_CornerOffsetK.GetValueOnGameThread(),
                CVar_Mode.GetValueOnGameThread(),
                CVar_LifetimeSeconds.GetValueOnGameThread(),
                CVar_MaxCells.GetValueOnGameThread(),
                ck::groundnav::debug::Get_IsMarkupDrawEnabled(),
                ck::groundnav::debug::Get_IsMarkupLiveGateBypassed());
        }));
}

// --------------------------------------------------------------------------------------------------------------------
