#include "CkGroundNav_PathSearch.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkGroundNav/Query/CkGroundNav_QueryCore.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Projection.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Reachability.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_groundnav_pathsearch
{
    // CkAStar's own default. Kept explicit because the budget it samples against is this driver's.
    constexpr auto TimecheckIntervalIterations = 16;

    constexpr auto MicrosecondsPerSecond = 1'000'000.0;

    auto Get_Microseconds(
        const FCk_Time& InTime) -> int64
    {
        const auto Seconds = InTime.Get_Seconds();

        // A negative duration is not a shorter budget than none; it is no budget.
        return Seconds > 0.0 ? static_cast<int64>(Seconds * MicrosecondsPerSecond) : 0;
    }

    auto DoAccumulate_Cost(
        ck::groundnav::FCk_GroundNav_QueryCost&       InOutCost,
        const ck::groundnav::FCk_GroundNav_QueryCost& InAdded) -> void
    {
        InOutCost._CellsRead += InAdded._CellsRead;
        InOutCost._TilesTouched += InAdded._TilesTouched;
        InOutCost._TouchedUnbuiltTile = InOutCost._TouchedUnbuiltTile || InAdded._TouchedUnbuiltTile;
    }

    /**
     * A failed end resolution, told apart the way a consumer needs it told apart.
     *
     * Unbuilt outranks the caller's no-surface answer whichever end asked: the ground the answer
     * would be on has not been looked at, and a caller that gave up there would give up on a route
     * that exists.
     */
    auto Get_EndFailureStatus(
        ECk_NavSurface_QueryStatus InStatus,
        ECk_GroundNav_PathStatus   InNoSurfaceStatus) -> ECk_GroundNav_PathStatus
    {
        switch (InStatus)
        {
            case ECk_NavSurface_QueryStatus::Unbuilt:
            { return ECk_GroundNav_PathStatus::Unbuilt; }

            case ECk_NavSurface_QueryStatus::Blocked:
            { return ECk_GroundNav_PathStatus::Blocked; }

            default:
            { return InNoSurfaceStatus; }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    auto
        FCk_GroundNav_PathSearch::
        Request_Begin(
            const FCk_GroundNav_FieldPtr&  InField,
            const FCk_GroundNav_PathQuery& InQuery)
        -> ECk_GroundNav_PathStatus
    {
        using namespace ck_groundnav_pathsearch;

        _Query = InQuery;
        _Graph = FCk_GroundNav_PlatePortalGraph{};
        _Search = astar::TSearchState<FCk_GroundNav_PathNodeId, FCk_GroundNav_PlatePortalGraph>{};
        _Result = FCk_GroundNav_PathResult{};
        _PreSearchCost = FCk_GroundNav_QueryCost{};

        if (NOT InField.IsValid() || InField->Get_BuiltTileCount() <= 0)
        { return DoSet_Status(ECk_GroundNav_PathStatus::Unbuilt); }

        const auto& Field = *InField;

        _Result._PlannedAgainstEpoch = Field._Epoch;

        if (NOT Get_IsRadiusAnswerable(Field, InQuery._Agent))
        { return DoSet_Status(ECk_GroundNav_PathStatus::Blocked); }

        const auto Start = DoResolve_End(Field, InQuery._Start);
        DoAccumulate_Cost(_PreSearchCost, Start._Cost);
        DoRefresh_Cost();

        if (NOT Start.Get_IsSuccess())
        {
            return DoSet_Status(
                Get_EndFailureStatus(Start._Status, ECk_GroundNav_PathStatus::NoStartSurface));
        }

        _Result._StartSurface = Start._Surface;
        _Result._StartPoint = FVector
        {
            InQuery._Start.X,
            InQuery._Start.Y,
            static_cast<double>(Start._SurfaceZUu)
        };

        const auto Goal = DoResolve_End(Field, InQuery._Goal);
        DoAccumulate_Cost(_PreSearchCost, Goal._Cost);
        DoRefresh_Cost();

        if (NOT Goal.Get_IsSuccess())
        {
            return DoSet_Status(
                Get_EndFailureStatus(Goal._Status, ECk_GroundNav_PathStatus::NoGoalSurface));
        }

        _Result._GoalSurface = Goal._Surface;
        _Result._GoalPoint = FVector
        {
            InQuery._Goal.X,
            InQuery._Goal.Y,
            static_cast<double>(Goal._SurfaceZUu)
        };

        // The near-constant-time refusal the labels exist for, and the only direction they can be
        // read in. It expands nothing, which is what the count says.
        if (Field.Get_AreProvablyDisconnected(
                Start._Surface._TileIndex, Start._Surface._PlateIndex,
                Goal._Surface._TileIndex, Goal._Surface._PlateIndex))
        {
            _Result._ExpansionCount = 0;
            return DoSet_Status(ECk_GroundNav_PathStatus::Unreachable);
        }

        const auto StartFlatPlate = Get_FlatPlateIndex(
            Field, Start._Surface._TileIndex, Start._Surface._PlateIndex);

        if (StartFlatPlate == INDEX_NONE)
        { return DoSet_Status(ECk_GroundNav_PathStatus::NoStartSurface); }

        const auto GoalFlatPlate = Get_FlatPlateIndex(
            Field, Goal._Surface._TileIndex, Goal._Surface._PlateIndex);

        if (GoalFlatPlate == INDEX_NONE)
        { return DoSet_Status(ECk_GroundNav_PathStatus::NoGoalSurface); }

        // A plate is a convex rectangle, so the two ends see each other across it and there is no
        // door to find. The corridor is the plate they share and nothing else.
        if (StartFlatPlate == GoalFlatPlate)
        {
            _Result._PlateCorridor.Add(StartFlatPlate);
            _Result._ExpansionCount = 0;

            return DoSet_Status(ECk_GroundNav_PathStatus::Ready);
        }

        const auto Shared = MakeShared<FCk_GroundNav_PathSharedData>();
        Shared->_Field = InField;
        Shared->_Epoch = Field._Epoch;
        Shared->_Agent = InQuery._Agent;
        Shared->_GoalFlatPlate = GoalFlatPlate;
        Shared->_GoalPoint = _Result._GoalPoint;
        Shared->_SourcePoint = _Result._StartPoint;
        Shared->_GreedyWeightW = InQuery._GreedyWeightW;
        Shared->_CellSizeUu = Field._Params._Config.Get_CellSizeUu();

        _Graph = FCk_GroundNav_PlatePortalGraph{Shared, StartFlatPlate};

        // Start and goal are the SAME node id because the goal is a plate, not a node: which crossing
        // enters the goal plate is the question the search is being asked, so nothing can name it in
        // advance and the graph's IsGoal is what terminates the walk.
        _Search = astar::TSearchState<FCk_GroundNav_PathNodeId, FCk_GroundNav_PlatePortalGraph>
        {
            _Graph,
            kPathSourceNode,
            kPathSourceNode
        };

        return DoSet_Status(ECk_GroundNav_PathStatus::InProgress);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_GroundNav_PathSearch::
        ContinueSearch(
            const FCk_GroundNav_PathSliceParams& InSlice)
        -> ECk_GroundNav_PathStatus
    {
        using namespace ck_groundnav_pathsearch;

        if (Get_IsTerminal())
        { return _Result._Status; }

        auto Params = astar::FSearchParams{};
        Params.BudgetMicroseconds = Get_Microseconds(InSlice._Budget);
        Params.MaxIterationsPerTick = DoGet_AllowedIterations(InSlice._MaxIterations);
        Params.TimecheckInterval = TimecheckIntervalIterations;

        const auto SearchStatus = _Search.ContinueSearch(Params);

        _Result._ExpansionCount = _Search.GetTotalIterations();
        DoRefresh_Cost();

        switch (SearchStatus)
        {
            case astar::ESearchStatus::InProgress:
            {
                // The clamp above stops a slice ON the cap, never past it, so a count beyond it is
                // the one expansion that proved there was no answer inside the budget.
                if (DoGet_HasExceededExpansionCap())
                { return DoSet_Status(ECk_GroundNav_PathStatus::BudgetExceeded); }

                return DoSet_Status(ECk_GroundNav_PathStatus::InProgress);
            }

            case astar::ESearchStatus::Complete:
            {
                if (NOT DoExtract_Corridor())
                { return DoSet_Status(ECk_GroundNav_PathStatus::Unreachable); }

                // A cap of N buys N expansions, so a corridor found on the Nth was found within
                // budget; only the N+1st would have been over it.
                if (DoGet_HasExceededExpansionCap())
                { return DoSet_Status(ECk_GroundNav_PathStatus::BudgetExceeded); }

                if (_Query._MaxCorridorLength > 0 &&
                    _Result._Crossings.Num() > _Query._MaxCorridorLength)
                { return DoSet_Status(ECk_GroundNav_PathStatus::BudgetExceeded); }

                return DoSet_Status(ECk_GroundNav_PathStatus::Ready);
            }

            case astar::ESearchStatus::Failed:
            { return DoSet_Status(ECk_GroundNav_PathStatus::Unreachable); }

            default:
            { return DoSet_Status(ECk_GroundNav_PathStatus::BudgetExceeded); }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_GroundNav_PathSearch::
        DoSet_Status(
            ECk_GroundNav_PathStatus InStatus)
        -> ECk_GroundNav_PathStatus
    {
        _Result._Status = InStatus;

        return InStatus;
    }

    auto
        FCk_GroundNav_PathSearch::
        DoResolve_End(
            const FCk_GroundNav_Field& InField,
            const FVector&             InLocation) const
        -> FCk_GroundNav_IsNavigableResult
    {
        auto Query = FCk_GroundNav_IsNavigableQuery{};
        Query._Location = InLocation;
        Query._VerticalToleranceUu = _Query._VerticalToleranceUu;

        // Radius zero: the body is already there. A plate's clearance is not what admits an agent —
        // the crossings on its route are, and the search gates every one of them on the real radius.
        return Get_IsNavigable(InField, Query);
    }

    auto
        FCk_GroundNav_PathSearch::
        DoExtract_Corridor()
        -> bool
    {
        const auto& Path = _Search.GetResultPath();

        const auto PathStartsAtSource = Path.Num() > 0 && Path[0] == kPathSourceNode;

        CK_ENSURE_IF_NOT(PathStartsAtSource,
            TEXT("A completed ground path of [{}] nodes does not begin at the node the search started from"),
            Path.Num())
        { return false; }

        _Result._PlateCorridor.Reset(Path.Num());
        _Result._Crossings.Reset(Path.Num() - 1);
        _Result._FunnelPortals.Reset(Path.Num() - 1);

        _Result._PlateCorridor.Add(_Graph.Get_SourceFlatPlate());

        for (auto NodeIndex = 1; NodeIndex < Path.Num(); ++NodeIndex)
        {
            const auto& Crossing = _Graph.Get_Crossing(Path[NodeIndex]);

            auto Portal = FCk_GroundNav_FunnelPortal{};
            Portal._Left = Crossing._Left;
            Portal._Right = Crossing._Right;

            _Result._Crossings.Add(Crossing);
            _Result._FunnelPortals.Add(Portal);
            _Result._PlateCorridor.Add(Crossing._ToFlatPlate);
        }

        _Result._SearchCost = _Search.GetResultCost();

        return true;
    }

    auto
        FCk_GroundNav_PathSearch::
        DoRefresh_Cost()
        -> void
    {
        using namespace ck_groundnav_pathsearch;

        // Rebuilt from the two running totals rather than added to, so a slice that reads the graph's
        // cumulative total again does not bill the same probes twice.
        _Result._Cost = _PreSearchCost;

        DoAccumulate_Cost(_Result._Cost, _Graph.Get_Cost());
    }

    auto
        FCk_GroundNav_PathSearch::
        DoGet_AllowedIterations(
            int32 InRequested) const
        -> int32
    {
        if (_Query._MaxExpansions <= 0)
        { return InRequested; }

        const auto Remaining = _Result._ExpansionCount < _Query._MaxExpansions
            ? _Query._MaxExpansions - _Result._ExpansionCount
            : 1;

        // Never zero while a cap is in force: CkAStar reads zero as no limit at all, which is the
        // one thing a capped search must not be handed.
        return InRequested > 0 ? FMath::Min(InRequested, Remaining) : Remaining;
    }

    auto
        FCk_GroundNav_PathSearch::
        DoGet_HasExceededExpansionCap() const
        -> bool
    {
        return _Query._MaxExpansions > 0 && _Result._ExpansionCount > _Query._MaxExpansions;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_Path(
            const FCk_GroundNav_FieldPtr&  InField,
            const FCk_GroundNav_PathQuery& InQuery)
        -> FCk_GroundNav_PathResult
    {
        auto Search = FCk_GroundNav_PathSearch{};
        Search.Request_Begin(InField, InQuery);

        // Every ceiling off, so one slice reaches a terminal status and the loop is a guard rather
        // than a schedule.
        const auto Slice = FCk_GroundNav_PathSliceParams{};

        while (NOT Search.Get_IsTerminal())
        { Search.ContinueSearch(Slice); }

        return Search.Get_Result();
    }
}

// --------------------------------------------------------------------------------------------------------------------
