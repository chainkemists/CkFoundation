#include "CkPathNetwork_DebugDraw_Processor.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkPathNetwork/CkPathNetwork_Stats.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include <DrawDebugHelpers.h>
#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_PathNetwork_DebugDraw);
CK_REGISTER_PROCESSOR(ck::FProcessor_PathNetworkFollower_DebugDrawCorridor);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("PathNetwork::DebugDraw"), STAT_CkPathNetwork_DebugDraw, STATGROUP_CkPathNetwork);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_pathnetwork_debugdraw
{
    static TAutoConsoleVariable<int32> CVarPathNetworkDebugDraw(
        TEXT("ck.PathNetwork.DebugDraw"),
        0,
        TEXT("Draw path networks and follower corridors. 0=off, 1=networks+corridors, 2=also chunk grid."),
        ECVF_Default);

    constexpr auto SingleFrame = -1.0f;
    constexpr auto NotPersistent = false;
    constexpr auto DepthPriority = uint8{0};
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_PathNetwork_DebugDraw::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_PathNetwork_Graph& InGraph) const
        -> void
    {
#if ENABLE_DRAW_DEBUG
        using namespace ck_pathnetwork_debugdraw;

        if (ck::debug_draw::Is_SuppressedForStreamerMode())
        { return; }

        const auto DrawLevel = CVarPathNetworkDebugDraw.GetValueOnGameThread();
        if (DrawLevel <= 0)
        { return; }

        SCOPE_CYCLE_COUNTER(STAT_CkPathNetwork_DebugDraw);

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (NOT IsValid(World))
        { return; }

        const auto& Network = InGraph.Get_Network();

        const auto ZLift = FVector{0.0, 0.0, 5.0};

        for (const auto& Edge : Network._Edges)
        {
            for (auto Index = 0; Index < Edge._Points.Num() - 1; ++Index)
            {
                const auto& A = Edge._Points[Index];
                const auto& B = Edge._Points[Index + 1];

                DrawDebugLine(World, A + ZLift, B + ZLift, FColor::Green, NotPersistent, SingleFrame, DepthPriority, 4.0f);

                const auto Tangent = (B - A).GetSafeNormal();
                const auto Right = FVector::CrossProduct(FVector::UpVector, Tangent).GetSafeNormal();

                const auto HalfWidthA = Edge._HalfWidths[Index];
                const auto HalfWidthB = Edge._HalfWidths[Index + 1];

                DrawDebugLine(World, A + Right * HalfWidthA + ZLift, B + Right * HalfWidthB + ZLift,
                    FColor::Emerald, NotPersistent, SingleFrame, DepthPriority, 1.5f);
                DrawDebugLine(World, A - Right * HalfWidthA + ZLift, B - Right * HalfWidthB + ZLift,
                    FColor::Emerald, NotPersistent, SingleFrame, DepthPriority, 1.5f);
            }
        }

        for (const auto& Node : Network._Nodes)
        { DrawDebugSphere(World, Node._Location + ZLift, 25.0f, 8, FColor::White, NotPersistent, SingleFrame, DepthPriority, 2.0f); }

        if (DrawLevel >= 2)
        {
            for (auto CellY = 0; CellY < Network._ChunkCountY; ++CellY)
            {
                for (auto CellX = 0; CellX < Network._ChunkCountX; ++CellX)
                {
                    const auto CellMin = Network._GridOrigin + FVector{CellX * Network._ChunkSize, CellY * Network._ChunkSize, 0.0f};
                    const auto Center = CellMin + FVector{Network._ChunkSize * 0.5f, Network._ChunkSize * 0.5f, 0.0};
                    const auto Extent = FVector{Network._ChunkSize * 0.5f, Network._ChunkSize * 0.5f, 10.0};

                    const auto HasEdges = Network._ChunkEdgeIds[CellY * Network._ChunkCountX + CellX].Num() > 0;

                    DrawDebugBox(World, Center, Extent, HasEdges ? FColor::Silver : FColor{40, 40, 40},
                        NotPersistent, SingleFrame, DepthPriority, HasEdges ? 1.5f : 0.5f);
                }
            }
        }
#endif
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_PathNetworkFollower_DebugDrawCorridor::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_PathNetworkFollower_Corridor& InCorridor) const
        -> void
    {
#if ENABLE_DRAW_DEBUG
        using namespace ck_pathnetwork_debugdraw;

        if (ck::debug_draw::Is_SuppressedForStreamerMode())
        { return; }

        if (CVarPathNetworkDebugDraw.GetValueOnGameThread() <= 0)
        { return; }

        const auto& Result = InCorridor.Get_Result();

        if (Result.Get_Status() != ECk_PathNetwork_RouteStatus::Ready)
        { return; }

        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (NOT IsValid(World))
        { return; }

        const auto ZLift = FVector{0.0, 0.0, 15.0};

        const auto& Waypoints = Result.Get_CompiledWaypoints();
        for (auto Index = 0; Index < Waypoints.Num() - 1; ++Index)
        {
            DrawDebugLine(World, Waypoints[Index] + ZLift, Waypoints[Index + 1] + ZLift,
                FColor::Yellow, NotPersistent, SingleFrame, DepthPriority, 2.0f);
        }

        for (const auto& Leg : Result.Get_Legs())
        {
            if (Leg.Get_LegType() != ECk_PathNetwork_CorridorLegType::OffPath)
            { continue; }

            const auto& LegWaypoints = Leg.Get_Waypoints();
            for (auto Index = 0; Index < LegWaypoints.Num() - 1; ++Index)
            {
                DrawDebugLine(World, LegWaypoints[Index] + ZLift, LegWaypoints[Index + 1] + ZLift,
                    FColor::Cyan, NotPersistent, SingleFrame, DepthPriority, 3.5f);
            }
        }

        DrawDebugSphere(World, Result.Get_GoalLocation() + ZLift, 30.0f, 8, FColor::Yellow,
            NotPersistent, SingleFrame, DepthPriority, 2.0f);
#endif
    }
}

// --------------------------------------------------------------------------------------------------------------------
