#pragma once

#include "CkAStar/Algorithm/CkAStar_Search.h"

#include "CkCore/Time/CkTime.h"

#include "CkGroundNav/Field/CkGroundNav_Field.h"
#include "CkGroundNav/Search/CkGroundNav_PlatePortalGraph.h"
#include "CkGroundNav/Search/CkGroundNav_SearchTypes.h"

#include <CoreMinimal.h>

// The strict overlay graph deliberately keeps the terminal role and monotonic escape state in its
// node identity. A surface alone aliases paths whose next dynamic admission is different.
namespace ck::groundnav
{
    struct FCk_GroundNav_PathSliceParams;

    /** Opt-in, value-only attribution for one strict cell search. It carries no graph or field ownership. */
    struct CKGROUNDNAV_API FCk_GroundNav_CellSearchTiming
    {
        bool _IsEnabled = false;
        int32 _SnapshotDiscCount = 0;
        int32 _SnapshotObbCount = 0;
        int32 _EdgeBuildCount = 0;
        int32 _StepAcrossCount = 0;
        int32 _StaticRayCount = 0;
        int32 _DynamicAdmissionCount = 0;
        bool _HasCurrentPlateCostBound = false;
        bool _HasGoalPlateCostBound = false;
        FCk_Time _EdgeBuildTime;
        FCk_Time _StepAcrossTime;
        FCk_Time _StaticRayTime;
        FCk_Time _DynamicAdmissionTime;
    };

    struct CKGROUNDNAV_API FCk_GroundNav_CellPathSearch
    {
    private:
        enum class ENodeKind : uint8 { SourceTerminal, SurfaceCentre, LinkEntry, LinkExit, GoalTerminal };
        enum class EEscapeState : uint8 { InsideInitialUnion, Escaped };

        struct FNode
        {
            FCk_GroundNav_SurfaceRef _Surface;
            ENodeKind _Kind = ENodeKind::SurfaceCentre;
            EEscapeState _EscapeState = EEscapeState::Escaped;
            int32 _LinkIndex = INDEX_NONE;
            bool _Forward = true;

            auto operator==(const FNode&) const -> bool = default;

            friend auto GetTypeHash(const FNode& InNode) -> uint32
            {
                auto Hash = HashCombineFast(::GetTypeHash(InNode._Surface._TileIndex), ::GetTypeHash(InNode._Surface._LayerIndex));
                Hash = HashCombineFast(Hash, ::GetTypeHash(InNode._Surface._CellX));
                Hash = HashCombineFast(Hash, ::GetTypeHash(InNode._Surface._CellY));
                Hash = HashCombineFast(Hash, ::GetTypeHash(InNode._Surface._PlateIndex));
                Hash = HashCombineFast(Hash, ::GetTypeHash(static_cast<uint8>(InNode._Kind)));
                Hash = HashCombineFast(Hash, ::GetTypeHash(static_cast<uint8>(InNode._EscapeState)));
                Hash = HashCombineFast(Hash, ::GetTypeHash(InNode._LinkIndex));
                return HashCombineFast(Hash, ::GetTypeHash(InNode._Forward));
            }
        };

        struct FEdge { FNode _To; FCk_GroundNav_CellRouteEdge _Route; };
        struct FLinkEndpoint { int32 _LinkIndex = INDEX_NONE; bool _Forward = true; };
        struct FGoalTerminalCandidate
        {
            FVector2D _XY = FVector2D::ZeroVector;
            double _TerminalCost = 0.0;
        };

        struct FGraphData
        {
            FCk_GroundNav_FieldPtr _Field;
            FCk_GroundNav_PathQuery _Query;
            FCk_GroundNav_PathResult _Ends;
            FCk_GroundNav_PathSharedData _Shared;
            FCk_GroundNav_QueryCost _Cost;
            FCk_GroundNav_QueryCost _PreSearchCost;
            FCk_GroundNav_CellSearchTiming _Timing;
            bool _DidIndexLinkEndpoints = false;
            FGoalTerminalCandidate _GoalTerminalCandidates[5];
            int32 _GoalTerminalCandidateCount = 0;
            bool _HasSingleLayerRectangularCostBound = false;
            bool _HasGoalPlateCostBound = false;
            int32 _GoalPlateTileIndex = INDEX_NONE;
            int32 _GoalPlateIndex = FCk_GroundNav_Plate::kNoPlate;
            int32 _GoalPlateMinX = 0;
            int32 _GoalPlateMinY = 0;
            int32 _GoalPlateMaxX = 0;
            int32 _GoalPlateMaxY = 0;
            double _GoalPlateBoundaryDistance = 0.0;
            double _GoalPlateMultiplier = 1.0;
            TMap<FNode, TArray<FEdge>> _Edges;
            TMap<FNode, TArray<FLinkEndpoint>> _LinkEndpoints;
        };

        struct FGraph
        {
            TSharedPtr<FGraphData> _Data;
            FGraph() = default;
            explicit FGraph(TSharedPtr<FGraphData> InData) : _Data(MoveTemp(InData)) {}

            auto Neighbors(const FNode& InNode) const -> TArray<FNode>;
            auto Cost(const FNode& InFrom, const FNode& InTo) const -> float;
            auto Heuristic(const FNode& InNode, const FNode& InGoal) const -> float;
            auto TieBreak(const FNode& InNode, const FNode& InGoal) const -> float;
            auto IsGoal(const FNode& InNode) const -> bool;
            auto Get_Edge(const FNode& InFrom, const FNode& InTo) const -> const FEdge*;
            auto Get_NodePoint(const FNode& InNode) const -> FVector;

        private:
            auto DoBuild_Edges(const FNode& InNode) const -> void;
            auto DoAdd_Edge(TArray<FEdge>& InOutEdges, const FNode& InFrom, FNode InTo,
                FCk_GroundNav_CellRouteEdge InRoute) const -> void;
            auto DoTry_AddGoal(TArray<FEdge>& InOutEdges, const FNode& InFrom,
                TConstArrayView<FCk_GroundNav_SurfaceRef> InCandidateSurfaces) const -> void;
            auto DoTry_AddLinks(TArray<FEdge>& InOutEdges, const FNode& InFrom,
                TConstArrayView<FCk_GroundNav_SurfaceRef> InCandidateSurfaces) const -> void;
            auto DoGet_DynamicAdmission(const FNode& InFrom, FNode& InOutTo,
                const FVector& InFromPoint, const FVector& InToPoint) const -> bool;
            auto DoGet_IsCellBlocked(const FCk_GroundNav_SurfaceRef& InSurface) const -> TOptional<bool>;
            auto DoGet_NodePoint(const FNode& InNode) const -> FVector;
            auto DoGet_ClearanceFactor(float InClearanceUu) const -> float;
            auto DoGet_IsStaticClear(const FVector& InFrom, const FVector& InTo) const -> bool;
            auto DoGet_IsLinkAllowed(int32 InLinkIndex, bool InForward) const -> bool;
            auto DoGet_AreaMultiplier(const FCk_GroundNav_SurfaceRef& InSurface) const -> float;
            auto DoGet_UnweightedHeuristic(const FNode& InNode) const -> double;
        };

    public:
        auto Request_Begin(const FCk_GroundNav_FieldPtr& InField, const FCk_GroundNav_PathQuery& InQuery,
            const FCk_GroundNav_PathResult& InResolvedEnds) -> ECk_GroundNav_PathStatus;
        auto ContinueSearch(const FCk_GroundNav_PathSliceParams& InSlice) -> ECk_GroundNav_PathStatus;
        auto Get_Result() const -> const FCk_GroundNav_PathResult& { return _Result; }
        auto Get_IsTerminal() const -> bool { return _Result._Status != ECk_GroundNav_PathStatus::InProgress; }
        auto Get_Timing() const -> FCk_GroundNav_CellSearchTiming;

    private:
        auto DoFinish(ECk_GroundNav_PathStatus InStatus, const FNode* InGoal = nullptr) -> ECk_GroundNav_PathStatus;
        auto DoExtract_Route(const FNode& InGoal) -> bool;
        auto DoExtract_Partial() -> bool;
        auto DoRefresh_Cost() -> void;
        auto DoGet_AllowedIterations(int32 InRequested) const -> int32;
        auto DoGet_HasExceededExpansionCap() const -> bool;

        TSharedPtr<FGraphData> _Data;
        FGraph _Graph;
        astar::TSearchState<FNode, FGraph> _Search;
        FCk_GroundNav_PathResult _Result;
    };

}
