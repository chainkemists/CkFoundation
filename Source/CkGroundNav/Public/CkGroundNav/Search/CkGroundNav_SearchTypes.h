#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"

#include "CkGroundNav/Field/CkGroundNav_FieldTypes.h"
#include "CkGroundNav/Query/CkGroundNav_Funnel.h"
#include "CkGroundNav/Query/CkGroundNav_QueryTypes.h"

#include <CoreMinimal.h>

#include "CkGroundNav_SearchTypes.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// What a path search over the plate graph is asked, and what it answers.
//
// A search is not a query. A query reads a published field and returns within the call; a search
// holds state across frames, so it is asked with a plan and answers with the epoch it planned
// against — the one thing a consumer must know before it trusts a corridor found two frames ago.
//
// The statuses are MODULE-LOCAL on purpose. CkNavigation's ECk_Nav_PathStatus and
// ECk_Nav_PathFailReason are the install-boundary vocabulary; the mapping onto them belongs at that
// boundary, so a reason this module can tell apart is never flattened before anybody has read it.
// --------------------------------------------------------------------------------------------------------------------

/**
 * Where a path search stands, and once it has stopped, why.
 *
 * Unbuilt is never conflated with either NoSurface: the first is ground nothing is known about and
 * worth waiting for, the second is ground with nowhere to stand on it, and a consumer defers on one
 * and gives up on the other. BudgetExceeded is likewise never a truncated Ready — a corridor that
 * ran out of budget is not a shorter corridor, it is no corridor, and a caller that walks it walks
 * into a wall.
 */
UENUM(BlueprintType)
enum class ECk_GroundNav_PathStatus : uint8
{
    // Alive and out of slice. Call it again; nothing it has already decided is revisited.
    InProgress,

    // A corridor from the start plate to the goal plate. The only status a follower may walk.
    Ready,

    // The search stopped short of the goal and answered with the corridor to the node that came
    // closest to it, priced only as far as that node — there is no leg onto the goal to add. Only
    // ever produced for a query that asked for it.
    Partial,

    // An end of the query, or the ground between them, lies over tiles nobody has baked. The answer
    // may well exist; nothing knows yet.
    Unbuilt,

    // The start is over built ground with nowhere to stand on it.
    NoStartSurface,

    // The goal is over built ground with nowhere to stand on it.
    NoGoalSurface,

    // The two ends are provably in different components, or the search exhausted its frontier
    // without finding a door into the goal plate.
    Unreachable,

    // Expansions, corridor length or the time slice ran out first. Never a truncated Ready.
    BudgetExceeded,

    // The body is wider than the field's clearance ceiling, so no clearance can admit it and the
    // field refuses rather than answering wrongly.
    Blocked
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_GroundNav_PathStatus);

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * A node of the search: an ARRIVAL, not a plate.
     *
     * Two rooms may be joined by two doors with two clearances, so a plate alone cannot say which
     * door a leg went through and what it cost. A node is therefore a canonical crossing id into the
     * search's own pool, which makes pricing a leg an O(1) read and makes the answered path exactly
     * the sequence the funnel consumes. A plain int32 is copyable, comparable and hashable, which is
     * everything CkAStar asks of a node id.
     */
    using FCk_GroundNav_PathNodeId = int32;

    /** The node a search starts from: the source point on the plate it stands on, before any door. */
    inline constexpr FCk_GroundNav_PathNodeId kPathSourceNode = 0;

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * The cost model one query asks for, over the same graph every other query reads.
     *
     * Zero slope and zero clearance bias are parity with what a navmesh prices, which is neither, so
     * a default overlay answers exactly as the unpriced search did. The corner constant is not the
     * search's at all — nothing in a plate corridor is a corner — and is consumed by the pass that
     * turns a funnelled polyline into waypoints.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_PathCostParams
    {
    public:
        // Penalty per unit of rise over run.
        float _SlopePenaltyK = 0.0f;

        // Bias away from tight crossings, in cell widths of clearance.
        float _ClearanceBiasK = 0.0f;

        // Inside-corner offset, as a multiple of the agent radius.
        float _CornerOffsetK = 1.0f;

        // Flat plate id to a multiplier this ONE query asks for, merged upward with what the field's
        // plate already carries. An empty table is therefore the field's own price and nothing else.
        TMap<int32, float> _PlateCostMultipliers;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * One path plan.
     *
     * The two budgets are separate ceilings because they fail differently: expansions bound the
     * work and the corridor length bounds the answer. Zero means no limit on each. What one
     * slice may spend is not here — it is asked per slice, by whoever is driving the frame.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_PathQuery
    {
    public:
        FVector _Start = FVector::ZeroVector;
        FVector _Goal = FVector::ZeroVector;

        // How far above or below each end the field is allowed to look for the surface it stands on.
        float _VerticalToleranceUu = 0.0f;

        FCk_GroundNav_QueryAgent _Agent;

        FCk_GroundNav_PathCostParams _Cost;

        // Greedy weight. One is admissible; above one trades optimality for expansions, bounded by
        // (w - 1) on the answered length.
        float _GreedyWeightW = 1.0f;

        // Zero means no limit.
        int32 _MaxExpansions = 0;

        // Crossings the corridor may hold. Zero means no limit.
        int32 _MaxCorridorLength = 0;

        // Enabled: a search that cannot reach the goal answers Partial with the corridor to the node
        // that came closest, rather than Unreachable with nothing.
        ECk_EnableDisable _AllowPartialPath = ECk_EnableDisable::Disable;
    };

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * What a search answered.
     *
     * The corridor is kept in three shapes because three consumers need three things: the plates are
     * what a debug view draws and what a later validation pass re-checks, the crossings are the doors
     * with their clearances, and the funnel portals are what Get_StringPull eats. They are the same
     * route, derived once here, so nothing downstream re-derives which door the route went through.
     */
    struct CKGROUNDNAV_API FCk_GroundNav_PathResult
    {
    public:
        ECk_GroundNav_PathStatus _Status = ECk_GroundNav_PathStatus::NoStartSurface;

        // Ordered flat plate ids, the start plate first.
        TArray<int32> _PlateCorridor;

        // The doors, in walk order. One fewer entry than the corridor has plates.
        TArray<FCk_GroundNav_Crossing> _Crossings;

        // The portal of each crossing, in the same order. A link crossing contributes TWO - its
        // entry and its exit - so this is longer than _Crossings wherever the route took one.
        TArray<FCk_GroundNav_FunnelPortal> _FunnelPortals;

        // The ends resolved onto the surfaces they stand on, which is what the funnel is run between.
        FVector _StartPoint = FVector::ZeroVector;
        FVector _GoalPoint = FVector::ZeroVector;

        FCk_GroundNav_SurfaceRef _StartSurface;
        FCk_GroundNav_SurfaceRef _GoalSurface;

        // The graph's own cost of the corridor, not its funnelled length.
        float _SearchCost = 0.0f;

        int32 _ExpansionCount = 0;

        FCk_GroundNav_QueryCost _Cost;

        // The field epoch this plan was made against. Staleness is derived by comparing it at the
        // install boundary and is never stored as a flag.
        FCk_GroundNav_Epoch _PlannedAgainstEpoch;

    public:
        auto Get_IsSuccess() const -> bool { return _Status == ECk_GroundNav_PathStatus::Ready; }
    };
}

// --------------------------------------------------------------------------------------------------------------------
