#pragma once

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"

#include "CkPathNetwork/Network/CkPathNetwork_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Consumes FTag_PathNetwork_NeedsBuild: builds the graph from the authored ribbons stamped by
    // UCk_Utils_PathNetwork_UE::Add. Build is pure math (no world access) — safe on any tick.
    class CKPATHNETWORK_API FProcessor_PathNetwork_Setup : public ck_exp::TProcessor<
        FProcessor_PathNetwork_Setup,
        FCk_Handle_PathNetwork,
        ck::TReadOnly<FFragment_PathNetwork_Params>,
        ck::TReadWrite<FFragment_PathNetwork_Graph>,
        FTag_PathNetwork_NeedsBuild,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FTag_PathNetwork_NeedsBuild;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_PathNetwork_Params& InParams,
            FFragment_PathNetwork_Graph& InGraph) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Drains FFragment_PathNetwork_Requests (Rebuild): replaces the ribbon set, rebuilds the
    // graph, bumps the epoch. Follower corridors notice the epoch move and replan
    // (FProcessor_PathNetworkFollower_InvalidateOnRebuild).
    class CKPATHNETWORK_API FProcessor_PathNetwork_HandleRequests : public ck_exp::TProcessor<
        FProcessor_PathNetwork_HandleRequests,
        FCk_Handle_PathNetwork,
        ck::TReadWrite<FFragment_PathNetwork_Params>,
        ck::TReadWrite<FFragment_PathNetwork_Graph>,
        ck::TReadWrite<FFragment_PathNetwork_Requests>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_PathNetwork_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_PathNetwork_Params& InParams,
            FFragment_PathNetwork_Graph& InGraph,
            FFragment_PathNetwork_Requests& InRequests) const -> void;

    private:
        static auto DoHandleRequest(
            HandleType InHandle,
            FFragment_PathNetwork_Params& InParams,
            FFragment_PathNetwork_Graph& InGraph,
            const FCk_Request_PathNetwork_Rebuild& InRequest) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Drains FFragment_PathNetworkFollower_Requests (FindRoute) under a per-frame budget
    // (UCk_PathNetwork_ProjectSettings_UE::_MaxRouteQueriesPerFrame). Each request runs the full
    // plan: overlay candidates -> A* over FRouteGraph -> navmesh validation/reprice loop ->
    // corridor compile (side-keeping offsets applied) -> signals.
    //
    // Budget-exceeded requests are re-queued verbatim and retried next tick.
    class CKPATHNETWORK_API FProcessor_PathNetworkFollower_HandleRequests : public ck_exp::TProcessor<
        FProcessor_PathNetworkFollower_HandleRequests,
        FCk_Handle_PathNetworkFollower,
        ck::TReadOnly<FFragment_PathNetworkFollower_Params>,
        ck::TReadWrite<FFragment_PathNetworkFollower_Corridor>,
        ck::TReadWrite<FFragment_PathNetworkFollower_Requests>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using MarkedDirtyBy = FFragment_PathNetworkFollower_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto DoTick(FCk_Time InDeltaT) -> void;

        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_PathNetworkFollower_Params& InParams,
            FFragment_PathNetworkFollower_Corridor& InCorridor,
            FFragment_PathNetworkFollower_Requests& InRequests) const -> void;

    private:
        auto DoHandleRequest(
            HandleType InHandle,
            const FFragment_PathNetworkFollower_Params& InParams,
            FFragment_PathNetworkFollower_Corridor& InCorridor,
            const FCk_Request_PathNetworkFollower_FindRoute& InRequest) const -> void;

    private:
        mutable int32 _BudgetRemainingThisTick = 0;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Detects corridors planned against a stale network epoch (a rebuild happened underneath) and
    // re-issues the route request for the same goal. O(1) per follower per tick.
    class CKPATHNETWORK_API FProcessor_PathNetworkFollower_InvalidateOnRebuild : public ck_exp::TProcessor<
        FProcessor_PathNetworkFollower_InvalidateOnRebuild,
        FCk_Handle_PathNetworkFollower,
        ck::TReadOnly<FFragment_PathNetworkFollower_Params>,
        ck::TReadWrite<FFragment_PathNetworkFollower_Corridor>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using TProcessor::TProcessor;

    public:
        auto ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_PathNetworkFollower_Params& InParams,
            FFragment_PathNetworkFollower_Corridor& InCorridor) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
