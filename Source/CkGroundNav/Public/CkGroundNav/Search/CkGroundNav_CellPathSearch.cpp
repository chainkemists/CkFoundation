#include "CkGroundNav_CellPathSearch.h"

#include "CkGroundNav/Query/CkGroundNav_Query_CellStep.h"
#include "CkGroundNav/Query/CkGroundNav_QueryCore.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Reachability.h"
#include "CkGroundNav/Query/CkGroundNav_Query_SurfaceWalk.h"
#include "CkGroundNav/Search/CkGroundNav_PathSearch.h"

#include <Algo/Reverse.h>

#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"

namespace ck::groundnav
{
    namespace cellpathsearch_private
    {
        constexpr auto TimecheckIntervalIterations = 16;
        constexpr auto MicrosecondsPerSecond = 1'000'000.0;

        static TAutoConsoleVariable<int32> CVarCellSearchTiming(
            TEXT("ck.GroundNav.Debug.CellSearchTiming"), 0,
            TEXT("1 accumulates opt-in strict-cell search timing for pending-timeout attribution. "
                 "It does not alter search admission, budgets, or scheduling."));

        auto Get_ShouldTimeCellSearch() -> bool
        {
            return CVarCellSearchTiming.GetValueOnGameThread() != 0;
        }

        auto AddElapsed(FCk_Time& InOutTime, double InStartSeconds) -> void
        {
            InOutTime = InOutTime + FCk_Time{FPlatformTime::Seconds() - InStartSeconds};
        }

        auto Get_Microseconds(const FCk_Time& InTime) -> int64
        {
            const auto Seconds = InTime.Get_Seconds();
            return Seconds > 0.0 ? static_cast<int64>(Seconds * MicrosecondsPerSecond) : 0;
        }

        auto Get_IsForwardAllowed(const FCk_GroundNav_ResolvedLink& InLink, bool InForward) -> bool
        {
            return InLink._Direction == ECk_GroundNav_LinkDirection::Bidirectional ||
                (InForward && InLink._Direction == ECk_GroundNav_LinkDirection::Forward) ||
                (NOT InForward && InLink._Direction == ECk_GroundNav_LinkDirection::Backward);
        }

        auto AccumulateCost(FCk_GroundNav_QueryCost& InOut, const FCk_GroundNav_QueryCost& InAdded) -> void
        {
            InOut._CellsRead += InAdded._CellsRead;
            InOut._TilesTouched += InAdded._TilesTouched;
            InOut._TouchedUnbuiltTile = InOut._TouchedUnbuiltTile || InAdded._TouchedUnbuiltTile;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto FCk_GroundNav_CellPathSearch::FGraph::Neighbors(const FNode& InNode) const -> TArray<FNode>
    {
        DoBuild_Edges(InNode);
        auto Result = TArray<FNode>{};
        if (const auto* Edges = _Data->_Edges.Find(InNode))
        {
            Result.Reserve(Edges->Num());
            for (const auto& Edge : *Edges) { Result.Add(Edge._To); }
        }
        return Result;
    }

    auto FCk_GroundNav_CellPathSearch::FGraph::Cost(const FNode& InFrom, const FNode& InTo) const -> float
    {
        if (const auto* Edge = Get_Edge(InFrom, InTo)) { return Edge->_Route._Cost; }
        return TNumericLimits<float>::Max();
    }

    auto FCk_GroundNav_CellPathSearch::FGraph::Heuristic(const FNode& InNode, const FNode& InGoal) const -> float
    {
        return static_cast<float>(DoGet_UnweightedHeuristic(InNode) *
            static_cast<double>(_Data->_Query._GreedyWeightW));
    }

    auto FCk_GroundNav_CellPathSearch::FGraph::TieBreak(const FNode& InNode, const FNode& InGoal) const -> float
    {
        return static_cast<float>(DoGet_UnweightedHeuristic(InNode));
    }

    auto FCk_GroundNav_CellPathSearch::FGraph::IsGoal(const FNode& InNode) const -> bool
    {
        return InNode._Kind == ENodeKind::GoalTerminal;
    }

    auto FCk_GroundNav_CellPathSearch::FGraph::Get_Edge(const FNode& InFrom, const FNode& InTo) const -> const FEdge*
    {
        DoBuild_Edges(InFrom);
        const auto* Edges = _Data->_Edges.Find(InFrom);
        return Edges == nullptr ? nullptr : Edges->FindByPredicate([&InTo](const FEdge& InEdge) { return InEdge._To == InTo; });
    }

    auto FCk_GroundNav_CellPathSearch::FGraph::Get_NodePoint(const FNode& InNode) const -> FVector
    {
        return DoGet_NodePoint(InNode);
    }

    auto FCk_GroundNav_CellPathSearch::FGraph::DoBuild_Edges(const FNode& InNode) const -> void
    {
        if (_Data->_Edges.Contains(InNode)) { return; }
        const auto ShouldTime = _Data->_Timing._IsEnabled;
        const auto StartSeconds = ShouldTime ? FPlatformTime::Seconds() : 0.0;
        if (ShouldTime) { ++_Data->_Timing._EdgeBuildCount; }
        if (NOT _Data->_DidIndexLinkEndpoints)
        {
            _Data->_DidIndexLinkEndpoints = true;
            for (auto LinkIndex = 0; LinkIndex < _Data->_Field->_ResolvedLinks.Num(); ++LinkIndex)
            {
                const auto& Link = _Data->_Field->_ResolvedLinks[LinkIndex];
                if (NOT Link.Get_IsResolved()) { continue; }
                auto Start = FNode{}; Start._Surface = Link._StartSurface;
                auto End = FNode{}; End._Surface = Link._EndSurface;
                _Data->_LinkEndpoints.FindOrAdd(Start).Add(FLinkEndpoint{LinkIndex, true});
                _Data->_LinkEndpoints.FindOrAdd(End).Add(FLinkEndpoint{LinkIndex, false});
            }
        }
        auto& Edges = _Data->_Edges.Add(InNode);
        if (NOT _Data->_Field.IsValid())
        {
            if (ShouldTime) { cellpathsearch_private::AddElapsed(_Data->_Timing._EdgeBuildTime, StartSeconds); }
            return;
        }

        if (InNode._Kind == ENodeKind::GoalTerminal)
        {
            if (ShouldTime) { cellpathsearch_private::AddElapsed(_Data->_Timing._EdgeBuildTime, StartSeconds); }
            return;
        }

        if (InNode._Kind == ENodeKind::LinkEntry)
        {
            if (NOT DoGet_IsLinkAllowed(InNode._LinkIndex, InNode._Forward))
            {
                if (ShouldTime) { cellpathsearch_private::AddElapsed(_Data->_Timing._EdgeBuildTime, StartSeconds); }
                return;
            }
            const auto& Link = _Data->_Field->_ResolvedLinks[InNode._LinkIndex];
            auto Exit = FNode{};
            Exit._Surface = InNode._Forward ? Link._EndSurface : Link._StartSurface;
            Exit._Kind = ENodeKind::LinkExit;
            Exit._EscapeState = EEscapeState::Escaped;
            Exit._LinkIndex = InNode._LinkIndex;
            Exit._Forward = InNode._Forward;
            auto Route = FCk_GroundNav_CellRouteEdge{};
            Route._FromSurface = InNode._Surface;
            Route._ToSurface = Exit._Surface;
            Route._FromPoint = DoGet_NodePoint(InNode);
            Route._ToPoint = DoGet_NodePoint(Exit);
            Route._Kind = ECk_GroundNav_CellRouteEdgeKind::Link;
            Route._LinkStableId = Link._Id;
            Route._LinkDirection = InNode._Forward ? ECk_GroundNav_LinkDirection::Forward : ECk_GroundNav_LinkDirection::Backward;
            const auto Rewrite = _Data->_Query._Cost._LinkCostMultipliers.FindRef(Link._Id);
            Route._Cost = static_cast<float>(FVector::Dist(Route._FromPoint, Route._ToPoint) *
                static_cast<double>(Rewrite > 0.0f ? Rewrite :
                    (InNode._Forward ? Link._CostMultiplierForward : Link._CostMultiplierBackward)));
            DoAdd_Edge(Edges, InNode, Exit, MoveTemp(Route));
            if (ShouldTime) { cellpathsearch_private::AddElapsed(_Data->_Timing._EdgeBuildTime, StartSeconds); }
            return;
        }

        auto Candidates = TArray<FCk_GroundNav_SurfaceRef, TInlineAllocator<5>>{};
        Candidates.Add(InNode._Surface);

        if (InNode._Kind != ENodeKind::LinkExit)
        {
            for (auto Direction = 0; Direction < 4; ++Direction)
            {
                auto To = FCk_GroundNav_SurfaceRef{};
                auto SurfaceZ = 0.0f;
                auto Clearance = 0.0f;
                const auto StepStartSeconds = ShouldTime ? FPlatformTime::Seconds() : 0.0;
                if (ShouldTime) { ++_Data->_Timing._StepAcrossCount; }
                const auto Verdict = Get_StepAcross(*_Data->_Field, InNode._Surface, Direction,
                    _Data->_Query._Agent, To, SurfaceZ, Clearance, _Data->_Cost);
                if (ShouldTime) { cellpathsearch_private::AddElapsed(_Data->_Timing._StepAcrossTime, StepStartSeconds); }
                if (Verdict == ECk_GroundNav_StepVerdict::Admitted) { Candidates.Add(To); }
            }
        }

        for (const auto& Candidate : Candidates)
        {
            if (InNode._Kind == ENodeKind::SurfaceCentre && Candidate == InNode._Surface) { continue; }

            auto Centre = FNode{};
            Centre._Surface = Candidate;
            Centre._Kind = ENodeKind::SurfaceCentre;
            Centre._EscapeState = InNode._EscapeState;

            const auto FlatPlate = Get_FlatPlateIndex(*_Data->_Field, Candidate._TileIndex, Candidate._PlateIndex);
            const auto IsStandingOnSource = InNode._Kind == ENodeKind::SourceTerminal && Candidate == InNode._Surface;
            if (NOT IsStandingOnSource && _Data->_Query._Cost._DeniedPlates.Contains(FlatPlate)) { continue; }

            const auto FromPoint = DoGet_NodePoint(InNode);
            const auto ToPoint = DoGet_NodePoint(Centre);
            if (NOT DoGet_IsStaticClear(FromPoint, ToPoint)) { continue; }

            auto Clearance = 0.0f;
            if (Candidate != InNode._Surface)
            {
                const auto& Tile = _Data->_Field->_Tiles[Candidate._TileIndex];
                Clearance = Tile._Clearance.Get_ClearanceAt(Candidate._CellX, Candidate._CellY, Candidate._LayerIndex);
            }
            auto Route = FCk_GroundNav_CellRouteEdge{};
            Route._FromSurface = InNode._Surface;
            Route._ToSurface = Candidate;
            Route._FromPoint = FromPoint;
            Route._ToPoint = ToPoint;
            Route._Kind = InNode._Kind == ENodeKind::SurfaceCentre ? ECk_GroundNav_CellRouteEdgeKind::Ordinary : ECk_GroundNav_CellRouteEdgeKind::Terminal;
            Route._Cost = Get_LegCost(_Data->_Shared, FromPoint, ToPoint, DoGet_AreaMultiplier(Candidate), DoGet_ClearanceFactor(Clearance));
            DoAdd_Edge(Edges, InNode, Centre, MoveTemp(Route));
        }

        DoTry_AddGoal(Edges, InNode, Candidates);
        if (InNode._Kind != ENodeKind::LinkExit) { DoTry_AddLinks(Edges, InNode, Candidates); }
        if (ShouldTime) { cellpathsearch_private::AddElapsed(_Data->_Timing._EdgeBuildTime, StartSeconds); }
    }

    auto FCk_GroundNav_CellPathSearch::FGraph::DoAdd_Edge(
        TArray<FEdge>& InOutEdges, const FNode& InFrom, FNode InTo,
        FCk_GroundNav_CellRouteEdge InRoute) const -> void
    {
        // A link is an authored traversal, not an ordinary ground segment: blockers gate its
        // landing cell, while the link itself may cross a footprint (jump/teleport semantics).
        if (InRoute._Kind == ECk_GroundNav_CellRouteEdgeKind::Link)
        {
            const auto Blocked = DoGet_IsCellBlocked(InTo._Surface);
            if (NOT Blocked.IsSet() || *Blocked) { return; }
            InTo._EscapeState = EEscapeState::Escaped;
        }
        else if (NOT DoGet_DynamicAdmission(InFrom, InTo, InRoute._FromPoint, InRoute._ToPoint)) { return; }
        if (auto* Existing = InOutEdges.FindByPredicate([&InTo](const FEdge& InEdge) { return InEdge._To == InTo; }))
        {
            if (InRoute._Cost < Existing->_Route._Cost) { *Existing = FEdge{MoveTemp(InTo), MoveTemp(InRoute)}; }
            return;
        }
        InOutEdges.Add(FEdge{MoveTemp(InTo), MoveTemp(InRoute)});
    }

    auto FCk_GroundNav_CellPathSearch::FGraph::DoTry_AddGoal(
        TArray<FEdge>& InOutEdges, const FNode& InFrom,
        TConstArrayView<FCk_GroundNav_SurfaceRef> InCandidateSurfaces) const -> void
    {
        auto ReachesGoalSurface = false;
        for (const auto& Surface : InCandidateSurfaces)
        {
            if (Surface == _Data->_Ends._GoalSurface)
            {
                ReachesGoalSurface = true;
                break;
            }
        }
        if (NOT ReachesGoalSurface) { return; }
        const auto FlatGoal = Get_FlatPlateIndex(*_Data->_Field, _Data->_Ends._GoalSurface._TileIndex, _Data->_Ends._GoalSurface._PlateIndex);
        if (_Data->_Query._Cost._DeniedPlates.Contains(FlatGoal)) { return; }
        auto Goal = FNode{};
        Goal._Surface = _Data->_Ends._GoalSurface;
        Goal._Kind = ENodeKind::GoalTerminal;
        Goal._EscapeState = InFrom._EscapeState;
        const auto FromPoint = DoGet_NodePoint(InFrom);
        const auto ToPoint = _Data->_Ends._GoalPoint;
        if (NOT DoGet_IsStaticClear(FromPoint, ToPoint)) { return; }
        auto Route = FCk_GroundNav_CellRouteEdge{};
        Route._FromSurface = InFrom._Surface;
        Route._ToSurface = Goal._Surface;
        Route._FromPoint = FromPoint;
        Route._ToPoint = ToPoint;
        Route._Kind = ECk_GroundNav_CellRouteEdgeKind::Terminal;
        Route._Cost = Get_LegCost(_Data->_Shared, FromPoint, ToPoint, DoGet_AreaMultiplier(Goal._Surface), 1.0f);
        DoAdd_Edge(InOutEdges, InFrom, Goal, MoveTemp(Route));
    }

    auto FCk_GroundNav_CellPathSearch::FGraph::DoTry_AddLinks(
        TArray<FEdge>& InOutEdges, const FNode& InFrom,
        TConstArrayView<FCk_GroundNav_SurfaceRef> InCandidateSurfaces) const -> void
    {
        if (InFrom._EscapeState != EEscapeState::Escaped) { return; }
        for (const auto& Surface : InCandidateSurfaces)
        {
            auto EndpointKey = FNode{};
            EndpointKey._Surface = Surface;
            if (const auto* Endpoints = _Data->_LinkEndpoints.Find(EndpointKey))
            {
                for (const auto& Endpoint : *Endpoints)
                {
                    if (NOT DoGet_IsLinkAllowed(Endpoint._LinkIndex, Endpoint._Forward)) { continue; }
                    const auto& Link = _Data->_Field->_ResolvedLinks[Endpoint._LinkIndex];
                    auto Entry = FNode{};
                    Entry._Surface = Surface;
                    Entry._Kind = ENodeKind::LinkEntry;
                    Entry._EscapeState = EEscapeState::Escaped;
                    Entry._LinkIndex = Endpoint._LinkIndex;
                    Entry._Forward = Endpoint._Forward;
                    const auto FromPoint = DoGet_NodePoint(InFrom);
                    const auto ToPoint = DoGet_NodePoint(Entry);
                    if (NOT DoGet_IsStaticClear(FromPoint, ToPoint)) { continue; }
                    auto Route = FCk_GroundNav_CellRouteEdge{};
                    Route._FromSurface = InFrom._Surface;
                    Route._ToSurface = Surface;
                    Route._FromPoint = FromPoint;
                    Route._ToPoint = ToPoint;
                    Route._Kind = ECk_GroundNav_CellRouteEdgeKind::Terminal;
                    Route._Cost = Get_LegCost(_Data->_Shared, FromPoint, ToPoint,
                        DoGet_AreaMultiplier(Surface), DoGet_ClearanceFactor(Link._ClearanceUu));
                    DoAdd_Edge(InOutEdges, InFrom, Entry, MoveTemp(Route));
                }
            }
        }
    }

    auto FCk_GroundNav_CellPathSearch::FGraph::DoGet_DynamicAdmission(
        const FNode& InFrom, FNode& InOutTo, const FVector& InFromPoint, const FVector& InToPoint) const -> bool
    {
        const auto ShouldTime = _Data->_Timing._IsEnabled;
        const auto StartSeconds = ShouldTime ? FPlatformTime::Seconds() : 0.0;
        if (ShouldTime) { ++_Data->_Timing._DynamicAdmissionCount; }
        const auto Union = Get_DynamicUnionEdge(_Data->_Query._DynamicObstacles, InFromPoint, InToPoint);
        if (NOT Union.IsSet())
        {
            if (ShouldTime) { cellpathsearch_private::AddElapsed(_Data->_Timing._DynamicAdmissionTime, StartSeconds); }
            return false;
        }

        if (InOutTo._Kind == ENodeKind::GoalTerminal)
        {
            if (InFrom._EscapeState == EEscapeState::Escaped)
            {
                const auto Result = *Union == ECk_GroundNav_DynamicUnionEdge::Clear;
                if (ShouldTime) { cellpathsearch_private::AddElapsed(_Data->_Timing._DynamicAdmissionTime, StartSeconds); }
                return Result;
            }
            if (*Union == ECk_GroundNav_DynamicUnionEdge::InsideAll)
            {
                if (ShouldTime) { cellpathsearch_private::AddElapsed(_Data->_Timing._DynamicAdmissionTime, StartSeconds); }
                return true;
            }
            if (*Union == ECk_GroundNav_DynamicUnionEdge::ExitsOnce)
            {
                InOutTo._EscapeState = EEscapeState::Escaped;
                if (ShouldTime) { cellpathsearch_private::AddElapsed(_Data->_Timing._DynamicAdmissionTime, StartSeconds); }
                return true;
            }
            if (ShouldTime) { cellpathsearch_private::AddElapsed(_Data->_Timing._DynamicAdmissionTime, StartSeconds); }
            return false;
        }

        const auto Blocked = DoGet_IsCellBlocked(InOutTo._Surface);
        if (NOT Blocked.IsSet())
        {
            if (ShouldTime) { cellpathsearch_private::AddElapsed(_Data->_Timing._DynamicAdmissionTime, StartSeconds); }
            return false;
        }
        // Closed-cell admission is the conservative hard veto. The ordinary leg itself must also
        // remain outside the exact union, otherwise a terminal-to-centre chord could enter a small
        // footprint whose next cell happened not to be conservatively covered.
        if (InFrom._EscapeState == EEscapeState::Escaped)
        {
            const auto Result = NOT *Blocked && *Union == ECk_GroundNav_DynamicUnionEdge::Clear;
            if (ShouldTime) { cellpathsearch_private::AddElapsed(_Data->_Timing._DynamicAdmissionTime, StartSeconds); }
            return Result;
        }

        if (*Union == ECk_GroundNav_DynamicUnionEdge::InsideAll)
        {
            if (ShouldTime) { cellpathsearch_private::AddElapsed(_Data->_Timing._DynamicAdmissionTime, StartSeconds); }
            return true;
        }
        if (*Union != ECk_GroundNav_DynamicUnionEdge::ExitsOnce || *Blocked)
        {
            if (ShouldTime) { cellpathsearch_private::AddElapsed(_Data->_Timing._DynamicAdmissionTime, StartSeconds); }
            return false;
        }
        InOutTo._EscapeState = EEscapeState::Escaped;
        if (ShouldTime) { cellpathsearch_private::AddElapsed(_Data->_Timing._DynamicAdmissionTime, StartSeconds); }
        return true;
    }

    auto FCk_GroundNav_CellPathSearch::FGraph::DoGet_IsCellBlocked(const FCk_GroundNav_SurfaceRef& InSurface) const -> TOptional<bool>
    {
        if (NOT _Data->_Field->_Tiles.IsValidIndex(InSurface._TileIndex)) { return {}; }
        const auto& Tile = _Data->_Field->_Tiles[InSurface._TileIndex];
        if (NOT Tile.Get_IsValidCell(InSurface._CellX, InSurface._CellY, InSurface._LayerIndex) || Tile._CellSizeUu <= 0.0f) { return {}; }
        return Get_IsDynamicObstacleCoveringCell(_Data->_Query._DynamicObstacles,
            FVector2D{Tile._Origin.X + InSurface._CellX * Tile._CellSizeUu, Tile._Origin.Y + InSurface._CellY * Tile._CellSizeUu},
            Tile._CellSizeUu, Tile.Get_SurfaceZAt(InSurface._CellX, InSurface._CellY, InSurface._LayerIndex));
    }

    auto FCk_GroundNav_CellPathSearch::FGraph::DoGet_NodePoint(const FNode& InNode) const -> FVector
    {
        if (InNode._Kind == ENodeKind::SourceTerminal) { return _Data->_Ends._StartPoint; }
        if (InNode._Kind == ENodeKind::GoalTerminal) { return _Data->_Ends._GoalPoint; }
        if (InNode._Kind == ENodeKind::LinkEntry || InNode._Kind == ENodeKind::LinkExit)
        {
            const auto& Link = _Data->_Field->_ResolvedLinks[InNode._LinkIndex];
            const auto AtStart = (InNode._Kind == ENodeKind::LinkEntry) == InNode._Forward;
            return AtStart ? Link._Start : Link._End;
        }
        return Get_SurfaceCentre(*_Data->_Field, InNode._Surface);
    }

    auto FCk_GroundNav_CellPathSearch::FGraph::DoGet_UnweightedHeuristic(const FNode& InNode) const -> double
    {
        const auto ExistingEuclidean = FVector::Dist(DoGet_NodePoint(InNode), _Data->_Ends._GoalPoint);
        if (InNode._Kind != ENodeKind::SurfaceCentre || _Data->_GoalTerminalCandidateCount == 0)
        { return ExistingEuclidean; }

        const auto CurrentPoint = DoGet_NodePoint(InNode);
        // The goal terminal is only emitted from the goal cell or one cardinal neighbour. Every
        // ordinary strict step costs at least its centre-to-centre Euclidean span, hence at least its
        // XY cardinal span; the final terminal leg costs at least its XY Euclidean span. This is a
        // lower bound even when one of these five hypothetical terminal origins is not traversable.
        auto GridLowerBound = TNumericLimits<double>::Max();
        for (auto Index = 0; Index < _Data->_GoalTerminalCandidateCount; ++Index)
        {
            const auto& Candidate = _Data->_GoalTerminalCandidates[Index];
            const auto CardinalCost = FMath::Abs(CurrentPoint.X - Candidate._XY.X) +
                FMath::Abs(CurrentPoint.Y - Candidate._XY.Y);
            GridLowerBound = FMath::Min(GridLowerBound, CardinalCost + Candidate._TerminalCost);
        }

        auto Result = FMath::Max(GridLowerBound, ExistingEuclidean);
        if (_Data->_HasSingleLayerRectangularCostBound &&
            _Data->_Field->_Tiles.IsValidIndex(InNode._Surface._TileIndex))
        {
            const auto& Tile = _Data->_Field->_Tiles[InNode._Surface._TileIndex];
            if (Tile.Get_IsValidCell(InNode._Surface._CellX, InNode._Surface._CellY, InNode._Surface._LayerIndex) &&
                Tile._Plates._Plates.IsValidIndex(InNode._Surface._PlateIndex))
            {
                const auto& Plate = Tile._Plates._Plates[InNode._Surface._PlateIndex];
                const auto BoundsAreValid =
                    Plate._LayerIndex == InNode._Surface._LayerIndex &&
                    Tile._Plates.Get_PlateIndexAt(InNode._Surface._CellX, InNode._Surface._CellY,
                        InNode._Surface._LayerIndex) == InNode._Surface._PlateIndex &&
                    Plate._MinX >= 0 && Plate._MinY >= 0 &&
                    Plate._MaxX >= Plate._MinX && Plate._MaxY >= Plate._MinY &&
                    Plate._MaxX < Tile._SizeX && Plate._MaxY < Tile._SizeY &&
                    InNode._Surface._CellX >= Plate._MinX && InNode._Surface._CellX <= Plate._MaxX &&
                    InNode._Surface._CellY >= Plate._MinY && InNode._Surface._CellY <= Plate._MaxY;
                const auto CellSize = static_cast<double>(Tile._CellSizeUu);
                const auto Multiplier = static_cast<double>(DoGet_AreaMultiplier(InNode._Surface));
                if (BoundsAreValid && FMath::IsFinite(CellSize) && CellSize > 0.0 &&
                    FMath::IsFinite(Multiplier) && Multiplier >= 1.0)
                {
                    const auto ExitCells = FMath::Min(
                        FMath::Min(InNode._Surface._CellX - Plate._MinX, Plate._MaxX - InNode._Surface._CellX),
                        FMath::Min(InNode._Surface._CellY - Plate._MinY, Plate._MaxY - InNode._Surface._CellY));
                    const auto ExitDistance = CellSize * static_cast<double>(ExitCells);
                    const auto Premium = (Multiplier - 1.0) * FMath::Min(ExitDistance, GridLowerBound);
                    const auto CostBound = GridLowerBound + Premium;
                    if (ExitCells >= 0 && FMath::IsFinite(ExitDistance) && FMath::IsFinite(Premium) &&
                        FMath::IsFinite(CostBound) && CostBound <= static_cast<double>(TNumericLimits<float>::Max()))
                    { Result = FMath::Max(Result, CostBound); }
                }
            }
        }

        if (NOT _Data->_HasGoalPlateCostBound ||
            NOT _Data->_Field->_Tiles.IsValidIndex(InNode._Surface._TileIndex))
        { return Result; }

        auto PremiumDistance = _Data->_GoalPlateBoundaryDistance;
        const auto IsInGoalPlate = InNode._Surface._TileIndex == _Data->_GoalPlateTileIndex &&
            InNode._Surface._PlateIndex == _Data->_GoalPlateIndex &&
            InNode._Surface._LayerIndex == 0;
        if (IsInGoalPlate)
        {
            const auto& GoalTile = _Data->_Field->_Tiles[_Data->_GoalPlateTileIndex];
            if (NOT GoalTile.Get_IsValidCell(InNode._Surface._CellX, InNode._Surface._CellY,
                InNode._Surface._LayerIndex))
            { return Result; }
            const auto ExitCells = FMath::Min(
                FMath::Min(InNode._Surface._CellX - _Data->_GoalPlateMinX,
                    _Data->_GoalPlateMaxX - InNode._Surface._CellX),
                FMath::Min(InNode._Surface._CellY - _Data->_GoalPlateMinY,
                    _Data->_GoalPlateMaxY - InNode._Surface._CellY));
            const auto ExitDistance = static_cast<double>(GoalTile._CellSizeUu) * static_cast<double>(ExitCells);
            if (ExitCells < 0 || NOT FMath::IsFinite(ExitDistance)) { return Result; }
            PremiumDistance = FMath::Min(GridLowerBound, ExitDistance + PremiumDistance);
        }

        const auto GoalPremium = (_Data->_GoalPlateMultiplier - 1.0) * PremiumDistance;
        const auto GoalCostBound = GridLowerBound + GoalPremium;
        if (FMath::IsFinite(PremiumDistance) && FMath::IsFinite(GoalPremium) && FMath::IsFinite(GoalCostBound) &&
            GoalCostBound <= static_cast<double>(TNumericLimits<float>::Max()))
        { Result = FMath::Max(Result, GoalCostBound); }
        return Result;
    }

    auto FCk_GroundNav_CellPathSearch::FGraph::DoGet_ClearanceFactor(float InClearanceUu) const -> float
    {
        if (_Data->_Shared._ClearanceBiasK <= 0.0f || _Data->_Shared._CellSizeUu <= 0.0f) { return 1.0f; }
        const auto ClearanceCells = static_cast<double>(InClearanceUu) / _Data->_Shared._CellSizeUu;
        return static_cast<float>(1.0 + _Data->_Shared._ClearanceBiasK / (ClearanceCells + 1.0));
    }

    auto FCk_GroundNav_CellPathSearch::FGraph::DoGet_IsStaticClear(const FVector& InFrom, const FVector& InTo) const -> bool
    {
        const auto ShouldTime = _Data->_Timing._IsEnabled;
        const auto StartSeconds = ShouldTime ? FPlatformTime::Seconds() : 0.0;
        if (ShouldTime) { ++_Data->_Timing._StaticRayCount; }
        auto Ray = FCk_GroundNav_RaycastQuery{};
        Ray._Start = InFrom;
        Ray._End = InTo;
        Ray._StartVerticalToleranceUu = _Data->_Query._VerticalToleranceUu;
        Ray._Agent = _Data->_Query._Agent;
        const auto IsClear = Get_SurfaceRaycast(*_Data->_Field, Ray).Get_IsClear();
        if (ShouldTime) { cellpathsearch_private::AddElapsed(_Data->_Timing._StaticRayTime, StartSeconds); }
        return IsClear;
    }

    auto FCk_GroundNav_CellPathSearch::FGraph::DoGet_IsLinkAllowed(int32 InLinkIndex, bool InForward) const -> bool
    {
        if (NOT _Data->_Field->_ResolvedLinks.IsValidIndex(InLinkIndex)) { return false; }
        const auto& Link = _Data->_Field->_ResolvedLinks[InLinkIndex];
        return Link.Get_IsTraversable() && Get_IsAdmitted(Link._ClearanceUu, _Data->_Query._Agent) &&
            cellpathsearch_private::Get_IsForwardAllowed(Link, InForward) &&
            NOT _Data->_Query._Cost._DeniedLinkIds.Contains(Link._Id) &&
            NOT (Link._UserTypeTag.IsValid() && Link._UserTypeTag.MatchesAny(_Data->_Query._Cost._DeniedLinkUserTypeTags));
    }

    auto FCk_GroundNav_CellPathSearch::FGraph::DoGet_AreaMultiplier(const FCk_GroundNav_SurfaceRef& InSurface) const -> float
    {
        return Get_AreaMultiplier(*_Data->_Field, _Data->_Shared,
            Get_FlatPlateIndex(*_Data->_Field, InSurface._TileIndex, InSurface._PlateIndex));
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto FCk_GroundNav_CellPathSearch::Request_Begin(
        const FCk_GroundNav_FieldPtr& InField, const FCk_GroundNav_PathQuery& InQuery,
        const FCk_GroundNav_PathResult& InResolvedEnds) -> ECk_GroundNav_PathStatus
    {
        _Result = InResolvedEnds;
        _Result._RouteKind = ECk_GroundNav_PathRouteKind::StrictCell;
        _Result._DynamicObstacles = InQuery._DynamicObstacles;
        _Result._CellRoute.Reset();
        _Data = MakeShared<FGraphData>();
        _Data->_Field = InField;
        _Data->_Query = InQuery;
        _Data->_Ends = _Result;
        _Data->_PreSearchCost = _Result._Cost;
        _Data->_Timing._IsEnabled = cellpathsearch_private::Get_ShouldTimeCellSearch();
        if (_Data->_Timing._IsEnabled)
        {
            _Data->_Timing._SnapshotDiscCount = InQuery._DynamicObstacles.Get_Discs().Num();
            _Data->_Timing._SnapshotObbCount = InQuery._DynamicObstacles.Get_Obbs().Num();
        }

        if (NOT InField.IsValid()) { return DoFinish(ECk_GroundNav_PathStatus::Unbuilt); }

        _Data->_Shared._Field = InField;
        _Data->_Shared._Epoch = InField->_Epoch;
        _Data->_Shared._Agent = InQuery._Agent;
        _Data->_Shared._GoalFlatPlate = Get_FlatPlateIndex(*InField, _Result._GoalSurface._TileIndex, _Result._GoalSurface._PlateIndex);
        _Data->_Shared._GoalPoint = _Result._GoalPoint;
        _Data->_Shared._SourcePoint = _Result._StartPoint;
        _Data->_Shared._GreedyWeightW = InQuery._GreedyWeightW;
        _Data->_Shared._SlopePenaltyK = InQuery._Cost._SlopePenaltyK;
        _Data->_Shared._ClearanceBiasK = InQuery._Cost._ClearanceBiasK;
        _Data->_Shared._PlateCostMultipliers = InQuery._Cost._PlateCostMultipliers;
        _Data->_Shared._DeniedPlates = InQuery._Cost._DeniedPlates;
        _Data->_Shared._DeniedLinkIds = InQuery._Cost._DeniedLinkIds;
        _Data->_Shared._DeniedLinkUserTypeTags = InQuery._Cost._DeniedLinkUserTypeTags;
        _Data->_Shared._LinkCostMultipliers = InQuery._Cost._LinkCostMultipliers;
        _Data->_Shared._CellSizeUu = InField->_Params._Config.Get_CellSizeUu();

        if (InField->_ResolvedLinks.IsEmpty() && InField->_Tiles.IsValidIndex(_Result._GoalSurface._TileIndex))
        {
            const auto& GoalTile = InField->_Tiles[_Result._GoalSurface._TileIndex];
            const auto CellSize = static_cast<double>(GoalTile._CellSizeUu);
            const auto GoalSurfaceIsValid = GoalTile.Get_IsValidCell(
                _Result._GoalSurface._CellX, _Result._GoalSurface._CellY, _Result._GoalSurface._LayerIndex);
            const auto GoalCellCentre = GoalSurfaceIsValid
                ? Get_SurfaceCentre(*InField, _Result._GoalSurface)
                : FVector::ZeroVector;
            const auto& GoalPoint = _Result._GoalPoint;
            const auto CoordinatesAreFinite =
                FMath::IsFinite(CellSize) && CellSize > 0.0 &&
                FMath::IsFinite(GoalCellCentre.X) && FMath::IsFinite(GoalCellCentre.Y) &&
                FMath::IsFinite(GoalPoint.X) && FMath::IsFinite(GoalPoint.Y) && FMath::IsFinite(GoalPoint.Z);
            if (GoalSurfaceIsValid && CoordinatesAreFinite)
            {
                const FIntPoint Offsets[] =
                {
                    FIntPoint{0, 0}, FIntPoint{1, 0}, FIntPoint{-1, 0}, FIntPoint{0, 1}, FIntPoint{0, -1}};
                FGoalTerminalCandidate Candidates[UE_ARRAY_COUNT(Offsets)];
                auto CandidatesAreFinite = true;

                for (auto Index = 0; Index < UE_ARRAY_COUNT(Offsets); ++Index)
                {
                    const auto& Offset = Offsets[Index];
                    auto& Candidate = Candidates[Index];
                    Candidate._XY = FVector2D{
                        GoalCellCentre.X + CellSize * Offset.X,
                        GoalCellCentre.Y + CellSize * Offset.Y};
                    Candidate._TerminalCost = FVector::Dist2D(FVector{Candidate._XY.X, Candidate._XY.Y, 0.0}, GoalPoint);
                    CandidatesAreFinite = CandidatesAreFinite &&
                        FMath::IsFinite(Candidate._XY.X) && FMath::IsFinite(Candidate._XY.Y) &&
                        FMath::IsFinite(Candidate._TerminalCost);
                }

                if (CandidatesAreFinite)
                {
                    for (auto Index = 0; Index < UE_ARRAY_COUNT(Candidates); ++Index)
                    { _Data->_GoalTerminalCandidates[Index] = Candidates[Index]; }
                    _Data->_GoalTerminalCandidateCount = UE_ARRAY_COUNT(Candidates);
                }
            }
        }

        if (InField->_ResolvedLinks.IsEmpty() && NOT InField->_Tiles.IsEmpty())
        {
            auto HasSingleLayerRectangularTiles = true;
            for (const auto& Tile : InField->_Tiles)
            {
                HasSingleLayerRectangularTiles = HasSingleLayerRectangularTiles &&
                    Tile.Get_IsBuilt() && Tile._LayerCount == 1 && Tile._Plates._LayerCount == 1 &&
                    Tile._Plates._SizeX == Tile._SizeX && Tile._Plates._SizeY == Tile._SizeY;
                if (NOT HasSingleLayerRectangularTiles) { break; }
            }
            _Data->_HasSingleLayerRectangularCostBound = HasSingleLayerRectangularTiles;
        }

        _Data->_Timing._HasCurrentPlateCostBound = _Data->_HasSingleLayerRectangularCostBound;
        if (_Data->_HasSingleLayerRectangularCostBound && _Data->_GoalTerminalCandidateCount > 0 &&
            InField->_Tiles.IsValidIndex(_Result._GoalSurface._TileIndex))
        {
            const auto& GoalTile = InField->_Tiles[_Result._GoalSurface._TileIndex];
            const auto GoalSurfaceIsValid = GoalTile.Get_IsValidCell(
                _Result._GoalSurface._CellX, _Result._GoalSurface._CellY, _Result._GoalSurface._LayerIndex);
            const auto GoalPlateIndex = GoalSurfaceIsValid
                ? GoalTile._Plates.Get_PlateIndexAt(_Result._GoalSurface._CellX, _Result._GoalSurface._CellY,
                    _Result._GoalSurface._LayerIndex)
                : FCk_GroundNav_Plate::kNoPlate;
            if (GoalTile._Plates._Plates.IsValidIndex(GoalPlateIndex))
            {
                const auto& GoalPlate = GoalTile._Plates._Plates[GoalPlateIndex];
                const auto GoalMultiplier = static_cast<double>(Get_AreaMultiplier(*InField, _Data->_Shared,
                    Get_FlatPlateIndex(*InField, _Result._GoalSurface._TileIndex, GoalPlateIndex)));
                const auto GoalBoundsAreValid = GoalPlateIndex == _Result._GoalSurface._PlateIndex &&
                    GoalPlate._LayerIndex == _Result._GoalSurface._LayerIndex &&
                    GoalPlate._MinX >= 0 && GoalPlate._MinY >= 0 &&
                    GoalPlate._MaxX >= GoalPlate._MinX && GoalPlate._MaxY >= GoalPlate._MinY &&
                    GoalPlate._MaxX < GoalTile._SizeX && GoalPlate._MaxY < GoalTile._SizeY &&
                    _Result._GoalSurface._CellX >= GoalPlate._MinX && _Result._GoalSurface._CellX <= GoalPlate._MaxX &&
                    _Result._GoalSurface._CellY >= GoalPlate._MinY && _Result._GoalSurface._CellY <= GoalPlate._MaxY;
                const auto MinCentre = GoalBoundsAreValid
                    ? GoalTile.Get_CellCentre(GoalPlate._MinX, GoalPlate._MinY, GoalPlate._LayerIndex)
                    : FVector::ZeroVector;
                const auto MaxCentre = GoalBoundsAreValid
                    ? GoalTile.Get_CellCentre(GoalPlate._MaxX, GoalPlate._MaxY, GoalPlate._LayerIndex)
                    : FVector::ZeroVector;
                const auto GeometryIsFinite = GoalBoundsAreValid && FMath::IsFinite(GoalTile._CellSizeUu) &&
                    GoalTile._CellSizeUu > 0.0f && FMath::IsFinite(MinCentre.X) && FMath::IsFinite(MinCentre.Y) &&
                    FMath::IsFinite(MaxCentre.X) && FMath::IsFinite(MaxCentre.Y) &&
                    FMath::IsFinite(GoalMultiplier) && GoalMultiplier >= 1.0;
                if (GeometryIsFinite)
                {
                    auto BoundaryDistance = TNumericLimits<double>::Max();
                    auto OutsideTerminalCost = TNumericLimits<double>::Max();
                    for (auto Index = 0; Index < _Data->_GoalTerminalCandidateCount; ++Index)
                    {
                        const auto& Candidate = _Data->_GoalTerminalCandidates[Index];
                        const auto IsInside = Candidate._XY.X >= MinCentre.X && Candidate._XY.X <= MaxCentre.X &&
                            Candidate._XY.Y >= MinCentre.Y && Candidate._XY.Y <= MaxCentre.Y;
                        const auto ToBoundary = IsInside
                            ? FMath::Min(FMath::Min(Candidate._XY.X - MinCentre.X, MaxCentre.X - Candidate._XY.X),
                                FMath::Min(Candidate._XY.Y - MinCentre.Y, MaxCentre.Y - Candidate._XY.Y))
                            : FMath::Abs(Candidate._XY.X - FMath::Clamp(Candidate._XY.X, MinCentre.X, MaxCentre.X)) +
                                FMath::Abs(Candidate._XY.Y - FMath::Clamp(Candidate._XY.Y, MinCentre.Y, MaxCentre.Y));
                        BoundaryDistance = FMath::Min(BoundaryDistance, ToBoundary + Candidate._TerminalCost);
                        if (NOT IsInside) { OutsideTerminalCost = FMath::Min(OutsideTerminalCost, Candidate._TerminalCost); }
                    }
                    const auto RequiredSuffix = FMath::Min(BoundaryDistance, OutsideTerminalCost);
                    if (FMath::IsFinite(RequiredSuffix) && RequiredSuffix >= 0.0)
                    {
                        _Data->_GoalPlateTileIndex = _Result._GoalSurface._TileIndex;
                        _Data->_GoalPlateIndex = GoalPlateIndex;
                        _Data->_GoalPlateMinX = GoalPlate._MinX;
                        _Data->_GoalPlateMinY = GoalPlate._MinY;
                        _Data->_GoalPlateMaxX = GoalPlate._MaxX;
                        _Data->_GoalPlateMaxY = GoalPlate._MaxY;
                        _Data->_GoalPlateBoundaryDistance = RequiredSuffix;
                        _Data->_GoalPlateMultiplier = GoalMultiplier;
                        _Data->_HasGoalPlateCostBound = true;
                    }
                }
            }
        }
        _Data->_Timing._HasGoalPlateCostBound = _Data->_HasGoalPlateCostBound;

        const auto StartUnion = Get_DynamicUnionEdge(InQuery._DynamicObstacles, _Result._StartPoint, _Result._StartPoint);
        if (NOT StartUnion.IsSet()) { return DoFinish(ECk_GroundNav_PathStatus::Unreachable); }
        auto Source = FNode{};
        Source._Surface = _Result._StartSurface;
        Source._Kind = ENodeKind::SourceTerminal;
        Source._EscapeState = *StartUnion == ECk_GroundNav_DynamicUnionEdge::InsideAll
            ? EEscapeState::InsideInitialUnion : EEscapeState::Escaped;
        auto Goal = FNode{};
        Goal._Surface = _Result._GoalSurface;
        Goal._Kind = ENodeKind::GoalTerminal;

        _Graph = FGraph{_Data};
        _Search = astar::TSearchState<FNode, FGraph>{_Graph, Source, Goal};
        _Result._Status = ECk_GroundNav_PathStatus::InProgress;
        return _Result._Status;
    }

    auto FCk_GroundNav_CellPathSearch::ContinueSearch(const FCk_GroundNav_PathSliceParams& InSlice) -> ECk_GroundNav_PathStatus
    {
        if (Get_IsTerminal()) { return _Result._Status; }
        auto Params = astar::FSearchParams{};
        Params.BudgetMicroseconds = cellpathsearch_private::Get_Microseconds(InSlice._Budget);
        Params.MaxIterationsPerTick = DoGet_AllowedIterations(InSlice._MaxIterations);
        Params.TimecheckInterval = cellpathsearch_private::TimecheckIntervalIterations;
        const auto SearchStatus = _Search.ContinueSearch(Params);
        _Result._ExpansionCount = _Search.GetTotalIterations();
        DoRefresh_Cost();

        switch (SearchStatus)
        {
            case astar::ESearchStatus::InProgress:
                if (DoGet_HasExceededExpansionCap())
                { return DoExtract_Partial() ? DoFinish(ECk_GroundNav_PathStatus::Partial) : DoFinish(ECk_GroundNav_PathStatus::BudgetExceeded); }
                return DoFinish(ECk_GroundNav_PathStatus::InProgress);
            case astar::ESearchStatus::Complete:
            {
                const auto& Path = _Search.GetResultPath();
                if (Path.IsEmpty() || NOT DoExtract_Route(Path.Last())) { return DoFinish(ECk_GroundNav_PathStatus::Unreachable); }
                if (_Data->_Query._MaxCorridorLength > 0 && _Result._CellRoute.Num() > _Data->_Query._MaxCorridorLength)
                { return DoFinish(ECk_GroundNav_PathStatus::BudgetExceeded); }
                if (DoGet_HasExceededExpansionCap())
                { return DoExtract_Partial() ? DoFinish(ECk_GroundNav_PathStatus::Partial) : DoFinish(ECk_GroundNav_PathStatus::BudgetExceeded); }
                return DoFinish(ECk_GroundNav_PathStatus::Ready, &Path.Last());
            }
            case astar::ESearchStatus::Failed:
                return DoExtract_Partial() ? DoFinish(ECk_GroundNav_PathStatus::Partial) : DoFinish(ECk_GroundNav_PathStatus::Unreachable);
            default:
                return DoFinish(ECk_GroundNav_PathStatus::BudgetExceeded);
        }
    }

    auto FCk_GroundNav_CellPathSearch::Get_Timing() const -> FCk_GroundNav_CellSearchTiming
    {
        return _Data.IsValid() ? _Data->_Timing : FCk_GroundNav_CellSearchTiming{};
    }

    auto FCk_GroundNav_CellPathSearch::DoFinish(ECk_GroundNav_PathStatus InStatus, const FNode* InGoal) -> ECk_GroundNav_PathStatus
    {
        _Result._Status = InStatus;
        DoRefresh_Cost();
        return InStatus;
    }

    auto FCk_GroundNav_CellPathSearch::DoExtract_Route(const FNode& InGoal) -> bool
    {
        const auto& Nodes = _Search.GetResultPath();
        if (Nodes.IsEmpty() || Nodes.Last() != InGoal) { return false; }
        _Result._CellRoute.Reset(Nodes.Num() - 1);
        _Result._PlateCorridor.Reset();
        _Result._PlateCorridor.Add(Get_FlatPlateIndex(*_Data->_Field, _Result._StartSurface._TileIndex, _Result._StartSurface._PlateIndex));
        for (auto Index = 1; Index < Nodes.Num(); ++Index)
        {
            const auto* Edge = _Graph.Get_Edge(Nodes[Index - 1], Nodes[Index]);
            if (Edge == nullptr) { return false; }
            _Result._CellRoute.Add(Edge->_Route);
            const auto Flat = Get_FlatPlateIndex(*_Data->_Field, Edge->_Route._ToSurface._TileIndex, Edge->_Route._ToSurface._PlateIndex);
            if (_Result._PlateCorridor.Last() != Flat) { _Result._PlateCorridor.Add(Flat); }
        }
        _Result._SearchCost = _Search.GetResultCost();
        return true;
    }

    auto FCk_GroundNav_CellPathSearch::DoExtract_Partial() -> bool
    {
        if (_Data->_Query._AllowPartialPath != ECk_EnableDisable::Enable) { return false; }
        auto Best = TOptional<FNode>{};
        auto BestHeuristic = TNumericLimits<float>::Max();
        for (const auto& Node : _Search.GetClosedSet())
        {
            if (Node._Kind == ENodeKind::SourceTerminal) { continue; }
            const auto Estimate = _Graph.Heuristic(Node, Node);
            if (Estimate < BestHeuristic) { BestHeuristic = Estimate; Best = Node; }
        }
        if (NOT Best.IsSet()) { return false; }
        auto Nodes = TArray<FNode>{Best.GetValue()};
        while (const auto* Parent = _Search.GetCameFrom().Find(Nodes.Last())) { Nodes.Add(*Parent); }
        Algo::Reverse(Nodes);
        _Result._CellRoute.Reset(Nodes.Num() - 1);
        _Result._PlateCorridor.Reset();
        _Result._PlateCorridor.Add(Get_FlatPlateIndex(*_Data->_Field, _Result._StartSurface._TileIndex, _Result._StartSurface._PlateIndex));
        for (auto Index = 1; Index < Nodes.Num(); ++Index)
        {
            const auto* Edge = _Graph.Get_Edge(Nodes[Index - 1], Nodes[Index]);
            if (Edge == nullptr) { return false; }
            _Result._CellRoute.Add(Edge->_Route);
            const auto Flat = Get_FlatPlateIndex(*_Data->_Field, Edge->_Route._ToSurface._TileIndex, Edge->_Route._ToSurface._PlateIndex);
            if (_Result._PlateCorridor.Last() != Flat) { _Result._PlateCorridor.Add(Flat); }
        }
        _Result._GoalPoint = _Graph.Get_NodePoint(Best.GetValue());
        _Result._SearchCost = _Search.GetGScores().FindRef(Best.GetValue());
        return true;
    }

    auto FCk_GroundNav_CellPathSearch::DoRefresh_Cost() -> void
    {
        if (NOT _Data.IsValid()) { return; }
        _Result._Cost = _Data->_PreSearchCost;
        cellpathsearch_private::AccumulateCost(_Result._Cost, _Data->_Cost);
    }

    auto FCk_GroundNav_CellPathSearch::DoGet_AllowedIterations(int32 InRequested) const -> int32
    {
        if (_Data->_Query._MaxExpansions <= 0) { return InRequested; }
        const auto Remaining = _Result._ExpansionCount < _Data->_Query._MaxExpansions
            ? _Data->_Query._MaxExpansions - _Result._ExpansionCount : 1;
        return InRequested > 0 ? FMath::Min(InRequested, Remaining) : Remaining;
    }

    auto FCk_GroundNav_CellPathSearch::DoGet_HasExceededExpansionCap() const -> bool
    {
        return _Data->_Query._MaxExpansions > 0 && _Result._ExpansionCount > _Data->_Query._MaxExpansions;
    }
}
