#include "CkGroundNav_PlatePortalGraph.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkGroundNav/Query/CkGroundNav_QueryCore.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Reachability.h"

#include <Misc/Crc.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_groundnav_plateportalgraph
{
    // A plate is a merged rectangle, so the doors it offers are the portals along four edges plus the
    // seams of whichever tiles it touches. A reservation, not a bound.
    constexpr auto TypicalCrossingCount = 8;

    constexpr auto NoMultiplier = 1.0;

    /**
     * One coordinate hashed by its BIT PATTERN, which is the rule its equality already uses.
     *
     * Negative zero is normalised away first: it compares EQUAL to zero and must therefore hash the
     * same as zero, and a comparison is the one form of that substitution an optimiser cannot fold
     * out from under it.
     */
    auto Get_ScalarHash(
        double InValue) -> uint32
    {
        const auto Normalised = InValue == 0.0 ? 0.0 : InValue;

        return FCrc::MemCrc32(&Normalised, sizeof(Normalised));
    }

    auto Get_PointHash(
        const FVector& InPoint) -> uint32
    {
        auto Hash = Get_ScalarHash(InPoint.X);
        Hash = HashCombine(Hash, Get_ScalarHash(InPoint.Y));

        return HashCombine(Hash, Get_ScalarHash(InPoint.Z));
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    auto
        GetTypeHash(
            const FCk_GroundNav_CrossingKey& InKey)
        -> uint32
    {
        using namespace ck_groundnav_plateportalgraph;

        auto Hash = ::GetTypeHash(InKey._FromFlatPlate);
        Hash = HashCombine(Hash, ::GetTypeHash(InKey._ToFlatPlate));
        Hash = HashCombine(Hash, ::GetTypeHash(InKey._Direction));
        Hash = HashCombine(Hash, Get_PointHash(InKey._Left));

        return HashCombine(Hash, Get_PointHash(InKey._Right));
    }

    auto
        Make_CrossingKey(
            const FCk_GroundNav_Crossing& InCrossing)
        -> FCk_GroundNav_CrossingKey
    {
        auto Key = FCk_GroundNav_CrossingKey{};
        Key._FromFlatPlate = InCrossing._FromFlatPlate;
        Key._ToFlatPlate = InCrossing._ToFlatPlate;
        Key._Direction = InCrossing._Direction;
        Key._Left = InCrossing._Left;
        Key._Right = InCrossing._Right;

        return Key;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_CrossingTransitionPoint(
            const FCk_GroundNav_Crossing& InCrossing)
        -> FVector
    {
        return (InCrossing._Left + InCrossing._Right) * 0.5;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        Get_AreaMultiplier(
            const FCk_GroundNav_Field&          InField,
            const FCk_GroundNav_PathSharedData& InShared,
            int32                               InFlatPlate)
        -> float
    {
        using namespace ck_groundnav_plateportalgraph;

        auto Multiplier = static_cast<float>(NoMultiplier);

        auto TileIndex = int32{INDEX_NONE};
        auto PlateIndex = int32{INDEX_NONE};

        if (Get_TileAndPlate(InField, InFlatPlate, TileIndex, PlateIndex) &&
            InField._Tiles.IsValidIndex(TileIndex))
        {
            const auto& PlateField = InField._Tiles[TileIndex]._Plates;

            if (PlateField._Plates.IsValidIndex(PlateIndex))
            { Multiplier = PlateField._Plates[PlateIndex]._CostMultiplier; }
        }

        if (const auto* TableMultiplier = InShared._PlateCostMultipliers.Find(InFlatPlate))
        { Multiplier = FMath::Max(Multiplier, *TableMultiplier); }

        return Multiplier;
    }

    auto
        Get_LegCost(
            const FCk_GroundNav_PathSharedData& InShared,
            const FVector&                      InFrom,
            const FVector&                      InTo,
            float                               InAreaMultiplier,
            float                               InClearanceFactor)
        -> float
    {
        using namespace ck_groundnav_plateportalgraph;

        const auto Delta = InTo - InFrom;

        // The leg is ONE segment across the plate its two ends share, and is integrated as such:
        // there is no per-cell walk to take a maximum over, and a plate is planar within the merge
        // tolerance by construction.
        const auto BaseUu = Delta.Size();

        const auto RiseUu = FMath::Abs(Delta.Z);
        const auto RunUu = Delta.Size2D();

        const auto SlopeFactor = RunUu > 0.0
            ? NoMultiplier + (static_cast<double>(InShared._SlopePenaltyK) * (RiseUu / RunUu))
            : NoMultiplier;

        // A distance times three factors that are each at least one, so the edge is never negative,
        // which is the whole of what A* asks of it.
        return static_cast<float>(
            BaseUu *
            static_cast<double>(InAreaMultiplier) *
            SlopeFactor *
            static_cast<double>(InClearanceFactor));
    }

    // ----------------------------------------------------------------------------------------------------------------

    FCk_GroundNav_PlatePortalGraph::
        FCk_GroundNav_PlatePortalGraph(
            TSharedPtr<const FCk_GroundNav_PathSharedData> InShared,
            int32                                         InSourceFlatPlate)
        : _Shared(MoveTemp(InShared))
        , _SourceFlatPlate(InSourceFlatPlate)
        , _Pool(MakeShared<FCk_GroundNav_PathNodePool>())
    {
        // Seeded so the closest-node memo starts from an estimate that was actually measured. The
        // search asks for the source's own estimate exactly once, when it seeds its open set, and a
        // first neighbour further from the goal than the source must not displace it.
        _Pool->_BestHeuristic = DoGet_HeuristicTo_Goal(kPathSourceNode);
        _Pool->_BestNode = kPathSourceNode;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_GroundNav_PlatePortalGraph::
        Neighbors(
            FCk_GroundNav_PathNodeId InNode) const
        -> TArray<FCk_GroundNav_PathNodeId>
    {
        using namespace ck_groundnav_plateportalgraph;

        auto Neighbors = TArray<FCk_GroundNav_PathNodeId>{};

        if (NOT Get_IsValid())
        { return Neighbors; }

        const auto ArrivalPlate = DoGet_ArrivalPlate(InNode);

        if (ArrivalPlate == INDEX_NONE)
        { return Neighbors; }

        // The plate this node came FROM, so a leg is never immediately walked back. The source node
        // came from nowhere, and no crossing names INDEX_NONE as its destination.
        const auto BackPlate = InNode == kPathSourceNode
            ? INDEX_NONE
            : _Pool->_Crossings[InNode - 1]._FromFlatPlate;

        auto Crossings = TArray<FCk_GroundNav_Crossing>{};
        Crossings.Reserve(TypicalCrossingCount);

        Get_CrossingsFrom(*_Shared->_Field, ArrivalPlate, Crossings, _Pool->_Cost);

        Neighbors.Reserve(Crossings.Num());

        // Returned in the enumeration's own order — this tile's portal table, then the field's seam
        // array by index — which is what lets a sliced search expand node for node like a one-shot
        // one: the push sequence, and so the heap layout that breaks equal scores, is identical.
        for (const auto& Crossing : Crossings)
        {
            if (NOT Get_IsAdmitted(Crossing._ClearanceUu, _Shared->_Agent))
            { continue; }

            if (Crossing._ToFlatPlate == BackPlate)
            { continue; }

            Neighbors.Add(DoGet_OrAdd_Node(Crossing));
        }

        return Neighbors;
    }

    auto
        FCk_GroundNav_PlatePortalGraph::
        Cost(
            FCk_GroundNav_PathNodeId InFrom,
            FCk_GroundNav_PathNodeId InTo) const
        -> float
    {
        if (NOT Get_IsValid())
        { return 0.0f; }

        return Get_LegCost(
            *_Shared,
            DoGet_Point(InFrom),
            DoGet_Point(InTo),
            Get_AreaMultiplier(*_Shared->_Field, *_Shared, DoGet_ArrivalPlate(InFrom)),
            static_cast<float>(DoGet_ClearanceFactor(InTo)));
    }

    auto
        FCk_GroundNav_PlatePortalGraph::
        Heuristic(
            FCk_GroundNav_PathNodeId InNode,
            FCk_GroundNav_PathNodeId InGoal) const
        -> float
    {
        return DoGet_HeuristicTo_Goal(InNode);
    }

    auto
        FCk_GroundNav_PlatePortalGraph::
        IsGoal(
            FCk_GroundNav_PathNodeId InNode) const
        -> bool
    {
        if (NOT Get_IsValid())
        { return false; }

        const auto Estimate = DoGet_HeuristicTo_Goal(InNode);

        if (Estimate < _Pool->_BestHeuristic)
        {
            _Pool->_BestHeuristic = Estimate;
            _Pool->_BestNode = InNode;
        }

        // The source node is never the goal, even standing on the goal plate: a query whose two ends
        // share a plate is answered before a search is built at all.
        if (InNode == kPathSourceNode)
        { return false; }

        if (NOT _Pool->_Crossings.IsValidIndex(InNode - 1))
        { return false; }

        return _Pool->_Crossings[InNode - 1]._ToFlatPlate == _Shared->_GoalFlatPlate;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_GroundNav_PlatePortalGraph::
        Get_Crossing(
            FCk_GroundNav_PathNodeId InNode) const
        -> const FCk_GroundNav_Crossing&
    {
        static const auto NoCrossing = FCk_GroundNav_Crossing{};

        const auto NodeIsACrossing = _Pool.IsValid() && _Pool->_Crossings.IsValidIndex(InNode - 1);

        CK_ENSURE_IF_NOT(NodeIsACrossing,
            TEXT("Path node [{}] arrives through no crossing of this search"),
            InNode)
        { return NoCrossing; }

        return _Pool->_Crossings[InNode - 1];
    }

    auto
        FCk_GroundNav_PlatePortalGraph::
        TryGet_NodeForKey(
            FCk_GroundNav_PathNodeId         InFromNode,
            const FCk_GroundNav_CrossingKey& InKey) const
        -> FCk_GroundNav_PathNodeId
    {
        // A rebuild renumbers plates, so the door is recognised by where it stands, not by which
        // plates it joined last time; the plate it leaves is the one the walk arrived on.
        for (const auto Candidate : Neighbors(InFromNode))
        {
            const auto& Crossing = Get_Crossing(Candidate);

            const auto IsSameDoor = Crossing._Direction == InKey._Direction &&
                Crossing._Left == InKey._Left &&
                Crossing._Right == InKey._Right;

            if (IsSameDoor)
            { return Candidate; }
        }

        return INDEX_NONE;
    }

    auto
        FCk_GroundNav_PlatePortalGraph::
        Get_TransitionPoint(
            FCk_GroundNav_PathNodeId InNode) const
        -> FVector
    {
        const auto NodeIsKnown = InNode == kPathSourceNode ||
            (_Pool.IsValid() && _Pool->_TransitionPoints.IsValidIndex(InNode - 1));

        CK_ENSURE_IF_NOT(NodeIsKnown,
            TEXT("Path node [{}] is no node of this search"),
            InNode)
        { return FVector::ZeroVector; }

        return DoGet_Point(InNode);
    }

    auto
        FCk_GroundNav_PlatePortalGraph::
        Get_Cost() const
        -> FCk_GroundNav_QueryCost
    {
        return _Pool.IsValid() ? _Pool->_Cost : FCk_GroundNav_QueryCost{};
    }

    auto
        FCk_GroundNav_PlatePortalGraph::
        Get_BestNode() const
        -> FCk_GroundNav_PathNodeId
    {
        return _Pool.IsValid() ? _Pool->_BestNode : kPathSourceNode;
    }

    auto
        FCk_GroundNav_PlatePortalGraph::
        Get_BestHeuristic() const
        -> float
    {
        return _Pool.IsValid() ? _Pool->_BestHeuristic : TNumericLimits<float>::Max();
    }

    auto
        FCk_GroundNav_PlatePortalGraph::
        Get_IsValid() const
        -> bool
    {
        return _Shared.IsValid() && _Shared->_Field.IsValid() && _Pool.IsValid();
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FCk_GroundNav_PlatePortalGraph::
        DoGet_ArrivalPlate(
            FCk_GroundNav_PathNodeId InNode) const
        -> int32
    {
        if (InNode == kPathSourceNode)
        { return _SourceFlatPlate; }

        if (NOT _Pool.IsValid() || NOT _Pool->_Crossings.IsValidIndex(InNode - 1))
        { return INDEX_NONE; }

        return _Pool->_Crossings[InNode - 1]._ToFlatPlate;
    }

    auto
        FCk_GroundNav_PlatePortalGraph::
        DoGet_Point(
            FCk_GroundNav_PathNodeId InNode) const
        -> FVector
    {
        if (NOT Get_IsValid())
        { return FVector::ZeroVector; }

        if (InNode == kPathSourceNode)
        { return _Shared->_SourcePoint; }

        if (NOT _Pool->_TransitionPoints.IsValidIndex(InNode - 1))
        { return FVector::ZeroVector; }

        return _Pool->_TransitionPoints[InNode - 1];
    }

    auto
        FCk_GroundNav_PlatePortalGraph::
        DoGet_HeuristicTo_Goal(
            FCk_GroundNav_PathNodeId InNode) const
        -> float
    {
        if (NOT Get_IsValid())
        { return 0.0f; }

        const auto DistanceUu = FVector::Dist(DoGet_Point(InNode), _Shared->_GoalPoint);

        return static_cast<float>(DistanceUu * static_cast<double>(_Shared->_GreedyWeightW));
    }

    auto
        FCk_GroundNav_PlatePortalGraph::
        DoGet_ClearanceFactor(
            FCk_GroundNav_PathNodeId InNode) const
        -> double
    {
        using namespace ck_groundnav_plateportalgraph;

        const auto ClearanceBiasK = static_cast<double>(_Shared->_ClearanceBiasK);

        if (ClearanceBiasK <= 0.0)
        { return NoMultiplier; }

        const auto CellSizeUu = static_cast<double>(_Shared->_CellSizeUu);

        if (CellSizeUu <= 0.0)
        { return NoMultiplier; }

        if (NOT _Pool->_Crossings.IsValidIndex(InNode - 1))
        { return NoMultiplier; }

        // In cell widths, because that is the unit the clearance was measured in and the only one a
        // constant tuned on one field means anything on another.
        const auto ClearanceCells =
            static_cast<double>(_Pool->_Crossings[InNode - 1]._ClearanceUu) / CellSizeUu;

        return NoMultiplier + (ClearanceBiasK / (ClearanceCells + 1.0));
    }

    auto
        FCk_GroundNav_PlatePortalGraph::
        DoGet_OrAdd_Node(
            const FCk_GroundNav_Crossing& InCrossing) const
        -> FCk_GroundNav_PathNodeId
    {
        const auto Key = Make_CrossingKey(InCrossing);

        if (const auto* Existing = _Pool->_NodeIds.Find(Key))
        { return *Existing; }

        const auto PoolIndex = _Pool->_Crossings.Add(InCrossing);

        _Pool->_TransitionPoints.Add(
            Get_CrossingTransitionPoint(InCrossing));

        // Ids run from one: zero is the source node, which arrives through no crossing.
        const auto NodeId = PoolIndex + 1;

        _Pool->_NodeIds.Add(Key, NodeId);

        return NodeId;
    }
}

// --------------------------------------------------------------------------------------------------------------------
