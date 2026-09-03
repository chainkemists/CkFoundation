#include "CkGroundNav_PathSearch.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkGroundNav/Query/CkGroundNav_QueryCore.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Projection.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Reachability.h"

#include <Algo/Reverse.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_groundnav_pathsearch
{
    // CkAStar's own default. Kept explicit because the budget it samples against is this driver's.
    constexpr auto TimecheckIntervalIterations = 16;

    constexpr auto MicrosecondsPerSecond = 1'000'000.0;

    // A leg through no door: the step onto the goal point, and a query whose two ends share a plate.
    constexpr auto NoClearanceFactor = 1.0f;

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
        const auto Seed = DoRun_PreSearch(InField, InQuery);

        if (Seed._Status != ECk_GroundNav_PathStatus::InProgress)
        { return Seed._Status; }

        DoSeed_Cold(Seed._StartFlatPlate);

        return DoSet_Status(ECk_GroundNav_PathStatus::InProgress);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_GroundNav_PathSearch::
        Request_BeginRepair(
            const FCk_GroundNav_FieldPtr&              InField,
            const FCk_GroundNav_PathQuery&             InQuery,
            TConstArrayView<FCk_GroundNav_CrossingKey> InExistingCorridor,
            FCk_GroundNav_Epoch                        InPlannedAgainstEpoch)
        -> ECk_GroundNav_PathStatus
    {
        using namespace ck_groundnav_pathsearch;

        const auto Seed = DoRun_PreSearch(InField, InQuery);

        if (Seed._Status != ECk_GroundNav_PathStatus::InProgress)
        { return Seed._Status; }

        _Graph = FCk_GroundNav_PlatePortalGraph{_Shared, Seed._StartFlatPlate};

        // Walked one door at a time from the source, because a key means nothing except relative to the
        // plate it leaves: a first key that no longer leaves the plate the start resolved onto resolves
        // to nothing, which is exactly what an agent that changed plates looks like from here.
        auto Prefix = TArray<FCk_GroundNav_PathNodeId>{};
        Prefix.Reserve(InExistingCorridor.Num() + 1);
        Prefix.Add(kPathSourceNode);

        for (const auto& Key : InExistingCorridor)
        {
            const auto Node = _Graph.TryGet_NodeForKey(Prefix.Last(), Key);

            if (Node == INDEX_NONE)
            { break; }

            Prefix.Add(Node);
        }

        const auto CorridorResolvedWhole = NOT InExistingCorridor.IsEmpty() &&
            Prefix.Num() == InExistingCorridor.Num() + 1;

        const auto FieldIsTheOnePlannedAgainst = InPlannedAgainstEpoch == InField->_Epoch;

        const auto CorridorStillArrivesAtTheGoal = CorridorResolvedWhole &&
            _Graph.Get_Crossing(Prefix.Last())._ToFlatPlate == Seed._GoalFlatPlate;

        // An epoch names ONE immutable snapshot, so a corridor that re-resolved whole onto the field it
        // was planned against is the corridor a search would answer with, and re-finding it is work
        // nobody has to pay for twice. Its legs are still priced: a free route is a lie every reader of
        // the cost inherits.
        if (FieldIsTheOnePlannedAgainst && CorridorStillArrivesAtTheGoal)
        {
            DoBuild_Corridor(Prefix);

            auto CorridorCost = 0.0f;

            for (auto NodeIndex = 0; NodeIndex < Prefix.Num() - 1; ++NodeIndex)
            { CorridorCost += _Graph.Cost(Prefix[NodeIndex], Prefix[NodeIndex + 1]); }

            _Result._SearchCost = CorridorCost + DoGet_FinalLegCost(Prefix.Last());
            _Result._ExpansionCount = 0;

            DoRefresh_Cost();

            _RepairVerdict = ECk_GroundNav_RepairVerdict::StillValid;

            return DoSet_Status(ECk_GroundNav_PathStatus::Ready);
        }

        // A door can outlive the edge that reached it — a plate merge rewrites what leaves a plate
        // without touching the interval — so how far the walk reached is an upper bound on the prefix
        // rather than the answer.
        const auto FirstBrokenStep = Prefix.Num() > 1
            ? astar::ValidateExistingPath(_Graph, Prefix)
            : 0;

        const auto ValidatedNodeCount = FirstBrokenStep < Prefix.Num()
            ? FirstBrokenStep + 1
            : Prefix.Num();

        const auto WarmStartFrom = FMath::Min(Prefix.Num(), ValidatedNodeCount);

        // A prefix of the source alone is a cold search under another name, and it is also the one
        // index the warm-start constructor cannot be handed: it reads the node before it unguarded.
        // Seeding cold rebuilds the pool, so the replan expands node for node what a plain begin would.
        if (WarmStartFrom < 2)
        {
            DoAccumulate_Cost(_PreSearchCost, _Graph.Get_Cost());

            DoSeed_Cold(Seed._StartFlatPlate);
            DoRefresh_Cost();

            _RepairVerdict = ECk_GroundNav_RepairVerdict::FullReplan;

            return DoSet_Status(ECk_GroundNav_PathStatus::InProgress);
        }

        // The kept prefix is re-priced by the graph's own Cost against the field given here, which is
        // what makes a warm start a plan validated against the current epoch rather than one trusted
        // from an older one.
        _Search = astar::TSearchState<FCk_GroundNav_PathNodeId, FCk_GroundNav_PlatePortalGraph>
        {
            _Graph,
            kPathSourceNode,
            kPathSourceNode,
            Prefix,
            WarmStartFrom
        };

        DoRefresh_Cost();

        _RepairVerdict = ECk_GroundNav_RepairVerdict::Repaired;

        return DoSet_Status(ECk_GroundNav_PathStatus::InProgress);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_GroundNav_PathSearch::
        DoRun_PreSearch(
            const FCk_GroundNav_FieldPtr&  InField,
            const FCk_GroundNav_PathQuery& InQuery)
        -> FSeedResult
    {
        using namespace ck_groundnav_pathsearch;

        _Query = InQuery;
        _Shared = nullptr;
        _Graph = FCk_GroundNav_PlatePortalGraph{};
        _Search = astar::TSearchState<FCk_GroundNav_PathNodeId, FCk_GroundNav_PlatePortalGraph>{};
        _Result = FCk_GroundNav_PathResult{};
        _PreSearchCost = FCk_GroundNav_QueryCost{};
        _RepairVerdict = ECk_GroundNav_RepairVerdict::None;

        const auto Stop = [this](ECk_GroundNav_PathStatus InStatus) -> FSeedResult
        {
            return FSeedResult{DoSet_Status(InStatus)};
        };

        if (NOT InField.IsValid() || InField->Get_BuiltTileCount() <= 0)
        { return Stop(ECk_GroundNav_PathStatus::Unbuilt); }

        const auto& Field = *InField;

        _Result._PlannedAgainstEpoch = Field._Epoch;

        if (NOT Get_IsRadiusAnswerable(Field, InQuery._Agent))
        { return Stop(ECk_GroundNav_PathStatus::Blocked); }

        const auto Start = DoResolve_End(Field, InQuery._Start);
        DoAccumulate_Cost(_PreSearchCost, Start._Cost);
        DoRefresh_Cost();

        if (NOT Start.Get_IsSuccess())
        {
            return Stop(
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
            return Stop(
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
            return Stop(ECk_GroundNav_PathStatus::Unreachable);
        }

        const auto StartFlatPlate = Get_FlatPlateIndex(
            Field, Start._Surface._TileIndex, Start._Surface._PlateIndex);

        if (StartFlatPlate == INDEX_NONE)
        { return Stop(ECk_GroundNav_PathStatus::NoStartSurface); }

        const auto GoalFlatPlate = Get_FlatPlateIndex(
            Field, Goal._Surface._TileIndex, Goal._Surface._PlateIndex);

        if (GoalFlatPlate == INDEX_NONE)
        { return Stop(ECk_GroundNav_PathStatus::NoGoalSurface); }

        const auto Shared = MakeShared<FCk_GroundNav_PathSharedData>();
        Shared->_Field = InField;
        Shared->_Epoch = Field._Epoch;
        Shared->_Agent = InQuery._Agent;
        Shared->_GoalFlatPlate = GoalFlatPlate;
        Shared->_GoalPoint = _Result._GoalPoint;
        Shared->_SourcePoint = _Result._StartPoint;
        Shared->_GreedyWeightW = InQuery._GreedyWeightW;
        Shared->_SlopePenaltyK = InQuery._Cost._SlopePenaltyK;
        Shared->_ClearanceBiasK = InQuery._Cost._ClearanceBiasK;
        Shared->_PlateCostMultipliers = InQuery._Cost._PlateCostMultipliers;
        Shared->_CellSizeUu = Field._Params._Config.Get_CellSizeUu();

        _Shared = Shared;

        // A plate is a convex rectangle, so the two ends see each other across it and there is no
        // door to find. The corridor is the plate they share and nothing else — and the one leg
        // across it is still priced, because a free route is a lie every reader of the cost inherits.
        if (StartFlatPlate == GoalFlatPlate)
        {
            _Result._PlateCorridor.Add(StartFlatPlate);
            _Result._ExpansionCount = 0;
            _Result._SearchCost = Get_LegCost(
                *Shared,
                _Result._StartPoint,
                _Result._GoalPoint,
                Get_AreaMultiplier(*Shared, StartFlatPlate),
                NoClearanceFactor);

            return Stop(ECk_GroundNav_PathStatus::Ready);
        }

        return FSeedResult{ECk_GroundNav_PathStatus::InProgress, StartFlatPlate, GoalFlatPlate};
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_GroundNav_PathSearch::
        DoSeed_Cold(
            int32 InStartFlatPlate)
        -> void
    {
        _Graph = FCk_GroundNav_PlatePortalGraph{_Shared, InStartFlatPlate};

        // Start and goal are the SAME node id because the goal is a plate, not a node: which crossing
        // enters the goal plate is the question the search is being asked, so nothing can name it in
        // advance and the graph's IsGoal is what terminates the walk.
        _Search = astar::TSearchState<FCk_GroundNav_PathNodeId, FCk_GroundNav_PlatePortalGraph>
        {
            _Graph,
            kPathSourceNode,
            kPathSourceNode
        };
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
                {
                    if (NOT DoGet_PartialIsAvailable())
                    { return DoSet_Status(ECk_GroundNav_PathStatus::BudgetExceeded); }

                    DoExtract_PartialCorridor(_Graph.Get_BestNode());

                    return DoSet_Status(ECk_GroundNav_PathStatus::Partial);
                }

                return DoSet_Status(ECk_GroundNav_PathStatus::InProgress);
            }

            case astar::ESearchStatus::Complete:
            {
                if (NOT DoExtract_Corridor())
                { return DoSet_Status(ECk_GroundNav_PathStatus::Unreachable); }

                // The length ceiling is on the ANSWER, not on the work, so asking for a partial
                // cannot buy a corridor longer than the caller said it would walk.
                if (_Query._MaxCorridorLength > 0 &&
                    _Result._Crossings.Num() > _Query._MaxCorridorLength)
                { return DoSet_Status(ECk_GroundNav_PathStatus::BudgetExceeded); }

                // A cap of N buys N expansions, so a corridor found on the Nth was found within
                // budget; only the N+1st would have been over it.
                if (DoGet_HasExceededExpansionCap())
                {
                    return DoSet_Status(DoGet_PartialIsAvailable()
                        ? ECk_GroundNav_PathStatus::Partial
                        : ECk_GroundNav_PathStatus::BudgetExceeded);
                }

                return DoSet_Status(ECk_GroundNav_PathStatus::Ready);
            }

            case astar::ESearchStatus::Failed:
            {
                if (NOT DoGet_PartialIsAvailable())
                { return DoSet_Status(ECk_GroundNav_PathStatus::Unreachable); }

                DoExtract_PartialCorridor(_Graph.Get_BestNode());

                return DoSet_Status(ECk_GroundNav_PathStatus::Partial);
            }

            default:
            { return DoSet_Status(ECk_GroundNav_PathStatus::BudgetExceeded); }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_GroundNav_PathSearch::
        Get_CorridorKeys() const
        -> TArray<FCk_GroundNav_CrossingKey>
    {
        auto Keys = TArray<FCk_GroundNav_CrossingKey>{};
        Keys.Reserve(_Result._Crossings.Num());

        for (const auto& Crossing : _Result._Crossings)
        { Keys.Add(Make_CrossingKey(Crossing)); }

        return Keys;
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
        using namespace ck_groundnav_pathsearch;

        const auto& Path = _Search.GetResultPath();

        const auto PathStartsAtSource = Path.Num() > 0 && Path[0] == kPathSourceNode;

        CK_ENSURE_IF_NOT(PathStartsAtSource,
            TEXT("A completed ground path of [{}] nodes does not begin at the node the search started from"),
            Path.Num())
        { return false; }

        const auto SeedIsHeld = _Shared.IsValid();

        CK_ENSURE_IF_NOT(SeedIsHeld,
            TEXT("A completed ground path of [{}] nodes is held by a search that kept none of what priced it"),
            Path.Num())
        { return false; }

        DoBuild_Corridor(Path);

        _Result._SearchCost = _Search.GetResultCost() + DoGet_FinalLegCost(Path.Last());

        return true;
    }

    auto
        FCk_GroundNav_PathSearch::
        DoExtract_PartialCorridor(
            FCk_GroundNav_PathNodeId InBestNode)
        -> void
    {
        const auto& CameFrom = _Search.GetCameFrom();

        // The walk CkAStar reconstructs a completed path with, over the same map, so a corridor that
        // stops short and one that arrives are the same corridor up to where the search stopped.
        auto Nodes = TArray<FCk_GroundNav_PathNodeId>{};
        auto Current = InBestNode;
        Nodes.Add(Current);

        while (const auto* Parent = CameFrom.Find(Current))
        {
            Current = *Parent;
            Nodes.Add(Current);
        }

        Algo::Reverse(Nodes);

        const auto WalkReachesTheSource = Nodes[0] == kPathSourceNode;

        CK_ENSURE_IF_NOT(WalkReachesTheSource,
            TEXT("The corridor to the closest node [{}] of a ground path search does not walk back to the node it started from"),
            InBestNode)
        { return; }

        DoBuild_Corridor(Nodes);

        // No leg onto the goal: a partial never reached the plate the goal stands on, so its cost is
        // what the search had already paid to stand where it stopped, and the point it hands on is
        // where it stopped rather than the goal it could not reach.
        _Result._SearchCost = _Search.GetGScores().FindRef(InBestNode);
        _Result._GoalPoint = _Graph.Get_TransitionPoint(InBestNode);
    }

    auto
        FCk_GroundNav_PathSearch::
        DoBuild_Corridor(
            TConstArrayView<FCk_GroundNav_PathNodeId> InNodes)
        -> void
    {
        _Result._PlateCorridor.Reset(InNodes.Num());
        _Result._Crossings.Reset(InNodes.Num() - 1);
        _Result._FunnelPortals.Reset(InNodes.Num() - 1);

        _Result._PlateCorridor.Add(_Graph.Get_SourceFlatPlate());

        for (auto NodeIndex = 1; NodeIndex < InNodes.Num(); ++NodeIndex)
        {
            const auto& Crossing = _Graph.Get_Crossing(InNodes[NodeIndex]);

            auto Portal = FCk_GroundNav_FunnelPortal{};
            Portal._Left = Crossing._Left;
            Portal._Right = Crossing._Right;

            _Result._Crossings.Add(Crossing);
            _Result._FunnelPortals.Add(Portal);
            _Result._PlateCorridor.Add(Crossing._ToFlatPlate);
        }
    }

    auto
        FCk_GroundNav_PathSearch::
        DoGet_FinalLegCost(
            FCk_GroundNav_PathNodeId InLastNode) const
        -> float
    {
        using namespace ck_groundnav_pathsearch;

        return Get_LegCost(
            *_Shared,
            _Graph.Get_TransitionPoint(InLastNode),
            _Result._GoalPoint,
            Get_AreaMultiplier(*_Shared, _Shared->_GoalFlatPlate),
            NoClearanceFactor);
    }

    auto
        FCk_GroundNav_PathSearch::
        DoGet_PartialIsAvailable() const
        -> bool
    {
        // The memo is seeded to the source, so a best node that is still the source means nothing the
        // search expanded ever stood closer to the goal than standing still did: there is no corridor
        // to walk back, and an empty one is not a partial answer.
        return _Query._AllowPartialPath == ECk_EnableDisable::Enable &&
            _Graph.Get_BestNode() != kPathSourceNode;
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
